#include "UnravelReverb.h"

#include <algorithm>
#include <cmath>

namespace threadbare::dsp
{

namespace
{
    constexpr float kTwoPi = 6.28318530718f;
    constexpr float kPi = 3.14159265359f;
    
    // Safe buffer sample getter with proper wrapping
    inline float getSampleSafe(const std::vector<float>& buffer, int index) noexcept
    {
        const int size = static_cast<int>(buffer.size());
        
        // Critical: Check for empty buffer to prevent infinite loop hang!
        if (size == 0)
            return 0.0f;
        
        // Handle negative wrapping
        while (index < 0)
            index += size;
        
        // Handle overflow wrapping
        while (index >= size)
            index -= size;
        
        return buffer[static_cast<std::size_t>(index)];
    }
    
    // Catmull-Rom / Hermite cubic interpolation
    inline float cubicInterp(float y0, float y1, float y2, float y3, float frac) noexcept
    {
        const float c0 = y1;
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }
}

void UnravelReverb::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = static_cast<int>(spec.sampleRate);
    
    // Initialize parameter smoothers with 50ms ramp time for "weighty" feel
    constexpr float smoothingTimeSec = 0.05f;
    sizeSmoother.reset(sampleRate, smoothingTimeSec);
    feedbackSmoother.reset(sampleRate, smoothingTimeSec);
    toneSmoother.reset(sampleRate, smoothingTimeSec);
    driftSmoother.reset(sampleRate, smoothingTimeSec);
    mixSmoother.reset(sampleRate, smoothingTimeSec);
    ghostSmoother.reset(sampleRate, smoothingTimeSec);
    
    // Set initial values
    sizeSmoother.setCurrentAndTargetValue(1.0f);
    feedbackSmoother.setCurrentAndTargetValue(0.5f);
    toneSmoother.setCurrentAndTargetValue(0.0f);
    driftSmoother.setCurrentAndTargetValue(0.0f);
    mixSmoother.setCurrentAndTargetValue(0.5f);
    ghostSmoother.setCurrentAndTargetValue(0.0f);
    
    // Allocate 2 seconds of buffer space for each delay line
    const auto bufferSize = static_cast<std::size_t>(2.0 * sampleRate);
    
    for (std::size_t i = 0; i < kNumLines; ++i)
    {
        delayLines[i].resize(bufferSize);
        std::fill(delayLines[i].begin(), delayLines[i].end(), 0.0f);
        writeIndices[i] = 0;
    }
    
    // Initialize LFOs with random phases and rates
    juce::Random rng(static_cast<juce::int64>(sampleRate) ^ 0x5F3759DF);

    for (std::size_t i = 0; i < kNumLines; ++i)
    {
        // Random starting phase
        lfoPhases[i] = rng.nextFloat() * kTwoPi;
        
        // Random rate between min and max
        const float rateHz = juce::jmap(rng.nextFloat(),
                                      0.0f,
                                      1.0f,
                                      threadbare::tuning::Modulation::kMinRateHz,
                                      threadbare::tuning::Modulation::kMaxRateHz);
        
        lfoInc[i] = kTwoPi * rateHz / static_cast<float>(sampleRate);
        
        // Reset filter state
        lpState[i] = 0.0f;
    }
    
    // Initialize Ghost Engine using tuning constant
    const auto historySize = static_cast<std::size_t>(
        threadbare::tuning::Ghost::kHistorySeconds * sampleRate);
    ghostHistory.resize(historySize);
    std::fill(ghostHistory.begin(), ghostHistory.end(), 0.0f);
    ghostWriteHead = 0;
    
    for (auto& grain : grainPool)
        grain.active = false;
    
    ghostRng.setSeedRandomly();
    samplesSinceLastSpawn = 0;
}

void UnravelReverb::reset() noexcept
{
    // Reset smoothers to current values (no jump)
    sizeSmoother.setCurrentAndTargetValue(sizeSmoother.getTargetValue());
    feedbackSmoother.setCurrentAndTargetValue(feedbackSmoother.getTargetValue());
    toneSmoother.setCurrentAndTargetValue(toneSmoother.getTargetValue());
    driftSmoother.setCurrentAndTargetValue(driftSmoother.getTargetValue());
    mixSmoother.setCurrentAndTargetValue(mixSmoother.getTargetValue());
    ghostSmoother.setCurrentAndTargetValue(ghostSmoother.getTargetValue());
    
    for (std::size_t i = 0; i < kNumLines; ++i)
    {
        std::fill(delayLines[i].begin(), delayLines[i].end(), 0.0f);
        writeIndices[i] = 0;
        lpState[i] = 0.0f;
    }
    
    // Reset Ghost Engine
    if (!ghostHistory.empty())
        std::fill(ghostHistory.begin(), ghostHistory.end(), 0.0f);
    
    ghostWriteHead = 0;
    
    for (auto& grain : grainPool)
        grain.active = false;
    
    samplesSinceLastSpawn = 0;
}

