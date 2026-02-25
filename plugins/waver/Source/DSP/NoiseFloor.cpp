#include "NoiseFloor.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace threadbare::dsp
{

void NoiseFloor::prepare(double sampleRate) noexcept
{
    sr = std::max(1.0, sampleRate);
    hissGain.reset(sr, 0.02);
    hissGain.setCurrentAndTargetValue(std::pow(10.0f, -60.0f / 20.0f));
    humPhase = 0.0f;
    humInc = 60.0f / static_cast<float>(sr);
    whirPhase = 0.0f;
    whirInc = 180.0f / static_cast<float>(sr);
    whirAmpPhase = 0.0f;
    whirAmpInc = 0.3f / static_cast<float>(sr);
    reset();
}

void NoiseFloor::reset() noexcept
{
    pinkB0 = 0.0f;
    pinkB1 = 0.0f;
    pinkB2 = 0.0f;
    humPhase = 0.0f;
    whirPhase = 0.0f;
    whirAmpPhase = 0.0f;
    hissGain.setCurrentAndTargetValue(hissGain.getTargetValue());
}

void NoiseFloor::setHissLevel(float level01) noexcept
{
    // -60 to -48 dBFS range.
    const float dB = -60.0f + std::clamp(level01, 0.0f, 1.0f) * 12.0f;
    hissGain.setTargetValue(std::pow(10.0f, dB / 20.0f));
}

void NoiseFloor::setHumFreq(float hz) noexcept
{
    humInc = std::clamp(hz, 40.0f, 70.0f) / static_cast<float>(sr);
}

void NoiseFloor::setAge(float age) noexcept
{
    ageParam = std::clamp(age, 0.0f, 1.0f);
}

float NoiseFloor::processSample() noexcept
{
    constexpr float twoPi = 2.0f * std::numbers::pi_v<float>;

    // Tape hiss (pink noise).
    const float hiss = pinkFilter(nextWhite()) * hissGain.getNextValue();

    // AC hum: fundamental + 2nd + 3rd harmonics at ~-72dBFS.
    constexpr float humGain = 0.00025f;
    const float hum = humGain * (
        std::sin(twoPi * humPhase) +
        0.4f * std::sin(twoPi * humPhase * 2.0f) +
        0.15f * std::sin(twoPi * humPhase * 3.0f));
    humPhase += humInc;
    if (humPhase >= 1.0f)
        humPhase -= 1.0f;

    // Mechanical whir (100-300Hz range, slow AM).
    const float whirAm = 0.5f + 0.5f * std::sin(twoPi * whirAmpPhase);
    const float whir = 0.0003f * (1.0f + ageParam) * whirAm * std::sin(twoPi * whirPhase);
    whirPhase += whirInc;
    if (whirPhase >= 1.0f)
        whirPhase -= 1.0f;
    whirAmpPhase += whirAmpInc;
    if (whirAmpPhase >= 1.0f)
        whirAmpPhase -= 1.0f;

    return hiss + hum + whir;
}

float NoiseFloor::nextWhite() noexcept
{
    rngState = rngState * 1664525u + 1013904223u;
    return (static_cast<float>((rngState >> 8) & 0xFFFFu) / 65535.0f) * 2.0f - 1.0f;
}

float NoiseFloor::pinkFilter(float white) noexcept
{
    // Paul Kellet's pink noise filter (simplified 3-band).
    pinkB0 = 0.99765f * pinkB0 + white * 0.0990460f;
    pinkB1 = 0.96300f * pinkB1 + white * 0.2965164f;
    pinkB2 = 0.57000f * pinkB2 + white * 1.0526913f;
    return (pinkB0 + pinkB1 + pinkB2 + white * 0.1848f) * 0.22f;
}

} // namespace threadbare::dsp
