#pragma once

namespace threadbare::tuning {

struct Fdn {
    // Number of delay lines in the FDN.
    // Evidence: 8–16 lines is common in high-quality algorithmic reverbs;
    // 8 is a sweet spot for musical use vs CPU.
    static constexpr int kNumLines = 8;

    // Base delay times in ms for Size = 1.0.
    // Incommensurate / prime-ish delays help avoid obvious metallic ringing.
    static constexpr float kBaseDelaysMs[kNumLines] = {
        31.0f, 37.0f, 41.0f, 53.0f, 61.0f, 71.0f, 83.0f, 97.0f
    };

    // Allowed range for Size scalar (tight ↔ huge).
    static constexpr float kSizeMin = 0.5f;
    static constexpr float kSizeMax = 2.0f;
    
    // Average delay time in ms (used for simplified feedback calculation).
    // Calculated as mean of kBaseDelaysMs: [31,37,41,53,61,71,83,97] = 59.125
    static constexpr float kAvgDelayMs = 59.125f;
};

struct Decay {
    // Global T60 bounds (seconds).
    // 0.4s keeps short settings usable; 50s is near-infinite reverb.
    static constexpr float kT60Min = 0.4f;
    static constexpr float kT60Max = 50.0f;

    // Puck Y decay multiplier (~ /3 to *3).
    static constexpr float kPuckYMultiplierMin = 1.0f / 3.0f;
    static constexpr float kPuckYMultiplierMax = 3.0f;
};

struct Damping {
    // Tone control → low-pass cutoff (Hz).
    // Lower cutoff for more aggressive darkening (underwater effect).
    static constexpr float kLowCutoffHz  = 400.0f;
    static constexpr float kMidCutoffHz  = 8000.0f;
    static constexpr float kHighCutoffHz = 16000.0f;

    // High-pass in loop to avoid boomy reverb.
    static constexpr float kLoopHighPassHz = 100.0f;
};

struct EarlyReflections {
    static constexpr std::size_t kNumTaps = 6;
    
    // Max pre-delay before ER cluster (ms)
    static constexpr float kMaxPreDelayMs = 100.0f;

    // Tap times for left channel (ms) - slightly asymmetric for width
    static constexpr float kTapTimesL[kNumTaps] = {
        7.0f, 13.0f, 19.0f, 29.0f, 43.0f, 57.0f
    };
    
    // Tap times for right channel (ms) - different pattern for stereo
    static constexpr float kTapTimesR[kNumTaps] = {
        5.0f, 11.0f, 23.0f, 31.0f, 41.0f, 61.0f
    };
    
    // Tap gains (decreasing over time for natural decay)
    static constexpr float kTapGains[kNumTaps] = {
        0.7f, 0.6f, 0.5f, 0.4f, 0.3f, 0.2f
    };
    
    // PuckX Proximity mapping:
    // Left (Physical): Strong ERs, weak FDN
    // Right (Ethereal): Weak ERs, strong FDN
    static constexpr float kErInjectionGain = 0.5f; // ERs feed into FDN at this level
};

struct Modulation {
    // LFO frequency range for delay modulation (Hz).
    // Wider range from slow to fast creates more obvious modulation.
    static constexpr float kMinRateHz = 0.1f;
    static constexpr float kMaxRateHz = 3.0f;

    // Max modulation depth in samples at drift=1, puckY=1.
    // 100 samples at 48kHz creates extreme tape warble/detune.
    static constexpr float kMaxDepthSamples = 100.0f;
};

struct Ghost {
    // How long the ghost remembers (seconds).
    // Extended to 1.2s for deeper memory recall while keeping lookback at 750ms.
    // This provides 450ms safety margin for extreme grain speeds/positions.
    static constexpr float kHistorySeconds = 1.2f;

    // Grain durations (seconds). Wider range for more texture variety.
    static constexpr float kGrainMinSec = 0.05f;  // Shorter for more density
    static constexpr float kGrainMaxSec = 0.30f;  // Long for smooth texture

    // Subtle detune range (in semitones) for most grains.
    static constexpr float kDetuneSemi = 0.2f; // ~20 cents

    // Shimmer grains at +12 semitones (octave up).
    static constexpr float kShimmerSemi = 12.0f;
    static constexpr float kShimmerProbability = 0.25f; // 25% for obvious sparkle