float UnravelReverb::readDelayInterpolated(std::size_t lineIndex, float readPosition) const noexcept
{
    const auto& buffer = delayLines[lineIndex];
    
    // Critical: Check for empty buffer to prevent infinite loop hang!
    if (buffer.empty())
        return 0.0f;
    
    const int bufferSize = static_cast<int>(buffer.size());
    
    // Wrap read position
    while (readPosition < 0.0f)
        readPosition += static_cast<float>(bufferSize);
    while (readPosition >= static_cast<float>(bufferSize))
        readPosition -= static_cast<float>(bufferSize);
    
    // Get integer part and fractional part
    const int baseIndex = static_cast<int>(readPosition);
    const float frac = readPosition - static_cast<float>(baseIndex);
    
    // Get 4 samples for cubic interpolation using SAFE wrapper
    const float y0 = getSampleSafe(buffer, baseIndex - 1);
    const float y1 = getSampleSafe(buffer, baseIndex);
    const float y2 = getSampleSafe(buffer, baseIndex + 1);
    const float y3 = getSampleSafe(buffer, baseIndex + 2);
    
    return cubicInterp(y0, y1, y2, y3, frac);
}

float UnravelReverb::readGhostHistory(float readPosition) const noexcept
{
    if (ghostHistory.empty())
        return 0.0f;

    const int bufferSize = static_cast<int>(ghostHistory.size());
    
    // Wrap read position
    while (readPosition < 0.0f)
        readPosition += static_cast<float>(bufferSize);
    while (readPosition >= static_cast<float>(bufferSize))
        readPosition -= static_cast<float>(bufferSize);
    
    // Get integer part and fractional part
    const int baseIndex = static_cast<int>(readPosition);
    const float frac = readPosition - static_cast<float>(baseIndex);
    
    // Get 4 samples for cubic interpolation using SAFE wrapper
    const float y0 = getSampleSafe(ghostHistory, baseIndex - 1);
    const float y1 = getSampleSafe(ghostHistory, baseIndex);
    const float y2 = getSampleSafe(ghostHistory, baseIndex + 1);
    const float y3 = getSampleSafe(ghostHistory, baseIndex + 2);
    
    return cubicInterp(y0, y1, y2, y3, frac);
}

void UnravelReverb::trySpawnGrain(float ghostAmount) noexcept
{
    if (ghostHistory.empty() || ghostAmount <= 0.0f)
        return;

    // Find an inactive grain
    Grain* inactiveGrain = nullptr;
    for (auto& grain : grainPool)
    {
        if (!grain.active)
        {
            inactiveGrain = &grain;
            break;
        }
    }
    
    if (!inactiveGrain)
        return; // All grains are active
    
    // Activate the grain
    inactiveGrain->active = true;
    
    // Random read position: 0.1 to 0.5 seconds behind write head
    const float historyWindowSec = 0.5f;
    const float offsetSamples = ghostRng.nextFloat() * historyWindowSec * static_cast<float>(sampleRate);
    inactiveGrain->pos = static_cast<float>(ghostWriteHead) - offsetSamples;

    // Grain duration: random between min and max (80-300ms for smooth overlap)
    const float durationSec = juce::jmap(ghostRng.nextFloat(),
                                         0.0f,
                                         1.0f,
                                         threadbare::tuning::Ghost::kGrainMinSec,
                                         threadbare::tuning::Ghost::kGrainMaxSec);
    const float durationSamples = durationSec * static_cast<float>(sampleRate);
    inactiveGrain->windowInc = 1.0f / durationSamples; // Phase runs 0 to 1 (normalized)
    inactiveGrain->windowPhase = 0.0f;

    // Pitch/speed: mostly subtle detune, with prominent shimmer
    float detuneSemi = juce::jmap(ghostRng.nextFloat(),
                                  0.0f,
                                  1.0f,
                                  -threadbare::tuning::Ghost::kDetuneSemi,
                                   threadbare::tuning::Ghost::kDetuneSemi);
    
    // 25% chance of shimmer (octave up) for obvious sparkle!
    if (ghostRng.nextFloat() < threadbare::tuning::Ghost::kShimmerProbability)
        detuneSemi = threadbare::tuning::Ghost::kShimmerSemi;

    // 10% chance of octave down for thickness
    else if (ghostRng.nextFloat() < 0.1f)
        detuneSemi = -12.0f;
    
    inactiveGrain->speed = std::pow(2.0f, detuneSemi / 12.0f);
    
    // Random stereo pan position
    inactiveGrain->pan = ghostRng.nextFloat();
    
    // Amplitude based on ghost amount (louder range for presence)
    const float gainDb = juce::jmap(ghostAmount,
                                    0.0f,
                                    1.0f,
                                    threadbare::tuning::Ghost::kMinGainDb,
                                    threadbare::tuning::Ghost::kMaxGainDb);
    inactiveGrain->amp = juce::Decibels::decibelsToGain(gainDb);
}

