#include "OtaFilter.h"

#include <numbers>

namespace threadbare::dsp
{
void OtaFilter::prepare(double newSampleRate) noexcept
{
    sampleRate = std::max(1.0, newSampleRate);
    setCutoffHz(cutoffHz);
    reset();
}

void OtaFilter::reset() noexcept
{
    z1 = z2 = z3 = z4 = 0.0f;
}

void OtaFilter::setCutoffHz(float hz) noexcept
{
    cutoffHz = std::clamp(hz, 20.0f, 20000.0f);
    const float wc = std::numbers::pi_v<float> * cutoffHz / static_cast<float>(sampleRate);
    g = std::tan(wc);
}

void OtaFilter::setResonance(float q) noexcept
{
    resonance = std::clamp(q, 0.0f, 1.0f);
}

float OtaFilter::process(float input) noexcept
{
    const float feedback = z4 * (resonance * 4.0f);
    float x = input - feedback;

    auto stage = [this](float s, float& z) {
        const float v = (s - z) * g / (1.0f + g);
        const float y = softClip(v + z);
        z = y + v;
        return y;
    };

    x = stage(x, z1);
    x = stage(x, z2);
    x = stage(x, z3);
    x = stage(x, z4);
    return x;
}

float OtaFilter::softClip(float x) const noexcept
{
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}
} // namespace threadbare::dsp
