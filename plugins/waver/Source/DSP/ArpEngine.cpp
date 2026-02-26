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
    pendingHead = 0;
    pendingCount = 0;
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
        pendingHead = 0;
        pendingCount = 0;
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

void ArpEngine::setHostTempo(double bpm) noexcept
{
    hostBpm = bpm;
}

void ArpEngine::setPuckParams(float puckX, float puckY) noexcept
{
    const float normX = (puckX + 1.0f) * 0.5f;
    const float normY = (puckY + 1.0f) * 0.5f;

    // 6 tempo-synced zones: slow (left) -> fast (right)
    // Divisor = notes per beat (e.g. 2.0 = eighth notes)
    static constexpr float kDivisors[] = {
        0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 6.0f
    };
    static constexpr Pattern kPatterns[] = {
        Pattern::up, Pattern::up, Pattern::upDown,
        Pattern::upDown, Pattern::downUp, Pattern::random
    };

    const int zone = std::min(5, static_cast<int>(normX * 6.0f));
    setPattern(kPatterns[zone]);

    const double bpm = hostBpm > 0.0 ? hostBpm : 120.0;
    setRate(static_cast<float>(bpm / 60.0 * static_cast<double>(kDivisors[zone])));

    setGate(0.15f + normY * 0.50f);
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

void ArpEngine::allNotesOff() noexcept
{
    for (auto& n : heldNotes)
        n.active = false;
    heldCount = 0;
    sortedCount = 0;
}

void ArpEngine::pushEvent(const NoteEvent& e) noexcept
{
    if (pendingCount >= kMaxPendingEvents)
        return;
    const int idx = (pendingHead + pendingCount) % kMaxPendingEvents;
    pendingEvents[static_cast<std::size_t>(idx)] = e;
    ++pendingCount;
}

ArpEngine::NoteEvent ArpEngine::popEvent() noexcept
{
    if (pendingCount <= 0)
        return {};
    const auto& e = pendingEvents[static_cast<std::size_t>(pendingHead)];
    pendingHead = (pendingHead + 1) % kMaxPendingEvents;
    --pendingCount;
    return e;
}

ArpEngine::NoteEvent ArpEngine::advance(int numSamples) noexcept
{
    if (pendingCount > 0)
        return popEvent();

    if (!enabled)
        return {};

    if (sortedCount == 0)
    {
        if (noteIsOn && currentNote >= 0)
        {
            noteIsOn = false;
            NoteEvent off;
            off.noteNumber = currentNote;
            off.velocity = 0.0f;
            off.isNoteOn = false;
            currentNote = -1;
            return off;
        }
        return {};
    }

    auto calcStepTiming = [&]() {
        const double stepDuration = sr / static_cast<double>(rateHz);
        const bool isSwung = (currentStep & 1) != 0;
        const double swing = isSwung ? swingAmount * stepDuration * 0.5 : 0.0;
        const double eDur = stepDuration + (isSwung ? swing : -swing * 0.5);
        return std::pair<double, double>{ eDur, eDur * gateRatio };
    };

    auto [effectiveStepDuration, gateEnd] = calcStepTiming();

    // Bug C fix: if a rate change moved gateEnd behind the current phase
    // since the last advance call, catch the missed note-off now (before
    // advancing phase further).
    if (noteIsOn && phase >= gateEnd)
    {
        noteIsOn = false;
        NoteEvent off;
        off.noteNumber = currentNote;
        off.velocity = 0.0f;
        off.isNoteOn = false;
        pushEvent(off);
    }

    const double prevPhase = phase;
    phase += static_cast<double>(std::max(0, numSamples));

    // Normal gate-end crossing detection within this block.
    if (noteIsOn && phase >= gateEnd && prevPhase < gateEnd)
    {
        noteIsOn = false;
        NoteEvent off;
        off.noteNumber = currentNote;
        off.velocity = 0.0f;
        off.isNoteOn = false;
        pushEvent(off);
    }

    // Advance through step boundaries. Recalculate timing per step (Bug D fix).
    // Push noteOff + noteOn pairs so no events are lost (Bug B fix).
    int safety = 0;
    while (phase >= effectiveStepDuration && safety++ < 8)
    {
        if (noteIsOn)
        {
            noteIsOn = false;
            NoteEvent off;
            off.noteNumber = currentNote;
            off.velocity = 0.0f;
            off.isNoteOn = false;
            pushEvent(off);
        }

        phase -= effectiveStepDuration;
        ++currentStep;

        std::tie(effectiveStepDuration, gateEnd) = calcStepTiming();

        const int note = nextPatternNote();
        if (note >= 0)
        {
            currentNote = heldNotes[static_cast<std::size_t>(note)].noteNumber;

            // Only start the note if there's audible gate time remaining.
            // If the phase remainder already exceeds gateEnd, the note's
            // entire duration fell within this block -- skip it to avoid
            // zero-duration note-on/off pairs that accumulate release tails.
            if (phase < gateEnd)
            {
                noteIsOn = true;

                NoteEvent on;
                on.noteNumber = currentNote;
                on.velocity = heldNotes[static_cast<std::size_t>(note)].velocity;
                on.isNoteOn = true;
                pushEvent(on);
            }
        }
    }

    return popEvent();
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
            if (sortedCount <= 1)
            {
                idx = 0;
            }
            else
            {
                for (int attempt = 0; attempt < 3; ++attempt)
                {
                    idx = static_cast<int>(nextRandom() % static_cast<std::uint32_t>(sortedCount));
                    if (heldNotes[static_cast<std::size_t>(sortedIndices[static_cast<std::size_t>(idx)])].noteNumber != currentNote)
                        break;
                }
            }
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
