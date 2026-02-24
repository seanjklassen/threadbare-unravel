#pragma once

#include <array>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace threadbare::dsp
{

class ArpEngine
{
public:
    static constexpr int kMaxHeldNotes = 16;

    enum class Pattern : std::uint8_t
    {
        up = 0,
        down,
        upDown,
        downUp,
        random
    };

    struct NoteEvent
    {
        int noteNumber = -1;
        float velocity = 0.0f;
        bool isNoteOn = false;
    };

    void prepare(double sampleRate, std::uint32_t seed) noexcept;
    void reset() noexcept;

    void setEnabled(bool on) noexcept;
    bool isEnabled() const noexcept { return enabled; }

    void setRate(float hz) noexcept;
    void setGate(float ratio) noexcept;
    void setSwing(float amount) noexcept;
    void setPattern(Pattern p) noexcept;

    void setPuckParams(float puckX, float puckY) noexcept;

    void noteOn(int noteNumber, float velocity) noexcept;
    void noteOff(int noteNumber) noexcept;

    NoteEvent advance(int numSamples) noexcept;

private:
    int nextPatternNote() noexcept;
    std::uint32_t nextRandom() noexcept;

    bool enabled = false;
    double sr = 44100.0;
    float rateHz = 4.0f;
    float gateRatio = 0.5f;
    float swingAmount = 0.0f;
    Pattern pattern = Pattern::up;

    struct HeldNote
    {
        int noteNumber = -1;
        float velocity = 0.0f;
        bool active = false;
    };

    std::array<HeldNote, kMaxHeldNotes> heldNotes{};
    int heldCount = 0;

    std::array<int, kMaxHeldNotes> sortedIndices{};
    int sortedCount = 0;

    double phase = 0.0;
    bool noteIsOn = false;
    int currentStep = 0;
    int patternIndex = 0;
    bool ascending = true;
    int currentNote = -1;

    std::uint32_t rngState = 1;
};

} // namespace threadbare::dsp
