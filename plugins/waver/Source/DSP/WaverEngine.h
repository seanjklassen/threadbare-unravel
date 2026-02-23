#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cstdint>
#include <span>

#include "BbdChorus.h"
#include "WaverVoiceAllocator.h"

namespace threadbare::dsp
{
class WaverEngine
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec, std::uint32_t driftSeed) noexcept;
    void reset() noexcept;
    void process(std::span<float> left, std::span<float> right) noexcept;
    void noteOn(int midiNote, float velocity) noexcept;
    void noteOff(int midiNote, float velocity) noexcept;
    void setSustainPedal(bool isDown) noexcept;
    void setPortamento(float glideMs, bool alwaysMode) noexcept;
    void setChorusMode(int modeIndex) noexcept;
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

    WaverVoiceAllocator& getAllocator() noexcept { return voiceAllocator; }

private:
    WaverVoiceAllocator voiceAllocator;
    BbdChorus chorus;
};
} // namespace threadbare::dsp
