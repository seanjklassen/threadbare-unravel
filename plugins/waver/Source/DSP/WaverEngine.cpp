#include "WaverEngine.h"

#include <algorithm>

namespace threadbare::dsp
{
void WaverEngine::prepare(const juce::dsp::ProcessSpec& spec, std::uint32_t driftSeed) noexcept
{
    voiceAllocator.prepare(spec.sampleRate, driftSeed);
    voiceAllocator.setPortamento(0.0f, false);
    chorus.prepare(spec.sampleRate, static_cast<std::size_t>(spec.maximumBlockSize));
    chorus.setMode(BbdChorus::Mode::modeI);
}

void WaverEngine::reset() noexcept
{
    voiceAllocator.reset();
    chorus.reset();
}

void WaverEngine::process(std::span<float> left, std::span<float> right) noexcept
{
    voiceAllocator.render(left, right);
    chorus.process(left.data(), right.data(), static_cast<int>(left.size()));
}

void WaverEngine::noteOn(int midiNote, float velocity) noexcept
{
    voiceAllocator.noteOn(midiNote, velocity);
}

void WaverEngine::noteOff(int midiNote, float velocity) noexcept
{
    juce::ignoreUnused(velocity);
    voiceAllocator.noteOff(midiNote);
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
} // namespace threadbare::dsp
