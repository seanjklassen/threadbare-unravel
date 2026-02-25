#include "WaverVoice.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../WaverTuning.h"

namespace threadbare::dsp
{

void ComponentTolerances::computeFromSeed(std::uint32_t seed) noexcept
{
    auto next = [&]() -> float {
        seed = seed * 1664525u + 1013904223u;
        return (static_cast<float>((seed >> 8) & 0xFFFFu) / 65535.0f) * 2.0f - 1.0f;
    };

    filterCutoffScale = 1.0f + next() * 0.02f;
    filterResScale = 1.0f + next() * 0.015f;
    const float vcaDb = next() * 0.5f;
    vcaGainScale = std::pow(10.0f, vcaDb / 20.0f);
    envAttackScale = 1.0f + next() * 0.04f;
    envReleaseScale = 1.0f + next() * 0.04f;
}

void WaverVoice::prepare(double newSampleRate, int voiceIndex, std::uint32_t driftSeed) noexcept
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

    ouDrift.prepare(sampleRate, driftSeed + static_cast<std::uint32_t>(voiceIndex) * 0x9E3779B9u);
    toyEngine.prepare(sampleRate);
    layerDcoLevel.reset(sampleRate, 0.015);
    layerToyLevel.reset(sampleRate, 0.015);
    layerDcoLevel.setCurrentAndTargetValue(1.0f);
    layerToyLevel.setCurrentAndTargetValue(0.0f);

    filterCutoffSmoothed.reset(sampleRate, 0.06);
    filterCutoffSmoothed.setCurrentAndTargetValue(8000.0f);
    filterResSmoothed.reset(sampleRate, 0.06);
    filterResSmoothed.setCurrentAndTargetValue(0.15f);

    const std::uint32_t tolSeed = driftSeed ^ (static_cast<std::uint32_t>(voiceIndex) * 2654435761u);
    tolerances.computeFromSeed(tolSeed);

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
    layerDcoLevel.setCurrentAndTargetValue(1.0f);
    layerToyLevel.setCurrentAndTargetValue(0.0f);
    filterCutoffSmoothed.setCurrentAndTargetValue(filterCutoffSmoothed.getTargetValue());
    filterResSmoothed.setCurrentAndTargetValue(filterResSmoothed.getTargetValue());
    pinkB0 = 0.0f;
    pinkB1 = 0.0f;
    pinkB2 = 0.0f;
    ouDrift.reset();
    toyEngine.reset();
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

    toyEngine.setNote(targetFrequencyHz);

