#include "UnravelReverb.h"

#include <algorithm>
#include <cmath>

namespace threadbare::dsp
{

namespace
{
    constexpr float kTwoPi = 6.28318530718f;
    constexpr float kPi = 3.14159265359f;
    
    // NOTE: We use std::tanh for limiting (not a fast approximation) because
    // approximations like Padé don't saturate at ±1.0 for large inputs,
    // causing digital clipping. std::tanh is hardware-accelerated on modern CPUs.
    
    // Fast sin approximation for LFOs (Bhaskara, accurate to ~0.2%)
    inline float fastSin(float x) noexcept
    {
        // Normalize to [-PI, PI]
        while (x > kPi) x -= kTwoPi;
        while (x < -kPi) x += kTwoPi;
        const float x2 = x * x;
        return (16.0f * x * (kPi - std::abs(x))) / 
               (5.0f * kPi * kPi - 4.0f * std::abs(x) * (kPi - std::abs(x)));
    }
    
    // Fast cos approximation (cos(x) = sin(x + PI/2))
    inline float fastCos(float x) noexcept
    {
        return fastSin(x + kPi * 0.5f);
    }
    
    // Fast index wrapping (replaces while loops)
    inline int wrapIndex(int index, int size) noexcept
    {
        if (size == 0) return 0;
        index = index % size;
        return (index < 0) ? index + size : index;
    }
    
    // Safe buffer sample getter with proper wrapping (uses fast wrapIndex)
    inline float getSampleSafe(const std::vector<float>& buffer, int index) noexcept
    {
        const int size = static_cast<int>(buffer.size());
        if (size == 0)
            return 0.0f;
        
        return buffer[static_cast<std::size_t>(wrapIndex(index, size))];
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
    constexpr float smoothingTimeSec = 0.2f; // 200ms for testing (was 50ms)
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
    
    // Pre-delay smoother - prevents zipper noise when adjusting pre-delay
    // Uses longer ramp (2x) because pre-delay is smoothed per-block, not per-sample
    preDelaySmoother.reset(sampleRate, smoothingTimeSec * 2.0f);
    preDelaySmoother.setCurrentAndTargetValue(0.0f);
    
    // === ANTI-CRACKLING SMOOTHERS ===
    // These eliminate block-rate stepping that causes periodic HF artifacts
    // Initialize to EXACT values for default puck position (0.0, 0.0) to prevent startup ramps
    
    // puckX = 0.0 → normX = 0.5 → erGain = 0.5, fdnSend = 0.6
    erGainSmoother.reset(sampleRate, smoothingTimeSec);
    erGainSmoother.setCurrentAndTargetValue(0.5f); // 1.0 - 0.5 = 0.5
    
    fdnSendSmoother.reset(sampleRate, smoothingTimeSec);
    fdnSendSmoother.setCurrentAndTargetValue(0.6f); // 0.2 + (0.8 * 0.5) = 0.6
    
    // duck defaults to 0.0 in UnravelState
    duckAmountSmoother.reset(sampleRate, smoothingTimeSec);
    duckAmountSmoother.setCurrentAndTargetValue(0.0f);
    
    // Per-line feedback smoothers - eliminates stepping when decay/puckY changes
    for (std::size_t i = 0; i < kNumLines; ++i)
    {
        lineFeedbackSmoothers[i].reset(sampleRate, smoothingTimeSec);
        lineFeedbackSmoothers[i].setCurrentAndTargetValue(0.5f);
    }
    
    // Allocate 2 seconds of buffer space for each delay line
    const auto bufferSize = static_cast<std::size_t>(2.0 * sampleRate);
    
    for (std::size_t i = 0; i < kNumLines; ++i)
    {
        delayLines[i].resize(bufferSize);
        std::fill(delayLines[i].begin(), delayLines[i].end(), 0.0f);
        writeIndices[i] = 0;
        
        // Pre-calculate base delay offsets in samples (cached to avoid per-block recalculation)
        const float delayMs = threadbare::tuning::Fdn::kBaseDelaysMs[i];
        baseDelayOffsetsSamples[i] = delayMs * 0.001f * static_cast<float>(sampleRate);
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
    
    // Pre-calculate grain spawn interval (every 15ms for dense texture)
    grainSpawnInterval = static_cast<int>(sampleRate * 0.015f);
    
    // ═══════════════════════════════════════════════════════════════════════
    // GLITCH SPARKLE INITIALIZATION
    // ═══════════════════════════════════════════════════════════════════════
    
    // Clear all sparkle voices
    for (auto& voice : sparkleVoices) {
        voice = SparkleVoice{};
    }
    
    // Initialize trigger timing
    sparkleTriggerSamples = 0;
    sparklePingPongLfoPhase = 0.0f;
    sparkleRng.setSeedRandomly();
    
    // Transient detection coefficients
    const float attackMs = threadbare::tuning::GlitchLooper::kEnvelopeAttackMs;
    const float releaseMs = threadbare::tuning::GlitchLooper::kEnvelopeReleaseMs;
    transientAttackCoeff = std::exp(-1.0f / (attackMs * 0.001f * sampleRate));
    transientReleaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * sampleRate));
    transientEnvelope = 0.0f;
    transientPeak = 0.0f;
    
    // Sparkle-only filter state
    sparkleHpfStateL = 0.0f;
    sparkleHpfStateR = 0.0f;
    sparkleLpfStateL = 0.0f;
    sparkleLpfStateR = 0.0f;
    
    // ═══════════════════════════════════════════════════════════════════════
    // DISINTEGRATION LOOPER INITIALIZATION
    // ═══════════════════════════════════════════════════════════════════════
    
    // Allocate 20-second loop buffers (supports 4 bars at 60 BPM)
    const auto disintLoopSize = static_cast<std::size_t>(
        threadbare::tuning::Disintegration::kLoopBufferSeconds * sampleRate);
    disintLoopL.resize(disintLoopSize);
    disintLoopR.resize(disintLoopSize);
    std::fill(disintLoopL.begin(), disintLoopL.end(), 0.0f);
    std::fill(disintLoopR.begin(), disintLoopR.end(), 0.0f);
    
    // Calculate crossfade samples from ms constant (prevents clicks at loop boundary)
    crossfadeSamples = static_cast<int>(
        threadbare::tuning::Disintegration::kCrossfadeMs * 0.001f * sampleRate);
    
    // Initialize looper state machine
    currentLooperState = LooperState::Idle;
    loopRecordHead = 0;
    loopPlayHead = 0;
    targetLoopLength = 0;
    actualLoopLength = 0;
    entropyAmount = 0.0f;
    lastButtonState = false;
    inputDetected = false;
    silentSampleCount = 0;
    
    // Initialize SVF filter state (zero to prevent pops on engage)
    hpfSvfL = {0.0f, 0.0f};
    hpfSvfR = {0.0f, 0.0f};
    lpfSvfL = {0.0f, 0.0f};
    lpfSvfR = {0.0f, 0.0f};
    
    // Initialize block-rate filter coefficients (separate L/R for Azimuth Drift)
    currentHpfG_L = 0.0f;
    currentHpfK_L = 0.0f;
    currentHpfG_R = 0.0f;
    currentHpfK_R = 0.0f;
    currentLpfG_L = 0.0f;
    currentLpfK_L = 0.0f;
    currentLpfG_R = 0.0f;
    currentLpfK_R = 0.0f;
    currentSatAmount = 0.0f;
    
    // Initialize separate diffuse LPF state for disintegration
    disintDiffuseLpfL = 0.0f;
    disintDiffuseLpfR = 0.0f;
    
    // ═══════════════════════════════════════════════════════════════════════
    // PHASE 3: PHYSICAL DEGRADATION INITIALIZATION
    // ═══════════════════════════════════════════════════════════════════════
    
    // Oxide Shedding: smoother coefficient (5ms time constant)
    const float oxideSmoothMs = threadbare::tuning::Disintegration::kOxideDropoutSmoothMs;
    oxideGainSmoothCoef = 1.0f - std::exp(-1.0f / (oxideSmoothMs * 0.001f * static_cast<float>(sampleRate)));
    oxideGainL = 1.0f;
    oxideGainR = 1.0f;
    oxideGainTarget = 1.0f;
    oxideDropoutCounter = 0;
    oxideCheckTimer = 0;
    oxideRngState = 0x12345678;  // Deterministic seed
    
    // Motor Death: reset Brownian walk
    motorDragValueL = 0.0f;
    motorDragValueR = 0.0f;
    motorDragReadOffsetL = 0.0f;
    motorDragReadOffsetR = 0.0f;
    
    // Azimuth Drift: L/R entropy offsets (asymmetric for stereo widening)
    // Full offset applied to each channel in opposite directions for maximum stereo spread
    azimuthOffsetL = threadbare::tuning::Disintegration::kAzimuthDriftMaxOffset;   // L degrades faster
    azimuthOffsetR = -threadbare::tuning::Disintegration::kAzimuthDriftMaxOffset;  // R degrades slower
    
    // Initialize exit fade (used when entropy reaches 1.0)
    exitFadeAmount = 1.0f;
    
    // === PROFESSIONAL DSP ENHANCEMENTS INITIALIZATION ===
    
    // DC Blocker coefficient: 1 - (2*pi*fc/fs)
    dcBlockerCoef = 1.0f - (2.0f * threadbare::tuning::Disintegration::kPi * 
                           threadbare::tuning::Disintegration::kDcBlockerFreqHz / 
                           static_cast<float>(sampleRate));
    dcBlockerX1L = dcBlockerY1L = 0.0f;
    dcBlockerX1R = dcBlockerY1R = 0.0f;
    
    // Wow & Flutter LFO phase increments
    wowPhaseInc = 2.0f * threadbare::tuning::Disintegration::kPi * 
                  threadbare::tuning::Disintegration::kWowFreqHz / 
                  static_cast<float>(sampleRate);
    flutterPhaseInc = 2.0f * threadbare::tuning::Disintegration::kPi * 
                      threadbare::tuning::Disintegration::kFlutterFreqHz / 
                      static_cast<float>(sampleRate);
    wowPhase = flutterPhase = 0.0f;
    
    // Pink noise generator
    pinkNoiseCounter = 0;
    pinkOctaveBands.fill(0.0f);
    pinkNoiseRunningSum = 0.0f;
    noiseHpfStateL = noiseHpfStateR = 0.0f;
    loopEntrySamples = 0;
    
    // Hysteresis saturation state
    hysteresisMagL = hysteresisMagR = 0.0f;
    
    // ADAA saturation state
    adaaX1L = adaaX1R = 0.0f;
    
    // Initialize disintegration smoothers
    loopGainSmoother.reset(sampleRate, threadbare::tuning::Disintegration::kTransitionTimeSeconds);
    loopGainSmoother.setCurrentAndTargetValue(1.0f);  // Full volume initially
    
    focusSmoother.reset(sampleRate, 0.05);  // 50ms smoothing for puck X (prevents zipper)
    focusSmoother.setCurrentAndTargetValue(0.0f);  // Center position initially
    
    diffuseAmountSmoother.reset(sampleRate, threadbare::tuning::Disintegration::kTransitionTimeSeconds);
    diffuseAmountSmoother.setCurrentAndTargetValue(0.0f);  // No diffuse initially
    
    entropySmoother.reset(sampleRate, smoothingTimeSec);
    entropySmoother.setCurrentAndTargetValue(0.0f);
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
    preDelaySmoother.setCurrentAndTargetValue(preDelaySmoother.getTargetValue());
    
    // Reset anti-crackling smoothers
    erGainSmoother.setCurrentAndTargetValue(erGainSmoother.getTargetValue());
    fdnSendSmoother.setCurrentAndTargetValue(fdnSendSmoother.getTargetValue());
    duckAmountSmoother.setCurrentAndTargetValue(duckAmountSmoother.getTargetValue());
    for (std::size_t i = 0; i < kNumLines; ++i)
        lineFeedbackSmoothers[i].setCurrentAndTargetValue(lineFeedbackSmoothers[i].getTargetValue());
    
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
    
