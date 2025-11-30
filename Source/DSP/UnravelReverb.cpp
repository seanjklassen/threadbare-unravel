#include "UnravelReverb.h"

#include <algorithm>
#include <cmath>

namespace threadbare::dsp
{

namespace
{
constexpr float kEpsilon = 1.0e-6f;
constexpr float kParamDelta = 1.0e-4f;
constexpr float kTwoPi = juce::MathConstants<float>::twoPi;

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
    inputMeter.prepare(sampleRate,
                       threadbare::tuning::Metering::kAttackSec,
                       threadbare::tuning::Metering::kReleaseSec);
    tailMeter.prepare(sampleRate,
                      threadbare::tuning::Metering::kAttackSec,
                      threadbare::tuning::Metering::kReleaseSec);
    duckingFollower.prepare(sampleRate,
                            threadbare::tuning::Ducking::kAttackSec,
                            threadbare::tuning::Ducking::kReleaseSec);
    ghostRng.setSeedRandomly();

    updateDelayBases(sampleRate);
    prepareEarlyReflections(sampleRate);
    prepareGhostEngine(sampleRate);

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
    initialiseLfos();

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

    if (! erBuffer.empty())
    {
        std::fill(erBuffer.begin(), erBuffer.end(), 0.0f);
        erWriteIndex = 0;
    }

    resetGhostEngine();

    for (auto& filter : lowPassFilters)
        filter.reset();

    for (auto& filter : highPassFilters)
        filter.reset();

    std::fill(loopOutputs.begin(), loopOutputs.end(), 0.0f);
    std::fill(feedbackVector.begin(), feedbackVector.end(), 0.0f);
    std::fill(lineInputs.begin(), lineInputs.end(), 0.0f);
    sizeSmoother.setCurrentAndTargetValue(1.0f);
    inputGainSmoother.setCurrentAndTargetValue(1.0f);
    inputMeter.reset();
    tailMeter.reset();
    duckingFollower.reset();
}

void UnravelReverb::process(std::span<float> left,
                            std::span<float> right,
                            UnravelState& state) noexcept
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
    const float puckX = juce::jlimit(-1.0f, 1.0f, state.puckX);
    const float puckY = juce::jlimit(-1.0f, 1.0f, state.puckY);
    const float ghostControl = juce::jlimit(0.0f, 1.0f, state.ghost);
    const float ghostBonus = 0.5f * (puckY + 1.0f) * threadbare::tuning::PuckMapping::kGhostYBonus;
    const float ghostMix = juce::jlimit(0.0f, 1.0f, ghostControl + ghostBonus);
    const float ghostActiveMix = state.freeze ? 0.0f : ghostMix;
    const float erMix = juce::jmap(puckX,
                                   -1.0f,
                                   1.0f,
                                   threadbare::tuning::EarlyReflections::kBodyMix,
                                   threadbare::tuning::EarlyReflections::kAirMix);
    const float fdnMix = juce::jmap(puckX,
                                    -1.0f,
                                    1.0f,
                                    threadbare::tuning::EarlyReflections::kBodyFdnMix,
                                    threadbare::tuning::EarlyReflections::kAirFdnMix);
    const float erInjection = threadbare::tuning::EarlyReflections::kInjectionGain
                            / static_cast<float>(kNumLines);
    const float modDepth = calcModDepth(state.drift, puckY);

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
        const float monoInput = 0.5f * (inL + inR);
        const float erSample = processEarlyReflections(monoInput * inputGain) * erMix;
        const float duckEnv = duckingFollower.process(monoInput);
        const float erPerLine = erSample * erInjection;
        const float fdnInputScale = inputGain * fdnMix;
        writeGhostSample((monoInput * inputGain) + erSample);
        const float ghostSample = processGhost(ghostActiveMix);
        const float ghostPerLine = ghostSample / static_cast<float>(kNumLines);

        for (std::size_t line = 0; line < kNumLines; ++line)
        {
            const float base = inputMatrix[line][0] * inL + inputMatrix[line][1] * inR;
            lineInputs[line] = fdnInputScale * base + erPerLine + ghostPerLine;
        }

