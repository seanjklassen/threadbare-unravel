// =============================================================================
// AUTO-GENERATED FILE - DO NOT EDIT MANUALLY
// Generated from: params.json
// Generated at: 2026-01-12T13:59:00.363Z
// =============================================================================
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>

namespace threadbare::unravel
{

struct UnravelGeneratedParams
{
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        params.push_back(std::make_unique<juce::AudioParameterFloat>("puckX", "Puck X", -1.0f, 1.0f, 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("puckY", "Puck Y", -1.0f, 1.0f, 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("mix", "Mix", 0.0f, 1.0f, 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("size", "Size", 0.5f, 2.0f, 1.0f));

        {
            auto range = juce::NormalisableRange<float>(0.4f, 50.0f);
            range.setSkewForCentre(2.0f);
            params.push_back(std::make_unique<juce::AudioParameterFloat>("decay", "Decay", range, 5.0f));
        }

        params.push_back(std::make_unique<juce::AudioParameterFloat>("tone", "Brightness", -1.0f, 1.0f, 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("drift", "Drift", 0.0f, 1.0f, 0.2f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("ghost", "Ghost", 0.0f, 1.0f, 0.15f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("duck", "Duck", 0.0f, 1.0f, 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("erPreDelay", "Distance", 0.0f, 100.0f, 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterBool>("freeze", "Freeze", false));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("output", "Output", -24.0f, 12.0f, 0.0f));

        return { params.begin(), params.end() };
    }

    // Parameter ID constants
    struct IDs
    {
        static constexpr const char* PUCK_X = "puckX";
        static constexpr const char* PUCK_Y = "puckY";
        static constexpr const char* MIX = "mix";
        static constexpr const char* SIZE = "size";
        static constexpr const char* DECAY = "decay";
        static constexpr const char* TONE = "tone";
        static constexpr const char* DRIFT = "drift";
        static constexpr const char* GHOST = "ghost";
        static constexpr const char* DUCK = "duck";
        static constexpr const char* ER_PRE_DELAY = "erPreDelay";
        static constexpr const char* FREEZE = "freeze";
        static constexpr const char* OUTPUT = "output";
    };

    // Parameter metadata (k-prefixed to avoid macro conflicts)
    struct Meta
    {
        static constexpr float kPUCK_X_MIN = -1.0f;
        static constexpr float kPUCK_X_MAX = 1.0f;
        static constexpr float kPUCK_X_DEFAULT = 0.0f;
        static constexpr float kPUCK_Y_MIN = -1.0f;
        static constexpr float kPUCK_Y_MAX = 1.0f;
        static constexpr float kPUCK_Y_DEFAULT = 0.0f;
        static constexpr float kMIX_MIN = 0.0f;
        static constexpr float kMIX_MAX = 1.0f;
        static constexpr float kMIX_DEFAULT = 0.5f;
        static constexpr float kSIZE_MIN = 0.5f;
        static constexpr float kSIZE_MAX = 2.0f;
        static constexpr float kSIZE_DEFAULT = 1.0f;
        static constexpr float kDECAY_MIN = 0.4f;
        static constexpr float kDECAY_MAX = 50.0f;
        static constexpr float kDECAY_DEFAULT = 5.0f;
        static constexpr float kDECAY_SKEW_CENTRE = 2.0f;
        static constexpr float kTONE_MIN = -1.0f;
        static constexpr float kTONE_MAX = 1.0f;
        static constexpr float kTONE_DEFAULT = 0.0f;
        static constexpr float kDRIFT_MIN = 0.0f;
        static constexpr float kDRIFT_MAX = 1.0f;
        static constexpr float kDRIFT_DEFAULT = 0.2f;
        static constexpr float kGHOST_MIN = 0.0f;
        static constexpr float kGHOST_MAX = 1.0f;
        static constexpr float kGHOST_DEFAULT = 0.15f;
        static constexpr float kDUCK_MIN = 0.0f;
        static constexpr float kDUCK_MAX = 1.0f;
        static constexpr float kDUCK_DEFAULT = 0.0f;
        static constexpr float kER_PRE_DELAY_MIN = 0.0f;
        static constexpr float kER_PRE_DELAY_MAX = 100.0f;
        static constexpr float kER_PRE_DELAY_DEFAULT = 0.0f;
        static constexpr bool kFREEZE_DEFAULT = false;
        static constexpr float kOUTPUT_MIN = -24.0f;
        static constexpr float kOUTPUT_MAX = 12.0f;
        static constexpr float kOUTPUT_DEFAULT = 0.0f;
    };
};

} // namespace threadbare::unravel