    if (stolen)
    {
        phase = 0.0f;
        subPhase = 0.0f;
        lfo.reset();
        otaFilter.reset();
        moogLadder.reset();
        toyEngine.reset();
        toyEngine.setNote(targetFrequencyHz);
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

    adsr.setParameters({
        adsrParameters.attack * tolerances.envAttackScale,
        adsrParameters.decay,
        adsrParameters.sustain,
        adsrParameters.release * tolerances.envReleaseScale
    });
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

    // OU drift: per-sample advance for determinism.
    const float drift = ouDrift.processSample(driftAmount, ageParam);

    // Drift -> pitch: ±2-8 cents scaled by driftAmount.
    const float pitchCents = drift *
        (threadbare::tuning::waver::kDriftMinCents +
         driftAmount * (threadbare::tuning::waver::kDriftMaxCents - threadbare::tuning::waver::kDriftMinCents));
    const float pitchMultiplier = std::pow(2.0f, pitchCents / 1200.0f);

    // LFO vibrato: pitch modulation in cents.
    const float lfoValue = lfo.processSample();
    const float vibratoMultiplier = std::pow(2.0f, (lfoToVibratoCents * lfoValue) / 1200.0f);

    currentFrequencyHz += (targetFrequencyHz - currentFrequencyHz) * (1.0f - glideCoeff);
    const float driftedFreq = currentFrequencyHz * pitchMultiplier * vibratoMultiplier;
    phaseIncrement = driftedFreq / static_cast<float>(sampleRate);
    const float subFreq = currentFrequencyHz * pitchMultiplier;
    subPhaseIncrement = (subFreq * subOctaveMultiplier) / static_cast<float>(sampleRate);

    // DCO oscillator.
    float saw = 2.0f * phase - 1.0f;
    saw -= polyBlep(phase, phaseIncrement);

    const float pwRaw = (basePulseWidth + lfoToPwmDepth * lfoValue - 0.5f) * 2.22f;
    const float pw = 0.5f + std::tanh(pwRaw) * 0.45f;
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
    const float white = (static_cast<float>((noiseState >> 8) & 0x00FFFFFFu) / static_cast<float>(0x00FFFFFFu)) * 2.0f - 1.0f;
    pinkB0 = 0.99765f * pinkB0 + white * 0.0990460f;
    pinkB1 = 0.96300f * pinkB1 + white * 0.2965164f;
    pinkB2 = 0.57000f * pinkB2 + white * 1.0526913f;
    const float pink = (pinkB0 + pinkB1 + pinkB2 + white * 0.1848f) * 0.22f;
    const float noise = white + noiseColorMix * (pink - white);

    phase += phaseIncrement;
    if (phase >= 1.0f)
        phase -= 1.0f;

    float dcoOut = saw * (1.0f - waveBlend) + pulse * waveBlend;
    dcoOut += noise * noiseLevel;

    // Toy engine: shares envelope for AM, tracks same note.
    toyEngine.setNote(driftedFreq);
    float envelope = adsr.getNextSample();
    if (!adsr.isActive())
    {
        active = false;
        currentLevel = 0.0f;
        return 0.0f;
    }

    const float toyOut = toyEngine.processSample(envelope);

    // Layer mix.
    const float dcoLevel = layerDcoLevel.getNextValue();
    const float toyLevel = layerToyLevel.getNextValue();
    float layerMixed = dcoOut * dcoLevel + toyOut * toyLevel;

    // Filter with drift, key tracking, envelope modulation, and component tolerances.
    const float filterDriftScale = 1.0f + drift *
        (threadbare::tuning::waver::kFilterDriftMin +
         driftAmount * (threadbare::tuning::waver::kFilterDriftMax - threadbare::tuning::waver::kFilterDriftMin));
    const float keyTrackSemitones = static_cast<float>(midiNote - 60) * filterKeyTrackAmount;
    const float keyTrackScale = std::pow(2.0f, keyTrackSemitones / 12.0f);
    const float envFilterScale = 1.0f + envToFilterAmount * envelope * 4.0f;
    const float effectiveCutoff = filterCutoffSmoothed.getNextValue()
        * filterDriftScale * tolerances.filterCutoffScale
        * keyTrackScale * std::max(envFilterScale, 0.05f);
    const float effectiveRes = filterResSmoothed.getNextValue() * tolerances.filterResScale;

    otaFilter.setCutoffHz(std::clamp(effectiveCutoff, 20.0f, 20000.0f));
    otaFilter.setResonance(std::clamp(effectiveRes, 0.0f, 1.0f));
    moogLadder.setCutoffHz(std::clamp(effectiveCutoff, 20.0f, 20000.0f));
    moogLadder.setResonance(std::clamp(effectiveRes, 0.0f, 1.0f));

    float filtered = useLadderFilter ? moogLadder.process(layerMixed) : otaFilter.process(layerMixed);

    // Sub bypasses the filter to avoid resonant amplitude pumping from
    // filter cutoff drift/modulation at low frequencies.
    filtered += sub * subLevel * dcoLevel;

    const float dcBlocked = filtered - dcX1 + dcBlockerR * dcY1;
    dcX1 = filtered;
    dcY1 = dcBlocked;

    // Ramps.
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
    currentLevel = envelope * velocityGain * tolerances.vcaGainScale;
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
    subPhaseIncrement = (currentFrequencyHz * subOctaveMultiplier) / static_cast<float>(sampleRate);
}

void WaverVoice::setPortamento(float glideMs, bool alwaysMode) noexcept
{
    glideAlwaysMode = alwaysMode;
    if (glideMs <= 0.0f)
    {
        glideCoeff = 0.0f;
        return;
    }

    // Square the normalized value so the low end of the slider is gentle.
    // At max (2000ms) glide is still available but 80% of the range stays subtle.
    const float norm = glideMs / 2000.0f;
    const float shaped = norm * norm * 2000.0f;
    const float samples = static_cast<float>(sampleRate) * (shaped * 0.001f);
    glideCoeff = std::exp(-4.0f / std::max(samples, 1.0f));
}

void WaverVoice::setGlideStartFrequency(float hz) noexcept
{
    if (hz > 0.0f)
        currentFrequencyHz = hz;
}

void WaverVoice::setFilter(float cutoffHz, float resonanceValue, bool ladderMode) noexcept
{
    filterCutoffSmoothed.setTargetValue(cutoffHz);
    filterResSmoothed.setTargetValue(resonanceValue);
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

void WaverVoice::setDriftAmount(float amount) noexcept
{
    driftAmount = std::clamp(amount, 0.0f, 1.0f);
}

void WaverVoice::setAge(float age) noexcept
{
    ageParam = std::clamp(age, 0.0f, 1.0f);
    toyEngine.setEnvelopeStepping(ageParam * 0.8f);
}

void WaverVoice::setSubLevel(float level) noexcept
{
    subLevel = std::clamp(level, 0.0f, 1.0f);
}

void WaverVoice::setNoiseLevel(float level) noexcept
{
    noiseLevel = std::clamp(level, 0.0f, 1.0f);
}

void WaverVoice::setLfoRate(float hz) noexcept
{
    lfo.setRateHz(hz);
}

void WaverVoice::setLfoShape(int shape) noexcept
{
    lfo.setShape(static_cast<WaverLFO::Shape>(std::clamp(shape, 0, 3)));
}

void WaverVoice::setLfoToVibrato(float cents) noexcept
{
    lfoToVibratoCents = std::clamp(cents, 0.0f, 50.0f);
}

void WaverVoice::setToyParams(float modIndex, float ratioNorm, float feedback) noexcept
{
    toyEngine.setModIndex(modIndex * 4.0f);
    toyEngine.setRatioNorm(ratioNorm);
    toyEngine.setFeedback(feedback);
}

void WaverVoice::setLayerLevels(float dco, float toy) noexcept
{
    layerDcoLevel.setTargetValue(std::clamp(dco, 0.0f, 1.0f));
    layerToyLevel.setTargetValue(std::clamp(toy, 0.0f, 1.0f));
}

void WaverVoice::setEnvelopeParams(float attack, float decay, float sustain, float release) noexcept
{
    adsrParameters.attack = attack;
    adsrParameters.decay = decay;
    adsrParameters.sustain = sustain;
    adsrParameters.release = release;
    adsr.setParameters({
        attack * tolerances.envAttackScale,
        decay,
        sustain,
        release * tolerances.envReleaseScale
    });
}

void WaverVoice::setFilterKeyTrack(float amount) noexcept
{
    filterKeyTrackAmount = std::clamp(amount, 0.0f, 1.0f);
}

void WaverVoice::setEnvToFilter(float amount) noexcept
{
    envToFilterAmount = std::clamp(amount, -1.0f, 1.0f);
}

void WaverVoice::setNoiseColor(float color) noexcept
{
    noiseColorMix = std::clamp(color, 0.0f, 1.0f);
}

void WaverVoice::setSubOctave(int octaveChoice) noexcept
{
    subOctaveMultiplier = (octaveChoice == 1) ? 0.25f : 0.5f;
}
} // namespace threadbare::dsp
