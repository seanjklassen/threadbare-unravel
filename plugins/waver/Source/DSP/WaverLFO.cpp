#include "WaverLFO.h"

#include <cmath>
#include <numbers>

namespace threadbare::dsp
{
void WaverLFO::prepare(double newSampleRate) noexcept
{
    sampleRate = std::max(1.0, newSampleRate);
    setRateHz(rateHz);
    reset();
}

void WaverLFO::reset() noexcept
{
    phase = 0.0f;
    heldSample = 0.0f;
}

void WaverLFO::setRateHz(float hz) noexcept
{
    rateHz = std::clamp(hz, 0.1f, 30.0f);
    increment = rateHz / static_cast<float>(sampleRate);
}

void WaverLFO::setShape(Shape s) noexcept
{
    shape = s;
}

float WaverLFO::processSample() noexcept
{
    const bool wrapped = (phase + increment) >= 1.0f;
    phase += increment;
    if (phase >= 1.0f)
        phase -= 1.0f;

    switch (shape)
    {
        case Shape::tri:
        {
            return 4.0f * std::abs(phase - 0.5f) - 1.0f;
        }
        case Shape::sine:
            return std::sin(2.0f * std::numbers::pi_v<float> * phase);
        case Shape::square:
            return phase < 0.5f ? 1.0f : -1.0f;
        case Shape::sampleHold:
        {
            if (wrapped)
                heldSample = random01() * 2.0f - 1.0f;
            return heldSample;
        }
        default:
            return 0.0f;
    }
}

float WaverLFO::random01() noexcept
{
    rng = rng * 1664525u + 1013904223u;
    return static_cast<float>((rng >> 8) & 0x00FFFFFFu) / static_cast<float>(0x00FFFFFFu);
}
} // namespace threadbare::dsp
