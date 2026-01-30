#pragma once

#include <array>
#include <span>
#include <vector>

#include <juce_dsp/juce_dsp.h>
#include "../UnravelTuning.h"

namespace threadbare::dsp
{

// ═══════════════════════════════════════════════════════════════════════════
// DISINTEGRATION LOOPER STATE MACHINE
// ═══════════════════════════════════════════════════════════════════════════
enum class LooperState {
    Idle,       // Normal reverb operation
    Recording,  // Capturing dry+wet into loop buffer
    Looping     // Playback with disintegration
};

struct UnravelState
{
    float size = 1.0f;
    float decaySeconds = 5.0f;
    float tone = 0.0f;
    float mix = 0.5f;
    float drift = 0.0f;
    float puckX = 0.0f;
    float puckY = 0.0f;
    float ghost = 0.0f;
    float glitch = 0.0f;     // Glitch Looper amount (0-1)
    float duck = 0.0f;
    float erPreDelay = 0.0f; // Early Reflections pre-delay (0-100ms)
    float inLevel = 0.0f;
    float tailLevel = 0.0f;
    bool freeze = false;     // Legacy: used as trigger input from UI
    float tempo = 120.0f;    // BPM from DAW host
    
    // === DISINTEGRATION LOOPER (output to UI) ===
    LooperState looperState = LooperState::Idle;  // Current state for UI feedback
    float loopProgress = 0.0f;      // 0-1 during recording (progress indicator)
    float entropy = 0.0f;           // Current disintegration amount (0-1)
    bool looperStateAdvance = false; // Signal from processor to advance state
    int looperTriggerAction = 0;    // 0 = none, 1 = start, 2 = stop (UI-triggered)
    
    // === TRANSPORT STATE (from DAW) ===
    bool isPlaying = true;          // DAW transport state (for auto-stop)
};

class UnravelReverb
{
public:
    UnravelReverb() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process(std::span<float> left, std::span<float> right, UnravelState& state) noexcept;
    
    // Disintegration looper state accessor (for processor to read current state)
    LooperState getLooperState() const noexcept { return currentLooperState; }

private:
    static constexpr std::size_t kNumLines = threadbare::tuning::Fdn::kNumLines;
    static constexpr std::size_t kMaxGrains = 8;
    
    int sampleRate = 48000;
    
    // Smoothed parameters for "creamy" knobs (no zipper noise)
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> sizeSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> feedbackSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> toneSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> driftSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> driftDepthSmoother; // PuckX macro depth
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> ghostSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> preDelaySmoother;     // Smooth pre-delay changes
    
    // Additional smoothers to eliminate block-rate stepping artifacts
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> erGainSmoother;       // ER gain from puckX
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> fdnSendSmoother;      // FDN send from puckX
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> duckAmountSmoother;   // Ducking depth
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, kNumLines> lineFeedbackSmoothers; // Per-line feedback
    
    // 8 delay lines for the FDN
    std::array<std::vector<float>, kNumLines> delayLines;
    std::array<int, kNumLines> writeIndices;
    std::array<float, kNumLines> baseDelayOffsetsSamples; // Cached delay offsets in samples (calculated in prepare())
    
    // LFO state for modulation
    std::array<float, kNumLines> lfoPhases;
    std::array<float, kNumLines> lfoInc;
    
    // Low-pass filter state for damping (tone control)
    std::array<float, kNumLines> lpState;
    
    // High-pass filter state for LF cleanup (prevents mud buildup)
    std::array<float, kNumLines> hpState;
    
    // Ghost Engine state
    struct Grain
    {
        float pos = 0.0f;           // Read position in history buffer
        float speed = 1.0f;         // Playback speed (pitch shift)
        float amp = 1.0f;           // Grain amplitude
        float windowPhase = 0.0f;   // Window phase (0 to 1, normalized)
        float windowInc = 0.0f;     // Window phase increment per sample
        float pan = 0.5f;           // Stereo pan position (0=L, 1=R)
        bool active = false;        // Is this grain active?
    };
    