    // ═══════════════════════════════════════════════════════════════════════
    // GLITCH SPARKLE RESET
    // ═══════════════════════════════════════════════════════════════════════
    for (auto& voice : sparkleVoices) {
        voice = SparkleVoice{};
    }
    sparkleTriggerSamples = 0;
    sparklePingPongLfoPhase = 0.0f;
    transientEnvelope = 0.0f;
    transientPeak = 0.0f;
    sparkleHpfStateL = 0.0f;
    sparkleHpfStateR = 0.0f;
    sparkleLpfStateL = 0.0f;
    sparkleLpfStateR = 0.0f;
    
    // ═══════════════════════════════════════════════════════════════════════
    // DISINTEGRATION LOOPER RESET
    // ═══════════════════════════════════════════════════════════════════════
    
    // Clear loop buffers
    if (!disintLoopL.empty())
        std::fill(disintLoopL.begin(), disintLoopL.end(), 0.0f);
    if (!disintLoopR.empty())
        std::fill(disintLoopR.begin(), disintLoopR.end(), 0.0f);
    
    // Reset state machine to Idle
    currentLooperState = LooperState::Idle;
    loopRecordHead = 0;
    loopPlayHead = 0;
    targetLoopLength = 0;
    actualLoopLength = 0;
    entropyAmount = 0.0f;
    lastButtonState = false;
    inputDetected = false;
    silentSampleCount = 0;
    
    // Reset SVF filter state (CRITICAL: prevents pop on next engage)
    hpfSvfL = {0.0f, 0.0f};
    hpfSvfR = {0.0f, 0.0f};
    lpfSvfL = {0.0f, 0.0f};
    lpfSvfR = {0.0f, 0.0f};
    
    // Reset block-rate coefficients (separate L/R for Azimuth Drift)
    currentHpfG_L = 0.0f;
    currentHpfK_L = 0.0f;
    currentHpfG_R = 0.0f;
    currentHpfK_R = 0.0f;
    currentLpfG_L = 0.0f;
    currentLpfK_L = 0.0f;
    currentLpfG_R = 0.0f;
    currentLpfK_R = 0.0f;
    
    // ═══════════════════════════════════════════════════════════════════════
    // PHASE 3: PHYSICAL DEGRADATION RESET
    // ═══════════════════════════════════════════════════════════════════════
    
    // Oxide Shedding reset
    oxideGainL = 1.0f;
    oxideGainR = 1.0f;
    oxideGainTarget = 1.0f;
    oxideDropoutCounter = 0;
    oxideCheckTimer = 0;
    // Don't reset RNG seed - keeps variation between sessions
    
    // Motor Death reset
    motorDragValueL = 0.0f;
    motorDragValueR = 0.0f;
    motorDragReadOffsetL = 0.0f;
    motorDragReadOffsetR = 0.0f;
    currentSatAmount = 0.0f;
    
    // Reset separate diffuse LPF state for disintegration
    disintDiffuseLpfL = 0.0f;
    disintDiffuseLpfR = 0.0f;
    
    // Reset exit fade
    exitFadeAmount = 1.0f;
    
    // === RESET PROFESSIONAL DSP ENHANCEMENTS ===
    dcBlockerX1L = dcBlockerY1L = 0.0f;
    dcBlockerX1R = dcBlockerY1R = 0.0f;
    wowPhase = flutterPhase = 0.0f;
    pinkNoiseCounter = 0;
    pinkOctaveBands.fill(0.0f);
    pinkNoiseRunningSum = 0.0f;
    noiseHpfStateL = noiseHpfStateR = 0.0f;
    loopEntrySamples = 0;
    hysteresisMagL = hysteresisMagR = 0.0f;
    adaaX1L = adaaX1R = 0.0f;
    
    // Reset disintegration smoothers
    loopGainSmoother.setCurrentAndTargetValue(1.0f);
    focusSmoother.setCurrentAndTargetValue(0.0f);
    diffuseAmountSmoother.setCurrentAndTargetValue(0.0f);
    entropySmoother.setCurrentAndTargetValue(0.0f);
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

    // Debug: enforce max active grain limit (0 = unlimited)
    if constexpr (threadbare::tuning::Debug::kMaxActiveGrains > 0)
    {
        int activeCount = 0;
        for (const auto& grain : grainPool)
        {
            if (grain.active)
                ++activeCount;
        }
        if (activeCount >= threadbare::tuning::Debug::kMaxActiveGrains)
            return; // Cap reached
    }

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
    
    // Activate grain
    inactiveGrain->active = true;
    
    // Calculate puckX bias once (used for both proximity and stereo width)
    // distantBias: 0.0 at left (body/recent), 1.0 at right (air/distant)
    const float distantBias = (1.0f + puckX) * 0.5f;
    
    // === MEMORY PROXIMITY ===
    const int historyLength = static_cast<int>(ghostHistory.size());
    
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

    // Grain duration: random between min and max (50-300ms for smooth overlap)
    const float durationSec = juce::jmap(ghostRng.nextFloat(),
                                         0.0f,
                                         1.0f,
                                         threadbare::tuning::Ghost::kGrainMinSec,
                                         threadbare::tuning::Ghost::kGrainMaxSec);
    const float durationSamples = durationSec * static_cast<float>(sampleRate);
    inactiveGrain->windowInc = 1.0f / durationSamples;
    inactiveGrain->windowPhase = 0.0f;

    // Pitch/speed: mostly subtle detune, with prominent shimmer
    float detuneSemi = 0.0f;
    
    if constexpr (threadbare::tuning::Debug::kShimmerGrainsOnly)
    {
        detuneSemi = threadbare::tuning::Ghost::kShimmerSemi;
    }
    else
    {
        detuneSemi = juce::jmap(ghostRng.nextFloat(),
                                0.0f,
                                1.0f,
                                -threadbare::tuning::Ghost::kDetuneSemi,
                                 threadbare::tuning::Ghost::kDetuneSemi);
        
        if (ghostRng.nextFloat() < threadbare::tuning::Ghost::kShimmerProbability)
            detuneSemi = threadbare::tuning::Ghost::kShimmerSemi;
        else if (ghostRng.nextFloat() < 0.1f)
            detuneSemi = -12.0f;
    }
    
    float speedRatio = std::pow(2.0f, detuneSemi / 12.0f);
    
    // === REVERSE MEMORY PLAYBACK ===
    bool isReverse = false;
    if (ghostAmount > 0.5f)
    {
        const float reverseProb = threadbare::tuning::Ghost::kReverseProbability 
                                  * ghostAmount * ghostAmount;
        if (ghostRng.nextFloat() < reverseProb)
        {
            isReverse = true;
            speedRatio = -speedRatio;
        }
    }
    
    inactiveGrain->speed = speedRatio;
    
    // === ENHANCED STEREO POSITIONING ===
    const float panWidth = threadbare::tuning::Ghost::kMinPanWidth + 
                          (ghostAmount * distantBias * 
                           (threadbare::tuning::Ghost::kMaxPanWidth - threadbare::tuning::Ghost::kMinPanWidth));
    
    const float panOffset = (ghostRng.nextFloat() - 0.5f) * panWidth;
    inactiveGrain->pan = 0.5f + panOffset;
    inactiveGrain->pan = juce::jlimit(0.0f, 1.0f, inactiveGrain->pan);
    
    if (isReverse && threadbare::tuning::Ghost::kMirrorReverseGrains)
    {
        inactiveGrain->pan = 1.0f - inactiveGrain->pan;
    }
    
    // Amplitude
    const float gainDb = juce::jmap(ghostAmount,
                                    0.0f,
                                    1.0f,
                                    threadbare::tuning::Ghost::kMinGainDb,
                                    threadbare::tuning::Ghost::kMaxGainDb);
    float grainAmp = juce::Decibels::decibelsToGain(gainDb);
    
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
    
    const int historySize = static_cast<int>(ghostHistory.size());
    const float historySizeF = static_cast<float>(historySize);
    
    // Safety zones to prevent shimmer grains (2x speed) from catching up to write head
    // Danger zone: 10ms - grains here are reading near-fresh data, crossfade to silence
    // Kill zone: 2ms - grains here are too close, kill them (should already be faded)
    const float dangerZoneSamples = 0.010f * static_cast<float>(sampleRate); // 10ms crossfade
    const float killZoneSamples = 0.002f * static_cast<float>(sampleRate);   // 2ms hard stop
    
