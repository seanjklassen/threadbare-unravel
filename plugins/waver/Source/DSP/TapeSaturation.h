#pragma once

namespace threadbare::dsp
{

// Cassette tape saturation with simplified hysteresis approximation.
class TapeSaturation
{
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void setDrive(float drive01) noexcept;
    float processSample(float input) noexcept;

private:
    double sr = 44100.0;
    float drive = 0.3f;

    // Tape head bandwidth LPF (~13kHz one-pole).
    float headCoeff = 0.0f;
    float headZ1 = 0.0f;

    // Hysteresis state (one-sample feedback).
    float hystState = 0.0f;
    float hystFeedback = 0.4f;
};

} // namespace threadbare::dsp
