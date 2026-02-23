#include "WaverVoice.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace threadbare::dsp
{
void WaverVoice::prepare(double newSampleRate) noexcept
{
    sampleRate = std::max(1.0, newSampleRate);
    adsr.setSampleRate(sampleRate);
    adsr.setParameters(adsrParameters);
    otaFilter.prepare(sampleRate);
    otaFilter.setCutoffHz(8000.0f);
    otaFilter.setResonance(0.15f);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 512;
    spec.numChannels = 1;
    moogLadder.prepare(spec);
    moogLadder.setCutoffHz(8000.0f);
    moogLadder.setResonance(0.15f);

    lfo.prepare(sampleRate);
    lfo.setRateHz(3.0f);
    lfo.setShape(WaverLFO::Shape::tri);

    dcBlockerR = std::exp((-2.0f * std::numbers::pi_v<float> * 10.0f) / static_cast<float>(sampleRate));
    reset();
}

void WaverVoice::reset() noexcept
{
    adsr.reset();
    phase = 0.0f;
    subPhase = 0.0f;
    phaseIncrement = 0.0f;
    subPhaseIncrement = 0.0f;
    currentFrequencyHz = 0.0f;
    targetFrequencyHz = 0.0f;
    glideCoeff = 0.0f;
    glideAlwaysMode = false;
    velocityGain = 0.0f;
    currentLevel = 0.0f;
    stealRampRemaining = 0;
    stealRampTotal = 0;
    onsetRampRemaining = 0;
    onsetRampTotal = 0;
    retriggerRampRemaining = 0;
    retriggerRampTotal = 0;
    ageCounter = 0;
    noiseState = 0xA341316Cu;
    midiNote = -1;
    active = false;
    held = false;
    sustained = false;
    dcX1 = 0.0f;
    dcY1 = 0.0f;
    retriggerStartSample = 0.0f;
    lastOutputSample = 0.0f;
}

void WaverVoice::noteOn(int noteNumber, float velocity, bool stolen) noexcept
{
    midiNote = noteNumber;
    velocityGain = juce::jlimit(0.0f, 1.0f, velocity);
    updateFrequencyFromMidi();
    if (currentFrequencyHz <= 0.0f)
        currentFrequencyHz = targetFrequencyHz;
    held = true;
    sustained = false;
    active = true;
    ageCounter = 0;

    // Keep continuity for normal retriggers; only hard-reset on stolen voices.
    if (stolen)
    {
        phase = 0.0f;
        subPhase = 0.0f;
        lfo.reset();
        otaFilter.reset();
        moogLadder.reset();
    }

    if (stolen)
    {
        stealRampTotal = static_cast<std::uint32_t>(sampleRate * 0.003);
        stealRampRemaining = stealRampTotal;
    }
    else
    {
        stealRampTotal = 0;
        stealRampRemaining = 0;
    }

    onsetRampTotal = static_cast<std::uint32_t>(sampleRate * 0.006);
    onsetRampRemaining = onsetRampTotal;
    retriggerRampTotal = static_cast<std::uint32_t>(sampleRate * 0.003);
    retriggerRampRemaining = retriggerRampTotal;
    retriggerStartSample = lastOutputSample;

    adsr.noteOn();
}

void WaverVoice::noteOff(bool sustainPedalDown) noexcept
{
    held = false;
    if (sustainPedalDown)
    {
        sustained = true;
        return;
    }

    adsr.noteOff();
}

void WaverVoice::releaseFromSustain() noexcept
{
    if (!sustained)
        return;

    sustained = false;
    adsr.noteOff();
}

float WaverVoice::processSample() noexcept
{
    if (!active)
        return 0.0f;

    currentFrequencyHz += (targetFrequencyHz - currentFrequencyHz) * (1.0f - glideCoeff);
    phaseIncrement = currentFrequencyHz / static_cast<float>(sampleRate);
    subPhaseIncrement = (currentFrequencyHz * 0.5f) / static_cast<float>(sampleRate);

    const float lfoValue = lfo.processSample();

    float saw = 2.0f * phase - 1.0f;
    saw -= polyBlep(phase, phaseIncrement);

    const float pw = juce::jlimit(0.05f, 0.95f, basePulseWidth + lfoToPwmDepth * lfoValue);
    float pulse = phase < pw ? 1.0f : -1.0f;
    pulse += polyBlamp(phase, phaseIncrement);
    float fallingEdge = phase - pw;
    if (fallingEdge < 0.0f)
        fallingEdge += 1.0f;
    pulse -= polyBlamp(fallingEdge, phaseIncrement);

    subPhase += subPhaseIncrement;
    if (subPhase >= 1.0f)
        subPhase -= 1.0f;

    const float sub = std::sin(2.0f * std::numbers::pi_v<float> * subPhase);
    noiseState = noiseState * 1664525u + 1013904223u;
    const float noise = (static_cast<float>((noiseState >> 8) & 0x00FFFFFFu) / static_cast<float>(0x00FFFFFFu)) * 2.0f - 1.0f;

    phase += phaseIncrement;
    if (phase >= 1.0f)
        phase -= 1.0f;

    float osc = saw * (1.0f - waveBlend) + pulse * waveBlend;
    osc += sub * subLevel;
    osc += noise * noiseLevel;

    float filtered = useLadderFilter ? moogLadder.process(osc) : otaFilter.process(osc);
    const float dcBlocked = filtered - dcX1 + dcBlockerR * dcY1;
    dcX1 = filtered;
    dcY1 = dcBlocked;

    float envelope = adsr.getNextSample();
    if (!adsr.isActive())
    {
        active = false;
        currentLevel = 0.0f;
        return 0.0f;
    }

    if (stealRampRemaining > 0 && stealRampTotal > 0)
    {
        const float ramp = 1.0f - static_cast<float>(stealRampRemaining) / static_cast<float>(stealRampTotal);
        envelope *= ramp;
        --stealRampRemaining;
    }

    if (onsetRampRemaining > 0 && onsetRampTotal > 0)
    {
        const float ramp = 1.0f - static_cast<float>(onsetRampRemaining) / static_cast<float>(onsetRampTotal);
        envelope *= ramp;
        --onsetRampRemaining;
    }

    ++ageCounter;
    currentLevel = envelope * velocityGain;
    float output = dcBlocked * currentLevel;

    if (retriggerRampRemaining > 0 && retriggerRampTotal > 0)
    {
        const float t = 1.0f - static_cast<float>(retriggerRampRemaining) / static_cast<float>(retriggerRampTotal);
        output = retriggerStartSample + (output - retriggerStartSample) * t;
        --retriggerRampRemaining;
    }

    lastOutputSample = output;
    return output;
}

float WaverVoice::polyBlep(float t, float dt) noexcept
{
    if (dt <= 0.0f)
        return 0.0f;

    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }

    if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }

    return 0.0f;
}

