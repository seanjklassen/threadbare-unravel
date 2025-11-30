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
    // 0.4s keeps short settings usable; 20s covers ambient washes.
    static constexpr float kT60Min = 0.4f;
    static constexpr float kT60Max = 20.0f;

    // Puck Y decay multiplier (~ /3 to *3).
    static constexpr float kPuckYMultiplierMin = 1.0f / 3.0f;
    static constexpr float kPuckYMultiplierMax = 3.0f;
};

struct Damping {
    // Tone control → low-pass cutoff (Hz).
    // These set the darkest, neutral, and brightest edges.
    static constexpr float kLowCutoffHz  = 1500.0f;
    static constexpr float kMidCutoffHz  = 8000.0f;
    static constexpr float kHighCutoffHz = 16000.0f;

    // High-pass in loop to avoid boomy reverb.
    static constexpr float kLoopHighPassHz = 100.0f;
};

struct Modulation {
    // LFO frequency range for delay modulation (Hz).
    // Slow rates keep it "alive" without obvious chorus wobble.
    static constexpr float kMinRateHz = 0.05f;
    static constexpr float kMaxRateHz = 0.4f;

    // Max modulation depth in samples at drift=1, puckY=1.
    // Safe range ~2–8; higher = wobblier.
    static constexpr float kMaxDepthSamples = 8.0f;
};

struct Ghost {
    // How long the ghost remembers (seconds).
    static constexpr float kHistorySeconds = 0.75f;

    // Grain durations (seconds). Shorter = more granular; longer = smeary.
    static constexpr float kGrainMinSec = 0.08f;
    static constexpr float kGrainMaxSec = 0.20f;

    // Subtle detune range (in semitones) for most grains.
    static constexpr float kDetuneSemi = 0.2f; // ~20 cents

    // Rare shimmer grains at +12 semitones.
    static constexpr float kShimmerSemi = 12.0f;
    static constexpr float kShimmerProbability = 0.05f; // 5%

    // Ghost gain bounds relative to FDN input (dB).
    static constexpr float kMinGainDb = -30.0f; // subtle
    static constexpr float kMaxGainDb = -12.0f; // strong but not dominating
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

struct Safety {
    // Tiny noise to avoid denormal CPU spikes in FDN feedback.
    static constexpr float kAntiDenormal = 1.0e-18f;
};

} // namespace threadbare::tuning


