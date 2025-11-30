#include "UnravelReverb.h"

#include <algorithm>
#include <cmath>

namespace threadbare::dsp
{

namespace
{
constexpr float kEpsilon = 1.0e-6f;
constexpr float kParamDelta = 1.0e-4f;

inline float catmull(float y0, float y1, float y2, float y3, float alpha) noexcept
{
    const auto halfY0 = 0.5f * y0;
    const auto halfY3 = 0.5f * y3;
    return y1 + alpha * ((0.5f * y2 - halfY0)
            + (alpha * (((y0 + 2.0f * y2) - (halfY3 + 2.5f * y1))
            + (alpha * ((halfY3 + 1.5f * y1) - (halfY0 + 1.5f * y2))))));
}
} // namespace

const std::array<std::array<float, UnravelReverb::kNumLines>, UnravelReverb::kNumLines>
    UnravelReverb::feedbackMatrix = {{
        {  kHadamardNorm,  kHadamardNorm,  kHadamardNorm,  kHadamardNorm,
           kHadamardNorm,  kHadamardNorm,  kHadamardNorm,  kHadamardNorm },
        {  kHadamardNorm, -kHadamardNorm,  kHadamardNorm, -kHadamardNorm,
           kHadamardNorm, -kHadamardNorm,  kHadamardNorm, -kHadamardNorm },
        {  kHadamardNorm,  kHadamardNorm, -kHadamardNorm, -kHadamardNorm,
           kHadamardNorm,  kHadamardNorm, -kHadamardNorm, -kHadamardNorm },
        {  kHadamardNorm, -kHadamardNorm, -kHadamardNorm,  kHadamardNorm,
           kHadamardNorm, -kHadamardNorm, -kHadamardNorm,  kHadamardNorm },
        {  kHadamardNorm,  kHadamardNorm,  kHadamardNorm,  kHadamardNorm,
          -kHadamardNorm, -kHadamardNorm, -kHadamardNorm, -kHadamardNorm },
        {  kHadamardNorm, -kHadamardNorm,  kHadamardNorm, -kHadamardNorm,
          -kHadamardNorm,  kHadamardNorm, -kHadamardNorm,  kHadamardNorm },
        {  kHadamardNorm,  kHadamardNorm, -kHadamardNorm, -kHadamardNorm,
          -kHadamardNorm, -kHadamardNorm,  kHadamardNorm,  kHadamardNorm },
        {  kHadamardNorm, -kHadamardNorm, -kHadamardNorm,  kHadamardNorm,
          -kHadamardNorm,  kHadamardNorm,  kHadamardNorm, -kHadamardNorm },
    }};

const std::array<std::array<float, 2>, UnravelReverb::kNumLines> UnravelReverb::inputMatrix = {{
    { 0.70f,  0.10f },
    { 0.10f,  0.70f },
    { 0.50f,  0.50f },
    {-0.45f,  0.40f },
    { 0.40f, -0.45f },
    {-0.35f, -0.35f },
    { 0.25f, -0.15f },
    {-0.15f,  0.25f },
}};

const std::array<std::array<float, UnravelReverb::kNumLines>, 2> UnravelReverb::outputMatrix = {{
    { 0.32f, -0.28f, 0.26f, -0.22f, 0.20f, -0.18f, 0.16f, -0.14f },
    {-0.28f,  0.32f,-0.22f,  0.26f,-0.18f,  0.20f,-0.14f,  0.16f },
}};

void UnravelReverb::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = std::max(1.0, spec.sampleRate);
    sizeSmoother.reset(sampleRate, kSizeSlewSeconds);
    sizeSmoother.setCurrentAndTargetValue(1.0f);
    inputGainSmoother.reset(sampleRate, 0.05f);
    inputGainSmoother.setCurrentAndTargetValue(1.0f);

    updateDelayBases(sampleRate);

    for (std::size_t line = 0; line < kNumLines; ++line)
    {
        const float maxDelay = baseDelaySamples[line] * threadbare::tuning::Fdn::kSizeMax
                               + threadbare::tuning::Modulation::kMaxDepthSamples + 8.0f;
        const auto numSamples = static_cast<std::size_t>(std::ceil(maxDelay));

        delayLines[line].buffer.assign(numSamples + 8u, 0.0f);
        delayLines[line].writeIndex = 0;

        auto lowState = juce::dsp::IIR::Coefficients<float>::makeLowPass(
            sampleRate, threadbare::tuning::Damping::kMidCutoffHz);
        lowPassFilters[line].coefficients = lowState;
        lowPassStates[line] = lowState;

        auto highState = juce::dsp::IIR::Coefficients<float>::makeHighPass(
            sampleRate, threadbare::tuning::Damping::kLoopHighPassHz);
        highPassFilters[line].coefficients = highState;
        highPassStates[line] = highState;
    }