    std::vector<float> ghostHistory;
    int ghostWriteHead = 0;
    std::array<Grain, kMaxGrains> grainPool;
    juce::Random ghostRng;
    int samplesSinceLastSpawn = 0;
    int grainSpawnInterval = 0; // Cached spawn interval in samples (calculated in prepare())
    
    // ═══════════════════════════════════════════════════════════════════════
    // GLITCH SPARKLE STATE
    // Multi-voice granular sparkle - shimmering, articulate fragments
    // ═══════════════════════════════════════════════════════════════════════
    static constexpr std::size_t kSparkleVoices = 4;
    
    struct SparkleVoice {
        float readPos = 0.0f;       // Current read position in ghostHistory (fractional)
        float startPos = 0.0f;      // Start position for repeat
        int lengthSamples = 0;      // Fragment duration in samples
        float speedRatio = 1.0f;    // Pitch (1.0 = normal, 2.0 = octave up, 4.0 = twinkle)
        int repeatsRemaining = 0;   // How many more times to play
        int sampleInSlice = 0;      // Current position within fragment
        float pan = 0.5f;           // Stereo position (0=L, 1=R)
        float panDir = 1.0f;        // Pan motion direction (+1 or -1)
        float panPhase = 0.0f;      // LFO phase for ping-pong motion
        float microDetune = 0.0f;   // Per-voice micro-detune factor (multiplier, e.g. 1.002)
        float microDelayL = 0.0f;   // Stereo micro-delay offset for L channel (samples)
        float microDelayR = 0.0f;   // Stereo micro-delay offset for R channel (samples)
        bool active = false;
    };
    
    std::array<SparkleVoice, kSparkleVoices> sparkleVoices;
    int sparkleTriggerSamples = 0;                // Countdown to next voice trigger
    float sparklePingPongLfoPhase = 0.0f;         // Global LFO phase for pan motion
    juce::Random sparkleRng;                      // Dedicated RNG
    
    // Sparkle-only filter state (1-pole HPF + LPF for beauty)
    float sparkleHpfStateL = 0.0f;
    float sparkleHpfStateR = 0.0f;
    float sparkleLpfStateL = 0.0f;
    float sparkleLpfStateR = 0.0f;
    
    // Transient detection state (for reactive triggering)
    float transientEnvelope = 0.0f;
    float transientPeak = 0.0f;
    float transientAttackCoeff = 0.0f;
    float transientReleaseCoeff = 0.0f;
    
    // ═══════════════════════════════════════════════════════════════════════
    // DISINTEGRATION LOOPER STATE
    // ═══════════════════════════════════════════════════════════════════════
    LooperState currentLooperState = LooperState::Idle;
    int loopRecordHead = 0;
    int loopPlayHead = 0;
    int targetLoopLength = 0;           // In samples (time-based)
    int actualLoopLength = 0;           // May differ if truncated early
    float entropyAmount = 0.0f;         // Current disintegration level (0-1)
    int crossfadeSamples = 0;           // Calculated from kCrossfadeMs in prepare()
    bool lastButtonState = false;       // For state-owned trigger logic
    
    // Disintegration loop buffers (20 seconds for 60 BPM support)
    std::vector<float> disintLoopL;
    std::vector<float> disintLoopR;
    
    // Recording gate
    bool inputDetected = false;
    int silentSampleCount = 0;
    
    // === ASCENSION SVF FILTER STATE ===
    // State Variable Filter for resonant "singing" sweep
    // Using Cytomic/Vadim topology for stability
    struct SvfState {
        float ic1eq = 0.0f;  // Integrator 1 state
        float ic2eq = 0.0f;  // Integrator 2 state
    };
    SvfState hpfSvfL, hpfSvfR;  // High-pass filter state (stereo)
    SvfState lpfSvfL, lpfSvfR;  // Low-pass filter state (stereo)
    
