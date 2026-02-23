#pragma once

#include <cmath>
#include <cstdint>

namespace threadbare::dsp
{

struct OuDrift
{
    void prepare(double sampleRate, std::uint32_t seed) noexcept;
    void reset() noexcept;

    // Returns drift value in [-1, +1] range, scaled by amount.
    // Age (0-1) couples the OU correlation time: higher age = slower, stickier, deeper excursions.
    float processSample(float amount, float age) noexcept;

    float getState() const noexcept { return state; }
    void setState(float s) noexcept { state = s; }
    std::uint32_t getSeed() const noexcept { return rngState; }

private:
    float nextNoise() noexcept;

    float state = 0.0f;
    float baseAlpha = 0.9991f;
    float baseBeta = 0.042f;
    std::uint32_t rngState = 0;
};

} // namespace threadbare::dsp