void UnravelReverb::processGhostEngine(float ghostAmount, float& outL, float& outR) noexcept
{
    outL = 0.0f;
    outR = 0.0f;
    
    if (ghostHistory.empty() || ghostAmount <= 0.0f)
        return;
    
    for (auto& grain : grainPool)
    {
        if (!grain.active)
            continue;
        
        // Read from history with cubic interpolation (essential for smooth pitch shifting!)
        const float sample = readGhostHistory(grain.pos);
        
        // Apply Hann window with proper formula: 0.5 * (1 - cos(2*PI*phase))
        // Phase is normalized 0 to 1, creating smooth bell curve that reaches ZERO at both ends
        const float window = 0.5f * (1.0f - std::cos(kTwoPi * grain.windowPhase));
        
        // Apply window and amplitude
        const float windowedSample = sample * window * grain.amp;
        
        // Apply constant-power stereo panning
        const float panAngle = grain.pan * (kPi * 0.5f); // 0 to PI/2
        const float gainL = std::cos(panAngle);
        const float gainR = std::sin(panAngle);
        
        outL += windowedSample * gainL;
        outR += windowedSample * gainR;
        
        // Advance grain position and window phase
        grain.pos += grain.speed;
        grain.windowPhase += grain.windowInc;
        
        // Deactivate ONLY when window FULLY completes (ensures zero amplitude)
        // Adding small epsilon to ensure we reach true zero
        if (grain.windowPhase >= 1.0f)
        {
            grain.active = false;
            grain.windowPhase = 0.0f; // Reset for next spawn
        }
    }
}