    // Ghost gain bounds relative to FDN input (dB).
    static constexpr float kMinGainDb = -24.0f; // Louder minimum
    static constexpr float kMaxGainDb = -6.0f;  // Reduced from -3dB to prevent clipping with many grains
    
    // === REVERSE MEMORY PLAYBACK ===
    // Probability of reverse grains at ghost=1.0 (squared scaling).
    // At ghost=0.5, actual probability = 0.25 × 0.25 = 6.25%
    // At ghost=1.0, actual probability = 25%
    // Evokes memories playing backwards through time.
    static constexpr float kReverseProbability = 0.25f;
    
    // Gain reduction for reverse grains (to reduce muddiness on percussive sources).
    // 0.75 = -2.5dB, helps reverse grains sit "behind" forward grains.
    static constexpr float kReverseGainReduction = 0.75f;
    
    // === MEMORY PROXIMITY (Puck X Mapping - Continuous Range) ===
    // Minimum lookback time at puckX=-1 (body/recent memories).
    static constexpr float kMinLookbackMs = 150.0f;
    // Maximum lookback time at puckX=+1 (air/distant memories).
    static constexpr float kMaxLookbackMs = 750.0f;
    // Uses continuous interpolation for smooth, predictable behavior.
    
    // === ENHANCED STEREO POSITIONING ===
    // Stereo pan width at ghost=0 (narrower = more focused).
    static constexpr float kMinPanWidth = 0.3f;
    // Stereo pan width at ghost=1 (capped at 85% for mono compatibility).
    static constexpr float kMaxPanWidth = 0.85f;
    // Whether to mirror reverse grains in stereo field.
    static constexpr bool kMirrorReverseGrains = true;
    
    // === SPECTRAL FREEZE ENHANCEMENTS ===
    // Shimmer probability when frozen (higher for more variation from limited material).
    static constexpr float kFreezeShimmerProbability = 0.40f; // vs 0.25f normally
};

struct Freeze {
    // === MULTI-HEAD LOOP (Smooth bed sound) ===
    // Multiple read heads at staggered positions with gentle filtering
    // for a warm, constant pad without audible looping or pumping.
    
    // Loop buffer duration in seconds (longer = more variation)
    static constexpr float kLoopBufferSeconds = 5.0f;
    
    // Number of read heads (more = smoother blend)
    // 6 heads provides excellent coverage without excessive CPU
    static constexpr int kNumReadHeads = 6;
    
    // Transition time when entering/exiting freeze
    static constexpr float kTransitionSeconds = 0.3f;
    
    // Per-head pitch modulation depth (cents) - subtle detuning
    static constexpr float kHeadDetuneCents = 6.0f;
    
    // Per-head pitch modulation rate range (Hz) - slow drift
    static constexpr float kHeadModRateMin = 0.03f;
    static constexpr float kHeadModRateMax = 0.12f;
    
    // Loop output warming filter (1-pole LPF coefficient)
    // Lower = darker/warmer, higher = brighter
    // 0.15 ≈ 1.5kHz cutoff - very warm, removes icy highs
    static constexpr float kLoopWarmingCoef = 0.15f;
    
    // === LEGACY SETTINGS (kept for compatibility) ===
    static constexpr float kFrozenFeedback = 1.0f;
    static constexpr float kFrozenMakeupGain = 1.0f;  // Unity to prevent energy accumulation
    static constexpr float kRampTimeSec = 0.05f;
    static constexpr float kFreezeDriftMultiplier = 2.5f;
    static constexpr float kFreezeMinDriftSamples = 25.0f;
    static constexpr float kFreezeGhostLevel = 0.25f;
    static constexpr float kFreezeLpfCoef = 0.75f;
};

// ═══════════════════════════════════════════════════════════════════════════
// DISINTEGRATION LOOPER
// William Basinski-inspired loop degradation with "Ascension" filter model
// The loop evaporates upward (HPF+LPF converge) with warm saturation
// ═══════════════════════════════════════════════════════════════════════════
struct Disintegration {
    // === BUFFER ===
    // 20 seconds supports 4 bars at 60 BPM with headroom
    static constexpr float kLoopBufferSeconds = 20.0f;
    