    updateFeedbackGains(3.0f, false);
    updateDampingFilters(0.0f);
    lastDecaySeconds = 3.0f;
    lastFreeze = false;
    lastTone = 0.0f;

    reset();
    isPrepared = true;
}

void UnravelReverb::reset() noexcept
{
    for (auto& line : delayLines)
    {
        std::fill(line.buffer.begin(), line.buffer.end(), 0.0f);
        line.writeIndex = 0;
    }

    for (auto& filter : lowPassFilters)
        filter.reset();

    for (auto& filter : highPassFilters)
        filter.reset();

    std::fill(loopOutputs.begin(), loopOutputs.end(), 0.0f);
    std::fill(feedbackVector.begin(), feedbackVector.end(), 0.0f);
    std::fill(lineInputs.begin(), lineInputs.end(), 0.0f);

    sizeSmoother.setCurrentAndTargetValue(1.0f);
    inputGainSmoother.setCurrentAndTargetValue(1.0f);
}

void UnravelReverb::process(std::span<float> left,
                            std::span<float> right,
                            const UnravelState& state) noexcept
{
    if (! isPrepared || left.size() == 0 || left.size() != right.size())
        return;

    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = left.size();
    const float targetSize = juce::jlimit(threadbare::tuning::Fdn::kSizeMin,
                                          threadbare::tuning::Fdn::kSizeMax,
                                          state.size);
    sizeSmoother.setTargetValue(targetSize);
    inputGainSmoother.setTargetValue(state.freeze ? 0.0f : 1.0f);

    if (state.freeze != lastFreeze
        || std::abs(state.decaySeconds - lastDecaySeconds) > kParamDelta)
    {
        updateFeedbackGains(state.decaySeconds, state.freeze);
        lastFreeze = state.freeze;
        lastDecaySeconds = state.decaySeconds;
    }

    if (std::abs(state.tone - lastTone) > kParamDelta)
    {
        updateDampingFilters(state.tone);
        lastTone = state.tone;
    }

    const float wet = juce::jlimit(0.0f, 1.0f, state.mix);
    const float dry = 1.0f - wet;

    for (std::size_t sample = 0; sample < numSamples; ++sample)
    {
        const float inL = left[sample];
        const float inR = right[sample];
        const float currentSize = sizeSmoother.getNextValue();
        const float inputGain = inputGainSmoother.getNextValue();

        for (std::size_t line = 0; line < kNumLines; ++line)
            lineInputs[line] = inputGain * (inputMatrix[line][0] * inL + inputMatrix[line][1] * inR);

        for (std::size_t line = 0; line < kNumLines; ++line)
        {
            const auto bufferLimit = static_cast<float>(delayLines[line].buffer.size() - 4u);
            float desiredDelay = baseDelaySamples[line] * currentSize;
            desiredDelay = juce::jlimit(1.0f, bufferLimit, desiredDelay);

            float sampleValue = readDelaySample(line, desiredDelay);
            sampleValue = lowPassFilters[line].processSample(sampleValue);
            sampleValue = highPassFilters[line].processSample(sampleValue);
            loopOutputs[line] = sampleValue;
        }

        for (std::size_t row = 0; row < kNumLines; ++row)
        {
            float sum = 0.0f;
            for (std::size_t col = 0; col < kNumLines; ++col)
                sum += feedbackMatrix[row][col] * loopOutputs[col];

            feedbackVector[row] = sum;
        }

        for (std::size_t line = 0; line < kNumLines; ++line)
        {
            const float writeSample = lineInputs[line]
                                    + feedbackVector[line] * feedbackGains[line]
                                    + threadbare::tuning::Safety::kAntiDenormal;
            writeDelaySample(line, writeSample);
        }

        float wetL = 0.0f;
        float wetR = 0.0f;

        for (std::size_t idx = 0; idx < kNumLines; ++idx)
        {
            wetL += outputMatrix[0][idx] * loopOutputs[idx];
            wetR += outputMatrix[1][idx] * loopOutputs[idx];
        }

        left[sample]  = dry * inL + wet * wetL;
        right[sample] = dry * inR + wet * wetR;
    }
}

