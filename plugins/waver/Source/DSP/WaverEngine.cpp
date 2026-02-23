#include "WaverEngine.h"

#include <algorithm>

namespace threadbare::dsp
{
void WaverEngine::prepare(const juce::dsp::ProcessSpec& spec) noexcept
{
    voiceAllocator.prepare(spec.sampleRate);
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
} // namespace threadbare::dsp
