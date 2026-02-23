#include "MoogLadder.h"

namespace threadbare::dsp
{
void MoogLadder::prepare(const juce::dsp::ProcessSpec& spec) noexcept
{
    ladder.prepare(spec);
    ladder.setMode(juce::dsp::LadderFilterMode::LPF24);
    ladder.setDrive(1.1f);
    ladder.setCutoffFrequencyHz(8000.0f);
    ladder.setResonance(0.15f);
}

void MoogLadder::reset() noexcept
{
    ladder.reset();
}

void MoogLadder::setCutoffHz(float hz) noexcept
{
    ladder.setCutoffFrequencyHz(hz);
}

void MoogLadder::setResonance(float q) noexcept
{
    ladder.setResonance(q);
}

float MoogLadder::process(float input) noexcept
{
    sampleScratch[0] = input;
    float* channels[] = { sampleScratch.data() };
    juce::dsp::AudioBlock<float> block(channels, 1, 1);
    juce::dsp::ProcessContextReplacing<float> context(block);
    ladder.process(context);
    return sampleScratch[0];
}
} // namespace threadbare::dsp
