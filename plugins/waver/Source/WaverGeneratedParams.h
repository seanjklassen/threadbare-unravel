// =============================================================================
// AUTO-GENERATED FILE - DO NOT EDIT MANUALLY
// Generated from: params.json
// Generated at: 2026-02-23T18:05:40.102Z
// =============================================================================
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>

namespace threadbare::waver
{

struct WaverGeneratedParams
{
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        params.push_back(std::make_unique<juce::AudioParameterFloat>("puckX", "presence", -1.0f, 1.0f, 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("puckY", "age", -1.0f, 1.0f, 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("blend", "blend", 0.15f, 0.6f, 0.35f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("momentSeed", "moment", 0.0f, 1.0f, 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterBool>("momentTrigger", "moment trigger", false));

        params.push_back(std::make_unique<juce::AudioParameterBool>("arpEnabled", "arp", false));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("outputGain", "output", -24.0f, 12.0f, 0.0f));

        {
            auto range = juce::NormalisableRange<float>(20.0f, 20000.0f);
            range.setSkewForCentre(1000.0f);
            params.push_back(std::make_unique<juce::AudioParameterFloat>("filterCutoff", "cutoff", range, 8000.0f));
        }

        params.push_back(std::make_unique<juce::AudioParameterFloat>("filterRes", "resonance", 0.0f, 1.0f, 0.15f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>("filterMode", "filter", juce::StringArray{ "OTA", "Ladder" }, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("filterKeyTrack", "tracking", 0.0f, 1.0f, 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("envToFilter", "envelope", -1.0f, 1.0f, 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("macroShape", "shape", 0.0f, 1.0f, 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("dcoSubLevel", "sub", 0.0f, 1.0f, 0.2f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>("dcoSubOctave", "sub octave", juce::StringArray{ "-1", "-2" }, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("noiseLevel", "noise", 0.0f, 1.0f, 0.1f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("noiseColor", "color", 0.0f, 1.0f, 0.35f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("toyIndex", "fm depth", 0.0f, 1.0f, 0.25f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("toyRatio", "fm ratio", 0.0f, 1.0f, 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("layerDco", "analog", 0.0f, 1.0f, 0.7f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("layerToy", "toy", 0.0f, 1.0f, 0.2f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("layerOrgan", "organ", 0.0f, 1.0f, 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("organ16", "16'", 0.0f, 8.0f, 5.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("organ8", "8'", 0.0f, 8.0f, 4.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("organ4", "4'", 0.0f, 8.0f, 2.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("organMix", "mixture", 0.0f, 8.0f, 3.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("lfoRate", "rate", 0.1f, 30.0f, 3.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>("lfoShape", "shape", juce::StringArray{ "Tri", "Sin", "Sq", "S&H" }, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("lfoToVibrato", "vibrato", 0.0f, 50.0f, 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("lfoToPwm", "pwm", 0.0f, 1.0f, 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>("chorusMode", "chorus", juce::StringArray{ "Off", "I", "II", "I+II" }, 1));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("driftAmount", "drift", 0.0f, 1.0f, 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("stereoWidth", "width", 0.0f, 1.0f, 0.8f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("portaTime", "glide", 0.0f, 2000.0f, 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>("portaMode", "glide mode", juce::StringArray{ "Legato", "Always" }, 0));

        {
            auto range = juce::NormalisableRange<float>(0.001f, 5.0f);
            range.setSkewForCentre(0.05f);
            params.push_back(std::make_unique<juce::AudioParameterFloat>("envAttack", "attack", range, 0.01f));
        }

        {
            auto range = juce::NormalisableRange<float>(0.001f, 10.0f);
            range.setSkewForCentre(0.08f);
            params.push_back(std::make_unique<juce::AudioParameterFloat>("envDecay", "decay", range, 0.2f));
        }

        params.push_back(std::make_unique<juce::AudioParameterFloat>("envSustain", "sustain", 0.0f, 1.0f, 0.7f));

        {
            auto range = juce::NormalisableRange<float>(0.001f, 15.0f);
            range.setSkewForCentre(0.2f);
            params.push_back(std::make_unique<juce::AudioParameterFloat>("envRelease", "release", range, 0.4f));
        }

        params.push_back(std::make_unique<juce::AudioParameterFloat>("driveGain", "grit", 0.0f, 1.0f, 0.2f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("tapeSat", "saturation", 0.0f, 1.0f, 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("wowDepth", "wow", 0.0f, 1.0f, 0.15f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("flutterDepth", "flutter", 0.0f, 1.0f, 0.1f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("hissLevel", "hiss", 0.0f, 1.0f, 0.1f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>("humFreq", "mains", juce::StringArray{ "50", "60" }, 1));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("printMix", "print mix", 0.0f, 1.0f, 0.75f));

        return { params.begin(), params.end() };
    }

    // Parameter ID constants
    struct IDs
    {
        static constexpr const char* PUCK_X = "puckX";
        static constexpr const char* PUCK_Y = "puckY";
        static constexpr const char* BLEND = "blend";
        static constexpr const char* MOMENT_SEED = "momentSeed";
        static constexpr const char* MOMENT_TRIGGER = "momentTrigger";
        static constexpr const char* ARP_ENABLED = "arpEnabled";
        static constexpr const char* OUTPUT_GAIN = "outputGain";
        static constexpr const char* FILTER_CUTOFF = "filterCutoff";
        static constexpr const char* FILTER_RES = "filterRes";
        static constexpr const char* FILTER_MODE = "filterMode";
        static constexpr const char* FILTER_KEY_TRACK = "filterKeyTrack";
        static constexpr const char* ENV_TO_FILTER = "envToFilter";
        static constexpr const char* MACRO_SHAPE = "macroShape";
        static constexpr const char* DCO_SUB_LEVEL = "dcoSubLevel";
        static constexpr const char* DCO_SUB_OCTAVE = "dcoSubOctave";
        static constexpr const char* NOISE_LEVEL = "noiseLevel";
        static constexpr const char* NOISE_COLOR = "noiseColor";
        static constexpr const char* TOY_INDEX = "toyIndex";
        static constexpr const char* TOY_RATIO = "toyRatio";
        static constexpr const char* LAYER_DCO = "layerDco";
        static constexpr const char* LAYER_TOY = "layerToy";
        static constexpr const char* LAYER_ORGAN = "layerOrgan";
        static constexpr const char* ORGAN16 = "organ16";
        static constexpr const char* ORGAN8 = "organ8";
        static constexpr const char* ORGAN4 = "organ4";
        static constexpr const char* ORGAN_MIX = "organMix";
        static constexpr const char* LFO_RATE = "lfoRate";
        static constexpr const char* LFO_SHAPE = "lfoShape";
        static constexpr const char* LFO_TO_VIBRATO = "lfoToVibrato";
        static constexpr const char* LFO_TO_PWM = "lfoToPwm";
        static constexpr const char* CHORUS_MODE = "chorusMode";
        static constexpr const char* DRIFT_AMOUNT = "driftAmount";
        static constexpr const char* STEREO_WIDTH = "stereoWidth";
        static constexpr const char* PORTA_TIME = "portaTime";
        static constexpr const char* PORTA_MODE = "portaMode";
        static constexpr const char* ENV_ATTACK = "envAttack";
        static constexpr const char* ENV_DECAY = "envDecay";
        static constexpr const char* ENV_SUSTAIN = "envSustain";
        static constexpr const char* ENV_RELEASE = "envRelease";
        static constexpr const char* DRIVE_GAIN = "driveGain";
        static constexpr const char* TAPE_SAT = "tapeSat";
        static constexpr const char* WOW_DEPTH = "wowDepth";
        static constexpr const char* FLUTTER_DEPTH = "flutterDepth";
        static constexpr const char* HISS_LEVEL = "hissLevel";
        static constexpr const char* HUM_FREQ = "humFreq";
        static constexpr const char* PRINT_MIX = "printMix";
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
        static constexpr float kBLEND_MIN = 0.15f;
        static constexpr float kBLEND_MAX = 0.6f;
        static constexpr float kBLEND_DEFAULT = 0.35f;
        static constexpr float kMOMENT_SEED_MIN = 0.0f;
        static constexpr float kMOMENT_SEED_MAX = 1.0f;
        static constexpr float kMOMENT_SEED_DEFAULT = 0.5f;
        static constexpr bool kMOMENT_TRIGGER_DEFAULT = false;
        static constexpr bool kARP_ENABLED_DEFAULT = false;
        static constexpr float kOUTPUT_GAIN_MIN = -24.0f;
        static constexpr float kOUTPUT_GAIN_MAX = 12.0f;
        static constexpr float kOUTPUT_GAIN_DEFAULT = 0.0f;
        static constexpr float kFILTER_CUTOFF_MIN = 20.0f;
        static constexpr float kFILTER_CUTOFF_MAX = 20000.0f;
        static constexpr float kFILTER_CUTOFF_DEFAULT = 8000.0f;
        static constexpr float kFILTER_CUTOFF_SKEW_CENTRE = 1000.0f;
        static constexpr float kFILTER_RES_MIN = 0.0f;
        static constexpr float kFILTER_RES_MAX = 1.0f;
        static constexpr float kFILTER_RES_DEFAULT = 0.15f;
        static constexpr int kFILTER_MODE_DEFAULT = 0;
        static constexpr const char* kFILTER_MODE_OPTIONS = "OTA,Ladder";
        static constexpr float kFILTER_KEY_TRACK_MIN = 0.0f;
        static constexpr float kFILTER_KEY_TRACK_MAX = 1.0f;
        static constexpr float kFILTER_KEY_TRACK_DEFAULT = 0.5f;
        static constexpr float kENV_TO_FILTER_MIN = -1.0f;
        static constexpr float kENV_TO_FILTER_MAX = 1.0f;
        static constexpr float kENV_TO_FILTER_DEFAULT = 0.3f;
        static constexpr float kMACRO_SHAPE_MIN = 0.0f;
        static constexpr float kMACRO_SHAPE_MAX = 1.0f;
        static constexpr float kMACRO_SHAPE_DEFAULT = 0.5f;
        static constexpr float kDCO_SUB_LEVEL_MIN = 0.0f;
        static constexpr float kDCO_SUB_LEVEL_MAX = 1.0f;
        static constexpr float kDCO_SUB_LEVEL_DEFAULT = 0.2f;
        static constexpr int kDCO_SUB_OCTAVE_DEFAULT = 0;
        static constexpr const char* kDCO_SUB_OCTAVE_OPTIONS = "-1,-2";
        static constexpr float kNOISE_LEVEL_MIN = 0.0f;
        static constexpr float kNOISE_LEVEL_MAX = 1.0f;
        static constexpr float kNOISE_LEVEL_DEFAULT = 0.1f;
        static constexpr float kNOISE_COLOR_MIN = 0.0f;
        static constexpr float kNOISE_COLOR_MAX = 1.0f;
        static constexpr float kNOISE_COLOR_DEFAULT = 0.35f;
        static constexpr float kTOY_INDEX_MIN = 0.0f;
        static constexpr float kTOY_INDEX_MAX = 1.0f;
        static constexpr float kTOY_INDEX_DEFAULT = 0.25f;
        static constexpr float kTOY_RATIO_MIN = 0.0f;
        static constexpr float kTOY_RATIO_MAX = 1.0f;
        static constexpr float kTOY_RATIO_DEFAULT = 0.5f;
        static constexpr float kLAYER_DCO_MIN = 0.0f;
        static constexpr float kLAYER_DCO_MAX = 1.0f;
        static constexpr float kLAYER_DCO_DEFAULT = 0.7f;
        static constexpr float kLAYER_TOY_MIN = 0.0f;
        static constexpr float kLAYER_TOY_MAX = 1.0f;
        static constexpr float kLAYER_TOY_DEFAULT = 0.2f;
        static constexpr float kLAYER_ORGAN_MIN = 0.0f;
        static constexpr float kLAYER_ORGAN_MAX = 1.0f;
        static constexpr float kLAYER_ORGAN_DEFAULT = 0.3f;
        static constexpr float kORGAN16_MIN = 0.0f;
        static constexpr float kORGAN16_MAX = 8.0f;
        static constexpr float kORGAN16_DEFAULT = 5.0f;
        static constexpr float kORGAN8_MIN = 0.0f;
        static constexpr float kORGAN8_MAX = 8.0f;
        static constexpr float kORGAN8_DEFAULT = 4.0f;
        static constexpr float kORGAN4_MIN = 0.0f;
        static constexpr float kORGAN4_MAX = 8.0f;
        static constexpr float kORGAN4_DEFAULT = 2.0f;
        static constexpr float kORGAN_MIX_MIN = 0.0f;
        static constexpr float kORGAN_MIX_MAX = 8.0f;
        static constexpr float kORGAN_MIX_DEFAULT = 3.0f;
        static constexpr float kLFO_RATE_MIN = 0.1f;
        static constexpr float kLFO_RATE_MAX = 30.0f;
        static constexpr float kLFO_RATE_DEFAULT = 3.0f;
        static constexpr int kLFO_SHAPE_DEFAULT = 0;
        static constexpr const char* kLFO_SHAPE_OPTIONS = "Tri,Sin,Sq,S&H";
        static constexpr float kLFO_TO_VIBRATO_MIN = 0.0f;
        static constexpr float kLFO_TO_VIBRATO_MAX = 50.0f;
        static constexpr float kLFO_TO_VIBRATO_DEFAULT = 0.0f;
        static constexpr float kLFO_TO_PWM_MIN = 0.0f;
        static constexpr float kLFO_TO_PWM_MAX = 1.0f;
        static constexpr float kLFO_TO_PWM_DEFAULT = 0.0f;
        static constexpr int kCHORUS_MODE_DEFAULT = 1;
        static constexpr const char* kCHORUS_MODE_OPTIONS = "Off,I,II,I+II";
        static constexpr float kDRIFT_AMOUNT_MIN = 0.0f;
        static constexpr float kDRIFT_AMOUNT_MAX = 1.0f;
        static constexpr float kDRIFT_AMOUNT_DEFAULT = 0.3f;
        static constexpr float kSTEREO_WIDTH_MIN = 0.0f;
        static constexpr float kSTEREO_WIDTH_MAX = 1.0f;
        static constexpr float kSTEREO_WIDTH_DEFAULT = 0.8f;
        static constexpr float kPORTA_TIME_MIN = 0.0f;
        static constexpr float kPORTA_TIME_MAX = 2000.0f;
        static constexpr float kPORTA_TIME_DEFAULT = 0.0f;
        static constexpr int kPORTA_MODE_DEFAULT = 0;
        static constexpr const char* kPORTA_MODE_OPTIONS = "Legato,Always";
        static constexpr float kENV_ATTACK_MIN = 0.001f;
        static constexpr float kENV_ATTACK_MAX = 5.0f;
        static constexpr float kENV_ATTACK_DEFAULT = 0.01f;
        static constexpr float kENV_ATTACK_SKEW_CENTRE = 0.05f;
        static constexpr float kENV_DECAY_MIN = 0.001f;
        static constexpr float kENV_DECAY_MAX = 10.0f;
        static constexpr float kENV_DECAY_DEFAULT = 0.2f;
        static constexpr float kENV_DECAY_SKEW_CENTRE = 0.08f;
        static constexpr float kENV_SUSTAIN_MIN = 0.0f;
        static constexpr float kENV_SUSTAIN_MAX = 1.0f;
        static constexpr float kENV_SUSTAIN_DEFAULT = 0.7f;
        static constexpr float kENV_RELEASE_MIN = 0.001f;
        static constexpr float kENV_RELEASE_MAX = 15.0f;
        static constexpr float kENV_RELEASE_DEFAULT = 0.4f;
        static constexpr float kENV_RELEASE_SKEW_CENTRE = 0.2f;
        static constexpr float kDRIVE_GAIN_MIN = 0.0f;
        static constexpr float kDRIVE_GAIN_MAX = 1.0f;
        static constexpr float kDRIVE_GAIN_DEFAULT = 0.2f;
        static constexpr float kTAPE_SAT_MIN = 0.0f;
        static constexpr float kTAPE_SAT_MAX = 1.0f;
        static constexpr float kTAPE_SAT_DEFAULT = 0.3f;
        static constexpr float kWOW_DEPTH_MIN = 0.0f;
        static constexpr float kWOW_DEPTH_MAX = 1.0f;
        static constexpr float kWOW_DEPTH_DEFAULT = 0.15f;
        static constexpr float kFLUTTER_DEPTH_MIN = 0.0f;
        static constexpr float kFLUTTER_DEPTH_MAX = 1.0f;
        static constexpr float kFLUTTER_DEPTH_DEFAULT = 0.1f;
        static constexpr float kHISS_LEVEL_MIN = 0.0f;
        static constexpr float kHISS_LEVEL_MAX = 1.0f;
        static constexpr float kHISS_LEVEL_DEFAULT = 0.1f;
        static constexpr int kHUM_FREQ_DEFAULT = 1;
        static constexpr const char* kHUM_FREQ_OPTIONS = "50,60";
        static constexpr float kPRINT_MIX_MIN = 0.0f;
        static constexpr float kPRINT_MIX_MAX = 1.0f;
        static constexpr float kPRINT_MIX_DEFAULT = 0.75f;
    };
};

} // namespace threadbare::waver
