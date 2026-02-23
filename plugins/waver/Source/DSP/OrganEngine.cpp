#include "OrganEngine.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace threadbare::dsp
{

void OrganEngine::prepare(double sr) noexcept
{
    sampleRate = std::max(1.0, sr);
    const auto srF = static_cast<float>(sampleRate);

    for (int i = 0; i < kNoteCount; ++i)
    {
        const float freq = 440.0f * std::pow(2.0f, (static_cast<float>(i) - 69.0f) / 12.0f);
        auto& n = notes[static_cast<std::size_t>(i)];
        n.phaseInc = freq / srF;
        n.phase = 0.0f;
        n.active = false;
        n.wasActivated = false;
        n.clickRemaining = 0.0f;
        n.clickGain = 0.0f;
        n.leakEnvelope = 0.0f;
    }

    // ~3 second decay for leakage after note-off.
    leakDecayCoeff = std::exp(-1.0f / (srF * 3.0f));

    recalcFormant();
}

void OrganEngine::reset() noexcept
{
    for (auto& n : notes)
    {
        n.phase = 0.0f;
        n.active = false;
        n.wasActivated = false;
        n.clickRemaining = 0.0f;
        n.clickGain = 0.0f;
        n.leakEnvelope = 0.0f;
    }
    bpZ1 = 0.0f;
    bpZ2 = 0.0f;
}

void OrganEngine::noteOn(int noteNumber) noexcept
{
    if (noteNumber < 0 || noteNumber >= kNoteCount)
        return;

    auto& n = notes[static_cast<std::size_t>(noteNumber)];
    n.active = true;
    n.wasActivated = true;
    n.leakEnvelope = 1.0f;

    const float clickMs = 8.0f;
    n.clickRemaining = static_cast<float>(sampleRate) * (clickMs * 0.001f);
    n.clickGain = 0.15f + ageParam * 0.45f;
}

void OrganEngine::noteOff(int noteNumber) noexcept
{
    if (noteNumber < 0 || noteNumber >= kNoteCount)
        return;

    notes[static_cast<std::size_t>(noteNumber)].active = false;
}

void OrganEngine::setDrawbars(float sub16, float fund8, float harm4, float mixture) noexcept
{
    draw16 = std::clamp(sub16, 0.0f, 8.0f) / 8.0f;
    draw8 = std::clamp(fund8, 0.0f, 8.0f) / 8.0f;
    draw4 = std::clamp(harm4, 0.0f, 8.0f) / 8.0f;
    drawMix = std::clamp(mixture, 0.0f, 8.0f) / 8.0f;
}

void OrganEngine::setAge(float age) noexcept
{
    ageParam = std::clamp(age, 0.0f, 1.0f);
    leakageLevel = ageParam * 0.008f;
}

void OrganEngine::setFormant(float centerHz, float q) noexcept
{
    formantCenter = std::clamp(centerHz, 200.0f, 4000.0f);
    formantQ = std::clamp(q, 0.3f, 4.0f);
    recalcFormant();
}

float OrganEngine::processSample() noexcept
{
    constexpr float twoPi = 2.0f * std::numbers::pi_v<float>;
    float sum = 0.0f;

    for (int i = 0; i < kNoteCount; ++i)
    {
        auto& n = notes[static_cast<std::size_t>(i)];

        // Phase accumulators always advance (divide-down behavior).
        n.phase += n.phaseInc;
        if (n.phase >= 1.0f)
            n.phase -= 1.0f;

        // Decay leakage envelope for inactive notes.
        if (!n.active && n.leakEnvelope > 0.0f)
            n.leakEnvelope *= leakDecayCoeff;

        const bool hasLeakage = n.wasActivated && !n.active && n.leakEnvelope > 1e-6f && leakageLevel > 0.0f;
        if (!n.active && n.clickRemaining <= 0.0f && !hasLeakage)
            continue;

        const float fund = std::sin(twoPi * n.phase);

        float tone = 0.0f;
        tone += draw16 * std::sin(twoPi * n.phase * 0.5f);
        tone += draw8 * fund;
        tone += draw4 * std::sin(twoPi * n.phase * 2.0f);
        tone += drawMix * 0.33f * (
            std::sin(twoPi * n.phase * 3.0f) +
            std::sin(twoPi * n.phase * 4.0f) +
            std::sin(twoPi * n.phase * 6.0f));

        float contribution = 0.0f;
        if (n.active)
            contribution = tone * 0.15f;

        if (hasLeakage)
            contribution = tone * leakageLevel * n.leakEnvelope;

        if (n.clickRemaining > 0.0f)
        {
            const float clickEnv = n.clickRemaining / (static_cast<float>(sampleRate) * 0.008f);
            contribution += n.clickGain * clickEnv * fund * 0.3f;
            n.clickRemaining -= 1.0f;
        }

        sum += contribution;
    }

    return formantProcess(sum);
}

float OrganEngine::formantProcess(float input) noexcept
{
    const float out = bpB0 * input + bpB1 * bpZ1 + bpB2 * bpZ2
                    - bpA1 * bpZ1 - bpA2 * bpZ2;
    bpZ2 = bpZ1;
    bpZ1 = out;
    return out;
}

void OrganEngine::recalcFormant() noexcept
{
    const float w0 = 2.0f * std::numbers::pi_v<float> * formantCenter / static_cast<float>(sampleRate);
    const float sinW = std::sin(w0);
    const float cosW = std::cos(w0);
    const float alpha = sinW / (2.0f * formantQ);

    const float a0Inv = 1.0f / (1.0f + alpha);
    bpB0 = (sinW * 0.5f) * a0Inv;
    bpB1 = 0.0f;
    bpB2 = -(sinW * 0.5f) * a0Inv;
    bpA1 = (-2.0f * cosW) * a0Inv;
    bpA2 = (1.0f - alpha) * a0Inv;
}

} // namespace threadbare::dsp
