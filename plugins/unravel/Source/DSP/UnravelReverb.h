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
    float loopLengthBars = 4.0f;    // Actual recorded length in bars
    bool looperStateAdvance = false; // Signal from processor to advance state
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
    
    // Spectral freeze state - locks grain spawn positions when freeze is active
    bool ghostFreezeActive = false;
    std::array<float, 8> frozenSpawnPositions;
    std::size_t numFrozenPositions = 0;
    
    // ═══════════════════════════════════════════════════════════════════════
    // DISINTEGRATION LOOPER STATE
    // ═══════════════════════════════════════════════════════════════════════
    LooperState currentLooperState = LooperState::Idle;
    int loopRecordHead = 0;
    int loopPlayHead = 0;
    int targetLoopLength = 0;           // In samples (calculated from tempo)
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
    float currentHpfG = 0.0f, currentHpfK = 0.0f;  // HPF g and k coefficients
    float currentLpfG = 0.0f, currentLpfK = 0.0f;  // LPF g and k coefficients
    float currentSatAmount = 0.0f;                  // Cached saturation amount
    
    // Separate diffuse LPF state for disintegration (avoids interference with freeze)
    float disintDiffuseLpfL = 0.0f;
    float disintDiffuseLpfR = 0.0f;
    
    // Exit fade: when entropy reaches 1.0, fade out to reverb over kFadeToReverbSeconds
    float exitFadeAmount = 1.0f;  // 1.0 = full loop, 0.0 = full reverb (then return to Idle)
    
    // Disintegration transition smoothers
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> loopGainSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> diffuseAmountSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> entropySmoother;
    
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
};

} // namespace threadbare::dsp
