#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace threadbare::dsp
{

// Tape transport wow and flutter via modulated stereo fractional delay.
class WowFlutter
{
public:
    void prepare(double sampleRate, std::size_t maxBlockSize) noexcept;
    void reset() noexcept;
    void setWowDepth(float depth01) noexcept;
    void setFlutterDepth(float depth01) noexcept;
    void setAge(float age) noexcept;
    void setTransitionDelay(float delayMs) noexcept;
    void process(float* left, float* right, int numSamples) noexcept;

private:
    float cubicHermite(const float* buf, int size, float frac, int index) const noexcept;
    float nextNoise() noexcept;

    double sampleRate = 44100.0;
    float wowDepthMs = 0.0f;
    float flutterDepthMs = 0.0f;
    float ageParam = 0.0f;

    // Wow LFO (~1Hz).
    float wowPhase = 0.0f;
    float wowInc = 0.0f;

    // Flutter LFO (~25Hz).
    float flutterPhase = 0.0f;
    float flutterInc = 0.0f;

    // Delay buffers (pre-allocated in prepare).
    std::vector<float> delayL;
    std::vector<float> delayR;
    int writePos = 0;
    int delaySize = 0;

    std::uint32_t rngState = 0x12345678u;
    float noiseLpZ = 0.0f;

    float transitionDelayTarget = 0.0f;
    float transitionDelayCurrent = 0.0f;
    float transitionDelayCoeff = 0.0f;

    // Sub-bass crossover (2nd-order Butterworth LP at 100 Hz).
    // Sub-bass bypasses the modulated delay to prevent audible pitch wobble.
    struct CrossoverLp
    {
        float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float s1 = 0.0f, s2 = 0.0f;
    };
    CrossoverLp xoverL, xoverR;
};

} // namespace threadbare::dsp
