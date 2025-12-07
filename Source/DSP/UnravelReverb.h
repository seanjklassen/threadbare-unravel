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
    float inLevel = 0.0f;
    float tailLevel = 0.0f;
    bool freeze = false;
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
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> ghostSmoother;
    
    // 8 delay lines for the FDN
    std::array<std::vector<float>, kNumLines> delayLines;
    std::array<int, kNumLines> writeIndices;
    
    // LFO state for modulation
    std::array<float, kNumLines> lfoPhases;
    std::array<float, kNumLines> lfoInc;
    
    // Low-pass filter state for damping
    std::array<float, kNumLines> lpState;
    
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
    void trySpawnGrain(float ghostAmount) noexcept;
    void processGhostEngine(float ghostAmount, float& outL, float& outR) noexcept;
};

} // namespace threadbare::dsp
