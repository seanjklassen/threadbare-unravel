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
    // Reduced to prevent stacking: sum = 0.35+0.3+0.25+0.2+0.15+0.1 = 1.35 (safe)
    static constexpr float kTapGains[kNumTaps] = {
        0.35f, 0.30f, 0.25f, 0.20f, 0.15f, 0.10f
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
    // Extended to 2.0s to support Glitch Looper slices at slow tempos.
    // At 60 BPM with 1/2 note division, slices can be 500ms + repeats + margin.
    static constexpr float kHistorySeconds = 2.0f;

    // Grain durations (seconds). Wider range for more texture variety.
    static constexpr float kGrainMinSec = 0.05f;  // Shorter for more density
    static constexpr float kGrainMaxSec = 0.30f;  // Long for smooth texture

    // Subtle detune range (in semitones) for most grains.
    static constexpr float kDetuneSemi = 0.2f; // ~20 cents

    // Shimmer grains at +12 semitones (octave up).
    static constexpr float kShimmerSemi = 12.0f;
    static constexpr float kShimmerProbability = 0.25f; // 25% for obvious sparkle

    // Ghost gain bounds relative to FDN input (dB).
    // With 8 grains, max sum at -12dB each = 8 * 0.25 = 2.0 linear (safe)
    static constexpr float kMinGainDb = -24.0f;
    static constexpr float kMaxGainDb = -12.0f;  // Reduced to prevent 8-grain stacking distortion
    
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
    
    // === CLOUD SPAWN DEFAULTS (used for blend=0 baseline) ===
    // These match the current hardcoded values in process() - centralizing for consistency
    static constexpr float kCloudSpawnIntervalMs = 15.0f;  // Current grainSpawnInterval (15ms)
    static constexpr float kCloudSpawnProbability = 0.9f;  // Current spawn probability factor
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
// GLITCH SPARKLE
// Multi-voice granular sparkle effect - shimmering, articulate fragments
// Inspired by Phantom Limb, tuned for beauty and clarity
// ═══════════════════════════════════════════════════════════════════════════
struct GlitchLooper {
    // === VOICE ARCHITECTURE ===
    static constexpr int kMaxVoices = 4;              // 4 simultaneous micro-loopers
    static constexpr int kVoicesAtLow = 1;            // Active voices at low glitch
    static constexpr int kVoicesAtMid = 3;            // Active voices at mid glitch  
    static constexpr int kVoicesAtHigh = 4;           // Active voices at high glitch
    
    // === FRAGMENT SIZES (ms) - longer for dreamy/ethereal ===
    static constexpr float kMinFragmentMs = 60.0f;    // Longer minimum (more melodic)
    static constexpr float kMaxFragmentMs = 400.0f;   // Extended max (flowing phrases)
    
    // === MEMORY SCRUBBING (random buffer access) ===
    static constexpr float kMinScrubDepth = 0.1f;     // 10% of buffer = recent
    static constexpr float kMaxScrubDepth = 0.8f;     // 80% of buffer = deep memory
    
    // === TRIGGER TIMING (ms between new voices) ===
    static constexpr float kMinTriggerMs = 30.0f;     // Slower minimum (more space)
    static constexpr float kMaxTriggerMs = 600.0f;    // Extended max (dreamy gaps)
    static constexpr float kTriggerJitter = 0.4f;     // More variation (organic)
    
    // === PITCH PALETTE (ethereal/dreamy with reverse) ===
    static constexpr float kRootProb = 0.25f;         // Normal pitch (grounding)
    static constexpr float kOctaveUpProb = 0.20f;     // 2x speed (sparkle)
    static constexpr float kDoubleOctaveProb = 0.08f; // 4x speed (reduced - less harsh)
    static constexpr float kFifthProb = 0.25f;        // 1.5x speed (ethereal - boosted)
    static constexpr float kOctaveDownProb = 0.15f;   // 0.5x speed (warmth - boosted)
    static constexpr float kMicroShimmerProb = 0.07f; // 1.02-1.08x (chorus)
    static constexpr float kMicroShimmerMin = 1.02f;
    static constexpr float kMicroShimmerMax = 1.08f;
    
    // === ENVELOPE (articulate, not smeared) ===
    static constexpr float kFadeRatio = 0.12f;        // 12% of slice is fade (short, articulate)
    static constexpr float kMinFadeMs = 3.0f;         // Minimum fade to prevent clicks
    
    // === STEREO SCATTERING + PING-PONG ===
    static constexpr float kPingPongDepthMin = 0.0f;  // Low glitch: static pan
    static constexpr float kPingPongDepthMax = 0.7f;  // High glitch: voices sweep 70% across
    static constexpr float kPingPongRateHz = 2.5f;    // LFO rate for pan motion
    
    // === GAIN STAGING (per voice) ===
    // Reduced to prevent clipping when multiple voices stack
    // 4 voices at -15dB = ~-9dB total (safe headroom)
    static constexpr float kVoiceGainLowDb = -18.0f;  // Per-voice gain at low glitch
    static constexpr float kVoiceGainHighDb = -12.0f; // Per-voice gain at high glitch
    
    // === REPEAT COUNT ===
    static constexpr int kMinRepeats = 1;             // Single sparkle
    static constexpr int kMaxRepeats = 3;             // Short stutter
    
    // === TRANSIENT DETECTION (still used for reactive triggering) ===
    static constexpr float kTransientThresholdDb = -36.0f;
    static constexpr float kTransientRatio = 1.5f;
    static constexpr float kEnvelopeAttackMs = 0.5f;
    static constexpr float kEnvelopeReleaseMs = 50.0f;
    
    // === REVERSE (more frequent for dreamy effect) ===
    static constexpr float kReverseProb = 0.30f;      // 30% chance of reverse grain
    
    // === TEMPO/SAFETY ===
    static constexpr float kFallbackTempo = 120.0f;
    static constexpr float kMinTempo = 20.0f;
    static constexpr float kMaxTempo = 300.0f;
    
    // ═══════════════════════════════════════════════════════════════════════
    // SPARKLE QUALITY ENHANCEMENTS
    // ═══════════════════════════════════════════════════════════════════════
    
    // === GRAIN ENVELOPE ===
    // Exponential: natural analog-style attack/release (musical, organic)
    // S-curve: polynomial smoothstep (mathematical, precise)
    static constexpr bool kUseExponentialEnvelope = true;
    static constexpr float kExpAttackRatio = 0.20f;   // 20% attack (softer entrance)
    static constexpr float kExpReleaseRatio = 0.35f;  // 35% release (longer fade)
    static constexpr float kExpCurvature = 3.0f;      // Gentler curve (dreamier)
    
    // === SPARKLE-ONLY FILTERING (HPF + LPF) ===
    // Disabled - was causing "underwater" sound even at conservative settings
    static constexpr bool kEnableSparkleFilters = false;
    static constexpr float kSparkleHpfHz = 30.0f;
    static constexpr float kSparkleLpfHz = 16000.0f;
    
    // === MICRO-DETUNE (per voice, adds organic richness) ===
    static constexpr bool kEnableMicroDetune = true;
    static constexpr float kMicroDetuneCents = 3.0f;       // Max detune ±3 cents (very subtle)
    
    // === STEREO MICRO-DELAY (Haas effect for width) ===
    static constexpr bool kEnableMicroDelay = true;
    static constexpr float kMicroDelayMinMs = 0.3f;        // Very subtle minimum
    static constexpr float kMicroDelayMaxMs = 1.5f;        // Small max (avoids phase issues)
    
    // === PITCH GATING FOR TINY GRAINS ===
    // Don't play 4x pitch on very short grains (sounds like clicks)
    static constexpr float kMinFragmentFor4xMs = 40.0f;    // Minimum 40ms for 4x pitch
};

// ═══════════════════════════════════════════════════════════════════════════
// DISINTEGRATION LOOPER
// William Basinski-inspired loop degradation with "Ascension" filter model
// The loop evaporates upward (HPF+LPF converge) with warm saturation
// ═══════════════════════════════════════════════════════════════════════════
struct Disintegration {
    // === BUFFER ===
    // Max loop buffer length (seconds)
    static constexpr float kLoopBufferSeconds = 60.0f;
    
    // === RECORDING ===
    static constexpr float kLoopRecordSeconds = 60.0f; // Time-based capture window
    static constexpr float kMinCaptureWetMix = 0.3f;      // Ensure some reverb character
    static constexpr float kInputGateThresholdDb = -60.0f; // Wait for signal before recording
    static constexpr float kRecordingTimeoutSeconds = 5.0f; // Cancel if no input detected
    
    // === LOOP BOUNDARY CROSSFADE (prevents clicks) ===
    static constexpr float kCrossfadeMs = 50.0f;  // 50ms crossfade with S-curve for seamless loops
    
    // === TAPE SHUTTLE EFFECT (simulates reel momentum at loop boundary) ===
    static constexpr float kLoopBoundaryPitchDropCents = -30.0f;  // Pitch sag at boundary
    static constexpr int kLoopBoundaryTransitionSamples = 2000;    // ~40ms transition zone
    
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
    static constexpr float kSaturationMax = 0.6f;         // More warmth available (was 0.4)
    
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
    // At 120 BPM with 4 bars (~8s loop): 2 loops ≈ 16s, 100 loops ≈ 13min
    static constexpr float kEntropyLoopsMin = 2.0f;       // Fastest: ~2 loop iterations (puck Y top)
    static constexpr float kEntropyLoopsMax = 10000.0f;   // Slowest: practically endless (puck Y bottom)
    
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
    static constexpr float kOxideDropoutProbabilityMax = 0.50f; // Max probability per check at entropy=1 (was 0.35)
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
    static constexpr float kAzimuthDriftMaxOffset = 0.18f;      // Max entropy offset between L/R (18%) - more dramatic stereo separation
    
    // --- STEREO MOTOR DIVERGENCE ---
    // L/R read positions drift apart over time (worn tape transport)
    static constexpr float kMotorStereoDivergence = 0.4f;       // How much L/R motor drag values can diverge (0-1)
    
    // === PROFESSIONAL DSP QUALITY ENHANCEMENTS ===
    
    // --- DC BLOCKER (prevents low-frequency drift) ---
    static constexpr float kDcBlockerFreqHz = 5.0f;
    
    // --- SOFT CLIP (prevents digital overs) ---
    static constexpr float kSoftClipThreshold = 0.9f;
    
    // --- WOW & FLUTTER (authentic tape transport wobble) ---
    static constexpr float kWowFreqHz = 0.5f;
    static constexpr float kWowDepthCents = 12.0f;
    static constexpr float kFlutterFreqHz = 6.0f;
    static constexpr float kFlutterDepthCents = 4.0f;
    
    // --- PINK NOISE FLOOR (constant tape hiss) ---
    static constexpr float kNoiseFloorMaxLevel = 0.0025f;  // ~-52dB, felt not heard
    static constexpr float kNoiseFloorBaseGain = 0.5f;     // Start at 50% when loop engages
    static constexpr float kNoiseEntryFadeMs = 500.0f;     // Fade-in time to prevent click
    static constexpr float kNoiseHpfCoef = 0.04f;          // ~300Hz HPF to remove rumble
    
    // --- HYSTERESIS SATURATION (magnetic tape memory) ---
    static constexpr float kHysteresisWidth = 0.25f;      // Coercivity (loop width)
    static constexpr float kHysteresisSat = 1.0f;         // Saturation level
    static constexpr float kHysteresisSmooth = 0.995f;    // State smoothing
    
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


