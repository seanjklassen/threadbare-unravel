#include "WaverEngine.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace threadbare::dsp
{
void WaverEngine::prepare(const juce::dsp::ProcessSpec& spec, std::uint32_t driftSeed) noexcept
{
    voiceAllocator.prepare(spec.sampleRate, driftSeed);
    voiceAllocator.setPortamento(0.0f, false);
    arp.prepare(spec.sampleRate, driftSeed ^ 0xABCD1234u);
    chorus.prepare(spec.sampleRate, static_cast<std::size_t>(spec.maximumBlockSize));
    chorus.setMode(BbdChorus::Mode::modeI);
    organ.prepare(spec.sampleRate);
    printChain.prepare(spec.sampleRate, static_cast<std::size_t>(spec.maximumBlockSize));
    organLevel.reset(spec.sampleRate, 0.02);
    organLevel.setCurrentAndTargetValue(0.3f);

    // 4th-order Butterworth HPF at 45 Hz (two cascaded 2nd-order sections).
    constexpr float hpfCutoff = 45.0f;
    const float srF = static_cast<float>(spec.sampleRate);
    const float w0 = 2.0f * std::numbers::pi_v<float> * hpfCutoff / srF;
    const float cosW0 = std::cos(w0);
    const float sinW0 = std::sin(w0);

    // 4th-order Butterworth Q values: 1/(2*cos(pi/8)), 1/(2*cos(3*pi/8))
    constexpr float q1 = 0.54119610f;
    constexpr float q2 = 1.30656296f;

    auto computeHpfStage = [&](BiquadStage& stage, float q) {
        const float alpha = sinW0 / (2.0f * q);
        const float a0 = 1.0f + alpha;
        stage.b0 = ((1.0f + cosW0) * 0.5f) / a0;
        stage.b1 = -(1.0f + cosW0) / a0;
        stage.b2 = stage.b0;
        stage.a1 = (-2.0f * cosW0) / a0;
        stage.a2 = (1.0f - alpha) / a0;
        stage.s1L = stage.s2L = 0.0f;
        stage.s1R = stage.s2R = 0.0f;
    };

    computeHpfStage(hpfStage1, q1);
    computeHpfStage(hpfStage2, q2);

    // Low-end mono collapse: 2nd-order Butterworth LP at 200 Hz (per-channel).
    {
        constexpr float mcCutoff = 200.0f;
        const float mcW0 = 2.0f * std::numbers::pi_v<float> * mcCutoff / srF;
        const float mcCos = std::cos(mcW0);
        const float mcSin = std::sin(mcW0);
        const float mcAlpha = mcSin / std::sqrt(2.0f);
        const float mcA0 = 1.0f + mcAlpha;

        auto initLp = [&](BiquadStage& s) {
            s.b0 = ((1.0f - mcCos) * 0.5f) / mcA0;
            s.b1 = (1.0f - mcCos) / mcA0;
            s.b2 = s.b0;
            s.a1 = (-2.0f * mcCos) / mcA0;
            s.a2 = (1.0f - mcAlpha) / mcA0;
            s.s1L = s.s2L = 0.0f;
            s.s1R = s.s2R = 0.0f;
        };
        initLp(monoCollapseL);
        initLp(monoCollapseR);
    }

    // Gentle HF rolloff: one-pole LP at 18 kHz.
    {
        const float g = std::tan(std::numbers::pi_v<float> * std::min(18000.0f, srF * 0.49f) / srF);
        hfCoeff = g / (1.0f + g);
        hfStateL = hfStateR = 0.0f;
    }
}

void WaverEngine::reset() noexcept
{
    voiceAllocator.reset();
    arp.reset();
    chorus.reset();
    organ.reset();
    printChain.reset();
    organLevel.setCurrentAndTargetValue(organLevel.getTargetValue());
    hpfStage1.s1L = hpfStage1.s2L = 0.0f;
    hpfStage1.s1R = hpfStage1.s2R = 0.0f;
    hpfStage2.s1L = hpfStage2.s2L = 0.0f;
    hpfStage2.s1R = hpfStage2.s2R = 0.0f;
    monoCollapseL.s1L = monoCollapseL.s2L = 0.0f;
    monoCollapseL.s1R = monoCollapseL.s2R = 0.0f;
    monoCollapseR.s1L = monoCollapseR.s2L = 0.0f;
    monoCollapseR.s1R = monoCollapseR.s2R = 0.0f;
    hfStateL = hfStateR = 0.0f;
}

void WaverEngine::process(std::span<float> left, std::span<float> right) noexcept
{
    if (arpEnabled)
    {
        auto event = arp.advance(static_cast<int>(left.size()));
        if (event.noteNumber >= 0)
        {
            if (event.isNoteOn)
            {
                voiceAllocator.noteOn(event.noteNumber, event.velocity);
                organ.noteOn(event.noteNumber);
            }
            else
            {
                voiceAllocator.noteOff(event.noteNumber);
                organ.noteOff(event.noteNumber);
            }
        }
    }

    voiceAllocator.render(left, right);

    for (std::size_t i = 0; i < left.size(); ++i)
    {
        const float organSample = organ.processSample() * organLevel.getNextValue();
        left[i] += organSample;
        right[i] += organSample;
    }

    // BBD chorus (stereo widening).
    chorus.process(left.data(), right.data(), static_cast<int>(left.size()));

    // Print chain (overdrive -> tape -> wow/flutter -> noise floor).
    printChain.process(left.data(), right.data(), static_cast<int>(left.size()));

    // --- Master output chain ---
    auto applyBiquad = [](BiquadStage& s, float in, float& s1, float& s2) {
        const float out = s.b0 * in + s1;
        s1 = s.b1 * in - s.a1 * out + s2;
        s2 = s.b2 * in - s.a2 * out;
        return out;
    };

    for (std::size_t i = 0; i < left.size(); ++i)
    {
        float L = left[i];
        float R = right[i];

        // 1. Subsonic HPF (4th-order Butterworth, 45 Hz, 24 dB/oct).
        L = applyBiquad(hpfStage1, L, hpfStage1.s1L, hpfStage1.s2L);
        L = applyBiquad(hpfStage2, L, hpfStage2.s1L, hpfStage2.s2L);
        R = applyBiquad(hpfStage1, R, hpfStage1.s1R, hpfStage1.s2R);
        R = applyBiquad(hpfStage2, R, hpfStage2.s1R, hpfStage2.s2R);

        // 2. Low-end mono collapse (sum L+R below 200 Hz).
        const float bassL = applyBiquad(monoCollapseL, L, monoCollapseL.s1L, monoCollapseL.s2L);
        const float bassR = applyBiquad(monoCollapseR, R, monoCollapseR.s1L, monoCollapseR.s2L);
        const float monoBass = 0.5f * (bassL + bassR);
        L = L - bassL + monoBass;
        R = R - bassR + monoBass;

        // 3. HF rolloff (one-pole LP, 18 kHz).
        hfStateL += hfCoeff * (L - hfStateL);
        L = hfStateL;
        hfStateR += hfCoeff * (R - hfStateR);
        R = hfStateR;

        // 4. Soft clipper (tanh waveshaper).
        left[i] = std::tanh(L);
        right[i] = std::tanh(R);
    }
}

void WaverEngine::noteOn(int midiNote, float velocity) noexcept
{
    voiceAllocator.noteOn(midiNote, velocity);
    organ.noteOn(midiNote);
}

void WaverEngine::noteOff(int midiNote, float velocity) noexcept
{
    juce::ignoreUnused(velocity);
    voiceAllocator.noteOff(midiNote);
    organ.noteOff(midiNote);
}

void WaverEngine::setSustainPedal(bool isDown) noexcept
{
    voiceAllocator.setSustainPedal(isDown);
}

void WaverEngine::setPortamento(float glideMs, bool alwaysMode) noexcept
{
    voiceAllocator.setPortamento(glideMs, alwaysMode);
}

void WaverEngine::setChorusMode(int modeIndex) noexcept
{
    const int clamped = std::clamp(modeIndex, 0, 3);
    chorus.setMode(static_cast<BbdChorus::Mode>(clamped));
}

void WaverEngine::setFilter(float cutoffHz, float resonance, bool ladderMode) noexcept
{
    voiceAllocator.setFilter(cutoffHz, resonance, ladderMode);
}

void WaverEngine::setWaveBlend(float blend) noexcept
{
    voiceAllocator.setWaveBlend(blend);
}

void WaverEngine::setLfoToPwm(float depth) noexcept
{
    voiceAllocator.setLfoToPwm(depth);
}

void WaverEngine::setDriftAmount(float amount) noexcept
{
    voiceAllocator.setDriftAmount(amount);
}

void WaverEngine::setAge(float age) noexcept
{
    voiceAllocator.setAge(age);
    organ.setAge(age);
    printChain.setAge(age);
}

void WaverEngine::setSubLevel(float level) noexcept
{
    voiceAllocator.setSubLevel(level);
}

void WaverEngine::setNoiseLevel(float level) noexcept
{
    voiceAllocator.setNoiseLevel(level);
}

void WaverEngine::setLfoRate(float hz) noexcept
{
    voiceAllocator.setLfoRate(hz);
}

void WaverEngine::setLfoShape(int shape) noexcept
{
    voiceAllocator.setLfoShape(shape);
}

void WaverEngine::setLfoToVibrato(float cents) noexcept
{
    voiceAllocator.setLfoToVibrato(cents);
}

void WaverEngine::setToyParams(float modIndex, float ratioNorm, float feedback) noexcept
{
    voiceAllocator.setToyParams(modIndex, ratioNorm, feedback);
}

void WaverEngine::setLayerLevels(float dco, float toy) noexcept
{
    voiceAllocator.setLayerLevels(dco, toy);
}

void WaverEngine::setEnvelopeParams(float attack, float decay, float sustain, float release) noexcept
{
    voiceAllocator.setEnvelopeParams(attack, decay, sustain, release);
}

void WaverEngine::setFilterKeyTrack(float amount) noexcept
{
    voiceAllocator.setFilterKeyTrack(amount);
}

void WaverEngine::setEnvToFilter(float amount) noexcept
{
    voiceAllocator.setEnvToFilter(amount);
}

void WaverEngine::setNoiseColor(float color) noexcept
{
    voiceAllocator.setNoiseColor(color);
}

void WaverEngine::setStereoWidth(float width) noexcept
{
    chorus.setStereoWidth(width);
}

void WaverEngine::setSubOctave(int octaveChoice) noexcept
{
    voiceAllocator.setSubOctave(octaveChoice);
}

void WaverEngine::setOrganDrawbars(float sub16, float fund8, float harm4, float mixture) noexcept
{
    organ.setDrawbars(sub16, fund8, harm4, mixture);
}

void WaverEngine::setOrganLevel(float level) noexcept
{
    organLevel.setTargetValue(std::clamp(level, 0.0f, 1.0f));
}

void WaverEngine::setPrintParams(float driveGain, float tapeSat, float wowDepth,
                                  float flutterDepth, float hissLevel, float humFreqHz,
                                  float printMix) noexcept
{
    printChain.setDriveGain(driveGain);
    printChain.setTapeSat(tapeSat);
    printChain.setWowDepth(wowDepth);
    printChain.setFlutterDepth(flutterDepth);
    printChain.setHissLevel(hissLevel);
    printChain.setHumFreq(humFreqHz);
    printChain.setMix(printMix);
}

void WaverEngine::setTransitionDelay(float delayMs) noexcept
{
    printChain.setTransitionDelay(delayMs);
}

void WaverEngine::setArpEnabled(bool on) noexcept
{
    if (on == arpEnabled)
        return;

    if (on)
    {
        voiceAllocator.releaseAllNotes();
        organ.allNotesOff();
    }
    else
    {
        arp.allNotesOff();
        voiceAllocator.releaseAllNotes();
        organ.allNotesOff();
    }

    arpEnabled = on;
    arp.setEnabled(on);
}

void WaverEngine::setArpPuck(float puckX, float puckY) noexcept
{
    arp.setPuckParams(puckX, puckY);
}

void WaverEngine::setArpHostTempo(double bpm) noexcept
{
    arp.setHostTempo(bpm);
}

void WaverEngine::arpNoteOn(int midiNote, float velocity) noexcept
{
    if (arpEnabled)
        arp.noteOn(midiNote, velocity);
    else
    {
        voiceAllocator.noteOn(midiNote, velocity);
        organ.noteOn(midiNote);
    }
}

void WaverEngine::arpNoteOff(int midiNote, float velocity) noexcept
{
    juce::ignoreUnused(velocity);
    if (arpEnabled)
        arp.noteOff(midiNote);
    else
    {
        voiceAllocator.noteOff(midiNote);
        organ.noteOff(midiNote);
    }
}

void WaverEngine::arpAllNotesOff() noexcept
{
    arp.allNotesOff();
}
} // namespace threadbare::dsp
