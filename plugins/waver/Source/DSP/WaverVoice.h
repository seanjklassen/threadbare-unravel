#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <cstdint>

#include "MoogLadder.h"
#include "OtaFilter.h"
#include "WaverLFO.h"

namespace threadbare::dsp
{
class WaverVoice
{
public:
    void prepare(double newSampleRate) noexcept;
    void reset() noexcept;

    void noteOn(int noteNumber, float velocity, bool stolen) noexcept;
    void noteOff(bool sustainPedalDown) noexcept;
    void releaseFromSustain() noexcept;
    void setPortamento(float glideMs, bool alwaysMode) noexcept;
    void setGlideStartFrequency(float hz) noexcept;
    void setFilter(float cutoffHz, float resonance, bool ladderMode) noexcept;
    void setWaveBlend(float blend) noexcept;
    void setLfoToPwm(float depth) noexcept;

    float processSample() noexcept;

    bool isActive() const noexcept { return active; }
    bool isHeld() const noexcept { return held; }
    bool isSustained() const noexcept { return sustained; }
    int getMidiNote() const noexcept { return midiNote; }
    float getCurrentLevel() const noexcept { return currentLevel; }
    std::uint64_t getAgeCounter() const noexcept { return ageCounter; }

private:
    static float polyBlep(float t, float dt) noexcept;
    static float polyBlamp(float t, float dt) noexcept;
    void updateFrequencyFromMidi() noexcept;

    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParameters { 0.01f, 0.2f, 0.7f, 0.4f };

    double sampleRate = 44100.0;
    float phase = 0.0f;
    float subPhase = 0.0f;
    float phaseIncrement = 0.0f;
    float subPhaseIncrement = 0.0f;
    float currentFrequencyHz = 0.0f;
    float targetFrequencyHz = 0.0f;
    float glideCoeff = 0.0f;
    bool glideAlwaysMode = false;
    float velocityGain = 0.0f;
    float currentLevel = 0.0f;
    std::uint32_t stealRampRemaining = 0;
    std::uint32_t stealRampTotal = 0;
    std::uint32_t onsetRampRemaining = 0;
    std::uint32_t onsetRampTotal = 0;
    std::uint32_t retriggerRampRemaining = 0;
    std::uint32_t retriggerRampTotal = 0;
    std::uint64_t ageCounter = 0;
    std::uint32_t noiseState = 0xA341316Cu;

    int midiNote = -1;
    bool active = false;
    bool held = false;
    bool sustained = false;

    float waveBlend = 0.0f;
    float basePulseWidth = 0.5f;
    float lfoToPwmDepth = 0.0f;
    float subLevel = 0.2f;
    float noiseLevel = 0.1f;
    bool useLadderFilter = false;
    float dcBlockerR = 0.995f;
    float dcX1 = 0.0f;
    float dcY1 = 0.0f;
    float retriggerStartSample = 0.0f;
    float lastOutputSample = 0.0f;

    OtaFilter otaFilter;
    MoogLadder moogLadder;
    WaverLFO lfo;
};
} // namespace threadbare::dsp
