#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cstdint>

namespace threadbare::dsp
{

// Composite noise floor: tape hiss (pink), AC hum, mechanical whir.
class NoiseFloor
{
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void setHissLevel(float level01) noexcept;
    void setHumFreq(float hz) noexcept;
    void setAge(float age) noexcept;
    float processSample() noexcept;

private:
    float nextWhite() noexcept;
    float pinkFilter(float white) noexcept;

    double sr = 44100.0;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> hissGain;
    float humPhase = 0.0f;
    float humInc = 0.0f;
    float whirPhase = 0.0f;
    float whirInc = 0.0f;
    float whirAmpPhase = 0.0f;
    float whirAmpInc = 0.0f;
    float ageParam = 0.0f;

    std::uint32_t rngState = 0xDEADBEEFu;

    // Voss-McCartney pink noise state (3 octave bands).
    float pinkB0 = 0.0f, pinkB1 = 0.0f, pinkB2 = 0.0f;
};

} // namespace threadbare::dsp
