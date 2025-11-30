#pragma once

#include <array>
#include <limits>
#include <span>
#include <vector>

#include <juce_dsp/juce_dsp.h>

#include "UnravelTuning.h"

namespace threadbare::dsp
{

struct UnravelState
{
    float size = 1.0f;
    float decaySeconds = 5.0f;
    float tone = 0.0f;
    float mix = 0.5f;
    float drift = 0.0f;
    float puckX = 0.0f;
    float puckY = 0.0f;
    float ghost = 0.0f;
    float duck = 0.0f;
    float inLevel = 0.0f;
    float tailLevel = 0.0f;
    bool freeze = false;
};

class UnravelReverb
{
public:
    UnravelReverb() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process(std::span<float> left, std::span<float> right, UnravelState& state) noexcept;

private:
    static constexpr std::size_t kNumLines = threadbare::tuning::Fdn::kNumLines;
    static constexpr float kHadamardNorm = 0.35355339059327379f; // 1 / sqrt(8)
    static constexpr float kSizeSlewSeconds = 0.02f;
    static constexpr std::size_t kNumErTaps = threadbare::tuning::EarlyReflections::kNumTaps;
    static constexpr std::size_t kNumGhostGrains = 2;
    static constexpr float kGhostSpawnRate = 0.20f; // grains/sec at full ghost

    struct DelayLineState
    {
        std::vector<float> buffer;
        std::size_t writeIndex = 0;
    };

    struct EnvelopeFollower
    {
        void prepare(double sampleRate,
                     float attackSeconds,
                     float releaseSeconds) noexcept;

        void reset() noexcept;
        float process(float input) noexcept;

    private:
        float attackCoeff = 0.0f;
        float releaseCoeff = 0.0f;
        float value = 0.0f;
    };

    struct Grain
    {
        bool isActive = false;
        float position = 0.0f;
        float speed = 1.0f;
        float windowPhase = 0.0f;
        float windowInc = 0.0f;
    };

    void updateDelayBases(double newSampleRate);
    void updateFeedbackGains(float decaySeconds, bool freeze) noexcept;
    void updateDampingFilters(float tone) noexcept;
    float readDelaySample(std::size_t lineIndex, float delayInSamples) noexcept;
    void writeDelaySample(std::size_t lineIndex, float sample) noexcept;
    void prepareEarlyReflections(double newSampleRate);
    float processEarlyReflections(float inputSample) noexcept;
    void initialiseLfos();
    float calcModDepth(float drift, float puckY) const noexcept;
    void updateMeters(float dryLevel, float wetLevel, UnravelState& state) noexcept;
    void prepareGhostEngine(double newSampleRate);
    void resetGhostEngine() noexcept;
    void writeGhostSample(float sample) noexcept;
    float processGhost(float ghostMix) noexcept;
    bool trySpawnGrain(Grain& grain, float ghostMix) noexcept;
    float readGhostBuffer(float position) const noexcept;

    double sampleRate = 44100.0;
    bool isPrepared = false;

    std::array<DelayLineState, kNumLines> delayLines{};
    std::array<float, kNumLines> baseDelaySamples{};
    std::array<float, kNumLines> feedbackGains{};
    std::array<float, kNumLines> loopOutputs{};
    std::array<float, kNumLines> feedbackVector{};
    std::array<float, kNumLines> lineInputs{};

    std::array<juce::dsp::IIR::Filter<float>, kNumLines> lowPassFilters{};
    std::array<juce::dsp::IIR::Filter<float>, kNumLines> highPassFilters{};
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, kNumLines> lowPassStates{};
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, kNumLines> highPassStates{};

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> sizeSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputGainSmoother;
    float lastDecaySeconds = std::numeric_limits<float>::lowest();
    float lastTone = std::numeric_limits<float>::lowest();
    bool lastFreeze = false;

    std::vector<float> erBuffer;
    std::size_t erWriteIndex = 0;
    std::array<int, kNumErTaps> erTapOffsets{};
    std::array<float, kNumErTaps> erTapGains{};

    std::array<float, kNumLines> lfoPhases{};
    std::array<float, kNumLines> lfoIncrements{};

    EnvelopeFollower inputMeter;
    EnvelopeFollower tailMeter;
    EnvelopeFollower duckingFollower;

    std::vector<float> ghostBuffer;
    std::size_t ghostWriteIndex = 0;
    std::array<Grain, kNumGhostGrains> grains{};
    juce::Random ghostRng;

    static const std::array<std::array<float, kNumLines>, kNumLines> feedbackMatrix;
    static const std::array<std::array<float, 2>, kNumLines> inputMatrix;
    static const std::array<std::array<float, kNumLines>, 2> outputMatrix;
};

} // namespace threadbare::dsp

