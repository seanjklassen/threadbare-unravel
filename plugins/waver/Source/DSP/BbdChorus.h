#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace threadbare::dsp
{
class BbdChorus
{
public:
    enum class Mode : std::uint8_t
    {
        off = 0,
        modeI = 1,
        modeII = 2,
        modeIPlusII = 3
    };

    void prepare(double newSampleRate, std::size_t maxBlockSize);
    void reset() noexcept;
    void setMode(Mode newMode) noexcept { mode = newMode; }
    void process(float* left, float* right, int numSamples) noexcept;

private:
    float readDelaySample(const std::vector<float>& line, float delaySamples, std::size_t writeIndex) const noexcept;

    double sampleRate = 44100.0;
    Mode mode = Mode::off;
    std::vector<float> delayL;
    std::vector<float> delayR;
    std::size_t writeIndex = 0;
    float phase1 = 0.0f;
    float phase2 = 0.5f;
    float noiseState = 0.0f;
    std::uint32_t noiseRng = 0x6D2B79F5u;
};
} // namespace threadbare::dsp
