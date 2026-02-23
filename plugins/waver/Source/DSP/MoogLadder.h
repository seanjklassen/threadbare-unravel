#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>

namespace threadbare::dsp
{
class MoogLadder
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec) noexcept;
    void reset() noexcept;
    void setCutoffHz(float hz) noexcept;
    void setResonance(float q) noexcept;
    float process(float input) noexcept;

private:
    juce::dsp::LadderFilter<float> ladder;
    std::array<float, 1> sampleScratch { 0.0f };
};
} // namespace threadbare::dsp
