#pragma once

namespace threadbare::dsp
{

// SD-1 style overdrive: pre-emphasis EQ, asymmetric tanh waveshaping, post-emphasis LPF.
class Overdrive
{
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void setGain(float gain01) noexcept;
    float processSample(float input) noexcept;

private:
    double sr = 44100.0;
    float gain = 1.0f;

    // Pre-emphasis bandpass (1kHz, Q ~0.8).
    float preB0 = 0.0f, preB1 = 0.0f, preB2 = 0.0f;
    float preA1 = 0.0f, preA2 = 0.0f;
    float preZ1 = 0.0f, preZ2 = 0.0f;

    // Post-emphasis LPF (~5kHz, 6dB/oct one-pole).
    float postCoeff = 0.0f;
    float postZ1 = 0.0f;

    void recalcCoeffs() noexcept;
};

} // namespace threadbare::dsp
