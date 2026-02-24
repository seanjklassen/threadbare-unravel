#include "OuDrift.h"

#include <algorithm>
#include <numbers>

namespace threadbare::dsp
{

void OuDrift::prepare(double sampleRate, std::uint32_t seed) noexcept
{
    rngState = seed;
    const double srRatio = std::max(1.0, sampleRate) / 44100.0;
    baseAlpha = static_cast<float>(std::pow(0.9991, srRatio));
    baseBeta = static_cast<float>(0.042 * std::sqrt(srRatio));
    state = 0.0f;
}

void OuDrift::reset() noexcept
{
    state = 0.0f;
}

float OuDrift::processSample(float amount, float age) noexcept
{
    // Age coupling: higher age -> alpha closer to 1.0 (slower mean-reversion),
    // beta scaled down (less injection but state wanders further before correcting).
    const float ageFactor = std::clamp(age, 0.0f, 1.0f);
    const float alpha = baseAlpha + (1.0f - baseAlpha) * ageFactor * 0.7f;
    const float beta = baseBeta * (1.0f + ageFactor * 1.5f);

    const float noise = nextNoise();
    state = alpha * state + beta * noise;
    state = std::clamp(state, -4.0f, 4.0f);

    return state * std::clamp(amount, 0.0f, 1.0f);
}

float OuDrift::nextNoise() noexcept
{
    rngState = rngState * 1664525u + 1013904223u;
    // Convert to [-1, 1] with approximately Gaussian-like distribution via
    // central limit of two uniform samples (cheaper than Box-Muller).
    const float u1 = static_cast<float>((rngState >> 8) & 0xFFFFu) / 65535.0f;
    rngState = rngState * 1664525u + 1013904223u;
    const float u2 = static_cast<float>((rngState >> 8) & 0xFFFFu) / 65535.0f;
    return (u1 + u2 - 1.0f);
}

} // namespace threadbare::dsp
