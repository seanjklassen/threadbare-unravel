#include "PrintChain.h"

#include <algorithm>

namespace threadbare::dsp
{

void PrintChain::prepare(double sampleRate, std::size_t maxBlockSize) noexcept
{
    overdriveL.prepare(sampleRate);
    overdriveR.prepare(sampleRate);
    tapeL.prepare(sampleRate);
    tapeR.prepare(sampleRate);
    wowFlutter.prepare(sampleRate, maxBlockSize);
    noiseFloor.prepare(sampleRate);
    mix.reset(sampleRate, 0.02);
    mix.setCurrentAndTargetValue(0.75f);
    wetMixScratch.assign(maxBlockSize, mix.getCurrentValue());
}

void PrintChain::reset() noexcept
{
    overdriveL.reset();
    overdriveR.reset();
    tapeL.reset();
    tapeR.reset();
    wowFlutter.reset();
    noiseFloor.reset();
    mix.setCurrentAndTargetValue(mix.getTargetValue());
}

void PrintChain::setDriveGain(float gain01) noexcept
{
    overdriveL.setGain(gain01);
    overdriveR.setGain(gain01);
}

void PrintChain::setTapeSat(float sat01) noexcept
{
    tapeL.setDrive(sat01);
    tapeR.setDrive(sat01);
}

void PrintChain::setWowDepth(float depth01) noexcept
{
    wowFlutter.setWowDepth(depth01);
}

void PrintChain::setFlutterDepth(float depth01) noexcept
{
    wowFlutter.setFlutterDepth(depth01);
}

void PrintChain::setHissLevel(float level01) noexcept
{
    noiseFloor.setHissLevel(level01);
}

void PrintChain::setHumFreq(float hz) noexcept
{
    noiseFloor.setHumFreq(hz);
}

void PrintChain::setMix(float mix01) noexcept
{
    mix.setTargetValue(std::clamp(mix01, 0.0f, 1.0f));
}

void PrintChain::setAge(float age) noexcept
{
    wowFlutter.setAge(age);
    noiseFloor.setAge(age);
}

void PrintChain::process(float* left, float* right, int numSamples) noexcept
{
    if (numSamples <= 0)
        return;
    const std::size_t scratchSize = wetMixScratch.size();

    for (int i = 0; i < numSamples; ++i)
    {
        const float wet = mix.getNextValue();
        const float dry = 1.0f - wet;
        if (static_cast<std::size_t>(i) < scratchSize)
            wetMixScratch[static_cast<std::size_t>(i)] = wet;
        const float dryL = left[i];
        const float dryR = right[i];

        float wL = overdriveL.processSample(left[i]);
        float wR = overdriveR.processSample(right[i]);

        wL = tapeL.processSample(wL);
        wR = tapeR.processSample(wR);

        left[i] = wL;
        right[i] = wR;

        left[i] = dryL * dry + wL * wet;
        right[i] = dryR * dry + wR * wet;
    }

    // Wow/Flutter operates on the full mixed signal (per spec: post-mixdown).
    wowFlutter.process(left, right, numSamples);

    // Noise floor is additive.
    for (int i = 0; i < numSamples; ++i)
    {
        const float noiseVal = noiseFloor.processSample();
        const float wet = static_cast<std::size_t>(i) < scratchSize
            ? wetMixScratch[static_cast<std::size_t>(i)]
            : mix.getCurrentValue();
        left[i] += noiseVal * wet;
        right[i] += noiseVal * wet;
    }
}

} // namespace threadbare::dsp
