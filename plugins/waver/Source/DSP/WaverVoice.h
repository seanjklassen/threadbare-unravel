#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <cstdint>

#include "MoogLadder.h"
#include "OtaFilter.h"
#include "OuDrift.h"
#include "ToyEngine.h"
#include "WaverLFO.h"

namespace threadbare::dsp
{

struct ComponentTolerances
{
    float filterCutoffScale = 1.0f;
    float filterResScale = 1.0f;
    float vcaGainScale = 1.0f;
    float envAttackScale = 1.0f;
    float envReleaseScale = 1.0f;

    void computeFromSeed(std::uint32_t seed) noexcept;
};

class WaverVoice
{
public:
    void prepare(double newSampleRate, int voiceIndex, std::uint32_t driftSeed) noexcept;
    void reset() noexcept;

    void noteOn(int noteNumber, float velocity, bool stolen) noexcept;
    void noteOff(bool sustainPedalDown) noexcept;
    void releaseFromSustain() noexcept;
    void setPortamento(float glideMs, bool alwaysMode) noexcept;
    void setGlideStartFrequency(float hz) noexcept;
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

    float processSample() noexcept;

    bool isActive() const noexcept { return active; }
    bool isHeld() const noexcept { return held; }
    bool isSustained() const noexcept { return sustained; }
    int getMidiNote() const noexcept { return midiNote; }
    float getCurrentLevel() const noexcept { return currentLevel; }
    std::uint64_t getAgeCounter() const noexcept { return ageCounter; }
    float getOuState() const noexcept { return ouDrift.getState(); }

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

    float driftAmount = 0.0f;
    float ageParam = 0.0f;
    float lfoToVibratoCents = 0.0f;
    float layerDcoLevel = 1.0f;
    float layerToyLevel = 0.0f;

    float baseFilterCutoffHz = 8000.0f;
    float baseFilterRes = 0.15f;

    OtaFilter otaFilter;
    MoogLadder moogLadder;
    WaverLFO lfo;
    OuDrift ouDrift;
    ToyEngine toyEngine;
    ComponentTolerances tolerances;
};
} // namespace threadbare::dsp
