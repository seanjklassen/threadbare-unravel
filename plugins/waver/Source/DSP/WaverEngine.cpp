#include "WaverEngine.h"

#include <algorithm>

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
}

void WaverEngine::reset() noexcept
{
    voiceAllocator.reset();
    arp.reset();
    chorus.reset();
    organ.reset();
    printChain.reset();
    organLevel.setCurrentAndTargetValue(organLevel.getTargetValue());
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
