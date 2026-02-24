#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

#include "../WaverGeneratedParams.h"
#include "../WaverTuning.h"
#include "../DSP/WaverEngine.h"
#include "ProcessorBase.h"

class WaverProcessor final : public threadbare::core::ProcessorBase
{
public:
    WaverProcessor();
    ~WaverProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Waver"; }
    double getTailLengthSeconds() const override { return 15.5; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    struct WaverState
    {
        float puckX = 0.0f;
        float puckY = 0.0f;
        float presetPuckX = 0.0f;
        float presetPuckY = 0.0f;
        float mix = 0.5f;
        float output = 0.0f;
        float inLevel = 0.0f;
        float outLevel = 0.0f;
        float rmsLevel = 0.0f;
        float peakLevel = 0.0f;
    };

    struct DeterminismState
    {
        std::uint64_t globalSeed = 0;
        std::uint64_t prngCounter = 0;
        std::array<std::uint64_t, 8> voiceSeeds{};
        std::array<float, 8> ouStates{};
    };

    bool popVisualState(WaverState& out) noexcept;
    void setMorphSnapshot(float puckX, float puckY, float blend) noexcept;
    void enqueueMomentTrigger() noexcept;
    void enqueueArpToggle(bool enabled) noexcept;

protected:
    void onSaveState(juce::ValueTree& state) override;
    void onRestoreState(const juce::ValueTree& tree) override;

    bool acceptsMidi() const override { return true; }
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

private:
    struct MorphSnapshot
    {
        float puckX = 0.0f;
        float puckY = 0.0f;
        float blend = 0.35f;
    };

    enum class EventType : std::uint8_t
    {
        momentTrigger = 1,
        arpToggle = 2
    };

    struct UiEvent
    {
        EventType type = EventType::momentTrigger;
        float value = 0.0f;
    };

    enum class QualityMode : std::uint8_t
    {
        lite = 0,
        standard = 1,
        hq = 2
    };

    void drainUiEvents() noexcept;

    struct Preset
    {
        juce::String name;
        std::map<juce::String, float> parameters;
        float puckX = 0.0f;
        float puckY = 0.0f;
    };

    void initialiseFactoryPresets();
    void applyPreset(const Preset& preset);
    void applyQualityMode(QualityMode mode);
    void configureOversampling(QualityMode mode);

    threadbare::dsp::WaverEngine engine;
    threadbare::core::StateQueue<WaverState> stateQueue;
    threadbare::core::StateQueue<UiEvent, 64> uiEventQueue;
    WaverState latestState;
    DeterminismState determinismState;
    threadbare::tuning::waver::RateDependent rateDependent;
    QualityMode qualityMode = QualityMode::standard;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    std::uint32_t preparedBlockSize = 0;
    std::uint32_t preparedChannels = 0;
    std::atomic<float> morphPuckX { 0.0f };
    std::atomic<float> morphPuckY { 0.0f };
    std::atomic<float> morphBlend { 0.35f };
    std::atomic<float> presetPuckX { 0.0f };
    std::atomic<float> presetPuckY { 0.0f };
    std::vector<Preset> factoryPresets;
    int currentProgramIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaverProcessor)
};