    // === RECORDING ===
    static constexpr int kDefaultBars = 4;
    static constexpr float kFallbackTempo = 120.0f;
    static constexpr float kMinCaptureWetMix = 0.3f;      // Ensure some reverb character
    static constexpr float kInputGateThresholdDb = -60.0f; // Wait for signal before recording
    static constexpr float kRecordingTimeoutSeconds = 5.0f; // Cancel if no input detected
    
    // === LOOP BOUNDARY CROSSFADE (prevents clicks) ===
    static constexpr float kCrossfadeMs = 10.0f;  // 10ms micro-crossfade at loop point
    
    // === TRANSITION (Recording -> Looping) ===
    static constexpr float kAutoDuckDb = -3.0f;           // Push loop to background
    static constexpr float kTransitionTimeSeconds = 0.5f; // Smooth duck/diffuse ramp
    static constexpr float kDiffuseAmount = 0.15f;        // Subtle blur on loop
    
    // === ASCENSION FILTER (converging HPF + LPF) ===
    // Using SVF for resonance - creates "singing" sweep as frequencies converge
    static constexpr float kHpfStartHz = 20.0f;
    static constexpr float kHpfEndHz = 800.0f;            // Max HPF at full entropy
    static constexpr float kLpfStartHz = 20000.0f;
    static constexpr float kLpfEndHz = 2000.0f;           // Min LPF at full entropy
    static constexpr float kFilterResonance = 0.3f;       // Subtle Q for musical sweep
    
    // === WARMTH (saturation increases with entropy) ===
    static constexpr float kSaturationMin = 0.0f;
    static constexpr float kSaturationMax = 0.4f;         // Warm blanket, not harsh
    
    // === FOCUS MAPPING (Puck X in loop mode) ===
    // Left (Ghost): Spectral thinning, emphasize highs - MORE DRAMATIC
    // Right (Fog): Diffuse smearing, preserve mids - MORE DRAMATIC
    // Center: Wistful balance
    static constexpr float kFocusGhostHpfBoost = 4.0f;    // HPF frequency multiplier (was 1.5)
    static constexpr float kFocusFogLpfBoost = 0.25f;     // LPF frequency multiplier (was 0.7)
    static constexpr float kFocusBaseHpfHz = 100.0f;      // Minimum audible HPF effect
    static constexpr float kFocusBaseLpfHz = 8000.0f;     // Maximum LPF in fog mode
    
    // === ENTROPY TIMING (in loop iterations, not wall-clock time) ===
    // These define how many times the loop plays before full disintegration
    // Rate is calculated at runtime based on actual loop length
    static constexpr float kEntropyLoopsMin = 1.0f;      // Fastest: 1 loop iteration to full entropy (puck Y top)
    static constexpr float kEntropyLoopsMax = 1000.0f;   // Slowest: 1000 iterations - practically endless (puck Y bottom)
    
    // === EXIT BEHAVIOR ===
    static constexpr float kFadeToReverbSeconds = 2.0f;   // Graceful fade when entropy=1
    static constexpr float kButtonDebounceMs = 200.0f;    // Prevent rapid state spam
    
    // ═══════════════════════════════════════════════════════════════════════
    // PHASE 3: PHYSICAL DEGRADATION (William Basinski tape failure simulation)
    // ═══════════════════════════════════════════════════════════════════════
    
    // --- OXIDE SHEDDING (Stochastic Dropouts) ---
    // As entropy increases, random "dropouts" occur (oxide flaking off tape)
    // Uses timer-based trigger (~40ms intervals) to avoid audio-rate noise
    static constexpr int kOxideCheckIntervalSamples = 2000;     // Check for dropout every ~40ms at 48kHz
    static constexpr float kOxideDropoutProbabilityMax = 0.35f; // Max probability per check at entropy=1
    static constexpr float kOxideDropoutDurationMs = 15.0f;     // Max dropout "gasp" duration
    static constexpr float kOxideDropoutSmoothMs = 5.0f;        // Gain reduction smoothing (prevents clicks)
    
