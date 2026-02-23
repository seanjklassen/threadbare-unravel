#include "ToyEngine.h"

#include <algorithm>
#include <numbers>

namespace threadbare::dsp
{

static constexpr float kOpllRatios[] = {
    0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f,
    7.0f, 8.0f, 9.0f, 10.0f, 12.0f, 15.0f
};
static constexpr int kOpllRatioCount = 13;

void ToyEngine::prepare(double sr) noexcept
{
    sampleRate = std::max(1.0, sr);
    reset();
}

void ToyEngine::reset() noexcept
{
    carrierPhase = 0.0f;
    modulatorPhase = 0.0f;
    lastModOutput = 0.0f;
}

void ToyEngine::setNote(float frequencyHz) noexcept
{
    carrierFreqHz = std::max(1.0f, frequencyHz);
    carrierInc = carrierFreqHz / static_cast<float>(sampleRate);
    modulatorInc = (carrierFreqHz * actualRatio) / static_cast<float>(sampleRate);
}

void ToyEngine::setModIndex(float index) noexcept
{
    modulationIndex = std::clamp(index, 0.0f, 8.0f);
}

void ToyEngine::setRatioNorm(float norm) noexcept
{
    ratioNorm = std::clamp(norm, 0.0f, 1.0f);
    actualRatio = quantizeToOpllRatio(ratioNorm);
    modulatorInc = (carrierFreqHz * actualRatio) / static_cast<float>(sampleRate);
}

void ToyEngine::setFeedback(float fb) noexcept
{
    feedbackAmount = std::clamp(fb, 0.0f, 1.0f);
}

void ToyEngine::setBitDepth(int bits) noexcept
{
    quantBits = std::clamp(bits, 4, 16);
    quantLevels = static_cast<float>(1 << quantBits);
}

void ToyEngine::setEnvelopeStepping(float stepping) noexcept
{
    envelopeStep = std::clamp(stepping, 0.0f, 1.0f);
}

float ToyEngine::processSample(float envelope) noexcept
{
    constexpr float twoPi = 2.0f * std::numbers::pi_v<float>;

    // Age-coupled envelope stepping: quantize the envelope to coarser levels.
    float env = envelope;
    if (envelopeStep > 0.0f)
    {
        const float steps = 4.0f + (1.0f - envelopeStep) * 60.0f;
        env = std::round(env * steps) / steps;
    }

    const float modPhaseAngle = twoPi * modulatorPhase + feedbackAmount * lastModOutput;
    const float modOutput = std::sin(modPhaseAngle);
    lastModOutput = modOutput;

    const float carrierPhaseAngle = twoPi * carrierPhase + modulationIndex * modOutput;
    float output = std::sin(carrierPhaseAngle);
    output = dacQuantize(output);
    output *= env;

    carrierPhase += carrierInc;
    if (carrierPhase >= 1.0f)
        carrierPhase -= 1.0f;

    modulatorPhase += modulatorInc;
    if (modulatorPhase >= 1.0f)
        modulatorPhase -= 1.0f;

    return output;
}

float ToyEngine::quantizeToOpllRatio(float normValue) noexcept
{
    const float scaled = normValue * static_cast<float>(kOpllRatioCount - 1);
    const int index = std::clamp(static_cast<int>(std::round(scaled)), 0, kOpllRatioCount - 1);
    return kOpllRatios[index];
}

float ToyEngine::dacQuantize(float sample) const noexcept
{
    return std::round(sample * quantLevels) / quantLevels;
}

} // namespace threadbare::dsp
