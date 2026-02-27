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
    void setFilterKeyTrack(float amount) noexcept;
    void setEnvToFilter(float amount) noexcept;
    void setNoiseColor(float color) noexcept;
    void setStereoWidth(float width) noexcept;
    void setSubOctave(int octaveChoice) noexcept;
    void setPitchBendSemitones(float semitones) noexcept;
    void setModWheelDepth(float depth01) noexcept;
    void setAftertouchCutoffOffset(float offsetHz) noexcept;

    void setOrganDrawbars(float sub16, float fund8, float harm4, float mixture) noexcept;
    void setOrganLevel(float level) noexcept;
    void setPrintParams(float driveGain, float tapeSat, float wowDepth,
                        float flutterDepth, float hissLevel, float humFreqHz,
                        float printMix) noexcept;
    void setTransitionDelay(float delayMs) noexcept;

    WaverVoiceAllocator& getAllocator() noexcept { return voiceAllocator; }
    ArpEngine& getArp() noexcept { return arp; }

    void setArpEnabled(bool on) noexcept;
    void setArpPuck(float puckX, float puckY) noexcept;
    void setArpHostTempo(double bpm) noexcept;
    void arpNoteOn(int midiNote, float velocity) noexcept;
    void arpNoteOff(int midiNote, float velocity) noexcept;
    void arpAllNotesOff() noexcept;

private:
    WaverVoiceAllocator voiceAllocator;
    ArpEngine arp;
    BbdChorus chorus;
    OrganEngine organ;
    PrintChain printChain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> organLevel;
    bool arpEnabled = false;

    struct BiquadStage
    {
        float b0 = 1.0f, b1 = -2.0f, b2 = 1.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float s1L = 0.0f, s2L = 0.0f;
        float s1R = 0.0f, s2R = 0.0f;
    };

    // 4th-order Butterworth HPF (two cascaded biquad sections).
    BiquadStage hpfStage1, hpfStage2;

    // Low-end mono collapse (2nd-order Butterworth LP at 200 Hz on side channel).
    // We lowpass the side and subtract it, effectively highpassing the side so bass is mono
    // without time-varying cancellation artifacts.
    BiquadStage monoCollapseSide;

    // Gentle HF rolloff (one-pole LP at 18 kHz).
    float hfCoeff = 0.0f;
    float hfStateL = 0.0f, hfStateR = 0.0f;
};
} // namespace threadbare::dsp