    for (auto& grain : grainPool)
    {
        if (!grain.active)
            continue;
        
        // Calculate distance from write head (accounting for circular buffer)
        float distanceFromHead = static_cast<float>(ghostWriteHead) - grain.pos;
        if (distanceFromHead < 0.0f)
            distanceFromHead += historySizeF;
        if (distanceFromHead > historySizeF * 0.5f)
            distanceFromHead = historySizeF - distanceFromHead; // Handle wrap-around
        
        // Kill zone: too close, already should be silent from crossfade
        if (distanceFromHead < killZoneSamples)
        {
            grain.active = false;
            grain.windowPhase = 0.0f;
            continue;
        }
        
        // Read from history with cubic interpolation (essential for smooth pitch shifting!)
        const float sample = readGhostHistory(grain.pos);
        
        // Apply Hann window with proper formula: 0.5 * (1 - cos(2*PI*phase))
        // Phase is normalized 0 to 1, creating smooth bell curve that reaches ZERO at both ends
        float window = 0.5f * (1.0f - fastCos(kTwoPi * grain.windowPhase));
        
        // Danger zone crossfade: smoothly reduce amplitude as grain approaches write head
        // This prevents discontinuities when shimmer grains catch up
        if (distanceFromHead < dangerZoneSamples)
        {
            const float fadeAmount = distanceFromHead / dangerZoneSamples; // 0 at kill, 1 at edge
            window *= fadeAmount * fadeAmount; // Quadratic fade for smooth tail
        }
        
        // Apply window and amplitude
        const float windowedSample = sample * window * grain.amp;
        
        // Apply constant-power stereo panning
        const float panAngle = grain.pan * (kPi * 0.5f); // 0 to PI/2
        const float gainL = fastCos(panAngle);
        const float gainR = fastSin(panAngle);
        
        outL += windowedSample * gainL;
        outR += windowedSample * gainR;
        
        // Advance grain position and window phase
        grain.pos += grain.speed;
        grain.windowPhase += grain.windowInc;
        
        // Wrap position within buffer bounds
        while (grain.pos < 0.0f) grain.pos += historySizeF;
        while (grain.pos >= historySizeF) grain.pos -= historySizeF;
        
        // Deactivate ONLY when window FULLY completes (ensures zero amplitude)
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
    const float avgDelaySec = threadbare::tuning::Fdn::kAvgDelayMs * 0.001f;
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
    
    const float targetGhost = juce::jlimit(0.0f, 1.0f, combinedGhost);
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
    //    NOW SMOOTHED: Prevents block-rate stepping when puck moves horizontally
    const float targetErGain = 1.0f - normX; // 1.0 at Left → 0.0 at Right
    const float targetFdnSend = 0.2f + (0.8f * normX); // 0.2 at Left → 1.0 at Right
    erGainSmoother.setTargetValue(targetErGain);
    fdnSendSmoother.setTargetValue(targetFdnSend);
    
    // ═════════════════════════════════════════════════════════════════════════
    // 5. GHOST ENGINE SPAWN TIMING
    // ═════════════════════════════════════════════════════════════════════════
    const float spawnIntervalMs = threadbare::tuning::Ghost::kCloudSpawnIntervalMs;
    const int effectiveSpawnInterval = std::max(1, static_cast<int>(spawnIntervalMs * 0.001f * sampleRate));
    const float spawnProb = threadbare::tuning::Ghost::kCloudSpawnProbability;
    
    // ═════════════════════════════════════════════════════════════════════════
    // 6. GLITCH LOOPER PER-BLOCK SETUP
    // ═════════════════════════════════════════════════════════════════════════
    const float glitchAmount = juce::jlimit(0.0f, 1.0f, state.glitch);
    
    // Guard tempo with safe fallback (prevents division by zero)
    const float safeGlitchTempo = juce::jlimit(
        threadbare::tuning::GlitchLooper::kMinTempo,
        threadbare::tuning::GlitchLooper::kMaxTempo,
        state.tempo > 0.0f ? state.tempo : threadbare::tuning::GlitchLooper::kFallbackTempo);
    
    // NOTE: Glitch sparkle processing is self-contained in processGlitchLooper()
    // No per-block setup needed - voices are triggered and rendered per-sample
    
    // Pre-delay is now smoothed per-sample (see inside loop) for click-free transitions
    preDelaySmoother.setTargetValue(state.erPreDelay);
    
    // Pre-calculate base tap offsets in samples (without pre-delay, added per-sample)
    std::array<float, threadbare::tuning::EarlyReflections::kNumTaps> erBaseTapOffsetsL;
    std::array<float, threadbare::tuning::EarlyReflections::kNumTaps> erBaseTapOffsetsR;
    for (std::size_t tap = 0; tap < threadbare::tuning::EarlyReflections::kNumTaps; ++tap)
    {
        erBaseTapOffsetsL[tap] = threadbare::tuning::EarlyReflections::kTapTimesL[tap] * 0.001f * sampleRate;
        erBaseTapOffsetsR[tap] = threadbare::tuning::EarlyReflections::kTapTimesR[tap] * 0.001f * sampleRate;
    }
    
    // Pre-calculate parameter-dependent values once per block
    // Duck amount - now smoothed to prevent zipper noise when adjusting duck parameter
    duckAmountSmoother.setTargetValue(juce::jlimit(0.0f, 1.0f, state.duck));
    
    // ═════════════════════════════════════════════════════════════════════════
    // PRE-CALCULATE PER-BLOCK VALUES (Performance optimization)
    // ═════════════════════════════════════════════════════════════════════════
    
    // Pre-calculate per-line feedback targets and set smoothers
    // NOW SMOOTHED: Prevents block-rate stepping when decay/puckY changes
    for (std::size_t i = 0; i < kNumLines; ++i)
    {
        const float delaySec = threadbare::tuning::Fdn::kBaseDelaysMs[i] * 0.001f;
        float targetFeedback = std::exp((sixtyDb * delaySec) / std::max(0.01f, effectiveDecay));
        targetFeedback = juce::jlimit(0.0f, 0.98f, targetFeedback);
        lineFeedbackSmoothers[i].setTargetValue(targetFeedback);
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // DISINTEGRATION LOOPER STATE MACHINE
    // Responds to BOTH rising and falling edges for proper toggle button behavior:
    //   - Rising edge (0→1) while Idle: Start Recording
    //   - Rising edge (0→1) while Recording: Force transition to Looping
    //   - ANY edge while Looping: Cancel and return to Idle
    // Looping also auto-exits when entropy reaches 1.0
    // ═══════════════════════════════════════════════════════════════════════
    using namespace threadbare::tuning;
    const bool buttonOn = state.freeze;
    const int triggerAction = state.looperTriggerAction;
    
    // Detect edges
    const bool risingEdge = buttonOn && !lastButtonState;
    const bool fallingEdge = !buttonOn && lastButtonState;
    const bool uiRisingEdge = triggerAction == 1;
    const bool uiFallingEdge = triggerAction == 2;
    const bool anyEdge = risingEdge || fallingEdge || uiRisingEdge || uiFallingEdge;
    
    // Handle toggle button edges
    if (anyEdge && currentLooperState == LooperState::Looping)
    {
        // ANY click while Looping → cancel and return to Idle
        currentLooperState = LooperState::Idle;
        loopRecordHead = 0;
        loopPlayHead = 0;
        actualLoopLength = 0;
        entropyAmount = 0.0f;
        exitFadeAmount = 1.0f;
    }
    else if ((fallingEdge || uiFallingEdge) && currentLooperState == LooperState::Recording)
    {
        // Click during Recording → COMMIT what we have and start looping
        // This allows custom/truncated loop lengths (manual "punch out")
        if (loopRecordHead > crossfadeSamples * 2)  // Must have enough for crossfade
        {
            actualLoopLength = loopRecordHead;
            currentLooperState = LooperState::Looping;
            loopPlayHead = 0;
            loopEntrySamples = 0;  // Reset for pink noise entry fade
            
            // Apply "Subliminal" transition (duck + diffuse)
            loopGainSmoother.setTargetValue(juce::Decibels::decibelsToGain(Disintegration::kAutoDuckDb));
            diffuseAmountSmoother.setTargetValue(Disintegration::kDiffuseAmount);
        }
        else
        {
            // Not enough recorded - cancel instead
            currentLooperState = LooperState::Idle;
            loopRecordHead = 0;
            loopPlayHead = 0;
            actualLoopLength = 0;
            entropyAmount = 0.0f;
            exitFadeAmount = 1.0f;
        }
    }
    else if (risingEdge || uiRisingEdge)
    {
        // Advance state machine based on current state
        switch (currentLooperState)
        {
            case LooperState::Idle:
                // Idle → Recording: Start capturing
                // Fixed time-based capture window (DAW-agnostic)
                {
                    targetLoopLength = static_cast<int>(
                        Disintegration::kLoopRecordSeconds * static_cast<float>(sampleRate));
                    targetLoopLength = std::min(targetLoopLength, static_cast<int>(disintLoopL.size()));
                    
                    // Safety: abort if we can't allocate a valid loop
                    if (targetLoopLength < crossfadeSamples * 2 || disintLoopL.empty())
                    {
                        break;  // Stay in Idle state
                    }
                }
                
                currentLooperState = LooperState::Recording;
                loopRecordHead = 0;
                inputDetected = false;
                silentSampleCount = 0;
                entropyAmount = 0.0f;
                exitFadeAmount = 1.0f;
                
                // Reset SVF filter state for clean start
                hpfSvfL = {0.0f, 0.0f};
                hpfSvfR = {0.0f, 0.0f};
                lpfSvfL = {0.0f, 0.0f};
                lpfSvfR = {0.0f, 0.0f};
                
                // Reset diffuse LPF state
                disintDiffuseLpfL = 0.0f;
                disintDiffuseLpfR = 0.0f;
                
                // === PHASE 3: Reset Physical Degradation state for clean start ===
                oxideGainL = 1.0f;
                oxideGainR = 1.0f;
                oxideGainTarget = 1.0f;
                oxideDropoutCounter = 0;
                oxideCheckTimer = 0;
                motorDragValueL = 0.0f;
                motorDragValueR = 0.0f;
                motorDragReadOffsetL = 0.0f;
                motorDragReadOffsetR = 0.0f;
                
                // Set transition smoothers for Recording → Looping transition
                loopGainSmoother.setTargetValue(1.0f);  // Full volume during recording
                diffuseAmountSmoother.setTargetValue(0.0f);  // No diffuse during recording
                break;
                
            case LooperState::Recording:
                // NOTE: With toggle button, this case won't be hit on second click
                // because button goes OFF (not ON). Kept for completeness.
                if (loopRecordHead > crossfadeSamples)
                {
                    actualLoopLength = loopRecordHead;
                    currentLooperState = LooperState::Looping;
                    loopPlayHead = 0;
                    loopEntrySamples = 0;  // Reset for pink noise entry fade
                    
                    loopGainSmoother.setTargetValue(juce::Decibels::decibelsToGain(Disintegration::kAutoDuckDb));
                    diffuseAmountSmoother.setTargetValue(Disintegration::kDiffuseAmount);
                }
                break;
                
            case LooperState::Looping:
                // User clicked button while Looping: cancel and return to Idle
                currentLooperState = LooperState::Idle;
                loopRecordHead = 0;
                loopPlayHead = 0;
                actualLoopLength = 0;
                entropyAmount = 0.0f;
                exitFadeAmount = 1.0f;
                break;
        }
    }
    lastButtonState = buttonOn;
    
    // Auto-transition: Recording → Looping when buffer is full AND we actually recorded something
    if (currentLooperState == LooperState::Recording && 
        loopRecordHead >= targetLoopLength && 
        inputDetected)  // Only transition if we detected input
    {
        actualLoopLength = targetLoopLength;
        currentLooperState = LooperState::Looping;
        loopPlayHead = 0;
        loopEntrySamples = 0;  // Reset for pink noise entry fade
        
        // Apply "Subliminal" transition
        loopGainSmoother.setTargetValue(juce::Decibels::decibelsToGain(Disintegration::kAutoDuckDb));
        diffuseAmountSmoother.setTargetValue(Disintegration::kDiffuseAmount);
    }
    
    // Safety: If somehow in Looping with no content, return to Idle
    if (currentLooperState == LooperState::Looping && actualLoopLength == 0)
    {
        currentLooperState = LooperState::Idle;
    }
    
    // === BLOCK-RATE SVF COEFFICIENT CALCULATION (Ascension Filter) ===
    // CRITICAL: Calculate ONCE per block, not per-sample (saves CPU)
    // PHASE 3: Separate L/R coefficients for Azimuth Drift stereo decoupling
    if (currentLooperState == LooperState::Looping && actualLoopLength > 0)
    {
        // Get smoothed entropy for stable coefficient calculation
        entropySmoother.setTargetValue(entropyAmount);
        const float smoothedEntropy = entropySmoother.getCurrentValue();
        
        // Focus: puckX controls character of disintegration
        // puckX = -1.0 (left, Ghost): Spectral thinning, emphasize highs
        // puckX = +1.0 (right, Fog): Diffuse smearing, preserve lows
        const float focus = state.puckX;  // -1 to +1
        const float focusAmount = std::abs(focus);
        const float focusNormBlock = (focus + 1.0f) * 0.5f;  // 0 (Ghost) to 1 (Fog)
        
        // ═══════════════════════════════════════════════════════════════════
        // PHASE 3: AZIMUTH DRIFT - Calculate separate L/R entropy values
        // Each channel degrades at a slightly different rate (tape head misalignment)
        // Focus: Ghost (left) = wider stereo tear (1.5x), Fog (right) = narrower (0.3x)
        // ═══════════════════════════════════════════════════════════════════
        const float azimuthFocusScale = 1.5f - focusNormBlock * 1.2f;  // 1.5 at Ghost, 0.3 at Fog
        const float scaledAzimuthL = azimuthOffsetL * azimuthFocusScale;
        const float scaledAzimuthR = azimuthOffsetR * azimuthFocusScale;
        const float entropyL = std::clamp(smoothedEntropy + scaledAzimuthL * smoothedEntropy, 0.0f, 1.0f);
        const float entropyR = std::clamp(smoothedEntropy + scaledAzimuthR * smoothedEntropy, 0.0f, 1.0f);
        
        // Helper lambda to calculate filter frequencies for a given entropy value
        auto calcFilterFreqs = [&](float entropy) -> std::pair<float, float> {
            // HPF rises with entropy (removes lows = "evaporating")
            // LPF falls with entropy (removes highs = "fading")
            float hpfHz = Disintegration::kHpfStartHz + 
                          entropy * (Disintegration::kHpfEndHz - Disintegration::kHpfStartHz);
            float lpfHz = Disintegration::kLpfStartHz - 
                          entropy * (Disintegration::kLpfStartHz - Disintegration::kLpfEndHz);
            
            // Focus effect is ALWAYS audible, not just with entropy
            if (focus < 0.0f)
            {
                // Ghost mode: thin out bass even at low entropy
                const float ghostHpf = Disintegration::kFocusBaseHpfHz + 
                                       focusAmount * (Disintegration::kHpfEndHz - Disintegration::kFocusBaseHpfHz);
                hpfHz = std::max(hpfHz, ghostHpf);
            }
            else if (focus > 0.0f)
            {
                // Fog mode: cut highs even at low entropy
                const float fogLpf = Disintegration::kLpfStartHz - 
                                     focusAmount * (Disintegration::kLpfStartHz - Disintegration::kFocusBaseLpfHz);
                lpfHz = std::min(lpfHz, fogLpf);
            }
            
            // Clamp to safe range
            hpfHz = juce::jlimit(20.0f, 5000.0f, hpfHz);
            lpfHz = juce::jlimit(500.0f, 20000.0f, lpfHz);
            
            return {hpfHz, lpfHz};
        };
        
        // Calculate frequencies for L and R channels (CORRECTION #2: separate coefficients)
        auto [hpfHz_L, lpfHz_L] = calcFilterFreqs(entropyL);
        auto [hpfHz_R, lpfHz_R] = calcFilterFreqs(entropyR);
        
        // Calculate SVF coefficients (Cytomic/Vadim TPT topology)
        // g = tan(π * fc / fs)
        const float srFloat = static_cast<float>(sampleRate);
        const float hpfNorm_L = Disintegration::kPi * hpfHz_L / srFloat;
        const float lpfNorm_L = Disintegration::kPi * lpfHz_L / srFloat;
        const float hpfNorm_R = Disintegration::kPi * hpfHz_R / srFloat;
        const float lpfNorm_R = Disintegration::kPi * lpfHz_R / srFloat;
        
        currentHpfG_L = std::tan(hpfNorm_L);
        currentLpfG_L = std::tan(lpfNorm_L);
        currentHpfG_R = std::tan(hpfNorm_R);
        currentLpfG_R = std::tan(lpfNorm_R);
        
        // k = 2 - 2*res gives: res=0 → k=2 (Q=0.5), res=0.3 → k=1.4 (Q≈0.7), res=1 → k=0 (infinite Q)
        // Clamp k >= 0.1 to prevent instability at high resonance
        const float kValue = std::max(0.1f, 2.0f - 2.0f * Disintegration::kFilterResonance);
        currentHpfK_L = kValue;
        currentLpfK_L = kValue;
        currentHpfK_R = kValue;
        currentLpfK_R = kValue;
        
        // Saturation amount scales with average entropy (warm blanket feeling)
        // Focus: Fog (right) = more saturation (1.6x), Ghost (left) = less (0.4x)
        const float avgEntropy = (entropyL + entropyR) * 0.5f;
        const float satFocusScale = 0.4f + focusNormBlock * 1.2f;  // 0.4x at Ghost, 1.6x at Fog
        currentSatAmount = (Disintegration::kSaturationMin + 
                          avgEntropy * (Disintegration::kSaturationMax - Disintegration::kSaturationMin))
                          * satFocusScale;
    }
    
    // Pre-calculate input gate threshold
    const float inputGateThreshold = juce::Decibels::decibelsToGain(Disintegration::kInputGateThresholdDb);
    
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
        
        // Calculate tone coefficient from smoothed value
        const float toneCoef = juce::jmap(currentTone, -1.0f, 1.0f, 0.1f, 0.9f);
        
        // Get smoothed puckX-derived values (eliminates block-rate crackling)
        const float currentErGain = erGainSmoother.getNextValue();
        const float currentFdnSend = fdnSendSmoother.getNextValue();
        const float currentDuckAmount = duckAmountSmoother.getNextValue();
        
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
            
            // ALWAYS advance pre-delay smoother to keep it in sync (even when ERs are silent)
            // This prevents sudden jumps when ERs become audible again
            const float preDelaySamples = preDelaySmoother.getNextValue() * 0.001f * static_cast<float>(sampleRate);
            
            // Only process taps when ER gain is significant (optimization)
            if (currentErGain > 0.001f)
            {
                // Sum all taps for left and right channels with interpolated reads
                for (std::size_t tap = 0; tap < threadbare::tuning::EarlyReflections::kNumTaps; ++tap)
                {
                    const float tapGain = threadbare::tuning::EarlyReflections::kTapGains[tap];
                    
                    // Calculate floating-point read positions (base offset + smoothed pre-delay)
                    float readPosL = static_cast<float>(erWriteHead) - (erBaseTapOffsetsL[tap] + preDelaySamples);
                    float readPosR = static_cast<float>(erWriteHead) - (erBaseTapOffsetsR[tap] + preDelaySamples);
                    
                    // Wrap to buffer bounds
                    while (readPosL < 0.0f) readPosL += static_cast<float>(erBufSize);
                    while (readPosR < 0.0f) readPosR += static_cast<float>(erBufSize);
                    
                    // Linear interpolation for smooth transitions
                    const int idxL0 = static_cast<int>(readPosL) % erBufSize;
                    const int idxL1 = (idxL0 + 1) % erBufSize;
                    const float fracL = readPosL - std::floor(readPosL);
                    const float sampleL = erBufferL[static_cast<std::size_t>(idxL0)] * (1.0f - fracL) 
                                        + erBufferL[static_cast<std::size_t>(idxL1)] * fracL;
                    
                    const int idxR0 = static_cast<int>(readPosR) % erBufSize;
                    const int idxR1 = (idxR0 + 1) % erBufSize;
                    const float fracR = readPosR - std::floor(readPosR);
                    const float sampleR = erBufferR[static_cast<std::size_t>(idxR0)] * (1.0f - fracR) 
                                        + erBufferR[static_cast<std::size_t>(idxR1)] * fracR;
                    
                    erOutputL += sampleL * tapGain;
                    erOutputR += sampleR * tapGain;
                }
                
                // Apply ER gain (proximity control) - now smoothed per-sample
                erOutputL *= currentErGain;
                erOutputR *= currentErGain;
            }
            
            // ALWAYS advance write head (ring buffer semantics)
            ++erWriteHead;
            if (erWriteHead >= erBufSize)
                erWriteHead = 0;
        }
        
        // B. Record input into Ghost History (before glitch processing)
        const float originalGainedInput = monoInput;
        
        if (!ghostHistory.empty())
        {
            ghostHistory[static_cast<std::size_t>(ghostWriteHead)] = originalGainedInput;
            ++ghostWriteHead;
            if (ghostWriteHead >= static_cast<int>(ghostHistory.size()))
                ghostWriteHead = 0;
        }
        
        // B2. GLITCH LOOPER - moved to final output stage for bypass effect
        // (Glitch now ducks the entire mix, not just the input)
        float glitchOutL = 0.0f, glitchOutR = 0.0f;
        if (glitchAmount > 0.01f) {
            processGlitchLooper(glitchOutL, glitchOutR, glitchAmount, safeGlitchTempo, puckX, puckY);
        }
        
        // Use original input for downstream processing (glitch applied at output)
        const float gainedInput = originalGainedInput;
        
        // C. Spawn grains at high density for smooth overlap (can be disabled via debug switch)
        float ghostOutputL = 0.0f;
        float ghostOutputR = 0.0f;
        
        if constexpr (threadbare::tuning::Debug::kEnableGhostEngine)
        {
            ++samplesSinceLastSpawn;
            
            if (samplesSinceLastSpawn >= effectiveSpawnInterval && currentGhost > 0.01f)
            {
                // Spawn probability scaled by ghost amount
                const float effectiveProb = currentGhost * spawnProb;
                
                if (ghostRng.nextFloat() < effectiveProb)
                    trySpawnGrain(currentGhost, puckX);
                
                samplesSinceLastSpawn = 0;
            }
            
            // D. Process Ghost Engine (stereo output)
            processGhostEngine(currentGhost, ghostOutputL, ghostOutputR);
        }
        
        // E. Mix dry + ghost + ERs into FDN input with proximity control
        //    fdnSend: 0.2 (Left/Physical) → 1.0 (Right/Ethereal)
        //    At Left: ERs dominate, less dry signal to FDN
        //    At Right: Full dry signal + ghost to FDN, no ERs
        // Scale down ghost sum based on expected density to prevent input clipping
        // Multiple overlapping grains can create massive peaks before hitting the FDN
        // With 8 grains at -12dB each, max sum = 2.0; headroom 0.35 brings to ~0.7
        constexpr float kGhostHeadroom = 0.35f;
        
        // Apply debug ghost injection gain (0dB normal, -6 or -12 for testing)
        const float ghostDebugGain = juce::Decibels::decibelsToGain(
            threadbare::tuning::Debug::kGhostInjectionGainDb);
        const float ghostMono = 0.5f * (ghostOutputL + ghostOutputR) * kGhostHeadroom * ghostDebugGain;
        const float erMono = 0.5f * (erOutputL + erOutputR);
        
        // E. Mix dry + ghost + ERs into FDN input with proximity control
        // NOTE: Ghost output is already gain-scaled per grain at spawn time (based on ghostAmount)
        //       so we don't multiply by currentGhost again here (was causing double-gain/clipping)
        const float fdnInputRaw = 
            (gainedInput * currentFdnSend) +  // Now smoothed per-sample
            ghostMono + 
            (erMono * threadbare::tuning::EarlyReflections::kErInjectionGain);
        
        // Soft-limit FDN input to prevent feedback runaway with high ghost + high decay
        // This catches peaks before they enter the recirculating feedback loop
        // Can be disabled via debug switch to test for aliasing
        float fdnInput = fdnInputRaw;
        if constexpr (threadbare::tuning::Debug::kEnableFdnInputLimiting)
        {
            // Apply internal headroom: scale down before tanh, scale up after
            // This reduces saturation/aliasing on loud transients
            // 6dB headroom = 0.5x before tanh, 2x after
            const float headroomGain = juce::Decibels::decibelsToGain(
                -threadbare::tuning::Debug::kInternalHeadroomDb);
            const float headroomCompensation = juce::Decibels::decibelsToGain(
                threadbare::tuning::Debug::kInternalHeadroomDb);
            fdnInput = std::tanh(fdnInputRaw * headroomGain) * headroomCompensation;
        }
        
        // Step A: Read from all 8 delay lines with modulation
        // TAPE WARP MAGIC: currentSize changes smoothly per-sample, causing read head to slide
        for (std::size_t i = 0; i < kNumLines; ++i)
        {
            // Update LFO phase - no random jitter, just the base rate
            // Random jitter was causing subtle grain artifacts in the tail
            lfoPhases[i] += lfoInc[i];
            if (lfoPhases[i] >= kTwoPi) lfoPhases[i] -= kTwoPi;
            if (lfoPhases[i] < 0.0f) lfoPhases[i] += kTwoPi;
            
            // Calculate modulation offset (can be disabled via debug switch)
            float modOffset = 0.0f;
            if constexpr (threadbare::tuning::Debug::kEnableDelayModulation)
            {
                modOffset = fastSin(lfoPhases[i]) * driftAmount;
            }
            
            // Calculate read position with smoothly changing size (creates pitch shift!)
            // As currentSize changes, this read position slides, creating Doppler effect
            float readPos = static_cast<float>(writeIndices[i]) - (baseDelayOffsetsSamples[i] * currentSize) + modOffset;
            
            // Read with cubic interpolation (essential for smooth sliding!)
            readOutputs[i] = readDelayInterpolated(i, readPos);
        }
        
        // Step B: The Matrix - Householder-style mixing with per-line feedback
        float sumOfReads = 0.0f;
        for (std::size_t i = 0; i < kNumLines; ++i)
            sumOfReads += readOutputs[i];
        
        // Householder-style FDN mixing (intentionally lossy for natural decay)
        constexpr float mixCoeff = -0.2f;

        for (std::size_t i = 0; i < kNumLines; ++i)
        {
            // Get smoothed per-line feedback (eliminates stepping when decay/puckY changes)
            const float smoothedLineFeedback = lineFeedbackSmoothers[i].getNextValue();
            
            // Householder-style mixing
            const float crossMix = sumOfReads * mixCoeff + readOutputs[i];
            
            nextInputs[i] = fdnInput + (crossMix * smoothedLineFeedback);
        }
        
        // Step C: Apply damping and write with safety clipping
        // Using coefficient interpolation for smooth freeze transitions (no output blending)
        for (std::size_t i = 0; i < kNumLines; ++i)
        {
            float processedSample = nextInputs[i];
            
            // EQ/Tone filtering (can be disabled via debug switch)
            if constexpr (threadbare::tuning::Debug::kEnableEqAndDuck)
            {
                // Low-pass filter for tone control
                lpState[i] += (nextInputs[i] - lpState[i]) * toneCoef;
                
                // High-pass filter to prevent mud buildup
                constexpr float hpCoef = 0.006f;
                hpState[i] += (lpState[i] - hpState[i]) * hpCoef;
                
                // NOTE: Anti-denormal additive noise REMOVED from feedback path
                // ScopedNoDenormals at function start + FTZ/DAZ handles denormals
                
                processedSample = lpState[i] - hpState[i];
            }
            
            // Soft limit - tanh provides smooth saturation (can be disabled via debug switch)
            // With internal headroom to reduce aliasing on loud transients
            float limitedSample = processedSample;
            if constexpr (threadbare::tuning::Debug::kEnableFeedbackNonlinearity)
            {
                // Apply headroom: scale down before tanh (reduces harmonic generation),
                // then scale back up. Old: 0.8f drive, 1.25f makeup
                // With 6dB headroom: effectively 0.4f drive (less saturation)
                const float headroomGain = juce::Decibels::decibelsToGain(
                    -threadbare::tuning::Debug::kInternalHeadroomDb);
                const float driveWithHeadroom = 0.8f * headroomGain;
                const float makeupWithHeadroom = 1.25f / headroomGain;
                limitedSample = std::tanh(processedSample * driveWithHeadroom) * makeupWithHeadroom;
            }
            
            delayLines[i][static_cast<std::size_t>(writeIndices[i])] = limitedSample;
            
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
        
        // Scale wet signal (reduced from 0.5 to 0.35 for more headroom)
        constexpr float wetScale = 0.35f;
        wetL *= wetScale;
        wetR *= wetScale;
        
        // ═══════════════════════════════════════════════════════════════════════
        // DISINTEGRATION LOOPER PER-SAMPLE PROCESSING
        // ═══════════════════════════════════════════════════════════════════════
        
        if (currentLooperState == LooperState::Recording && !disintLoopL.empty())
        {
            // === RECORDING STATE ===
            // Record immediately - no input gate, captures whatever is on the track
            // (live input, MIDI playback, audio clips, etc.)
            inputDetected = true;  // Always consider input detected
            
            // === TRANSPORT-AWARE CANCEL ===
            // If DAW transport stops during recording, cancel and return to Idle
            if (!state.isPlaying)
            {
                currentLooperState = LooperState::Idle;
                loopRecordHead = 0;
                actualLoopLength = 0;
                transportWasPlaying = true;
                state.looperState = currentLooperState;
            }
            
            if (loopRecordHead < targetLoopLength)
            {
                // Capture mix: ensure minimum wet content for character
                const float captureMix = std::max(currentMix, Disintegration::kMinCaptureWetMix);
                float captureL = inputL * (1.0f - captureMix) + wetL * captureMix;
                float captureR = inputR * (1.0f - captureMix) + wetR * captureMix;
                
                // === S-CURVE CROSSFADE at loop boundaries (matches playback crossfade) ===
                // Use same S-curve as playback to ensure crossfade regions align perfectly
                float recordCrossfadeGain = 1.0f;
                
                // Fade in at the start
                if (loopRecordHead < crossfadeSamples)
                {
                    const float linearFade = static_cast<float>(loopRecordHead) / static_cast<float>(crossfadeSamples);
                    recordCrossfadeGain = std::sin(linearFade * threadbare::tuning::Disintegration::kPi * 0.5f);
                }
                // Fade out at the end (only if we're near target length)
                else
                {
                    const int samplesFromEnd = targetLoopLength - loopRecordHead;
                    if (samplesFromEnd < crossfadeSamples && samplesFromEnd > 0)
                    {
                        const float linearFade = static_cast<float>(samplesFromEnd) / static_cast<float>(crossfadeSamples);
                        recordCrossfadeGain = std::sin(linearFade * threadbare::tuning::Disintegration::kPi * 0.5f);
                    }
                }
                
                captureL *= recordCrossfadeGain;
                captureR *= recordCrossfadeGain;
                
                // Write to buffer
                disintLoopL[static_cast<std::size_t>(loopRecordHead)] = captureL;
                disintLoopR[static_cast<std::size_t>(loopRecordHead)] = captureR;
                loopRecordHead++;
                }
                
            // Update progress for UI (protect against division by zero)
            state.loopProgress = (targetLoopLength > 0) 
                ? static_cast<float>(loopRecordHead) / static_cast<float>(targetLoopLength)
                : 0.0f;
        }
        else if (currentLooperState == LooperState::Looping && actualLoopLength > 0)
        {
            // === LOOPING STATE with Disintegration + Phase 3 Physical Degradation ===
            
            // === TRANSPORT-AWARE SHUTDOWN ===
            // Detect when DAW transport stops and begin graceful fade
            if (!state.isPlaying && transportWasPlaying)
            {
                // Transport just stopped - begin 2-second fade
                transportFadeAmount = 1.0f;
            }
            transportWasPlaying = state.isPlaying;
            
            // Get smoothed loop gain
            const float loopGain = loopGainSmoother.getNextValue();
            
            // === ENTROPY ACCUMULATION (puckY controls rate) ===
            // Rate is based on LOOP ITERATIONS, not wall-clock time
            // puckY +1.0 (top) = fast (~2 loops to full entropy)
            // puckY  0.0 (mid) = moderate (~20 loops)
            // puckY -1.0 (bottom) = practically endless (~10000 loops)
            // Use exponential curve: top half degrades normally, bottom half becomes endless
            const float normalizedY = (puckY + 1.0f) * 0.5f;  // 0 (bottom) to 1 (top)
            const float curvedY = normalizedY * normalizedY;   // Quadratic: more resolution at top
            const float targetLoops = Disintegration::kEntropyLoopsMax * 
                                      std::pow(Disintegration::kEntropyLoopsMin / Disintegration::kEntropyLoopsMax, curvedY);
            // Entropy rate = 1.0 / (loopLength * targetLoops) so full entropy after targetLoops iterations
            const float entropyRate = 1.0f / (static_cast<float>(actualLoopLength) * targetLoops);
            entropyAmount = std::min(1.0f, entropyAmount + entropyRate);
            const float currentEntropy = entropyAmount;  // Cache for this sample
            
            // === FOCUS (puckX) controls Phase 3 character ===
            // Left (Ghost/Spectral): More dropouts, less pitch drag, wider stereo tear
            // Right (Fog/Diffuse): Fewer dropouts, more pitch drag, narrower stereo
            // Use smoothed value to prevent zipper noise when moving puck
            focusSmoother.setTargetValue(puckX);
            const float focus = focusSmoother.getNextValue();  // -1 to +1 smoothed
            const float focusNorm = (focus + 1.0f) * 0.5f;  // 0 (Ghost) to 1 (Fog)
            
            // ═══════════════════════════════════════════════════════════════
            // PHASE 3B: MOTOR DEATH (Asymmetric Pitch Drag via Brownian Noise)
            // Modulates the read head position with downward-biased random walk
            // Focus: Fog (right) = more drag, Ghost (left) = less drag
            // ═══════════════════════════════════════════════════════════════
            
            // Focus scales motor drag: 0.3x at Ghost, 1.7x at Fog
            const float motorFocusScale = 0.3f + focusNorm * 1.4f;
            
            // Update Brownian walk (per channel for stereo width)
            const float dragStep = Disintegration::kMotorDragStepSize * motorFocusScale;
            const float dragInertia = Disintegration::kMotorDragInertia;
            const float dragBias = Disintegration::kMotorDragBias;
            
            // Stereo divergence: L and R channels have slightly different biases
            // This causes them to drift apart over time, creating stereo width
            const float stereoDivergence = Disintegration::kMotorStereoDivergence * currentEntropy;
            const float biasL = dragBias - stereoDivergence * 0.3f;  // L tends slightly higher
            const float biasR = dragBias + stereoDivergence * 0.3f;  // R tends slightly lower
            
            // Random step + channel-specific bias (motor struggling asymmetrically)
            motorDragValueL = dragInertia * motorDragValueL + 
                              (1.0f - dragInertia) * (fastRandBipolar() + biasL) * dragStep * 100.0f;
            motorDragValueR = dragInertia * motorDragValueR + 
                              (1.0f - dragInertia) * (fastRandBipolar() + biasR) * dragStep * 100.0f;
            
            // Clamp to prevent runaway
            motorDragValueL = std::clamp(motorDragValueL, -1.0f, 1.0f);
            motorDragValueR = std::clamp(motorDragValueR, -1.0f, 1.0f);
            
            // ═══════════════════════════════════════════════════════════════
            // TAPE SHUTTLE EFFECT: Pitch sag at loop boundary
            // Simulates reel momentum - brief pitch drop when approaching loop point
            // ═══════════════════════════════════════════════════════════════
            float tapeShuttlePitchCents = 0.0f;
            const int boundaryZone = Disintegration::kLoopBoundaryTransitionSamples;
            
            // Near end of loop: ramp down pitch (approaching splice)
            if (loopPlayHead >= actualLoopLength - boundaryZone)
            {
                const float distFromEnd = static_cast<float>(actualLoopLength - loopPlayHead);
                const float boundaryProgress = 1.0f - (distFromEnd / static_cast<float>(boundaryZone));
                tapeShuttlePitchCents = Disintegration::kLoopBoundaryPitchDropCents * boundaryProgress;
            }
            // Near start of loop: recover from pitch sag
            else if (loopPlayHead < boundaryZone)
            {
                const float distFromStart = static_cast<float>(loopPlayHead);
                const float recoveryProgress = distFromStart / static_cast<float>(boundaryZone);
                tapeShuttlePitchCents = Disintegration::kLoopBoundaryPitchDropCents * (1.0f - recoveryProgress);
            }
            
            // Smooth the tape shuttle modulation (prevents clicks)
            // Use faster smoothing: ~0.5ms time constant to track boundary transitions
            const float tapeShuttleSmoothCoef = 1.0f - std::exp(-1.0f / (0.5f * 0.001f * static_cast<float>(sampleRate)));
            loopBoundaryPitchMod += tapeShuttleSmoothCoef * (tapeShuttlePitchCents - loopBoundaryPitchMod);
            
            // === WOW & FLUTTER MODULATION ===
            // Authentic tape transport wobble that increases with entropy
            float wowMod = std::sin(wowPhase) * Disintegration::kWowDepthCents * currentEntropy;
            float flutterMod = std::sin(flutterPhase) * Disintegration::kFlutterDepthCents * currentEntropy;
            wowPhase += wowPhaseInc;
            flutterPhase += flutterPhaseInc;
            if (wowPhase > 2.0f * Disintegration::kPi) wowPhase -= 2.0f * Disintegration::kPi;
            if (flutterPhase > 2.0f * Disintegration::kPi) flutterPhase -= 2.0f * Disintegration::kPi;
            
            // Convert cents deviation to speed ratio: 2^(cents/1200)
            // Scale by entropy so effect intensifies over time
            // Focus also scales max cents: more at Fog, less at Ghost
            const float maxCents = Disintegration::kMotorDragMaxCents * currentEntropy * motorFocusScale;
            // Fast approximation: 2^x ≈ 1 + 0.693*x for small x (cents/1200 < 0.03)
            // Include tape shuttle + wow + flutter pitch modulation
            const float centsL = motorDragValueL * maxCents + loopBoundaryPitchMod + wowMod + flutterMod;
            const float centsR = motorDragValueR * maxCents + loopBoundaryPitchMod + wowMod + flutterMod;
            const float xL = centsL / 1200.0f;
            const float xR = centsR / 1200.0f;
            const float speedRatioL = 1.0f + 0.693147f * xL + 0.240226f * xL * xL;
            const float speedRatioR = 1.0f + 0.693147f * xR + 0.240226f * xR * xR;
                
            // Accumulate fractional read offset
            motorDragReadOffsetL += (speedRatioL - 1.0f);
            motorDragReadOffsetR += (speedRatioR - 1.0f);
            
            // CORRECTION #3: Wrap offset immediately to prevent precision loss
            const float loopLenF = static_cast<float>(actualLoopLength);
            if (motorDragReadOffsetL > loopLenF) motorDragReadOffsetL -= loopLenF;
            if (motorDragReadOffsetL < -loopLenF) motorDragReadOffsetL += loopLenF;
            if (motorDragReadOffsetR > loopLenF) motorDragReadOffsetR -= loopLenF;
            if (motorDragReadOffsetR < -loopLenF) motorDragReadOffsetR += loopLenF;
                    
            // Calculate modulated read positions
            const float readPosL = static_cast<float>(loopPlayHead) + motorDragReadOffsetL;
            const float readPosR = static_cast<float>(loopPlayHead) + motorDragReadOffsetR;
                    
            // Wrap positions (handle negative values correctly)
            const float wrappedPosL = std::fmod(std::fmod(readPosL, loopLenF) + loopLenF, loopLenF);
            const float wrappedPosR = std::fmod(std::fmod(readPosR, loopLenF) + loopLenF, loopLenF);
                    
            // Hermite 4-point interpolation for smooth pitch modulation (safer than linear)
            const int idxL0 = static_cast<int>(wrappedPosL);
            const float fracL = wrappedPosL - std::floor(wrappedPosL);
            
            const int idxR0 = static_cast<int>(wrappedPosR);
            const float fracR = wrappedPosR - std::floor(wrappedPosR);
            
            // Safe buffer reads with index wrapping for Hermite (needs y[-1], y[0], y[1], y[2])
            auto getSafeL = [&](int idx) { 
                return disintLoopL[static_cast<size_t>(wrapIndex(idx, actualLoopLength))]; 
            };
            auto getSafeR = [&](int idx) { 
                return disintLoopR[static_cast<size_t>(wrapIndex(idx, actualLoopLength))]; 
            };
            
            float disintL = hermite4(fracL, 
                                     getSafeL(idxL0 - 1), getSafeL(idxL0), 
                                     getSafeL(idxL0 + 1), getSafeL(idxL0 + 2));
            float disintR = hermite4(fracR, 
                                     getSafeR(idxR0 - 1), getSafeR(idxR0), 
                                     getSafeR(idxR0 + 1), getSafeR(idxR0 + 2));
            
            // ═══════════════════════════════════════════════════════════════
            // PHASE 3C: OXIDE SHEDDING (Stochastic Dropouts)
            // Random gain cuts that become more frequent as entropy increases
            // CORRECTION #1: Timer-based trigger instead of per-sample
            // Focus: Ghost (left) = more dropouts, Fog (right) = fewer dropouts
            // ═══════════════════════════════════════════════════════════════
            
            // Focus scales oxide dropout: 2.5x probability at Ghost, 0.2x at Fog
            // More dramatic difference for noticeable puck X response
            const float oxideFocusScale = 2.5f - focusNorm * 2.3f;
            
            // Only check for new dropout on timer intervals (~40ms)
            oxideCheckTimer++;
            if (oxideCheckTimer >= Disintegration::kOxideCheckIntervalSamples)
            {
                oxideCheckTimer = 0;
                
                // Only consider dropout if not already in one
                if (oxideDropoutCounter <= 0)
                {
                    // Probability scales with entropy AND focus
                    const float dropoutProbability = currentEntropy * Disintegration::kOxideDropoutProbabilityMax * oxideFocusScale;
                    const float rand = fastRand01();
                    
                    if (rand < dropoutProbability)
                    {
                        // Trigger a dropout!
                        oxideGainTarget = 0.0f;
                        // Duration scales with entropy (longer dropouts at higher entropy)
                        const float durationMs = Disintegration::kOxideDropoutDurationMs * (0.3f + 0.7f * currentEntropy);
                        oxideDropoutCounter = static_cast<int>(durationMs * 0.001f * static_cast<float>(sampleRate));
                    }
                }
            }
                
            // Count down dropout duration
            if (oxideDropoutCounter > 0)
            {
                oxideDropoutCounter--;
                if (oxideDropoutCounter <= 0)
                {
                    oxideGainTarget = 1.0f;  // Dropout ended, restore gain
                }
            }
            
            // Smooth the gain transition (prevents clicks - soft "gasp")
            oxideGainL += oxideGainSmoothCoef * (oxideGainTarget - oxideGainL);
            oxideGainR += oxideGainSmoothCoef * (oxideGainTarget - oxideGainR);
            
            // Apply dropout gain
            disintL *= oxideGainL;
            disintR *= oxideGainR;
            
            // === ASCENSION FILTER (sound gets thinner as entropy increases) ===
            // PHASE 3: Uses separate L/R coefficients for stereo decoupling
            if (currentEntropy > 0.05f && currentHpfG_L > 0.0001f && currentLpfG_L > 0.0001f)
            {
                // High-pass: removes bass, creates "evaporating" feel
                disintL = processSvfHp(disintL, hpfSvfL, currentHpfG_L, currentHpfK_L);
                disintR = processSvfHp(disintR, hpfSvfR, currentHpfG_R, currentHpfK_R);
                
                // Low-pass: removes highs, creates "fading" feel
                disintL = processSvfLp(disintL, lpfSvfL, currentLpfG_L, currentLpfK_L);
                disintR = processSvfLp(disintR, lpfSvfR, currentLpfG_R, currentLpfK_R);
            }
            
            // === HYSTERESIS + ADAA SATURATION (increases with entropy) ===
            // Chain: Hysteresis (magnetic memory) → ADAA (anti-aliased saturation)
            if (currentSatAmount > 0.01f)
            {
                const float drive = 1.0f + currentSatAmount * 2.0f;
                const float makeup = 1.0f / (1.0f + currentSatAmount);
                
                // Stage 1: Hysteresis (asymmetric tape saturation with memory)
                disintL = hysteresis(disintL * drive, hysteresisMagL);
                disintR = hysteresis(disintR * drive, hysteresisMagR);
                
                // Stage 2: ADAA saturation (anti-aliased tanh)
                disintL = adaaFastTanh(disintL, adaaX1L) * makeup;
                disintR = adaaFastTanh(disintR, adaaX1R) * makeup;
            }
            
            // ═══════════════════════════════════════════════════════════════
            // BUFFER DEGRADATION: Write processed audio back to the buffer
            // This creates authentic "disintegration loops" behavior where
            // each pass through the loop degrades the stored audio permanently.
            // The amount of degradation scales with entropy.
            // ═══════════════════════════════════════════════════════════════
            if (currentEntropy > 0.1f)  // Start degrading after some entropy accumulates
            {
                // Degradation amount: how much of the processed audio replaces original
                // Starts subtle (10%) and increases with entropy (up to 50%)
                const float degradeAmount = juce::jmap(currentEntropy, 0.1f, 1.0f, 0.1f, 0.5f);
                
                // Only apply degradation in the middle of the loop (not at crossfade boundaries)
                // This prevents clicks from accumulated boundary processing
                // Use soft transition at boundary edges to prevent clicks
                const int safeMargin = crossfadeSamples * 3;  // Extra margin for safety
                const int safeTransition = crossfadeSamples;  // Smooth transition zone
                
                float safeZoneFade = 1.0f;
                if (loopPlayHead < safeMargin)
                {
                    // Fade in at start of safe zone
                    safeZoneFade = std::max(0.0f, static_cast<float>(loopPlayHead - crossfadeSamples * 2) / static_cast<float>(safeTransition));
                }
                else if (loopPlayHead > actualLoopLength - safeMargin)
                {
                    // Fade out at end of safe zone
                    safeZoneFade = std::max(0.0f, static_cast<float>(actualLoopLength - crossfadeSamples * 2 - loopPlayHead) / static_cast<float>(safeTransition));
                }
                
                if (safeZoneFade > 0.0f)
                {
                    // Apply degradation with smooth fade at boundaries
                    const float effectiveDegradeAmount = degradeAmount * safeZoneFade;
                    const int writeIdx = loopPlayHead;
                    disintLoopL[static_cast<std::size_t>(writeIdx)] = 
                        disintLoopL[static_cast<std::size_t>(writeIdx)] * (1.0f - effectiveDegradeAmount) + 
                        disintL * effectiveDegradeAmount;
                    disintLoopR[static_cast<std::size_t>(writeIdx)] = 
                        disintLoopR[static_cast<std::size_t>(writeIdx)] * (1.0f - effectiveDegradeAmount) + 
                        disintR * effectiveDegradeAmount;
                }
            }
            
            // === S-CURVE CROSSFADE at loop boundary (equal power, seamless) ===
            // IMPORTANT: Use motor-dragged read positions for crossfade, not loopPlayHead
            // This ensures the crossfade matches where we're actually reading from
            float crossfadeGainL = 1.0f;
            float crossfadeGainR = 1.0f;
            const float crossfadeSamplesF = static_cast<float>(crossfadeSamples);
            const float loopLengthF = static_cast<float>(actualLoopLength);
            
            if (actualLoopLength > crossfadeSamples * 2)
            {
                // Lambda to calculate crossfade gain based on position
                auto calcCrossfadeGain = [&](float wrappedPos) -> float {
                    if (wrappedPos < crossfadeSamplesF)
                    {
                        // Fade in at start with S-curve
                        const float linearFade = wrappedPos / crossfadeSamplesF;
                        return std::sin(linearFade * Disintegration::kPi * 0.5f);
                    }
                    else if (wrappedPos >= loopLengthF - crossfadeSamplesF)
                    {
                        // Fade out at end with S-curve
                        const float samplesFromEnd = loopLengthF - wrappedPos;
                        const float linearFade = samplesFromEnd / crossfadeSamplesF;
                        return std::sin(linearFade * Disintegration::kPi * 0.5f);
                    }
                    return 1.0f;
                };
                
                crossfadeGainL = calcCrossfadeGain(wrappedPosL);
                crossfadeGainR = calcCrossfadeGain(wrappedPosR);
            }
                
            // === GRADUAL FADE based on entropy ===
            // As entropy increases 0→1, volume decreases smoothly
            // Using a curve that keeps most volume until later in the cycle
            const float entropyFade = 1.0f - (currentEntropy * currentEntropy);  // Quadratic curve
            
            // Apply loop gain with per-channel crossfade and entropy fade
            disintL *= loopGain * crossfadeGainL * entropyFade;
            disintR *= loopGain * crossfadeGainR * entropyFade;
            
            // === TRANSPORT FADE: Smooth fade when DAW stops ===
            if (!state.isPlaying)
            {
                // Fade out over 2 seconds when transport is stopped
                const float transportFadeRate = 1.0f / (2.0f * static_cast<float>(sampleRate));
                transportFadeAmount = std::max(0.0f, transportFadeAmount - transportFadeRate);
                
                // Apply transport fade to output
                disintL *= transportFadeAmount;
                disintR *= transportFadeAmount;
                
                // Return to Idle when fade completes
                if (transportFadeAmount <= 0.0f)
                {
                    currentLooperState = LooperState::Idle;
                    loopRecordHead = 0;
                    loopPlayHead = 0;
                    actualLoopLength = 0;
                    entropyAmount = 0.0f;
                    exitFadeAmount = 1.0f;
                    transportFadeAmount = 1.0f;
                    transportWasPlaying = true;
                }
            }
            else
            {
                // Transport is playing - reset fade amount
                transportFadeAmount = 1.0f;
            }
                
            // === EXIT BEHAVIOR: Return to Idle when entropy reaches 1.0 ===
            if (entropyAmount >= 1.0f)
            {
                // Give a brief moment at zero before returning to idle
                const float fadeRate = 1.0f / (Disintegration::kFadeToReverbSeconds * static_cast<float>(sampleRate));
                exitFadeAmount = std::max(0.0f, exitFadeAmount - fadeRate);
                
                if (exitFadeAmount <= 0.0f)
                {
                    currentLooperState = LooperState::Idle;
                    loopRecordHead = 0;
                    loopPlayHead = 0;
                    actualLoopLength = 0;
                    entropyAmount = 0.0f;
                    exitFadeAmount = 1.0f;
                    transportFadeAmount = 1.0f;
                    transportWasPlaying = true;
                }
            }
            
            // === PINK NOISE FLOOR (constant tape hiss, felt not heard) ===
            // Increment loop entry counter for fade-in
            loopEntrySamples++;
            
            // Entry fade: 500ms fade-in to prevent click when loop engages
            const int entryFadeSamples = static_cast<int>(Disintegration::kNoiseEntryFadeMs * 0.001f * static_cast<float>(sampleRate));
            const float entryFade = std::min(1.0f, static_cast<float>(loopEntrySamples) / static_cast<float>(entryFadeSamples));
            
            // Floor + Ramp model: starts at 50%, rises to 100% at max entropy
            const float noiseGain = Disintegration::kNoiseFloorBaseGain + 
                                    ((1.0f - Disintegration::kNoiseFloorBaseGain) * currentEntropy);
            const float noiseLevel = Disintegration::kNoiseFloorMaxLevel * noiseGain * entryFade;
            
            if (noiseLevel > 0.00001f)
            {
                // Generate pink noise for each channel
                float noiseL = generatePinkNoise() * noiseLevel;
                float noiseR = generatePinkNoise() * noiseLevel;
                
                // High-pass filter to remove low-end rumble (~300Hz), makes it "hiss" not "rumble"
                noiseHpfStateL += Disintegration::kNoiseHpfCoef * (noiseL - noiseHpfStateL);
                noiseHpfStateR += Disintegration::kNoiseHpfCoef * (noiseR - noiseHpfStateR);
                noiseL -= noiseHpfStateL;
                noiseR -= noiseHpfStateR;
                
                disintL += noiseL;
                disintR += noiseR;
            }
            
            // === DC BLOCKER (removes low-frequency drift) ===
            disintL = dcBlock(disintL, dcBlockerX1L, dcBlockerY1L);
            disintR = dcBlock(disintR, dcBlockerX1R, dcBlockerY1R);
            
            // === SOFT CLIP (prevents digital overs) ===
            disintL = softClip(disintL);
            disintR = softClip(disintR);
        
            // === NaN PROTECTION ===
            if (std::isnan(disintL) || std::isinf(disintL)) disintL = 0.0f;
            if (std::isnan(disintR) || std::isinf(disintR)) disintR = 0.0f;
            
            // MIX loop INTO reverb (additive)
            wetL += disintL;
            wetR += disintR;
            
            // Advance play head
            loopPlayHead++;
            if (loopPlayHead >= actualLoopLength)
                loopPlayHead = 0;
            
            // Update UI state
            state.loopProgress = (actualLoopLength > 0)
                ? static_cast<float>(loopPlayHead) / static_cast<float>(actualLoopLength)
                : 0.0f;
        }
        
        // Update looper state for UI
        state.looperState = currentLooperState;
        state.entropy = entropyAmount;
        
        // BUG FIX 2: Implement ducking (sidechain-style) - can be disabled via debug switch
        if constexpr (threadbare::tuning::Debug::kEnableEqAndDuck)
        {
            // Envelope follower on input signal
            const float duckTarget = std::abs(monoInput);
            constexpr float duckAttackCoeff = 0.9990f;  // Fast attack (~10ms)
            constexpr float duckReleaseCoeff = 0.9995f; // Slower release (~250ms)
            const float duckCoeff = (duckTarget > duckingEnvelope) ? duckAttackCoeff : duckReleaseCoeff;
            duckingEnvelope = duckTarget + duckCoeff * (duckingEnvelope - duckTarget);
            
            // Apply ducking to wet signal (now uses smoothed duckAmount per-sample)
            float duckGain = 1.0f - (currentDuckAmount * duckingEnvelope);
            duckGain = juce::jlimit(threadbare::tuning::Ducking::kMinWetFactor, 1.0f, duckGain);
            
            wetL *= duckGain;
            wetR *= duckGain;
        }
        
        // Mix: Dry + Wet FDN + Early Reflections
        // Soft limit ER output to prevent tap stacking distortion
        const float limitedErL = std::tanh(erOutputL) * 0.8f;
        const float limitedErR = std::tanh(erOutputR) * 0.8f;
        
        const float dry = 1.0f - currentMix;
        float outL = inputL * dry + wetL * currentMix + limitedErL;
        float outR = inputR * dry + wetR * currentMix + limitedErR;
        
        // ═══════════════════════════════════════════════════════════════════════
        // GLITCH SPARKLE INJECTION (at output stage - bypasses reverb entirely)
        // Layer sparkle fragments on top of reverb - gain already applied per voice
        // ═══════════════════════════════════════════════════════════════════════
        if (glitchAmount > 0.01f) {
            // Soft limit glitch output before mixing to prevent stacking distortion
            // tanh provides gentle saturation that sounds musical
            constexpr float glitchHeadroom = 0.7f;  // Leave room for reverb
            const float limitedGlitchL = std::tanh(glitchOutL) * glitchHeadroom;
            const float limitedGlitchR = std::tanh(glitchOutR) * glitchHeadroom;
            
            outL = outL + limitedGlitchL;
            outR = outR + limitedGlitchR;
        }
        
        // Final safety: soft clip to catch any summation peaks
        // Can be disabled via debug switch to test for aliasing
        float clippedL = outL;
        float clippedR = outR;
        if constexpr (threadbare::tuning::Debug::kEnableOutputClipping)
        {
            // Apply headroom before final clipping to reduce aliasing
            const float headroomGain = juce::Decibels::decibelsToGain(
                -threadbare::tuning::Debug::kInternalHeadroomDb);
            const float headroomCompensation = juce::Decibels::decibelsToGain(
                threadbare::tuning::Debug::kInternalHeadroomDb);
            clippedL = std::tanh(outL * headroomGain) * headroomCompensation;
            clippedR = std::tanh(outR * headroomGain) * headroomCompensation;
        }
        
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

// ═══════════════════════════════════════════════════════════════════════════════
// GLITCH LOOPER IMPLEMENTATION
// Rhythmic stutter effect that captures and repeats audio slices
// ═══════════════════════════════════════════════════════════════════════════════

float UnravelReverb::readGhostHistoryInterpolated(float position) const noexcept
{
    // Read from ghostHistory buffer using Catmull-Rom interpolation
    // Per project rules in .cursorrules: use CatmullRom for warping/pitch effects
    
    const int historySize = static_cast<int>(ghostHistory.size());
    if (historySize < 4) return 0.0f;
    
    const float histSizeF = static_cast<float>(historySize);
    
    // Wrap position using subtraction (avoids fmod per-sample cost)
    while (position < 0.0f) position += histSizeF;
    while (position >= histSizeF) position -= histSizeF;
    
    // Get integer and fractional parts
    const int idx = static_cast<int>(position);
    const float frac = position - static_cast<float>(idx);
    
    // Get 4 samples for Catmull-Rom (wrap indices)
    const int i0 = (idx - 1 + historySize) % historySize;
    const int i1 = idx % historySize;
    const int i2 = (idx + 1) % historySize;
    const int i3 = (idx + 2) % historySize;
    
    const float y0 = ghostHistory[static_cast<size_t>(i0)];
    const float y1 = ghostHistory[static_cast<size_t>(i1)];
    const float y2 = ghostHistory[static_cast<size_t>(i2)];
    const float y3 = ghostHistory[static_cast<size_t>(i3)];
    
    // Catmull-Rom interpolation coefficients
    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

void UnravelReverb::processGlitchLooper(
    float& outL, float& outR,
    float glitchAmount, float safeTempo, float puckX, float puckY) noexcept
{
    using namespace threadbare::tuning;
    
    const int historySize = static_cast<int>(ghostHistory.size());
    if (historySize < 1024) {
        outL = 0.0f;
        outR = 0.0f;
        return;
    }
    
    const float histSizeF = static_cast<float>(historySize);
    const float srFloat = static_cast<float>(sampleRate);
    constexpr float kTwoPi = 6.28318530718f;
    
    // === PUCK MODULATION ===
    // Puck X (-1 to +1): Body (left) ↔ Air (right)
    //   - Body: Recent memories, warmer pitches
    //   - Air: Deep memory scrub, brighter pitches
    const float normPuckX = (puckX + 1.0f) * 0.5f;  // 0 (body) to 1 (air)
    
    // Puck Y (-1 to +1): Bottom ↔ Top
    //   - Bottom: Longer fragments, slower
    //   - Top: Shorter fragments, more active
    const float normPuckY = (puckY + 1.0f) * 0.5f;  // 0 (bottom) to 1 (top)
    
    // ═══════════════════════════════════════════════════════════════════════
    // 1. TRANSIENT DETECTION (for reactive triggering)
    // ═══════════════════════════════════════════════════════════════════════
    const float recentSample = ghostHistory[static_cast<std::size_t>(
        (ghostWriteHead - 1 + historySize) % historySize)];
    const float inputLevel = std::abs(recentSample);
    
    if (inputLevel > transientPeak) {
        transientPeak = inputLevel;
    } else {
        transientPeak *= transientReleaseCoeff;
    }
    
    const float envCoeff = (inputLevel > transientEnvelope) ? transientAttackCoeff : transientReleaseCoeff;
    transientEnvelope = inputLevel + envCoeff * (transientEnvelope - inputLevel);
    
    const float thresholdLin = juce::Decibels::decibelsToGain(GlitchLooper::kTransientThresholdDb);
    const bool isTransient = (inputLevel > thresholdLin) && 
        (transientEnvelope > 0.0001f) && 
        (transientPeak > transientEnvelope * GlitchLooper::kTransientRatio);
    
    // ═══════════════════════════════════════════════════════════════════════
    // 2. VOICE TRIGGERING
    // ═══════════════════════════════════════════════════════════════════════
    --sparkleTriggerSamples;
    
    // Count active voices
    int activeVoices = 0;
    for (const auto& voice : sparkleVoices) {
        if (voice.active) ++activeVoices;
    }
    
    // Determine max voices based on glitch amount
    const int maxActiveVoices = static_cast<int>(juce::jmap(glitchAmount,
        static_cast<float>(GlitchLooper::kVoicesAtLow),
        static_cast<float>(GlitchLooper::kVoicesAtHigh)));
    
    // Try to trigger new voice
    if (glitchAmount > 0.01f && activeVoices < maxActiveVoices && sparkleTriggerSamples <= 0) {
        // Find inactive voice
        SparkleVoice* freeVoice = nullptr;
        for (auto& voice : sparkleVoices) {
            if (!voice.active) {
                freeVoice = &voice;
                break;
            }
        }
        
        if (freeVoice != nullptr) {
            // Trigger probability: higher with transient, always some chance
            const float triggerProb = isTransient ? 0.95f : (0.3f + 0.5f * glitchAmount);
            
            if (sparkleRng.nextFloat() < triggerProb) {
                // ═══════════════════════════════════════════════════════════
                // SPAWN NEW SPARKLE VOICE
                // ═══════════════════════════════════════════════════════════
                
                // Fragment length: shorter at high glitch, modulated by puck Y
                // Puck Y: Bottom = longer (flowing), Top = shorter (granular)
                float lengthMs = juce::jmap(glitchAmount,
                    GlitchLooper::kMaxFragmentMs, GlitchLooper::kMinFragmentMs);
                lengthMs *= (2.5f - 2.0f * normPuckY);  // 0.5x at top, 2.5x at bottom (more dramatic)
                if (isTransient) {
                    lengthMs *= 0.6f;  // 40% shorter on transients (snappier)
                }
                const float lengthVariation = 0.5f + sparkleRng.nextFloat(); // 0.5-1.5x
                const float actualLengthMs = lengthMs * lengthVariation;
                freeVoice->lengthSamples = std::max(64, 
                    static_cast<int>(actualLengthMs * 0.001f * srFloat));
                
                // Memory scrubbing: random position in history buffer
                // Puck X modulates depth: Body (left) = recent, Air (right) = deep history
                const float baseScrubDepth = juce::jmap(glitchAmount,
                    GlitchLooper::kMinScrubDepth, GlitchLooper::kMaxScrubDepth);
                const float scrubDepth = baseScrubDepth * (0.1f + 0.9f * normPuckX);  // 10-100% based on puckX (dramatic)
                const float randomDepth = sparkleRng.nextFloat() * scrubDepth;
                const int safetyMargin = freeVoice->lengthSamples + 512;
                float startPos = static_cast<float>(ghostWriteHead) - 
                    static_cast<float>(safetyMargin) - 
                    randomDepth * static_cast<float>(historySize - safetyMargin);
                while (startPos < 0.0f) startPos += histSizeF;
                
                // Pitch selection (harmonic palette for sparkle)
                // On transients: bias toward higher pitches (more sparkle/attack)
                const float pitchRoll = sparkleRng.nextFloat();
                float speed = 1.0f;
                
                if (isTransient) {
                    // Transient palette: more octave up and fifth, less root/down
                    if (pitchRoll < 0.15f) {
                        speed = 1.0f;  // Root (reduced)
                    } else if (pitchRoll < 0.50f) {
                        speed = 2.0f;  // Octave up (boosted for attack)
                    } else if (pitchRoll < 0.65f) {
                        speed = 1.4983f;  // Fifth (boosted)
                    } else if (pitchRoll < 0.80f && actualLengthMs >= GlitchLooper::kMinFragmentFor4xMs) {
                        speed = 4.0f;  // 2 octaves (twinkle)
                    } else {
                        speed = 2.0f;  // Fallback to octave up
                    }
                } else {
                    // Normal palette from tuning, modulated by puck X
                    // Puck X: Body (left) biases toward warmer, Air (right) biases toward brighter
                    // Shift pitchRoll to favor different ranges based on puckX
                    const float puckXBias = (normPuckX - 0.5f) * 0.5f;  // -0.25 to +0.25 (dramatic)
                    const float biasedRoll = juce::jlimit(0.0f, 0.999f, pitchRoll + puckXBias);
                    
                    float cumProb = GlitchLooper::kRootProb;
                    if (biasedRoll < cumProb) {
                        speed = 1.0f;  // Root
                    } else if ((cumProb += GlitchLooper::kOctaveUpProb), biasedRoll < cumProb) {
                        speed = 2.0f;  // Octave up (sparkle)
                    } else if ((cumProb += GlitchLooper::kDoubleOctaveProb), biasedRoll < cumProb) {
                        if (actualLengthMs >= GlitchLooper::kMinFragmentFor4xMs) {
                            speed = 4.0f;  // 2 octaves up (twinkle)
                        } else {
                            speed = 2.0f;  // Fall back to octave up
                        }
                    } else if ((cumProb += GlitchLooper::kFifthProb), biasedRoll < cumProb) {
                        speed = 1.4983f;  // Fifth (ethereal)
                    } else if ((cumProb += GlitchLooper::kOctaveDownProb), biasedRoll < cumProb) {
                        speed = 0.5f;  // Octave down (warmth)
                    } else {
                        // Micro-shimmer (chorus-like)
                        speed = juce::jmap(sparkleRng.nextFloat(),
                            GlitchLooper::kMicroShimmerMin, GlitchLooper::kMicroShimmerMax);
                    }
                }
                
                // Occasional reverse (slightly less likely on transients)
                const float reverseProb = isTransient ? 
                    GlitchLooper::kReverseProb * 0.5f : GlitchLooper::kReverseProb;
                if (sparkleRng.nextFloat() < reverseProb) {
                    speed = -speed;
                }
                
                // === MICRO-DETUNE (per-voice, adds organic richness) ===
                if constexpr (GlitchLooper::kEnableMicroDetune) {
                    // Random ±cents converted to speed multiplier
                    const float detuneRand = sparkleRng.nextFloat() * 2.0f - 1.0f; // -1 to +1
                    const float detuneCents = detuneRand * GlitchLooper::kMicroDetuneCents;
                    freeVoice->microDetune = std::pow(2.0f, detuneCents / 1200.0f);
                } else {
                    freeVoice->microDetune = 1.0f;
                }
                
                // === STEREO MICRO-DELAY (subtle Haas effect for width) ===
                if constexpr (GlitchLooper::kEnableMicroDelay) {
                    const float delayMs = GlitchLooper::kMicroDelayMinMs + 
                        sparkleRng.nextFloat() * (GlitchLooper::kMicroDelayMaxMs - GlitchLooper::kMicroDelayMinMs);
                    const float delaySamples = delayMs * 0.001f * srFloat;
                    // Randomly assign delay to L or R (creates width without comb filtering)
                    if (sparkleRng.nextFloat() < 0.5f) {
                        freeVoice->microDelayL = delaySamples;
                        freeVoice->microDelayR = 0.0f;
                    } else {
                        freeVoice->microDelayL = 0.0f;
                        freeVoice->microDelayR = delaySamples;
                    }
                } else {
                    freeVoice->microDelayL = 0.0f;
                    freeVoice->microDelayR = 0.0f;
                }
                
                // Repeat count
                freeVoice->repeatsRemaining = GlitchLooper::kMinRepeats + 
                    static_cast<int>(sparkleRng.nextFloat() * 
                        static_cast<float>(GlitchLooper::kMaxRepeats - GlitchLooper::kMinRepeats));
                
                // Stereo position (random scatter)
                freeVoice->pan = sparkleRng.nextFloat();
                freeVoice->panDir = (sparkleRng.nextFloat() < 0.5f) ? -1.0f : 1.0f;
                freeVoice->panPhase = sparkleRng.nextFloat() * kTwoPi; // Random LFO phase
                
                // Set positions
                freeVoice->startPos = startPos;
                freeVoice->readPos = (speed < 0.0f) 
                    ? (startPos + static_cast<float>(freeVoice->lengthSamples))
                    : startPos;
                freeVoice->speedRatio = speed;
                freeVoice->sampleInSlice = 0;
                freeVoice->active = true;
                
                // Reset transient peak to prevent re-triggering
                if (isTransient) {
                    transientPeak = transientEnvelope;
                }
            }
        }
        
        // Schedule next trigger (faster after transients for burst effect)
        float triggerMs = juce::jmap(glitchAmount,
            GlitchLooper::kMaxTriggerMs, GlitchLooper::kMinTriggerMs);
        if (isTransient) {
            triggerMs *= 0.4f;  // 60% faster retrigger after transient (burst of voices)
        }
        const float jitter = 1.0f + (sparkleRng.nextFloat() * 2.0f - 1.0f) * GlitchLooper::kTriggerJitter;
        sparkleTriggerSamples = std::max(1, static_cast<int>(triggerMs * jitter * 0.001f * srFloat));
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // 3. UPDATE GLOBAL PING-PONG LFO
    // ═══════════════════════════════════════════════════════════════════════
    const float pingPongDepth = juce::jmap(glitchAmount,
        GlitchLooper::kPingPongDepthMin, GlitchLooper::kPingPongDepthMax);
    const float lfoIncrement = GlitchLooper::kPingPongRateHz * kTwoPi / srFloat;
    sparklePingPongLfoPhase += lfoIncrement;
    if (sparklePingPongLfoPhase > kTwoPi) sparklePingPongLfoPhase -= kTwoPi;
    
    // ═══════════════════════════════════════════════════════════════════════
    // 4. RENDER ALL ACTIVE VOICES
    // ═══════════════════════════════════════════════════════════════════════
    float sumL = 0.0f;
    float sumR = 0.0f;
    
    // Per-voice gain (louder at high glitch)
    const float voiceGainDb = juce::jmap(glitchAmount,
        GlitchLooper::kVoiceGainLowDb, GlitchLooper::kVoiceGainHighDb);
    const float voiceGain = juce::Decibels::decibelsToGain(voiceGainDb);
    
    for (auto& voice : sparkleVoices) {
        if (!voice.active) continue;
        
        // Read samples with optional micro-delay offset for stereo width
        float sampleL, sampleR;
        if constexpr (GlitchLooper::kEnableMicroDelay) {
            // Offset read positions for L/R (subtle Haas effect)
            float readPosL = voice.readPos - voice.microDelayL;
            float readPosR = voice.readPos - voice.microDelayR;
            // Wrap positions
            while (readPosL < 0.0f) readPosL += histSizeF;
            while (readPosR < 0.0f) readPosR += histSizeF;
            sampleL = readGhostHistoryInterpolated(readPosL);
            sampleR = readGhostHistoryInterpolated(readPosR);
        } else {
            const float sample = readGhostHistoryInterpolated(voice.readPos);
            sampleL = sample;
            sampleR = sample;
        }
        
        // === GRAIN ENVELOPE ===
        float envelope = 1.0f;
        if constexpr (GlitchLooper::kUseExponentialEnvelope) {
            // Exponential envelope: natural analog-style attack/release
            // Attack: 1 - exp(-k*t) rises quickly then slows
            // Release: exp(-k*t) decays naturally
            const float phase = static_cast<float>(voice.sampleInSlice) / 
                static_cast<float>(voice.lengthSamples);
            const float attackEnd = GlitchLooper::kExpAttackRatio;
            const float releaseStart = 1.0f - GlitchLooper::kExpReleaseRatio;
            const float k = GlitchLooper::kExpCurvature;
            
            if (phase < attackEnd) {
                // Attack phase: exponential rise
                const float t = phase / attackEnd;  // 0 to 1 within attack
                envelope = 1.0f - std::exp(-k * t);
                // Normalize so it reaches ~1.0 at end of attack
                envelope = envelope / (1.0f - std::exp(-k));
            } else if (phase > releaseStart) {
                // Release phase: exponential decay
                const float t = (phase - releaseStart) / GlitchLooper::kExpReleaseRatio;  // 0 to 1
                envelope = std::exp(-k * t);
            }
            // else envelope stays 1.0 (sustain)
        } else {
            // Fallback: S-curve envelope
            const int fadeLen = std::max(1, static_cast<int>(
                std::max(GlitchLooper::kMinFadeMs * 0.001f * srFloat,
                         static_cast<float>(voice.lengthSamples) * GlitchLooper::kFadeRatio)));
            
            if (voice.sampleInSlice < fadeLen) {
                envelope = static_cast<float>(voice.sampleInSlice) / static_cast<float>(fadeLen);
            } else if (voice.sampleInSlice >= voice.lengthSamples - fadeLen) {
                const int samplesFromEnd = voice.lengthSamples - voice.sampleInSlice;
                envelope = juce::jlimit(0.0f, 1.0f, 
                    static_cast<float>(samplesFromEnd) / static_cast<float>(fadeLen));
            }
            // S-curve for smoother fade
            envelope = envelope * envelope * (3.0f - 2.0f * envelope);
        }
        
        // Calculate stereo position with ping-pong motion
        float currentPan = voice.pan;
        if (pingPongDepth > 0.01f) {
            const float lfoValue = std::sin(voice.panPhase + sparklePingPongLfoPhase);
            currentPan = juce::jlimit(0.0f, 1.0f, 
                voice.pan + voice.panDir * pingPongDepth * lfoValue * 0.5f);
        }
        
        // Apply gain, envelope, and pan
        const float voiceGainEnv = envelope * voiceGain;
        sumL += sampleL * voiceGainEnv * (1.0f - currentPan);
        sumR += sampleR * voiceGainEnv * currentPan;
        
        // Advance read position with micro-detune applied
        const float effectiveSpeed = voice.speedRatio * voice.microDetune;
        voice.readPos += effectiveSpeed;
        while (voice.readPos >= histSizeF) voice.readPos -= histSizeF;
        while (voice.readPos < 0.0f) voice.readPos += histSizeF;
        
        // Advance sample counter
        ++voice.sampleInSlice;
        if (voice.sampleInSlice >= voice.lengthSamples) {
            --voice.repeatsRemaining;
            if (voice.repeatsRemaining <= 0) {
                voice.active = false;
            } else {
                // Reset for next repeat
                voice.sampleInSlice = 0;
                voice.readPos = (voice.speedRatio < 0.0f)
                    ? (voice.startPos + static_cast<float>(voice.lengthSamples))
                    : voice.startPos;
            }
        }
    }
    
    // === SPARKLE-ONLY FILTERING (gentle HPF + LPF) ===
    if constexpr (GlitchLooper::kEnableSparkleFilters) {
        // Simple 1-pole DC blocker (HPF at 30Hz - inaudible, just removes DC)
        const float hpfAlpha = 1.0f - std::exp(-kTwoPi * GlitchLooper::kSparkleHpfHz / srFloat);
        sparkleHpfStateL += hpfAlpha * (sumL - sparkleHpfStateL);
        sparkleHpfStateR += hpfAlpha * (sumR - sparkleHpfStateR);
        sumL = sumL - sparkleHpfStateL;
        sumR = sumR - sparkleHpfStateR;
        
        // Simple 1-pole LPF at 16kHz (catches aliasing from pitch shifting)
        const float lpfAlpha = 1.0f - std::exp(-kTwoPi * GlitchLooper::kSparkleLpfHz / srFloat);
        sparkleLpfStateL += lpfAlpha * (sumL - sparkleLpfStateL);
        sparkleLpfStateR += lpfAlpha * (sumR - sparkleLpfStateR);
        sumL = sparkleLpfStateL;
        sumR = sparkleLpfStateR;
    }
    
    outL = sumL;
    outR = sumR;
}

} // namespace threadbare::dsp
