#include "ArpEngine.h"

namespace threadbare::dsp
{

void ArpEngine::prepare(double sampleRate, std::uint32_t seed) noexcept
{
    sr = sampleRate;
    rngState = seed != 0 ? seed : 1;
    reset();
}

void ArpEngine::reset() noexcept
{
    for (auto& n : heldNotes)
        n = {};
    heldCount = 0;
    sortedCount = 0;
    phase = 0.0;
    noteIsOn = false;
    currentStep = 0;
    patternIndex = 0;
    ascending = true;
    currentNote = -1;
}

void ArpEngine::setEnabled(bool on) noexcept
{
    if (enabled && !on)
    {
        phase = 0.0;
        noteIsOn = false;
        currentStep = 0;
        patternIndex = 0;
        ascending = true;
        currentNote = -1;
    }
    enabled = on;
}

void ArpEngine::setRate(float hz) noexcept
{
    rateHz = std::max(0.5f, std::min(32.0f, hz));
}

void ArpEngine::setGate(float ratio) noexcept
{
    gateRatio = std::max(0.05f, std::min(0.95f, ratio));
}

void ArpEngine::setSwing(float amount) noexcept
{
    swingAmount = std::max(0.0f, std::min(0.75f, amount));
}

void ArpEngine::setPattern(Pattern p) noexcept
{
    pattern = p;
}

void ArpEngine::setPuckParams(float puckX, float puckY) noexcept
{
    const float normX = (puckX + 1.0f) * 0.5f;
    const float normY = (puckY + 1.0f) * 0.5f;

    setRate(0.5f + normX * 15.0f);

    if (normX < 0.25f)
        setPattern(Pattern::up);
    else if (normX < 0.5f)
        setPattern(Pattern::upDown);
    else if (normX < 0.75f)
        setPattern(Pattern::downUp);
    else
        setPattern(Pattern::random);

    setGate(0.15f + normY * 0.7f);
    setSwing(normY * 0.4f);
}

void ArpEngine::noteOn(int noteNumber, float velocity) noexcept
{
    for (auto& n : heldNotes)
    {
        if (n.active && n.noteNumber == noteNumber)
        {
            n.velocity = velocity;
            return;
        }
    }

    for (auto& n : heldNotes)
    {
        if (!n.active)
        {
            n.noteNumber = noteNumber;
            n.velocity = velocity;
            n.active = true;
            ++heldCount;

            sortedCount = 0;
            for (int i = 0; i < kMaxHeldNotes; ++i)
            {
                if (heldNotes[static_cast<std::size_t>(i)].active)
                    sortedIndices[static_cast<std::size_t>(sortedCount++)] = i;
            }
            std::sort(sortedIndices.begin(),
                      sortedIndices.begin() + sortedCount,
                      [this](int a, int b) {
                          return heldNotes[static_cast<std::size_t>(a)].noteNumber
                               < heldNotes[static_cast<std::size_t>(b)].noteNumber;
                      });
            return;
        }
    }
}

void ArpEngine::noteOff(int noteNumber) noexcept
{
    for (auto& n : heldNotes)
    {
        if (n.active && n.noteNumber == noteNumber)
        {
            n.active = false;
            --heldCount;

            sortedCount = 0;
            for (int i = 0; i < kMaxHeldNotes; ++i)
            {
                if (heldNotes[static_cast<std::size_t>(i)].active)
                    sortedIndices[static_cast<std::size_t>(sortedCount++)] = i;
            }
            std::sort(sortedIndices.begin(),
                      sortedIndices.begin() + sortedCount,
                      [this](int a, int b) {
                          return heldNotes[static_cast<std::size_t>(a)].noteNumber
                               < heldNotes[static_cast<std::size_t>(b)].noteNumber;
                      });
            return;
        }
    }
}

ArpEngine::NoteEvent ArpEngine::advance(int numSamples) noexcept
{
    NoteEvent event;

    if (!enabled || sortedCount == 0)
        return event;

    const double stepDuration = sr / static_cast<double>(rateHz);
    const bool isSwungStep = (currentStep & 1) != 0;
    const double swingOffset = isSwungStep ? swingAmount * stepDuration * 0.5 : 0.0;
    const double effectiveStepDuration = stepDuration + (isSwungStep ? swingOffset : -swingOffset * 0.5);

    const double gateEnd = effectiveStepDuration * gateRatio;

    const double prevPhase = phase;
    phase += static_cast<double>(numSamples);

    if (noteIsOn && phase >= gateEnd && prevPhase < gateEnd)
    {
        noteIsOn = false;
        event.noteNumber = currentNote;
        event.velocity = 0.0f;
        event.isNoteOn = false;
        return event;
    }

    if (phase >= effectiveStepDuration)
    {
        phase -= effectiveStepDuration;
        ++currentStep;

        const int note = nextPatternNote();
        if (note >= 0)
        {
            currentNote = heldNotes[static_cast<std::size_t>(note)].noteNumber;
            noteIsOn = true;
            event.noteNumber = currentNote;
            event.velocity = heldNotes[static_cast<std::size_t>(note)].velocity;
            event.isNoteOn = true;
        }
    }

    return event;
}

int ArpEngine::nextPatternNote() noexcept
{
    if (sortedCount == 0) return -1;

    int idx = -1;

    switch (pattern)
    {
        case Pattern::up:
            idx = patternIndex % sortedCount;
            ++patternIndex;
            break;

        case Pattern::down:
            idx = (sortedCount - 1) - (patternIndex % sortedCount);
            ++patternIndex;
            break;

        case Pattern::upDown:
        {
            const int cycle = std::max(1, sortedCount * 2 - 2);
            const int pos = patternIndex % cycle;
            idx = pos < sortedCount ? pos : cycle - pos;
            ++patternIndex;
            break;
        }

        case Pattern::downUp:
        {
            const int cycle = std::max(1, sortedCount * 2 - 2);
            const int pos = patternIndex % cycle;
            const int upIdx = pos < sortedCount ? pos : cycle - pos;
            idx = (sortedCount - 1) - upIdx;
            ++patternIndex;
            break;
        }

        case Pattern::random:
        {
            const auto r = nextRandom();
            idx = static_cast<int>(r % static_cast<std::uint32_t>(sortedCount));
            break;
        }
    }

    if (idx < 0 || idx >= sortedCount) idx = 0;
    return sortedIndices[static_cast<std::size_t>(idx)];
}

std::uint32_t ArpEngine::nextRandom() noexcept
{
    rngState ^= rngState << 13;
    rngState ^= rngState >> 17;
    rngState ^= rngState << 5;
    return rngState;
}

} // namespace threadbare::dsp
