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
    static constexpr int kNumTaps = 6;

    // Tap times in milliseconds, spread between 5 ms and 60 ms.
    static constexpr float kTapTimesMs[kNumTaps] = {
        5.0f, 12.5f, 19.0f, 30.0f, 42.0f, 58.0f
    };

    // Gain per tap, decaying gently to keep ERs natural.
    static constexpr float kTapGains[kNumTaps] = {
        0.95f, 0.78f, 0.66f, 0.52f, 0.40f, 0.30f
    };

    // Body ↔ Air crossfade endpoints for ER loudness.
    static constexpr float kBodyMix = 1.0f;
    static constexpr float kAirMix  = 0.25f;

    // Complementary crossfade endpoints for feeding the FDN.
    static constexpr float kBodyFdnMix = 0.45f;
    static constexpr float kAirFdnMix  = 1.0f;

    // Normalisation for distributing ER energy into the tank.
    static constexpr float kInjectionGain = 0.5f;
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
    static constexpr float kHistorySeconds = 0.75f;

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
    static constexpr float kMaxGainDb = -3.0f;  // Very loud for massive presence
};

struct Freeze {
    // Feedback gain while frozen (slightly <1.0 to avoid blow-up).
    static constexpr float kFrozenFeedback = 0.999f;

    // Ramp time when entering/exiting freeze (seconds).
    static constexpr float kRampTimeSec = 0.05f;
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
    // Tiny noise to avoid denormal CPU spikes in FDN feedback.
    static constexpr float kAntiDenormal = 1.0e-18f;
};

} // namespace threadbare::tuning


