#pragma once

#include <array>
#include <cstdint>

namespace threadbare::dsp
{

// Baldwin-style divide-down organ with pre-allocated phase accumulators.
// Global (not per-voice): note-on activates an index, note-off deactivates it.
class OrganEngine
{
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void noteOn(int noteNumber) noexcept;
    void noteOff(int noteNumber) noexcept;
    void allNotesOff() noexcept;

    void setDrawbars(float sub16, float fund8, float harm4, float mixture) noexcept;
    void setAge(float age) noexcept;
    void setFormant(float centerHz, float q) noexcept;

    float processSample() noexcept;

private:
    static constexpr int kNoteCount = 128;

    struct NoteState
    {
        float phase = 0.0f;
        float phaseInc = 0.0f;
        float clickRemaining = 0.0f;
        float clickGain = 0.0f;
        float leakEnvelope = 0.0f;
        bool active = false;
        bool wasActivated = false;
    };

    float formantProcess(float input) noexcept;

    std::array<NoteState, kNoteCount> notes{};
    double sampleRate = 44100.0;

    float draw16 = 5.0f / 8.0f;
    float draw8 = 4.0f / 8.0f;
    float draw4 = 2.0f / 8.0f;
    float drawMix = 3.0f / 8.0f;

    float ageParam = 0.0f;
    float leakageLevel = 0.0f;
    float leakDecayCoeff = 0.9999f;

    // Formant bandpass: second-order biquad state.
    float bpB0 = 0.0f, bpB1 = 0.0f, bpB2 = 0.0f;
    float bpA1 = 0.0f, bpA2 = 0.0f;
    float bpZ1 = 0.0f, bpZ2 = 0.0f;
    float formantCenter = 1000.0f;
    float formantQ = 0.9f;

    void recalcFormant() noexcept;
};

} // namespace threadbare::dsp
