#pragma once

#include <array>
#include <span>
#include <vector>

#include <juce_dsp/juce_dsp.h>
#include "../UnravelTuning.h"

namespace threadbare::dsp
{

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
    bool freeze = false;
    float tempo = 120.0f;  // BPM from DAW host
};

class UnravelReverb
{
public:
    UnravelReverb() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process(std::span<float> left, std::span<float> right, UnravelState& state) noexcept;

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
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> freezeAmountSmoother; // Smooth freeze transitions
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> preDelaySmoother;     // Smooth pre-delay changes
    
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
    
    // === MULTI-HEAD LOOP (Smooth bed sound) ===
    // Multiple read heads at staggered positions for constant pad
    static constexpr int kFreezeNumHeads = 6;
    
    std::vector<float> freezeLoopL;
    std::vector<float> freezeLoopR;
    int freezeLoopLength = 0;           // Actual captured length in samples
    bool freezeLoopActive = false;      // True when playing from loop buffer
    float freezeTransitionAmount = 0.0f; // 0 = FDN output, 1 = loop output
    
    // Warming filter state (removes icy highs from loop playback)
    float freezeLpfStateL = 0.0f;
    float freezeLpfStateR = 0.0f;
    
    // Per-head state: position, direction, modulation
    struct FreezeHead {
        float readPos = 0.0f;       // Current read position
        float direction = 1.0f;     // 1.0 = forward, -1.0 = reverse
        float modPhase = 0.0f;      // LFO phase for pitch modulation
        float modInc = 0.0f;        // LFO increment (set in prepare)
        float speedMod = 1.0f;      // Current playback speed (1.0 ± detune)
    };
    std::array<FreezeHead, kFreezeNumHeads> freezeHeads;
    
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
