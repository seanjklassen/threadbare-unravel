#pragma once

#include <juce_dsp/juce_dsp.h>
#include <span>

#include "BbdChorus.h"
#include "WaverVoiceAllocator.h"

namespace threadbare::dsp
{
class WaverEngine
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec) noexcept;
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

private:
    WaverVoiceAllocator voiceAllocator;
    BbdChorus chorus;
};
} // namespace threadbare::dsp
