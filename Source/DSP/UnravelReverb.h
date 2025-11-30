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
    bool freeze = false;
};

class UnravelReverb
{
public:
    UnravelReverb() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process(std::span<float> left, std::span<float> right, const UnravelState& state) noexcept;

private:
    static constexpr std::size_t kNumLines = threadbare::tuning::Fdn::kNumLines;
    static constexpr float kHadamardNorm = 0.35355339059327379f; // 1 / sqrt(8)
    static constexpr float kSizeSlewSeconds = 0.02f;

    struct DelayLineState
    {
        std::vector<float> buffer;
        std::size_t writeIndex = 0;
    };

    void updateDelayBases(double newSampleRate);
    void updateFeedbackGains(float decaySeconds, bool freeze) noexcept;
    void updateDampingFilters(float tone) noexcept;
    float readDelaySample(std::size_t lineIndex, float delayInSamples) noexcept;
    void writeDelaySample(std::size_t lineIndex, float sample) noexcept;

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

    static const std::array<std::array<float, kNumLines>, kNumLines> feedbackMatrix;
    static const std::array<std::array<float, 2>, kNumLines> inputMatrix;
    static const std::array<std::array<float, kNumLines>, 2> outputMatrix;
};

} // namespace threadbare::dsp

