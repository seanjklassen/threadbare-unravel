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
    
    int sampleRate = 48000;
    
    // 8 delay lines for the FDN
    std::array<std::vector<float>, kNumLines> delayLines;
    std::array<int, kNumLines> writeIndices;
};

} // namespace threadbare::dsp
