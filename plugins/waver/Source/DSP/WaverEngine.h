#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cstdint>
#include <span>

#include "ArpEngine.h"
#include "BbdChorus.h"
#include "OrganEngine.h"
#include "PrintChain.h"
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

    void setOrganDrawbars(float sub16, float fund8, float harm4, float mixture) noexcept;
    void setOrganLevel(float level) noexcept;
    void setPrintParams(float driveGain, float tapeSat, float wowDepth,
                        float flutterDepth, float hissLevel, float humFreqHz,
                        float printMix) noexcept;

    WaverVoiceAllocator& getAllocator() noexcept { return voiceAllocator; }
    ArpEngine& getArp() noexcept { return arp; }

    void setArpEnabled(bool on) noexcept;
    void setArpPuck(float puckX, float puckY) noexcept;
    void arpNoteOn(int midiNote, float velocity) noexcept;
    void arpNoteOff(int midiNote, float velocity) noexcept;

private:
    WaverVoiceAllocator voiceAllocator;
    ArpEngine arp;
    BbdChorus chorus;
    OrganEngine organ;
    PrintChain printChain;
    float organLevel = 0.3f;
    bool arpEnabled = false;
};
} // namespace threadbare::dsp