    // Block-rate filter coefficients (calculated ONCE per block, not per-sample)
    // CRITICAL: Per-sample trig calculation would kill CPU
    // PHASE 3: Separate L/R coefficients for Azimuth Drift stereo decoupling
    float currentHpfG_L = 0.0f, currentHpfK_L = 0.0f;  // HPF g and k coefficients (Left)
    float currentHpfG_R = 0.0f, currentHpfK_R = 0.0f;  // HPF g and k coefficients (Right)
    float currentLpfG_L = 0.0f, currentLpfK_L = 0.0f;  // LPF g and k coefficients (Left)
    float currentLpfG_R = 0.0f, currentLpfK_R = 0.0f;  // LPF g and k coefficients (Right)
    float currentSatAmount = 0.0f;                      // Cached saturation amount
    
    // Separate diffuse LPF state for disintegration (avoids interference with freeze)
    float disintDiffuseLpfL = 0.0f;
    float disintDiffuseLpfR = 0.0f;
    
    // Exit fade: when entropy reaches 1.0, fade out to reverb over kFadeToReverbSeconds
    float exitFadeAmount = 1.0f;  // 1.0 = full loop, 0.0 = full reverb (then return to Idle)
    
    // Disintegration transition smoothers
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> loopGainSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> diffuseAmountSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> entropySmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> focusSmoother;  // Prevents zipper when moving puck X
    
    // ═══════════════════════════════════════════════════════════════════════
    // PHASE 3: PHYSICAL DEGRADATION STATE
    // ═══════════════════════════════════════════════════════════════════════
    
    // --- Oxide Shedding (Stochastic Dropouts) ---
    uint32_t oxideRngState = 0x12345678;           // LCG seed (fast PRNG)
    float oxideGainL = 1.0f;                        // Current dropout gain L (smoothed)
    float oxideGainR = 1.0f;                        // Current dropout gain R (smoothed)
    float oxideGainTarget = 1.0f;                   // Target gain (0 during dropout)
    float oxideGainSmoothCoef = 0.0f;               // 1-pole smoother coefficient
    int oxideDropoutCounter = 0;                    // Samples remaining in current dropout
    int oxideCheckTimer = 0;                        // Timer for dropout dice roll (CORRECTION #1)
    
    // --- Motor Death (Brownian Pitch Drag) ---
    float motorDragValueL = 0.0f;                   // Current Brownian walk position L (-1 to +1)
    float motorDragValueR = 0.0f;                   // Current Brownian walk position R
    float motorDragReadOffsetL = 0.0f;              // Accumulated fractional read offset L
    float motorDragReadOffsetR = 0.0f;              // Accumulated fractional read offset R
    
    // --- Azimuth Drift (Stereo Decoupling) ---
    float azimuthOffsetL = 0.0f;                    // Entropy offset for left channel
    float azimuthOffsetR = 0.0f;                    // Entropy offset for right channel
    
    // --- Tape Shuttle Effect (pitch modulation at loop boundary) ---
    float loopBoundaryPitchMod = 0.0f;              // Smoothed pitch modulation at boundaries
    
    // --- Transport-Aware Shutdown ---
    bool transportWasPlaying = true;                // Previous transport state
    float transportFadeAmount = 1.0f;               // Fade amount when transport stops
    
    // ═══════════════════════════════════════════════════════════════════════
    // PROFESSIONAL DSP QUALITY ENHANCEMENTS
    // ═══════════════════════════════════════════════════════════════════════
    
    // --- DC Blocker State ---
    float dcBlockerX1L = 0.0f, dcBlockerY1L = 0.0f;
    float dcBlockerX1R = 0.0f, dcBlockerY1R = 0.0f;
    float dcBlockerCoef = 0.0f;  // Calculated in prepare()
    
    // --- Wow & Flutter LFO State ---
    float wowPhase = 0.0f;
    float flutterPhase = 0.0f;
    float wowPhaseInc = 0.0f;      // Calculated in prepare()
    float flutterPhaseInc = 0.0f;
    