    // --- MOTOR DEATH (Asymmetric Pitch Drag) ---
    // Brownian noise modulates playback speed, biased downward (motor struggling)
    static constexpr float kMotorDragMaxCents = 40.0f;          // Max pitch deviation at entropy=1
    static constexpr float kMotorDragBias = -0.6f;              // Downward bias (-1 = always down, 0 = symmetric)
    static constexpr float kMotorDragInertia = 0.9995f;         // Brownian walk smoothing (higher = slower drift)
    static constexpr float kMotorDragStepSize = 0.002f;         // Per-sample random step magnitude
    
    // --- AZIMUTH DRIFT (Stereo Decoupling) ---
    // L/R channels degrade at slightly different rates (tape head misalignment)
    static constexpr float kAzimuthDriftMaxOffset = 0.08f;      // Max entropy offset between L/R (8%)
    
    // === MATH CONSTANTS ===
    static constexpr float kPi = 3.14159265359f;
    
    // Helper: Convert desired duration to entropy rate
    // Usage: secondsToEntropyRate(30.0f, 48000.0f) → ~0.0000007
    static constexpr float secondsToEntropyRate(float seconds, float sampleRate) {
        return 1.0f / (seconds * sampleRate);
    }
};

struct Ducking {
    // Envelope times (seconds).
    static constexpr float kAttackSec  = 0.01f; // 10 ms: snappy enough
    static constexpr float kReleaseSec = 0.25f; // 250 ms: natural decay

    // Minimum wet proportion at full duck (0..1).
    // Keeps some ambience even when duck knob is maxed.
    static constexpr float kMinWetFactor = 0.15f;
};

struct PuckMapping {
    // Y influence on decay multiplier. 3.0 means ~ /3 to *3 across pad.
    static constexpr float kDecayYFactor = 3.0f;

    // Extra ghost added at top of pad (0..1).
    static constexpr float kGhostYBonus  = 0.3f;

    // Extra drift added at top of pad (0..1).
    static constexpr float kDriftYBonus  = 0.25f;
};

struct Metering {
    // Envelope follower times for orb metering.
    static constexpr float kAttackSec  = 0.01f; // ~10 ms
    static constexpr float kReleaseSec = 0.10f; // ~100 ms
};

struct Safety {
    // Anti-denormal is now handled by ScopedNoDenormals + FTZ/DAZ
    // No additive noise needed in feedback path
    static constexpr float kAntiDenormal = 0.0f; // DISABLED - was causing audible grain
};

// ═══════════════════════════════════════════════════════════════════════════
// DEBUG SWITCHES: Toggle subsystems to isolate crackling/distortion sources
// Set to false to disable each subsystem during debugging
// ═══════════════════════════════════════════════════════════════════════════
struct Debug {
    // (1) Additive noise in feedback path (anti-denormal, LFO jitter)
    static constexpr bool kEnableNoiseInjection = false;
    
    // (2) Delay modulation (LFO-based read position modulation)
    static constexpr bool kEnableDelayModulation = true;
    
    // (3) Nonlinear feedback limiting (tanh saturation in FDN write path)
    static constexpr bool kEnableFeedbackNonlinearity = true;
    
    // (4) EQ / Ducking (tone filters, ducking envelope)
    static constexpr bool kEnableEqAndDuck = true;
    
    // (5) Ghost engine (granular clouds)
    static constexpr bool kEnableGhostEngine = true;
    
    // === DISTORTION DEBUGGING (loud transient aliasing) ===
    
    // (6) FDN input soft limiting (tanh before feedback loop)
    static constexpr bool kEnableFdnInputLimiting = true;
    
    // (7) Final output soft clipping (tanh on mix output)
    static constexpr bool kEnableOutputClipping = true;
    
    // (8) Shimmer grains only (disable normal/reverse grains for isolation)
    static constexpr bool kShimmerGrainsOnly = false;
    
    // (9) Limit max active grains (0 = unlimited, 1-8 for testing)
    static constexpr int kMaxActiveGrains = 0; // 0 = use full pool
    
    // (10) Ghost injection gain reduction in dB (0 = normal, -6 or -12 for testing)
    static constexpr float kGhostInjectionGainDb = 0.0f;
    
    // (11) Internal headroom boost (scales down before nonlinearities, up after)
    // Higher values = more headroom, less saturation/aliasing
    static constexpr float kInternalHeadroomDb = 6.0f; // 6dB extra headroom
};

} // namespace threadbare::tuning


