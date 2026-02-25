#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include "NoiseFloor.h"
#include "Overdrive.h"
#include "TapeSaturation.h"
#include "WowFlutter.h"

#include <cstddef>
#include <vector>

namespace threadbare::dsp
{

// Fixed-order print chain: Overdrive -> Tape Saturation -> Wow/Flutter -> Noise Floor.
class PrintChain
{
public:
    void prepare(double sampleRate, std::size_t maxBlockSize) noexcept;
    void reset() noexcept;

    void setDriveGain(float gain01) noexcept;
    void setTapeSat(float sat01) noexcept;
    void setWowDepth(float depth01) noexcept;
    void setFlutterDepth(float depth01) noexcept;
    void setHissLevel(float level01) noexcept;
    void setHumFreq(float hz) noexcept;
    void setMix(float mix01) noexcept;
    void setAge(float age) noexcept;
    void setTransitionDelay(float delayMs) noexcept;

    void process(float* left, float* right, int numSamples) noexcept;

private:
    Overdrive overdriveL, overdriveR;
    TapeSaturation tapeL, tapeR;
    WowFlutter wowFlutter;
    NoiseFloor noiseFloor;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mix;
    std::vector<float> wetMixScratch;
    std::vector<float> wetL;
    std::vector<float> wetR;
};

} // namespace threadbare::dsp