float WaverVoice::polyBlamp(float t, float dt) noexcept
{
    if (dt <= 0.0f)
        return 0.0f;

    if (t < dt)
    {
        t /= dt;
        return t - 0.5f * t * t - 0.5f;
    }

    if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return 0.5f * t * t + t + 0.5f;
    }

    return 0.0f;
}

void WaverVoice::updateFrequencyFromMidi() noexcept
{
    const float note = static_cast<float>(midiNote);
    targetFrequencyHz = 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
    if (currentFrequencyHz <= 0.0f)
        currentFrequencyHz = targetFrequencyHz;
    phaseIncrement = currentFrequencyHz / static_cast<float>(sampleRate);
    subPhaseIncrement = (currentFrequencyHz * 0.5f) / static_cast<float>(sampleRate);
}

void WaverVoice::setPortamento(float glideMs, bool alwaysMode) noexcept
{
    glideAlwaysMode = alwaysMode;
    if (glideMs <= 0.0f)
    {
        glideCoeff = 0.0f;
        return;
    }

    const float samples = static_cast<float>(sampleRate) * (glideMs * 0.001f);
    glideCoeff = std::exp(-1.0f / std::max(samples, 1.0f));
}

void WaverVoice::setGlideStartFrequency(float hz) noexcept
{
    if (hz > 0.0f)
        currentFrequencyHz = hz;
}

void WaverVoice::setFilter(float cutoffHz, float resonanceValue, bool ladderMode) noexcept
{
    otaFilter.setCutoffHz(cutoffHz);
    otaFilter.setResonance(resonanceValue);
    moogLadder.setCutoffHz(cutoffHz);
    moogLadder.setResonance(resonanceValue);
    useLadderFilter = ladderMode;
}

void WaverVoice::setWaveBlend(float blend) noexcept
{
    waveBlend = juce::jlimit(0.0f, 1.0f, blend);
}

void WaverVoice::setLfoToPwm(float depth) noexcept
{
    lfoToPwmDepth = juce::jlimit(0.0f, 1.0f, depth);
}
} // namespace threadbare::dsp
