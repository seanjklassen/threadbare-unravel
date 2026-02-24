#pragma once

#include <cmath>
#include <cstdint>

namespace threadbare::dsp
{

// Two-operator PM engine inspired by YM2413/OPLL.
// Per-voice: carrier + modulator with DAC quantization.
class ToyEngine
{
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void setNote(float frequencyHz) noexcept;
    void setModIndex(float index) noexcept;
    void setRatioNorm(float normRatio) noexcept;
    void setFeedback(float fb) noexcept;
    void setBitDepth(int bits) noexcept;
    void setEnvelopeStepping(float stepping) noexcept;

    float processSample(float envelope) noexcept;

private:
    static float quantizeToOpllRatio(float normValue) noexcept;
    float dacQuantize(float sample) const noexcept;

    double sampleRate = 44100.0;
    float carrierPhase = 0.0f;
    float modulatorPhase = 0.0f;
    float carrierInc = 0.0f;
    float modulatorInc = 0.0f;
    float carrierFreqHz = 440.0f;
    float modulationIndex = 0.25f;
    float ratioNorm = 0.5f;
    float actualRatio = 2.0f;
    float feedbackAmount = 0.0f;
    float lastModOutput = 0.0f;
    int quantBits = 9;
    float quantLevels = 512.0f;
    float envelopeStep = 0.0f;
};

} // namespace threadbare::dsp
