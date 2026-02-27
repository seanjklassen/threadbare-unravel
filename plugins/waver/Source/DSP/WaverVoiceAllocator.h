#pragma once

#include "WaverVoice.h"

#include <array>
#include <cstdint>
#include <span>

namespace threadbare::dsp
{
class WaverVoiceAllocator
{
public:
    static constexpr std::size_t kVoiceCount = 8;

    void prepare(double sampleRate, std::uint32_t driftSeed) noexcept;
    void reset() noexcept;

    void noteOn(int noteNumber, float velocity) noexcept;
    void noteOff(int noteNumber) noexcept;
    void releaseAllNotes() noexcept;
    void setSustainPedal(bool isDown) noexcept;
    void setPortamento(float glideMs, bool alwaysMode) noexcept;
    void setFilter(float cutoffHz, float resonance, bool ladderMode) noexcept;
    void setWaveBlend(float blend) noexcept;
    void setLfoToPwm(float depth) noexcept;
    void setDriftAmount(float amount) noexcept;
    void setAge(float age) noexcept;
    void setSubLevel(float level) noexcept;
    void setNoiseLevel(float level) noexcept;
    void setLfoRate(float hz) noexcept;
    void setLfoShape(int shape) noexcept;
    void setLfoToVibrato(float cents) noexcept;
    void setToyParams(float modIndex, float ratioNorm, float feedback) noexcept;
    void setLayerLevels(float dco, float toy) noexcept;
    void setEnvelopeParams(float attack, float decay, float sustain, float release) noexcept;
    void setFilterKeyTrack(float amount) noexcept;
    void setEnvToFilter(float amount) noexcept;
    void setNoiseColor(float color) noexcept;
    void setSubOctave(int octaveChoice) noexcept;
    void setPitchBendSemitones(float semitones) noexcept;
    void setModWheelDepth(float depth01) noexcept;
    void setAftertouchCutoffOffset(float offsetHz) noexcept;

    void render(std::span<float> left, std::span<float> right) noexcept;

    std::array<WaverVoice, kVoiceCount>& getVoices() noexcept { return voices; }

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
