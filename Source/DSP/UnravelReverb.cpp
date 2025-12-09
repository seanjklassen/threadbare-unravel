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
    // BUG FIX 1: Validate sample rate to prevent division by zero
    if (spec.sampleRate <= 0.0)
    {
        jassertfalse; // Alert in debug builds
        return; // Gracefully fail in release
    }
    
    sampleRate = static_cast<int>(spec.sampleRate);
    
    // Initialize parameter smoothers with 50ms ramp time for "weighty" feel
    constexpr float smoothingTimeSec = 0.05f;
    sizeSmoother.reset(sampleRate, smoothingTimeSec);
    feedbackSmoother.reset(sampleRate, smoothingTimeSec);
    toneSmoother.reset(sampleRate, smoothingTimeSec);
    driftSmoother.reset(sampleRate, smoothingTimeSec);
    driftDepthSmoother.reset(sampleRate, smoothingTimeSec);
    mixSmoother.reset(sampleRate, smoothingTimeSec);
    ghostSmoother.reset(sampleRate, smoothingTimeSec);
    
    // Set initial values
    sizeSmoother.setCurrentAndTargetValue(1.0f);
    feedbackSmoother.setCurrentAndTargetValue(0.5f);
    toneSmoother.setCurrentAndTargetValue(0.0f);
    driftSmoother.setCurrentAndTargetValue(0.0f);
    driftDepthSmoother.setCurrentAndTargetValue(50.0f); // Center of 20-80 range
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
        
        // Reset filter states
        lpState[i] = 0.0f;
        hpState[i] = 0.0f;
    }
    
    // Initialize Ghost Engine using tuning constant
    const auto historySize = static_cast<std::size_t>(
        threadbare::tuning::Ghost::kHistorySeconds * sampleRate);
    ghostHistory.resize(historySize);
    std::fill(ghostHistory.begin(), ghostHistory.end(), 0.0f);
    ghostWriteHead = 0;
    
    for (auto& grain : grainPool)
        grain.active = false;
    
    // Initialize Early Reflections buffers (stereo multi-tap delay)
    // Buffer must accommodate pre-delay + longest tap time to prevent overflow
    // Max tap times: L=57ms, R=61ms → use 61ms
    // At max settings: (100ms pre-delay + 61ms tap) * 48kHz = 7,728 samples
    const float maxTapTimeMs = 61.0f; // From kTapTimesR[5]
    const auto erBufferSize = static_cast<std::size_t>(
        (threadbare::tuning::EarlyReflections::kMaxPreDelayMs + maxTapTimeMs) 
        * 0.001f * sampleRate + 100);
    erBufferL.resize(erBufferSize);
    erBufferR.resize(erBufferSize);
    std::fill(erBufferL.begin(), erBufferL.end(), 0.0f);
    std::fill(erBufferR.begin(), erBufferR.end(), 0.0f);
    erWriteHead = 0;
    
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
        hpState[i] = 0.0f;
    }
    
    // Reset Ghost Engine
    if (!ghostHistory.empty())
        std::fill(ghostHistory.begin(), ghostHistory.end(), 0.0f);
    
    ghostWriteHead = 0;
    
    for (auto& grain : grainPool)
        grain.active = false;
    
    samplesSinceLastSpawn = 0;
    
    // Reset metering and ducking
    inputMeterState = 0.0f;
    tailMeterState = 0.0f;
    duckingEnvelope = 0.0f;
    
    // Reset DC offset tracking
    dcOffsetL = 0.0f;
    dcOffsetR = 0.0f;
    
    // Reset Early Reflections
    std::fill(erBufferL.begin(), erBufferL.end(), 0.0f);
    std::fill(erBufferR.begin(), erBufferR.end(), 0.0f);
    erWriteHead = 0;
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

