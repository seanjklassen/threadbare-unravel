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
    wetL.assign(maxBlockSize, 0.0f);
    wetR.assign(maxBlockSize, 0.0f);
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

void PrintChain::setTransitionDelay(float delayMs) noexcept
{
    wowFlutter.setTransitionDelay(delayMs);
}

void PrintChain::process(float* left, float* right, int numSamples) noexcept
{
    if (numSamples <= 0)
        return;
    const std::size_t scratchSize = wetMixScratch.size();
    const std::size_t wetSize = std::min(wetL.size(), wetR.size());
    if (wetSize == 0)
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        const float wet = mix.getNextValue();
        if (static_cast<std::size_t>(i) < scratchSize)
            wetMixScratch[static_cast<std::size_t>(i)] = wet;
        const float dryL = left[i];
        const float dryR = right[i];

        float wL = overdriveL.processSample(dryL);
        float wR = overdriveR.processSample(dryR);

        wL = tapeL.processSample(wL);
        wR = tapeR.processSample(wR);

        if (static_cast<std::size_t>(i) < wetSize)
        {
            wetL[static_cast<std::size_t>(i)] = wL;
            wetR[static_cast<std::size_t>(i)] = wR;
        }
    }

    // Wow/Flutter operates on the wet tape path so it scales with print mix.
    const int n = static_cast<int>(std::min<std::size_t>(static_cast<std::size_t>(numSamples), wetSize));
    wowFlutter.process(wetL.data(), wetR.data(), n);

    for (int i = 0; i < n; ++i)
    {
        const float wet = static_cast<std::size_t>(i) < scratchSize
            ? wetMixScratch[static_cast<std::size_t>(i)]
            : mix.getCurrentValue();
        const float dry = 1.0f - wet;
        left[i] = left[i] * dry + wetL[static_cast<std::size_t>(i)] * wet;
        right[i] = right[i] * dry + wetR[static_cast<std::size_t>(i)] * wet;
    }

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
