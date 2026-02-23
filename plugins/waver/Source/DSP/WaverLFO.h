#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace threadbare::dsp
{
class WaverLFO
{
public:
    enum class Shape : std::uint8_t
    {
        tri = 0,
        sine = 1,
        square = 2,
        sampleHold = 3
    };

    void prepare(double newSampleRate) noexcept;
    void reset() noexcept;
    void setRateHz(float hz) noexcept;
    void setShape(Shape s) noexcept;
    float processSample() noexcept;

private:
    float random01() noexcept;

    double sampleRate = 44100.0;
    float rateHz = 3.0f;
    float phase = 0.0f;
    float increment = 0.0f;
    float heldSample = 0.0f;
    Shape shape = Shape::tri;
    std::uint32_t rng = 0x12345678u;
};
} // namespace threadbare::dsp
