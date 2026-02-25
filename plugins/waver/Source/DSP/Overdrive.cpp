#include "Overdrive.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace threadbare::dsp
{

void Overdrive::prepare(double sampleRate) noexcept
{
    sr = std::max(1.0, sampleRate);
    gain.reset(sr, 0.02);
    gain.setCurrentAndTargetValue(1.0f);
    recalcCoeffs();
    reset();
}

void Overdrive::reset() noexcept
{
    preZ1 = 0.0f;
    preZ2 = 0.0f;
    postZ1 = 0.0f;
    gain.setCurrentAndTargetValue(gain.getTargetValue());
}

void Overdrive::setGain(float gain01) noexcept
{
    gain.setTargetValue(1.0f + std::clamp(gain01, 0.0f, 1.0f) * 15.0f);
}

float Overdrive::processSample(float input) noexcept
{
    const float drive = gain.getNextValue();

    // Pre-emphasis bandpass.
    const float bpOut = preB0 * input + preB1 * preZ1 + preB2 * preZ2
                      - preA1 * preZ1 - preA2 * preZ2;
    preZ2 = preZ1;
    preZ1 = bpOut;

    const float mid = input + bpOut * 0.6f;

    // Asymmetric tanh waveshaping.
    float shaped;
    if (mid >= 0.0f)
        shaped = std::tanh(drive * mid);
    else
        shaped = std::tanh(drive * mid * 0.7f);

    // Post-emphasis one-pole LPF.
    postZ1 += postCoeff * (shaped - postZ1);
    return postZ1;
}

void Overdrive::recalcCoeffs() noexcept
{
    constexpr float centerHz = 1000.0f;
    constexpr float q = 0.8f;
    const float w0 = 2.0f * std::numbers::pi_v<float> * centerHz / static_cast<float>(sr);
    const float sinW = std::sin(w0);
    const float cosW = std::cos(w0);
    const float alpha = sinW / (2.0f * q);
    const float a0Inv = 1.0f / (1.0f + alpha);

    preB0 = (sinW * 0.5f) * a0Inv;
    preB1 = 0.0f;
    preB2 = -(sinW * 0.5f) * a0Inv;
    preA1 = (-2.0f * cosW) * a0Inv;
    preA2 = (1.0f - alpha) * a0Inv;

    constexpr float lpHz = 5000.0f;
    postCoeff = 1.0f - std::exp(-2.0f * std::numbers::pi_v<float> * lpHz / static_cast<float>(sr));
}

} // namespace threadbare::dsp
