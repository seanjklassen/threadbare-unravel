#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <map>
#include <vector>

#include "../DSP/UnravelReverb.h"

class UnravelProcessor final : public juce::AudioProcessor
{
public:
    UnravelProcessor();
    ~UnravelProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return apvts; }
    const threadbare::dsp::UnravelState& getCurrentState() const noexcept { return currentState; }
    bool popVisualState(threadbare::dsp::UnravelState& state) noexcept;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    struct Preset
    {
        juce::String name;
        std::map<juce::String, float> parameters;
    };

    threadbare::dsp::UnravelReverb reverbEngine;
    threadbare::dsp::UnravelState currentState;

    juce::AudioProcessorValueTreeState apvts;

    struct StateQueue
    {
        static constexpr int kCapacity = 16;

        void reset() noexcept;
        bool push(const threadbare::dsp::UnravelState& state) noexcept;
        bool pop(threadbare::dsp::UnravelState& state) noexcept;

    private:
        void discardOldest() noexcept;

        std::array<threadbare::dsp::UnravelState, kCapacity> buffer{};
        juce::AbstractFifo fifo { kCapacity };
    };

    StateQueue stateQueue;

    juce::AudioParameterFloat* puckXParam = nullptr;
    juce::AudioParameterFloat* puckYParam = nullptr;
    juce::AudioParameterFloat* mixParam = nullptr;
    juce::AudioParameterFloat* sizeParam = nullptr;
    juce::AudioParameterFloat* decayParam = nullptr;
    juce::AudioParameterFloat* toneParam = nullptr;
    juce::AudioParameterFloat* driftParam = nullptr;
    juce::AudioParameterFloat* ghostParam = nullptr;
    juce::AudioParameterFloat* duckParam = nullptr;
    juce::AudioParameterBool* freezeParam = nullptr;
    juce::AudioParameterFloat* outputParam = nullptr;

    std::vector<Preset> factoryPresets;
    int currentProgramIndex = 0;

    void initialiseFactoryPresets();
    void applyPreset(const Preset& preset);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UnravelProcessor)
};

