#include "WaverVoiceAllocator.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace threadbare::dsp
{
void WaverVoiceAllocator::prepare(double sampleRate) noexcept
{
    for (auto& voice : voices)
    {
        voice.prepare(sampleRate);
        voice.setPortamento(glideMs, glideAlwaysMode);
    }
}

void WaverVoiceAllocator::reset() noexcept
{
    for (auto& voice : voices)
        voice.reset();
}

void WaverVoiceAllocator::noteOn(int noteNumber, float velocity) noexcept
{
    const bool legato = countHeldVoices() > 0;
    const bool shouldGlide = glideAlwaysMode || legato;

    if (auto* existing = findVoiceForNote(noteNumber))
    {
        if (shouldGlide)
            existing->setGlideStartFrequency(lastTriggerHz);
        existing->noteOn(noteNumber, velocity, false);
        lastTriggerHz = midiNoteToHz(noteNumber);
        return;
    }

    if (auto* freeVoice = findFreeVoice())
    {
        if (shouldGlide)
            freeVoice->setGlideStartFrequency(lastTriggerHz);
        freeVoice->noteOn(noteNumber, velocity, false);
        lastTriggerHz = midiNoteToHz(noteNumber);
        return;
    }

    if (auto* stolenVoice = chooseVoiceToSteal())
    {
        if (shouldGlide)
            stolenVoice->setGlideStartFrequency(lastTriggerHz);
        stolenVoice->noteOn(noteNumber, velocity, true);
        lastTriggerHz = midiNoteToHz(noteNumber);
    }
}

void WaverVoiceAllocator::noteOff(int noteNumber) noexcept
{
    if (auto* voice = findVoiceForNote(noteNumber))
        voice->noteOff(sustainPedalDown);
}

void WaverVoiceAllocator::setSustainPedal(bool isDown) noexcept
{
    sustainPedalDown = isDown;
    if (sustainPedalDown)
        return;

    for (auto& voice : voices)
    {
        if (!voice.isHeld() && voice.isSustained())
            voice.releaseFromSustain();
    }
}

void WaverVoiceAllocator::setPortamento(float newGlideMs, bool alwaysMode) noexcept
{
    glideMs = std::clamp(newGlideMs, 0.0f, 2000.0f);
    glideAlwaysMode = alwaysMode;
    for (auto& voice : voices)
        voice.setPortamento(glideMs, glideAlwaysMode);
}

void WaverVoiceAllocator::setFilter(float cutoffHz, float resonance, bool ladderMode) noexcept
{
    for (auto& voice : voices)
        voice.setFilter(cutoffHz, resonance, ladderMode);
}

void WaverVoiceAllocator::setWaveBlend(float blend) noexcept
{
    for (auto& voice : voices)
        voice.setWaveBlend(blend);
}

void WaverVoiceAllocator::setLfoToPwm(float depth) noexcept
{
    for (auto& voice : voices)
        voice.setLfoToPwm(depth);
}

void WaverVoiceAllocator::render(std::span<float> left, std::span<float> right) noexcept
{
    const auto sampleCount = left.size();
    for (std::size_t i = 0; i < sampleCount; ++i)
    {
        float mixed = 0.0f;
        for (auto& voice : voices)
            mixed += voice.processSample();

        left[i] = mixed;
        right[i] = mixed;
    }
}

WaverVoice* WaverVoiceAllocator::findFreeVoice() noexcept
{
    for (auto& voice : voices)
    {
        if (!voice.isActive())
            return &voice;
    }
    return nullptr;
}

WaverVoice* WaverVoiceAllocator::findVoiceForNote(int noteNumber) noexcept
{
    for (auto& voice : voices)
    {
        if (voice.isActive() && voice.getMidiNote() == noteNumber)
            return &voice;
    }
    return nullptr;
}

WaverVoice* WaverVoiceAllocator::chooseVoiceToSteal() noexcept
{
    WaverVoice* best = nullptr;
    float bestLevel = std::numeric_limits<float>::max();
    std::uint64_t bestAge = 0;

    for (auto& voice : voices)
    {
        if (!voice.isActive())
            return &voice;

        const float level = voice.getCurrentLevel();
        const auto age = voice.getAgeCounter();
        if (level < bestLevel || (level == bestLevel && age > bestAge))
        {
            best = &voice;
            bestLevel = level;
            bestAge = age;
        }
    }

    return best;
}

int WaverVoiceAllocator::countHeldVoices() const noexcept
{
    int held = 0;
    for (const auto& voice : voices)
    {
        if (voice.isActive() && voice.isHeld())
            ++held;
    }
    return held;
}

float WaverVoiceAllocator::midiNoteToHz(int noteNumber) noexcept
{
    return 440.0f * std::pow(2.0f, (static_cast<float>(noteNumber) - 69.0f) / 12.0f);
}
} // namespace threadbare::dsp
