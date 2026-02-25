#include "TapeSaturation.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace threadbare::dsp
{

void TapeSaturation::prepare(double sampleRate) noexcept
{
    sr = std::max(1.0, sampleRate);
    headCoeff = 1.0f - std::exp(-2.0f * std::numbers::pi_v<float> * 13000.0f / static_cast<float>(sr));
    postCoeff = 1.0f - std::exp(-2.0f * std::numbers::pi_v<float> * 10000.0f / static_cast<float>(sr));
    reset();
}

void TapeSaturation::reset() noexcept
{
    headZ1 = 0.0f;
    postZ1 = 0.0f;
    hystState = 0.0f;
}

void TapeSaturation::setDrive(float drive01) noexcept
{
    drive = std::clamp(drive01, 0.0f, 1.0f);
    hystFeedback = 0.2f + drive * 0.5f;
}

float TapeSaturation::processSample(float input) noexcept
{
    // Tape head bandwidth pre-filter.
    headZ1 += headCoeff * (input - headZ1);

    // Hysteresis: one-sample feedback for magnetization stickiness.
    const float driveScale = 1.0f + drive * 4.0f;
    const float x = headZ1 * driveScale + hystState * hystFeedback;

    // Asymmetric soft clip modeling oxide saturation.
    float sat;
    if (x >= 0.0f)
        sat = std::tanh(x);
    else
        sat = std::tanh(x * 0.85f);

    hystState = sat * 0.3f;

    const float out = sat / driveScale;
    postZ1 += postCoeff * (out - postZ1);
    return postZ1;
}

} // namespace threadbare::dsp