void UnravelReverb::trySpawnGrain(float ghostAmount, float puckX) noexcept
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
    
    // Calculate puckX bias once (used for both proximity and stereo width)
    // distantBias: 0.0 at left (body/recent), 1.0 at right (air/distant)
    const float distantBias = (1.0f + puckX) * 0.5f;
    
    // === SPECTRAL FREEZE / MEMORY PROXIMITY (Phase 3 & 4) ===
    // If frozen: use locked positions; otherwise: use proximity-based spawn
    const int historyLength = static_cast<int>(ghostHistory.size());
    
    if (ghostFreezeActive && numFrozenPositions > 0)
    {
        // FROZEN MODE: Pick from frozen positions using weighted random
        // This creates organic variation without audible patterns
        const std::size_t randomIndex = static_cast<std::size_t>(
            ghostRng.nextFloat() * static_cast<float>(numFrozenPositions));
        const std::size_t safeIndex = std::min(randomIndex, numFrozenPositions - 1);
        
        inactiveGrain->pos = frozenSpawnPositions[safeIndex];
        
        // Ensure position is still valid within current buffer
        while (inactiveGrain->pos < 0.0f)
            inactiveGrain->pos += static_cast<float>(historyLength);
        while (inactiveGrain->pos >= static_cast<float>(historyLength))
            inactiveGrain->pos -= static_cast<float>(historyLength);
    }
    else
    {
        // NORMAL MODE: Use proximity-based position (Phase 3)
        // Interpolate between min and max lookback time based on puckX
        // puckX = -1.0 (Body): maxLookback = 150ms (recent)
        // puckX =  0.0 (Center): maxLookback = 450ms (medium)
        // puckX = +1.0 (Air): maxLookback = 750ms (distant)
        const float maxLookbackMs = threadbare::tuning::Ghost::kMinLookbackMs + 
                                    (distantBias * (threadbare::tuning::Ghost::kMaxLookbackMs - 
                                                    threadbare::tuning::Ghost::kMinLookbackMs));
        
        // Random position within [0, maxLookback]
        const float spawnPosMs = ghostRng.nextFloat() * maxLookbackMs;
        const float sampleOffset = (spawnPosMs * static_cast<float>(sampleRate)) / 1000.0f;
        
        // Set spawn position relative to write head
        inactiveGrain->pos = static_cast<float>(ghostWriteHead) - sampleOffset;
        
        // Wrap within buffer bounds
        while (inactiveGrain->pos < 0.0f)
            inactiveGrain->pos += static_cast<float>(historyLength);
        while (inactiveGrain->pos >= static_cast<float>(historyLength))
            inactiveGrain->pos -= static_cast<float>(historyLength);
    }

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
    
    // === SPECTRAL FREEZE SHIMMER ENHANCEMENT (Phase 4) ===
    // Increase shimmer probability when frozen to add variation from limited source material
    const float shimmerProb = ghostFreezeActive 
                              ? threadbare::tuning::Ghost::kFreezeShimmerProbability 
                              : threadbare::tuning::Ghost::kShimmerProbability;
    
    if (ghostRng.nextFloat() < shimmerProb)
        detuneSemi = threadbare::tuning::Ghost::kShimmerSemi; // Octave up

    // 10% chance of octave down for thickness
    else if (ghostRng.nextFloat() < 0.1f)
        detuneSemi = -12.0f;
    
    // Calculate speed ratio from detune
    float speedRatio = std::pow(2.0f, detuneSemi / 12.0f);
    
    // === REVERSE MEMORY PLAYBACK (Phase 1) ===
    // Determine if this grain should play backwards through memory
    bool isReverse = false;
    if (ghostAmount > 0.5f) // Only consider reverse at moderate-to-high ghost
    {
        // Probability scales with ghost² for gentle onset
        const float reverseProb = threadbare::tuning::Ghost::kReverseProbability 
                                  * ghostAmount * ghostAmount;
        if (ghostRng.nextFloat() < reverseProb)
        {
            isReverse = true;
            speedRatio = -speedRatio; // Negative speed = backward playback
        }
    }
    
    inactiveGrain->speed = speedRatio;
    
    // === ENHANCED STEREO POSITIONING (Phase 2) ===
    // Calculate dynamic pan width based on ghost amount and puckX position
    // Recent memories (left puck) = narrower, distant memories (right puck) = wider
    const float panWidth = threadbare::tuning::Ghost::kMinPanWidth + 
                          (ghostAmount * distantBias * 
                           (threadbare::tuning::Ghost::kMaxPanWidth - threadbare::tuning::Ghost::kMinPanWidth));
    
    // Randomize pan within calculated width, centered at 0.5
    const float panOffset = (ghostRng.nextFloat() - 0.5f) * panWidth; // -width/2 to +width/2
    inactiveGrain->pan = 0.5f + panOffset; // Center ± offset
    inactiveGrain->pan = juce::jlimit(0.0f, 1.0f, inactiveGrain->pan);
    
    // Mirror reverse grains in stereo field for spatial separation
    if (isReverse && threadbare::tuning::Ghost::kMirrorReverseGrains)
    {
        inactiveGrain->pan = 1.0f - inactiveGrain->pan;
    }
    
    // Amplitude based on ghost amount (louder range for presence)
    const float gainDb = juce::jmap(ghostAmount,
                                    0.0f,
                                    1.0f,
                                    threadbare::tuning::Ghost::kMinGainDb,
                                    threadbare::tuning::Ghost::kMaxGainDb);
    float grainAmp = juce::Decibels::decibelsToGain(gainDb);
    
    // Apply gain reduction to reverse grains (helps them sit "behind" forward grains)
    if (isReverse)
        grainAmp *= threadbare::tuning::Ghost::kReverseGainReduction;
    
    inactiveGrain->amp = grainAmp;
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
    const float puckY = juce::jlimit(-1.0f, 1.0f, state.puckY);
    
    // SUBTLE DOPPLER: Map PuckY to Size for gentle pitch warp (adds "life" without overwhelming)
    // PuckY Down (-1.0): Slightly smaller (0.92x) → subtle pitch up
    // PuckY Up (+1.0): Slightly larger (1.08x) → subtle pitch down
    // Manual Size knob remains the primary size control
    const float puckYSize = juce::jmap(puckY, -1.0f, 1.0f, 0.92f, 1.08f); // ±8% range
    const float baseSize = juce::jlimit(threadbare::tuning::Fdn::kSizeMin,
                                       threadbare::tuning::Fdn::kSizeMax,
                                       state.size);
    const float combinedSize = baseSize * puckYSize; // Subtle multiplicative blend
    const float targetSize = juce::jlimit(0.25f, 5.0f, combinedSize);
    sizeSmoother.setTargetValue(targetSize);
    
    // BUG FIX 1 & 2: Calculate feedback based on decay time and puckY multiplier
    const float decaySeconds = juce::jlimit(threadbare::tuning::Decay::kT60Min,
                                           threadbare::tuning::Decay::kT60Max,
                                           state.decaySeconds);
    
    // PuckY modifies decay time (1/3 to 3x multiplier)
    const float puckYMult = juce::jmap(puckY,
                                   -1.0f,
                                   1.0f,
                                      threadbare::tuning::Decay::kPuckYMultiplierMin,
                                      threadbare::tuning::Decay::kPuckYMultiplierMax);
    const float effectiveDecay = decaySeconds * puckYMult;
    
    // Calculate average feedback gain for approximate T60
    // Formula: feedback = exp(-6.9 * avgDelayTime / T60)
    // Using simplified approach: single feedback gain for all lines
    constexpr float avgDelayMs = 59.125f; // Average of [31,37,41,53,61,71,83,97]
    const float avgDelaySec = avgDelayMs * 0.001f;
    constexpr float sixtyDb = -6.90775527898f; // ln(0.001)
    
    float targetFeedback;
    if (state.freeze)
    {
        // Frozen: near-unity feedback for infinite sustain
        targetFeedback = threadbare::tuning::Freeze::kFrozenFeedback;
    }
    else
    {
        // Normal: calculate from decay time
        targetFeedback = std::exp((sixtyDb * avgDelaySec) / std::max(0.01f, effectiveDecay));
        targetFeedback = juce::jlimit(0.0f, 0.98f, targetFeedback); // Safety clamp
    }
    feedbackSmoother.setTargetValue(targetFeedback);
    
    // ═════════════════════════════════════════════════════════════════════════
    // SPECTRAL FREEZE (Phase 4): Lock grain spawn positions when freeze activates
    // ═════════════════════════════════════════════════════════════════════════
    const bool freezeTransition = (state.freeze != ghostFreezeActive);
    const bool freezeActivating = (!ghostFreezeActive && state.freeze);
    const bool freezeDeactivating = (ghostFreezeActive && !state.freeze);
    
    if (freezeActivating)
    {
        // Snapshot current grain positions as frozen spawn points
        numFrozenPositions = 0;
        
        // Capture positions of currently active grains
        for (const auto& grain : grainPool)
        {
            if (grain.active && numFrozenPositions < frozenSpawnPositions.size())
            {
                frozenSpawnPositions[numFrozenPositions++] = grain.pos;
            }
        }
        
        // If fewer than 4 positions captured, add random recent positions
        while (numFrozenPositions < 4 && numFrozenPositions < frozenSpawnPositions.size())
        {
            const float recentMs = ghostRng.nextFloat() * 500.0f; // Random within last 500ms
            const float sampleOffset = (recentMs * static_cast<float>(sampleRate)) / 1000.0f;
            float pos = static_cast<float>(ghostWriteHead) - sampleOffset;
            
            // Wrap within buffer bounds
            const int historyLength = static_cast<int>(ghostHistory.size());
            while (pos < 0.0f) pos += static_cast<float>(historyLength);
            while (pos >= static_cast<float>(historyLength)) pos -= static_cast<float>(historyLength);
            
            frozenSpawnPositions[numFrozenPositions++] = pos;
        }
        
        ghostFreezeActive = true;
    }
    
    if (freezeDeactivating)
    {
        // Clear frozen state
        numFrozenPositions = 0;
        ghostFreezeActive = false;
    }
    
    // ═════════════════════════════════════════════════════════════════════════
    // PUCK X MACRO: "Physical/Close → Ethereal/Distant" (Proximity Control)
    // ═════════════════════════════════════════════════════════════════════════
    const float puckX = juce::jlimit(-1.0f, 1.0f, state.puckX);
    const float normX = (puckX + 1.0f) * 0.5f; // 0.0 (Left/Physical) to 1.0 (Right/Ethereal)
    
    // 1. TONE (Filter): Dark (400Hz) → Bright (18kHz)
    //    Map normX to tone parameter (-1.0 = dark, +1.0 = bright)
    //    Use exponential mapping for perceptually even frequency spread
    const float macroTone = juce::jmap(normX, 0.0f, 1.0f, -1.0f, 1.0f);
    const float baseTone = juce::jlimit(-1.0f, 1.0f, state.tone); // Manual knob
    const float targetTone = juce::jlimit(-1.0f, 1.0f, baseTone + (macroTone * 0.7f)); // Macro dominates
    toneSmoother.setTargetValue(targetTone);
    
    // 2. GHOST (Granular Clouds): None (0.0) → Full (0.7)
    //    Clouds only appear when dragging Right
    const float macroGhost = juce::jmap(normX, 0.0f, 1.0f, 0.0f, 0.7f);
    const float baseGhost = juce::jlimit(0.0f, 1.0f, state.ghost); // Manual knob
    const float combinedGhost = baseGhost * (1.0f - normX * 0.3f) + macroGhost; // Blend
    const float targetGhost = state.freeze ? 0.0f : juce::jlimit(0.0f, 1.0f, combinedGhost);
    ghostSmoother.setTargetValue(targetGhost);
    
    // 3. DRIFT (Tape Warble): Stable (20 samples) → Seasick (80 samples)
    //    Creates increasing chaos as you move Right
    //    PuckX macro overrides the standard depth (kMaxDepthSamples)
    //    Range preserves PuckY's +0.25 drift boost as noticeable (+5 to +20 samples)
    const float baseDrift = juce::jlimit(0.0f, 1.0f, state.drift); // Manual knob
    const float puckYNorm = (puckY + 1.0f) * 0.5f;
    // Combine: manual drift + puckY boost
    const float totalDrift = baseDrift + (puckYNorm * threadbare::tuning::PuckMapping::kDriftYBonus);
    const float targetDrift = juce::jlimit(0.0f, 1.0f, totalDrift);
    driftSmoother.setTargetValue(targetDrift);
    
    const float targetMix = juce::jlimit(0.0f, 1.0f, state.mix);
    mixSmoother.setTargetValue(targetMix);
    
    // PuckX macro drift depth - SMOOTHED to prevent clicks when moving puck horizontally!
    // This overrides the standard kMaxDepthSamples with a puckX-driven range
    // Range 20-80 samples preserves PuckY's noticeable impact while creating the Stable→Chaotic macro
    const float targetDriftDepth = juce::jmap(normX, 0.0f, 1.0f, 20.0f, 80.0f);
    driftDepthSmoother.setTargetValue(targetDriftDepth);
    
    // 4. EARLY REFLECTIONS (Proximity): Physical/Close → Ethereal/Distant
    //    ER Gain: Loudest at Left (Physical), silent at Right (Ethereal)
    //    FDN Send: Weak at Left (ERs dominate), strong at Right (FDN dominates)
    const float erGain = 1.0f - normX; // 1.0 at Left → 0.0 at Right
    const float fdnSend = 0.2f + (0.8f * normX); // 0.2 at Left → 1.0 at Right
    
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
        const float currentDriftDepth = driftDepthSmoother.getNextValue(); // Smoothed PuckX macro!
        const float currentMix = mixSmoother.getNextValue();
        const float currentGhost = ghostSmoother.getNextValue();
        
        // BUG FIX 2: Input gain = 0 when frozen (no new signal enters)
        const float inputGain = state.freeze ? 0.0f : 1.0f;
        
        // Calculate tone coefficient from smoothed value
        const float toneCoef = juce::jmap(currentTone, -1.0f, 1.0f, 0.1f, 0.9f);
        
        // Calculate drift amount using smoothed PuckX macro depth (20-80 samples)
        // Smoothing prevents clicks when moving puck horizontally!
        const float driftAmount = currentDrift * currentDriftDepth;
        
        // ═════════════════════════════════════════════════════════════════════
        // A. EARLY REFLECTIONS (Proximity - Physical to Ethereal)
        // ═════════════════════════════════════════════════════════════════════
        float erOutputL = 0.0f;
        float erOutputR = 0.0f;
        
        if (!erBufferL.empty())
        {
            const int erBufSize = static_cast<int>(erBufferL.size());
            
            // ALWAYS write dry input to ER buffers (ring buffer must advance unconditionally)
            erBufferL[static_cast<std::size_t>(erWriteHead)] = inputL;
            erBufferR[static_cast<std::size_t>(erWriteHead)] = inputR;
            
            // Only process taps when ER gain is significant (optimization)
            if (erGain > 0.001f)
            {
                // Sum all taps for left and right channels
                // Pre-delay shifts entire ER cluster later in time
                const float preDelaySamples = state.erPreDelay * 0.001f * sampleRate;
                
                for (std::size_t tap = 0; tap < threadbare::tuning::EarlyReflections::kNumTaps; ++tap)
                {
                    const float tapTimeL = threadbare::tuning::EarlyReflections::kTapTimesL[tap];
                    const float tapTimeR = threadbare::tuning::EarlyReflections::kTapTimesR[tap];
                    const float tapGain = threadbare::tuning::EarlyReflections::kTapGains[tap];
                    
                    // Calculate tap offsets in samples (tap time + pre-delay)
                    const int offsetL = static_cast<int>((tapTimeL * 0.001f * sampleRate) + preDelaySamples);
                    const int offsetR = static_cast<int>((tapTimeR * 0.001f * sampleRate) + preDelaySamples);
                    
                    // Read from buffer with wrapping
                    int readIndexL = erWriteHead - offsetL;
                    int readIndexR = erWriteHead - offsetR;
                    
                    while (readIndexL < 0) readIndexL += erBufSize;
                    while (readIndexR < 0) readIndexR += erBufSize;
                    
                    erOutputL += erBufferL[static_cast<std::size_t>(readIndexL)] * tapGain;
                    erOutputR += erBufferR[static_cast<std::size_t>(readIndexR)] * tapGain;
                }
                
                // Apply ER gain (proximity control)
                erOutputL *= erGain;
                erOutputR *= erGain;
            }
            
            // ALWAYS advance write head (ring buffer semantics)
            ++erWriteHead;
            if (erWriteHead >= erBufSize)
                erWriteHead = 0;
        }
        
        // B. Record input into Ghost History (with input gain applied)
        const float gainedInput = monoInput * inputGain;
        
        if (!ghostHistory.empty())
        {
            ghostHistory[static_cast<std::size_t>(ghostWriteHead)] = gainedInput;
            ++ghostWriteHead;
            if (ghostWriteHead >= static_cast<int>(ghostHistory.size()))
                ghostWriteHead = 0;
        }
        
        // C. Spawn grains at high density for smooth overlap
        ++samplesSinceLastSpawn;
        const int spawnInterval = static_cast<int>(sampleRate * 0.015f); // Every 15ms for dense texture
        if (samplesSinceLastSpawn >= spawnInterval && currentGhost > 0.01f)
        {
            // Probabilistic spawning based on ghost amount
            if (ghostRng.nextFloat() < (currentGhost * 0.9f)) // Higher ghost = more grains
                trySpawnGrain(currentGhost, puckX);
            
            samplesSinceLastSpawn = 0;
        }
        
        // D. Process Ghost Engine (stereo output)
        float ghostOutputL = 0.0f;
        float ghostOutputR = 0.0f;
        processGhostEngine(currentGhost, ghostOutputL, ghostOutputR);
        
        // E. Mix dry + ghost + ERs into FDN input with proximity control
        //    fdnSend: 0.2 (Left/Physical) → 1.0 (Right/Ethereal)
        //    At Left: ERs dominate, less dry signal to FDN
        //    At Right: Full dry signal + ghost to FDN, no ERs
        const float ghostMono = 0.5f * (ghostOutputL + ghostOutputR);
        const float erMono = 0.5f * (erOutputL + erOutputR);
        const float fdnInput = (gainedInput * fdnSend) + (ghostMono * currentGhost) + 
                               (erMono * threadbare::tuning::EarlyReflections::kErInjectionGain);
        
        // Step A: Read from all 8 delay lines with modulation
        // TAPE WARP MAGIC: currentSize changes smoothly per-sample, causing read head to slide
        for (std::size_t i = 0; i < kNumLines; ++i)
        {
            // Update LFO phase with safe wrapping (prevents accumulation drift)
            lfoPhases[i] += lfoInc[i];
            while (lfoPhases[i] >= kTwoPi)
                lfoPhases[i] -= kTwoPi;
            while (lfoPhases[i] < 0.0f)
                lfoPhases[i] += kTwoPi;
            
            // Calculate modulation offset
            const float modOffset = std::sin(lfoPhases[i]) * driftAmount;
            
            // Calculate read position with smoothly changing size (creates pitch shift!)
            // As currentSize changes, this read position slides, creating Doppler effect
            float readPos = static_cast<float>(writeIndices[i]) - (baseDelayOffsets[i] * currentSize) + modOffset;
            
            // Read with cubic interpolation (essential for smooth sliding!)
            readOutputs[i] = readDelayInterpolated(i, readPos);
        }
        
        // Step B: The Matrix - Householder-style mixing with per-line feedback
        float sumOfReads = 0.0f;
    for (std::size_t i = 0; i < kNumLines; ++i)
            sumOfReads += readOutputs[i];
        
        constexpr float mixCoeff = -0.2f;
        constexpr float sixtyDb = -6.90775527898f; // ln(0.001) for -60dB

    for (std::size_t i = 0; i < kNumLines; ++i)
    {
            // PHASE 8: Calculate per-line feedback for even decay across all lines
            // Formula: g = exp(-6.9 * delayTime / T60)
            const float delaySec = threadbare::tuning::Fdn::kBaseDelaysMs[i] * 0.001f;
            float lineFeedback = std::exp((sixtyDb * delaySec) / std::max(0.01f, effectiveDecay));
            lineFeedback = juce::jlimit(0.0f, 0.98f, lineFeedback);
            
            // Override with freeze feedback if frozen
            if (state.freeze)
                lineFeedback = threadbare::tuning::Freeze::kFrozenFeedback;
            
            const float crossMix = sumOfReads * mixCoeff + readOutputs[i];
            nextInputs[i] = fdnInput + (crossMix * lineFeedback);
        }
        
        // Step C: Apply damping, HPF, and write with safety clipping
        for (std::size_t i = 0; i < kNumLines; ++i)
        {
            // 1-pole low-pass filter for damping with smoothed tone (removes highs)
            lpState[i] += (nextInputs[i] - lpState[i]) * toneCoef;
            
            // PHASE 8: 1-pole high-pass filter to prevent LF mud buildup (removes lows)
            // HPF = Input - LowPass(Input)
            // Cutoff ~150Hz @ 48kHz
            constexpr float hpCoef = 0.006f; // ~150Hz cutoff
            hpState[i] += (lpState[i] - hpState[i]) * hpCoef;
            const float hpfSample = lpState[i] - hpState[i];
            
            // Write with safety clipping
            delayLines[i][static_cast<std::size_t>(writeIndices[i])] = std::tanh(hpfSample);
            
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
        
        // Scale wet signal
        constexpr float wetScale = 0.5f;
        wetL *= wetScale;
        wetR *= wetScale;
        
        // BUG FIX 2: Implement ducking (sidechain-style)
        // Envelope follower on input signal
        const float duckTarget = std::abs(monoInput);
        constexpr float duckAttackCoeff = 0.9990f;  // Fast attack (~10ms)
        constexpr float duckReleaseCoeff = 0.9995f; // Slower release (~250ms)
        const float duckCoeff = (duckTarget > duckingEnvelope) ? duckAttackCoeff : duckReleaseCoeff;
        duckingEnvelope = duckTarget + duckCoeff * (duckingEnvelope - duckTarget);
        
        // Apply ducking to wet signal
        const float duckAmount = juce::jlimit(0.0f, 1.0f, state.duck);
        float duckGain = 1.0f - (duckAmount * duckingEnvelope);
        duckGain = juce::jlimit(threadbare::tuning::Ducking::kMinWetFactor, 1.0f, duckGain);
        
        wetL *= duckGain;
        wetR *= duckGain;
        
        // Mix: Dry + Wet FDN + Early Reflections
        // ERs are added directly (already scaled by erGain/proximity control)
        const float dry = 1.0f - currentMix;
        float outL = inputL * dry + wetL * currentMix + erOutputL;
        float outR = inputR * dry + wetR * currentMix + erOutputR;
        
        // Final safety: soft clip to catch any summation peaks
        float clippedL = std::tanh(outL);
        float clippedR = std::tanh(outR);
        
        // Remove any DC offset from final output (very gentle 1-pole HPF @ ~3Hz)
        // This prevents slow DC drift without affecting audio frequency content
        constexpr float dcCoeff = 0.9999f; // ~3Hz @ 48kHz
        dcOffsetL = clippedL + dcCoeff * (dcOffsetL - clippedL);
        dcOffsetR = clippedR + dcCoeff * (dcOffsetR - clippedR);
        
        left[sample] = clippedL - dcOffsetL;
        right[sample] = clippedR - dcOffsetR;
        
        // BUG FIX 3: Calculate metering with simple envelope followers
        const float dryLevel = std::max(std::abs(inputL), std::abs(inputR));
        const float wetLevel = std::max(std::abs(wetL), std::abs(wetR));
        
        // Simple 1-pole envelope followers (attack/release ~10ms at 48kHz)
        constexpr float meterCoeff = 0.9995f;
        const float inputTarget = dryLevel;
        const float tailTarget = wetLevel;
        
        inputMeterState = inputTarget + meterCoeff * (inputMeterState - inputTarget);
        tailMeterState = tailTarget + meterCoeff * (tailMeterState - tailTarget);
    }
    
    // Update metering state from envelope followers
    state.inLevel = inputMeterState;
    state.tailLevel = tailMeterState;
}

} // namespace threadbare::dsp