        for (std::size_t line = 0; line < kNumLines; ++line)
        {
            const auto bufferLimit = static_cast<float>(delayLines[line].buffer.size() - 4u);
            float desiredDelay = baseDelaySamples[line] * currentSize;
            const float lfoValue = std::sin(lfoPhases[line]);
            lfoPhases[line] += lfoIncrements[line];
            if (lfoPhases[line] >= kTwoPi)
                lfoPhases[line] -= kTwoPi;
            desiredDelay += lfoValue * modDepth;
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

        float duckGain = 1.0f - juce::jlimit(0.0f, 1.0f, state.duck) * duckEnv;
        const float minWet = threadbare::tuning::Ducking::kMinWetFactor;
        duckGain = juce::jlimit(minWet, 1.0f, duckGain);
        left[sample]  = dry * inL + wet * (wetL * duckGain);
        right[sample] = dry * inR + wet * (wetR * duckGain);

        const float dryMeter = std::max(std::abs(inL), std::abs(inR));
        const float wetMeter = std::max(std::abs(wetL), std::abs(wetR));
        updateMeters(dryMeter, wetMeter, state);
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

void UnravelReverb::prepareEarlyReflections(double newSampleRate)
{
    const float maxTapMs = threadbare::tuning::EarlyReflections::kTapTimesMs[kNumErTaps - 1];
    const auto bufferSamples = juce::jmax<std::size_t>(
        8u,
        static_cast<std::size_t>(std::ceil((maxTapMs * 0.001f + 0.01f) * newSampleRate)) + 4u);

    erBuffer.assign(bufferSamples, 0.0f);
    erWriteIndex = 0;

    for (std::size_t tap = 0; tap < kNumErTaps; ++tap)
    {
        const float tapSeconds = threadbare::tuning::EarlyReflections::kTapTimesMs[tap] * 0.001f;
        erTapOffsets[tap] = juce::jmax(1, static_cast<int>(std::round(tapSeconds * newSampleRate)));
        erTapGains[tap] = threadbare::tuning::EarlyReflections::kTapGains[tap];
    }
}

float UnravelReverb::processEarlyReflections(float inputSample) noexcept
{
    if (erBuffer.empty())
        return 0.0f;

    erBuffer[erWriteIndex] = inputSample;
    float sum = 0.0f;
    const int bufferSize = static_cast<int>(erBuffer.size());

    for (std::size_t tap = 0; tap < kNumErTaps; ++tap)
    {
        int readIndex = static_cast<int>(erWriteIndex) - erTapOffsets[tap];
        while (readIndex < 0)
            readIndex += bufferSize;

        sum += erBuffer[static_cast<std::size_t>(readIndex)] * erTapGains[tap];
    }

    erWriteIndex = (erWriteIndex + 1u) % erBuffer.size();
    return sum;
}

void UnravelReverb::initialiseLfos()
{
    juce::Random rng(static_cast<juce::int64>(sampleRate) ^ static_cast<juce::int64>(0x5F3759DF));

    for (std::size_t line = 0; line < kNumLines; ++line)
    {
        const float rate = juce::jmap(rng.nextFloat(),
                                      0.0f,
                                      1.0f,
                                      threadbare::tuning::Modulation::kMinRateHz,
                                      threadbare::tuning::Modulation::kMaxRateHz);
        lfoIncrements[line] = kTwoPi * rate / static_cast<float>(sampleRate);
        lfoPhases[line] = rng.nextFloat() * kTwoPi;
    }
}

float UnravelReverb::calcModDepth(float drift, float puckY) const noexcept
{
    const float clampedDrift = juce::jlimit(0.0f, 1.0f, drift);
    const float clampedY = juce::jlimit(-1.0f, 1.0f, puckY);
    const float half = threadbare::tuning::Fdn::kSizeMin;
    const float yContribution = (clampedY + 1.0f) * half * threadbare::tuning::PuckMapping::kDriftYBonus;
    const float total = juce::jlimit(0.0f, 1.0f, clampedDrift + yContribution);
    return total * threadbare::tuning::Modulation::kMaxDepthSamples;
}

void UnravelReverb::updateMeters(float dryLevel, float wetLevel, UnravelState& state) noexcept
{
    state.inLevel = inputMeter.process(dryLevel);
    state.tailLevel = tailMeter.process(wetLevel);
}

void UnravelReverb::prepareGhostEngine(double newSampleRate)
{
    const auto historySamples = static_cast<std::size_t>(
        std::ceil(threadbare::tuning::Ghost::kHistorySeconds * newSampleRate)) + 8u;
    ghostBuffer.assign(juce::jmax<std::size_t>(historySamples, 8u), 0.0f);
    ghostWriteIndex = 0;
    resetGhostEngine();
}

void UnravelReverb::resetGhostEngine() noexcept
{
    for (auto& grain : grains)
        grain.isActive = false;

    if (! ghostBuffer.empty())
        std::fill(ghostBuffer.begin(), ghostBuffer.end(), 0.0f);
    ghostWriteIndex = 0;
}

void UnravelReverb::writeGhostSample(float sample) noexcept
{
    if (ghostBuffer.empty())
        return;

    ghostBuffer[ghostWriteIndex] = sample;
    ghostWriteIndex = (ghostWriteIndex + 1u) % ghostBuffer.size();
}

float UnravelReverb::processGhost(float ghostMix) noexcept
{
    if (ghostBuffer.empty() || ghostMix <= 0.0f)
        return 0.0f;

    const float gainDb = juce::jmap(ghostMix,
                                    0.0f,
                                    1.0f,
                                    threadbare::tuning::Ghost::kMinGainDb,
                                    threadbare::tuning::Ghost::kMaxGainDb);
    const float ghostGain = juce::Decibels::decibelsToGain(gainDb);
    float sum = 0.0f;

    for (auto& grain : grains)
    {
        if (! grain.isActive)
            trySpawnGrain(grain, ghostMix);

        if (! grain.isActive)
            continue;

        const float window = 0.5f * (1.0f - std::cos(kTwoPi * grain.windowPhase));
        const float sample = readGhostBuffer(grain.position);
        sum += sample * window;

        grain.position += grain.speed;
        grain.windowPhase += grain.windowInc;

        if (grain.windowPhase >= 1.0f)
            grain.isActive = false;
    }

    return sum * ghostGain;
}

bool UnravelReverb::trySpawnGrain(Grain& grain, float ghostMix) noexcept
{
    if (ghostBuffer.empty())
        return false;

    const float probability = ghostMix * (kGhostSpawnRate / static_cast<float>(sampleRate));
    if (ghostRng.nextFloat() > probability)
        return false;

    const float historyWindowSec = 0.5f;
    const float offsetSamples = ghostRng.nextFloat() * historyWindowSec * static_cast<float>(sampleRate);
    float startPosition = static_cast<float>(ghostWriteIndex) - offsetSamples;

    const float durationSec = juce::jmap(ghostRng.nextFloat(),
                                         0.0f,
                                         1.0f,
                                         threadbare::tuning::Ghost::kGrainMinSec,
                                         threadbare::tuning::Ghost::kGrainMaxSec);
    const float durationSamples = juce::jmax(16.0f, durationSec * static_cast<float>(sampleRate));

    float detuneSemi = juce::jmap(ghostRng.nextFloat(),
                                  0.0f,
                                  1.0f,
                                  -threadbare::tuning::Ghost::kDetuneSemi,
                                   threadbare::tuning::Ghost::kDetuneSemi);
    if (ghostRng.nextFloat() < threadbare::tuning::Ghost::kShimmerProbability)
        detuneSemi = threadbare::tuning::Ghost::kShimmerSemi;

    const float speed = std::pow(2.0f, detuneSemi / 12.0f);

    grain.isActive = true;
    grain.position = startPosition;
    grain.speed = speed;
    grain.windowPhase = 0.0f;
    grain.windowInc = 1.0f / durationSamples;
    return true;
}

float UnravelReverb::readGhostBuffer(float position) const noexcept
{
    if (ghostBuffer.empty())
        return 0.0f;

    const int bufferSize = static_cast<int>(ghostBuffer.size());
    float pos = position;
    while (pos < 0.0f)
        pos += static_cast<float>(bufferSize);
    while (pos >= static_cast<float>(bufferSize))
        pos -= static_cast<float>(bufferSize);

    const int baseIndex = static_cast<int>(pos);
    const float frac = pos - static_cast<float>(baseIndex);

    const auto sampleAt = [this, bufferSize](int idx) noexcept
    {
        idx %= bufferSize;
        if (idx < 0)
            idx += bufferSize;
        return ghostBuffer[static_cast<std::size_t>(idx)];
    };

    const float y0 = sampleAt(baseIndex - 1);
    const float y1 = sampleAt(baseIndex);
    const float y2 = sampleAt(baseIndex + 1);
    const float y3 = sampleAt(baseIndex + 2);

    return catmull(y0, y1, y2, y3, frac);
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

void UnravelReverb::EnvelopeFollower::prepare(double sampleRate,
                                              float attackSeconds,
                                              float releaseSeconds) noexcept
{
    const float sr = static_cast<float>(sampleRate);
    const float attack = juce::jmax(attackSeconds, 1.0e-5f);
    const float release = juce::jmax(releaseSeconds, 1.0e-5f);
    attackCoeff = std::exp(-1.0f / (sr * attack));
    releaseCoeff = std::exp(-1.0f / (sr * release));
    value = 0.0f;
}

void UnravelReverb::EnvelopeFollower::reset() noexcept
{
    value = 0.0f;
}

float UnravelReverb::EnvelopeFollower::process(float input) noexcept
{
    const float target = std::abs(input);
    const float coeff = target > value ? attackCoeff : releaseCoeff;
    value = target + coeff * (value - target);
    return value;
}

} // namespace threadbare::dsp