void UnravelReverb::process(std::span<float> left, 
                            std::span<float> right, 
                            UnravelState& state) noexcept
{
    if (delayLines[0].empty() || left.size() != right.size())
        return;
    
    juce::ScopedNoDenormals noDenormals;
    
    const auto numSamples = left.size();
    const int bufferSize = static_cast<int>(delayLines[0].size());
    
    // Set target values once per block (these will ramp smoothly)
    const float targetSize = juce::jlimit(threadbare::tuning::Fdn::kSizeMin,
                                          threadbare::tuning::Fdn::kSizeMax,
                                          state.size);
    sizeSmoother.setTargetValue(targetSize);
    
    const float puckY = juce::jlimit(-1.0f, 1.0f, state.puckY);
    const float targetFeedback = juce::jmap(puckY, -1.0f, 1.0f, 0.0f, 0.85f);
    feedbackSmoother.setTargetValue(targetFeedback);
    
    const float targetTone = juce::jlimit(-1.0f, 1.0f, state.tone);
    toneSmoother.setTargetValue(targetTone);
    
    const float drift = juce::jlimit(0.0f, 1.0f, state.drift);
    const float puckYNorm = (puckY + 1.0f) * 0.5f;
    const float driftContribution = drift + (puckYNorm * threadbare::tuning::PuckMapping::kDriftYBonus);
    const float targetDrift = juce::jlimit(0.0f, 1.0f, driftContribution);
    driftSmoother.setTargetValue(targetDrift);
    
    const float targetMix = juce::jlimit(0.0f, 1.0f, state.mix);
    mixSmoother.setTargetValue(targetMix);
    
    const float targetGhost = juce::jlimit(0.0f, 1.0f, state.ghost);
    ghostSmoother.setTargetValue(targetGhost);
    
    // Pre-calculate base delay offsets in samples for each line
    std::array<float, kNumLines> baseDelayOffsets;
    for (std::size_t i = 0; i < kNumLines; ++i)
    {
        const float delayMs = threadbare::tuning::Fdn::kBaseDelaysMs[i];
        baseDelayOffsets[i] = delayMs * 0.001f * static_cast<float>(sampleRate);
    }
    
    // Temporary storage for FDN processing
    std::array<float, kNumLines> readOutputs;
    std::array<float, kNumLines> nextInputs;
    
    for (std::size_t sample = 0; sample < numSamples; ++sample)
    {
        const float inputL = left[sample];
        const float inputR = right[sample];
        const float monoInput = 0.5f * (inputL + inputR);
        
        // Get smoothed values per-sample (this creates tape warp when they change!)
        const float currentSize = sizeSmoother.getNextValue();
        const float currentFeedback = feedbackSmoother.getNextValue();
        const float currentTone = toneSmoother.getNextValue();
        const float currentDrift = driftSmoother.getNextValue();
        const float currentMix = mixSmoother.getNextValue();
        const float currentGhost = ghostSmoother.getNextValue();
        
        // Calculate tone coefficient from smoothed value
        const float toneCoef = juce::jmap(currentTone, -1.0f, 1.0f, 0.1f, 0.9f);
        
        // Calculate drift amount from smoothed value
        const float driftAmount = currentDrift * threadbare::tuning::Modulation::kMaxDepthSamples;
        
        // A. Record input into Ghost History
        if (!ghostHistory.empty())
        {
            ghostHistory[static_cast<std::size_t>(ghostWriteHead)] = monoInput;
            ++ghostWriteHead;
            if (ghostWriteHead >= static_cast<int>(ghostHistory.size()))
                ghostWriteHead = 0;
        }
        
        // B. Spawn grains at high density for smooth overlap
        ++samplesSinceLastSpawn;
        const int spawnInterval = static_cast<int>(sampleRate * 0.015f); // Every 15ms for dense texture
        if (samplesSinceLastSpawn >= spawnInterval && currentGhost > 0.01f)
        {
            // Probabilistic spawning based on ghost amount
            if (ghostRng.nextFloat() < (currentGhost * 0.9f)) // Higher ghost = more grains
                trySpawnGrain(currentGhost);
            
            samplesSinceLastSpawn = 0;
        }
        
        // C. Process Ghost Engine (stereo output)
        float ghostOutputL = 0.0f;
        float ghostOutputR = 0.0f;
        processGhostEngine(currentGhost, ghostOutputL, ghostOutputR);
        
        // D. Mix ghost into FDN input (mono sum for now, stereo width comes from grain panning)
        const float ghostMono = 0.5f * (ghostOutputL + ghostOutputR);
        const float fdnInput = monoInput + (ghostMono * currentGhost);
        
        // Step A: Read from all 8 delay lines with modulation
        // TAPE WARP MAGIC: currentSize changes smoothly per-sample, causing read head to slide
        for (std::size_t i = 0; i < kNumLines; ++i)
        {
            // Update LFO phase
            lfoPhases[i] += lfoInc[i];
            if (lfoPhases[i] >= kTwoPi)
                lfoPhases[i] -= kTwoPi;
            
            // Calculate modulation offset
            const float modOffset = std::sin(lfoPhases[i]) * driftAmount;
            
            // Calculate read position with smoothly changing size (creates pitch shift!)
            // As currentSize changes, this read position slides, creating Doppler effect
            float readPos = static_cast<float>(writeIndices[i]) - (baseDelayOffsets[i] * currentSize) + modOffset;
            
            // Read with cubic interpolation (essential for smooth sliding!)
            readOutputs[i] = readDelayInterpolated(i, readPos);
        }
        
        // Step B: The Matrix - Householder-style mixing
        float sumOfReads = 0.0f;
        for (std::size_t i = 0; i < kNumLines; ++i)
            sumOfReads += readOutputs[i];
        
        constexpr float mixCoeff = -0.2f;
        for (std::size_t i = 0; i < kNumLines; ++i)
        {
            const float crossMix = sumOfReads * mixCoeff + readOutputs[i];
            nextInputs[i] = fdnInput + (crossMix * currentFeedback);
        }
        
        // Step C: Apply damping and write with safety clipping
        for (std::size_t i = 0; i < kNumLines; ++i)
        {
            // 1-pole low-pass filter for damping with smoothed tone
            lpState[i] += (nextInputs[i] - lpState[i]) * toneCoef;
            
            // Write with safety clipping
            delayLines[i][static_cast<std::size_t>(writeIndices[i])] = std::tanh(lpState[i]);
            
            // Advance write index with wrapping
            ++writeIndices[i];
            if (writeIndices[i] >= bufferSize)
                writeIndices[i] = 0;
        }
        
        // Step D: Output - even lines to L, odd lines to R
        float wetL = 0.0f;
        float wetR = 0.0f;
        
        for (std::size_t i = 0; i < kNumLines; ++i)
        {
            if (i % 2 == 0)
                wetL += readOutputs[i];
            else
                wetR += readOutputs[i];
        }
        
        // Scale and apply smoothed dry/wet mix
        constexpr float wetScale = 0.5f;
        wetL *= wetScale;
        wetR *= wetScale;
        
        const float dry = 1.0f - currentMix;
        float outL = inputL * dry + wetL * currentMix;
        float outR = inputR * dry + wetR * currentMix;
        
        // Final safety: soft clip to catch any summation peaks
        left[sample] = std::tanh(outL);
        right[sample] = std::tanh(outR);
    }
    
    // Simple metering
    state.inLevel = 0.0f;
    state.tailLevel = 0.0f;
}

} // namespace threadbare::dsp