    // --- Pink Noise Generator State (Instance-Safe - NO STATIC LOCALS) ---
    uint32_t pinkNoiseCounter = 0;
    std::array<float, 8> pinkOctaveBands = {};
    float pinkNoiseRunningSum = 0.0f;
    float noiseHpfStateL = 0.0f;  // HPF state for removing low-end rumble
    float noiseHpfStateR = 0.0f;
    int loopEntrySamples = 0;     // Counter for entry fade
    
    // --- Hysteresis Saturation State ---
    float hysteresisMagL = 0.0f;
    float hysteresisMagR = 0.0f;
    
    // --- ADAA Saturation State ---
    float adaaX1L = 0.0f;
    float adaaX1R = 0.0f;
    
    // --- Fast LCG PRNG (real-time safe) ---
    inline float fastRand01() noexcept
    {
        // Linear Congruential Generator (Numerical Recipes)
        oxideRngState = oxideRngState * 1664525u + 1013904223u;
        return static_cast<float>(oxideRngState) / 4294967296.0f;  // 0.0 to 1.0
    }
    
    inline float fastRandBipolar() noexcept
    {
        return fastRand01() * 2.0f - 1.0f;  // -1.0 to +1.0
    }
    
    // === PROFESSIONAL DSP HELPERS ===
    
    // DC Blocker: 1-pole HPF y[n] = x[n] - x[n-1] + coef * y[n-1]
    inline float dcBlock(float in, float& x1, float& y1) noexcept
    {
        float out = in - x1 + dcBlockerCoef * y1;
        x1 = in;
        y1 = out;
        return out;
    }
    
    // Soft Clip: Gentle limiting above threshold using tanh
    inline float softClip(float x) noexcept
    {
        using namespace threadbare::tuning;
        constexpr float thresh = Disintegration::kSoftClipThreshold;
        if (std::abs(x) <= thresh) return x;
        float sign = (x > 0.0f) ? 1.0f : -1.0f;
        return sign * (thresh + (1.0f - thresh) * std::tanh((std::abs(x) - thresh) / (1.0f - thresh)));
    }
    
    // Safe buffer index wrapping for circular access
    inline int wrapIndex(int idx, int len) noexcept
    {
        while (idx < 0) idx += len;
        while (idx >= len) idx -= len;
        return idx;
    }
    
    // Hermite 4-point interpolation (smoother than linear)
    inline float hermite4(float frac, float y0, float y1, float y2, float y3) noexcept
    {
        float c0 = y1;
        float c1 = 0.5f * (y2 - y0);
        float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }
    
    // Pink Noise Generator (Voss-McCartney algorithm, instance-safe)
    inline float generatePinkNoise() noexcept
    {
        int changed = static_cast<int>(pinkNoiseCounter ^ (pinkNoiseCounter + 1));
        pinkNoiseCounter++;
        
        for (int i = 0; i < 8; i++) {
            if (changed & (1 << i)) {
                pinkNoiseRunningSum -= pinkOctaveBands[static_cast<size_t>(i)];
                pinkOctaveBands[static_cast<size_t>(i)] = fastRandBipolar();
                pinkNoiseRunningSum += pinkOctaveBands[static_cast<size_t>(i)];
            }
        }
        return pinkNoiseRunningSum * 0.125f;  // /8 for normalization
    }
    
    // Hysteresis Saturation (simplified Jiles-Atherton)
    inline float hysteresis(float input, float& mag) noexcept
    {
        using namespace threadbare::tuning;
        float target = std::tanh(input / Disintegration::kHysteresisSat);
        float delta = target - mag;
        float threshold = Disintegration::kHysteresisWidth * (1.0f - std::abs(mag));
        
        if (std::abs(delta) > threshold) {
            float sign = (delta > 0.0f) ? 1.0f : -1.0f;
            float excess = std::abs(delta) - threshold;
            mag += sign * excess * (1.0f - Disintegration::kHysteresisSmooth);
        }
        mag = std::clamp(mag, -1.0f, 1.0f);
        return mag * Disintegration::kHysteresisSat;
    }
    