void UnravelReverb::updateDelayBases(double newSampleRate)
{
    const auto sr = static_cast<float>(std::max(1.0, newSampleRate));
    for (std::size_t i = 0; i < kNumLines; ++i)
        baseDelaySamples[i] = 0.001f * threadbare::tuning::Fdn::kBaseDelaysMs[i] * sr;
}

void UnravelReverb::updateFeedbackGains(float decaySeconds, bool freeze) noexcept
{
    const float clampedDecay = juce::jlimit(threadbare::tuning::Decay::kT60Min,
                                            threadbare::tuning::Decay::kT60Max,
                                            decaySeconds);

    if (freeze)
    {
        feedbackGains.fill(threadbare::tuning::Freeze::kFrozenFeedback);
        return;
    }

    const float decay = std::max(clampedDecay, kEpsilon);
    constexpr float sixtyDb = -6.90775527898f; // ln(0.001)

    for (std::size_t i = 0; i < kNumLines; ++i)
    {
        const float delaySeconds = baseDelaySamples[i] / static_cast<float>(sampleRate);
        feedbackGains[i] = std::exp((sixtyDb * delaySeconds) / decay);
    }
}

void UnravelReverb::updateDampingFilters(float tone) noexcept
{
    const float toneClamped = juce::jlimit(-1.0f, 1.0f, tone);

    float cutoff = threadbare::tuning::Damping::kMidCutoffHz;
    if (toneClamped >= 0.0f)
        cutoff = juce::jmap(toneClamped,
                            0.0f,
                            1.0f,
                            threadbare::tuning::Damping::kMidCutoffHz,
                            threadbare::tuning::Damping::kHighCutoffHz);
    else
        cutoff = juce::jmap(toneClamped,
                            -1.0f,
                            0.0f,
                            threadbare::tuning::Damping::kLowCutoffHz,
                            threadbare::tuning::Damping::kMidCutoffHz);

    cutoff = std::clamp(cutoff, 20.0f, static_cast<float>(sampleRate * 0.45));

    const auto lowCoeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(sampleRate, cutoff);
    const auto highCoeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(
        sampleRate, threadbare::tuning::Damping::kLoopHighPassHz);

    for (std::size_t line = 0; line < kNumLines; ++line)
    {
        if (auto* lowPtr = lowPassStates[line].get())
        {
            auto* dst = lowPtr->getRawCoefficients();
            for (std::size_t i = 0; i < lowCoeffs.size(); ++i)
                dst[i] = lowCoeffs[i];
        }

        if (auto* highPtr = highPassStates[line].get())
        {
            auto* dst = highPtr->getRawCoefficients();
            for (std::size_t i = 0; i < highCoeffs.size(); ++i)
                dst[i] = highCoeffs[i];
        }
    }
}

float UnravelReverb::readDelaySample(std::size_t lineIndex, float delayInSamples) noexcept
{
    auto& state = delayLines[lineIndex];
    const int bufferSize = static_cast<int>(state.buffer.size());

    float readPosition = static_cast<float>(state.writeIndex) - delayInSamples;
    while (readPosition < 0.0f)
        readPosition += static_cast<float>(bufferSize);
    while (readPosition >= static_cast<float>(bufferSize))
        readPosition -= static_cast<float>(bufferSize);

    const int baseIndex = static_cast<int>(readPosition);
    const float frac = readPosition - static_cast<float>(baseIndex);

    const auto sampleAt = [&state, bufferSize](int idx) noexcept
    {
        idx %= bufferSize;
        if (idx < 0)
            idx += bufferSize;
        return state.buffer[static_cast<std::size_t>(idx)];
    };

    const float y0 = sampleAt(baseIndex - 1);
    const float y1 = sampleAt(baseIndex);
    const float y2 = sampleAt(baseIndex + 1);
    const float y3 = sampleAt(baseIndex + 2);

    return catmull(y0, y1, y2, y3, frac);
}

void UnravelReverb::writeDelaySample(std::size_t lineIndex, float sample) noexcept
{
    auto& state = delayLines[lineIndex];
    if (state.buffer.empty())
        return;

    state.buffer[state.writeIndex] = sample;
    state.writeIndex = (state.writeIndex + 1u) % state.buffer.size();
}

} // namespace threadbare::dsp

