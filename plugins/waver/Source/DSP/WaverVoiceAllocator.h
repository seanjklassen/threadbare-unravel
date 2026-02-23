#pragma once

#include "WaverVoice.h"

#include <array>
#include <span>

namespace threadbare::dsp
{
class WaverVoiceAllocator
{
public:
    static constexpr std::size_t kVoiceCount = 8;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void noteOn(int noteNumber, float velocity) noexcept;
    void noteOff(int noteNumber) noexcept;
    void setSustainPedal(bool isDown) noexcept;
    void setPortamento(float glideMs, bool alwaysMode) noexcept;
    void setFilter(float cutoffHz, float resonance, bool ladderMode) noexcept;
    void setWaveBlend(float blend) noexcept;
    void setLfoToPwm(float depth) noexcept;

    void render(std::span<float> left, std::span<float> right) noexcept;

private:
    WaverVoice* findFreeVoice() noexcept;
    WaverVoice* findVoiceForNote(int noteNumber) noexcept;
    WaverVoice* chooseVoiceToSteal() noexcept;
    int countHeldVoices() const noexcept;
    static float midiNoteToHz(int noteNumber) noexcept;

    std::array<WaverVoice, kVoiceCount> voices;
    bool sustainPedalDown = false;
    float glideMs = 0.0f;
    bool glideAlwaysMode = false;
    float lastTriggerHz = 0.0f;
};
} // namespace threadbare::dsp