    // Fast tanh approximation: x / (1 + |x| + 0.28*x^2)
    inline float fastTanh(float x) noexcept
    {
        float x2 = x * x;
        return x / (1.0f + std::abs(x) + 0.28f * x2);
    }
    
    // Fast antiderivative of tanh: approximation of ln(cosh(x))
    // CRITICAL: This is an EVEN function (symmetric across Y-axis)
    inline float fastTanhAD(float x) noexcept
    {
        float ax = std::abs(x);
        
        // Zone 1: Small x - Taylor series approximation (x^2/2)
        if (ax < 2.0f) {
            return 0.5f * x * x;  // Even function: x^2
        }
        
        // Zone 2: Large x - Linear asymptote: |x| - ln(2)
        return ax - 0.693147f;  // Even function: |x|
    }
    
    // ADAA (Anti-Derivative Anti-Aliasing) saturation
    inline float adaaFastTanh(float x, float& x1) noexcept
    {
        float diff = x - x1;
        float result;
        if (std::abs(diff) < 1e-5f) {
            result = fastTanh(0.5f * (x + x1));  // Midpoint fallback
        } else {
            result = (fastTanhAD(x) - fastTanhAD(x1)) / diff;
        }
        x1 = x;
        return result;
    }
    
    // === DISINTEGRATION DSP HELPERS ===
    // SVF implementation (Cytomic/Vadim TPT topology - correct formula)
    // g = tan(pi * fc / fs), k = 2 - 2*resonance (k=2 for no resonance)
    inline float processSvfHp(float input, SvfState& s, float g, float k) noexcept
    {
        // Coefficients
        const float a1 = 1.0f / (1.0f + g * (g + k));
        const float a2 = g * a1;
        const float a3 = g * a2;
        
        // Process
        const float v3 = input - s.ic2eq;
        const float v1 = a1 * s.ic1eq + a2 * v3;
        const float v2 = s.ic2eq + a2 * s.ic1eq + a3 * v3;
        
        // Update state
        s.ic1eq = 2.0f * v1 - s.ic1eq;
        s.ic2eq = 2.0f * v2 - s.ic2eq;
        
        return input - k * v1 - v2;  // HP output
    }
    
    // SVF Low-Pass implementation
    inline float processSvfLp(float input, SvfState& s, float g, float k) noexcept
    {
        // Coefficients
        const float a1 = 1.0f / (1.0f + g * (g + k));
        const float a2 = g * a1;
        const float a3 = g * a2;
        
        // Process
        const float v3 = input - s.ic2eq;
        const float v1 = a1 * s.ic1eq + a2 * v3;
        const float v2 = s.ic2eq + a2 * s.ic1eq + a3 * v3;
        
        // Update state
        s.ic1eq = 2.0f * v1 - s.ic1eq;
        s.ic2eq = 2.0f * v2 - s.ic2eq;
        
        return v2;  // LP output
    }
    
    // Early Reflections state (stereo multi-tap delay)
    std::vector<float> erBufferL;
    std::vector<float> erBufferR;
    int erWriteHead = 0;
    
    // Simple metering state (envelope followers)
    float inputMeterState = 0.0f;
    float tailMeterState = 0.0f;
    
    // Ducking envelope follower
    float duckingEnvelope = 0.0f;
    
    // DC offset removal on final output (not in feedback loop - safe!)
    float dcOffsetL = 0.0f;
    float dcOffsetR = 0.0f;
    
    // Helper functions
    float readDelayInterpolated(std::size_t lineIndex, float readPosition) const noexcept;
    float readGhostHistory(float readPosition) const noexcept;
    void trySpawnGrain(float ghostAmount, float puckX) noexcept;
    void processGhostEngine(float ghostAmount, float& outL, float& outR) noexcept;
    
    // Glitch Looper functions
    void processGlitchLooper(float& outL, float& outR, float glitchAmount, float safeTempo, float puckX, float puckY) noexcept;
    void triggerGlitchSlice(float tempo, float glitchAmount) noexcept;
    float readGhostHistoryInterpolated(float position) const noexcept;
};

} // namespace threadbare::dsp
