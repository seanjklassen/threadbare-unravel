#pragma once

#include <span>
#include <vector>

#include <juce_dsp/juce_dsp.h>

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
    int sampleRate = 48000;
    int writeIndex = 0;
    
    std::vector<float> delayBufferL;
    std::vector<float> delayBufferR;
};

} // namespace threadbare::dsp
