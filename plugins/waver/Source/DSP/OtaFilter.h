#pragma once

#include <algorithm>
#include <cmath>

namespace threadbare::dsp
{
class OtaFilter
{
public:
    void prepare(double newSampleRate) noexcept;
    void reset() noexcept;

    void setCutoffHz(float hz) noexcept;
    void setResonance(float q) noexcept;

    float process(float input) noexcept;

private:
    float softClip(float x) const noexcept;

    double sampleRate = 44100.0;
    float cutoffHz = 8000.0f;
    float resonance = 0.15f;
    float g = 0.0f;
    float z1 = 0.0f;
    float z2 = 0.0f;
    float z3 = 0.0f;
    float z4 = 0.0f;
};
} // namespace threadbare::dsp
