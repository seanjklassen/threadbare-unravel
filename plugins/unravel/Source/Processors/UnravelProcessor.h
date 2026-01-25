#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <map>
#include <vector>

#include "../DSP/UnravelReverb.h"
#include "ProcessorBase.h"

class UnravelProcessor final : public threadbare::core::ProcessorBase
{
public:
    UnravelProcessor();
    ~UnravelProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    const threadbare::dsp::UnravelState& getCurrentState() const noexcept { return currentState; }
    bool popVisualState(threadbare::dsp::UnravelState& state) noexcept;
    void pushCurrentState() noexcept;  // Force immediate state update (for preset changes)

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

protected:
    //==============================================================================
    // ProcessorBase hooks for state persistence
    void onSaveState(juce::ValueTree& state) override;
    void onRestoreState(const juce::ValueTree& tree) override;
    void onStateRestored() override;

private:
    struct Preset
    {
        juce::String name;
        std::map<juce::String, float> parameters;
    };

    threadbare::dsp::UnravelReverb reverbEngine;
    threadbare::dsp::UnravelState currentState;

    // Use shared StateQueue template
    threadbare::core::StateQueue<threadbare::dsp::UnravelState> stateQueue;

    juce::AudioParameterFloat* puckXParam = nullptr;
    juce::AudioParameterFloat* puckYParam = nullptr;
    juce::AudioParameterFloat* mixParam = nullptr;
    juce::AudioParameterFloat* sizeParam = nullptr;
    juce::AudioParameterFloat* decayParam = nullptr;
    juce::AudioParameterFloat* toneParam = nullptr;
    juce::AudioParameterFloat* driftParam = nullptr;
    juce::AudioParameterFloat* ghostParam = nullptr;
    juce::AudioParameterFloat* glitchParam = nullptr;
    juce::AudioParameterFloat* duckParam = nullptr;
    juce::AudioParameterFloat* erPreDelayParam = nullptr;
    juce::AudioParameterBool* freezeParam = nullptr;
    juce::AudioParameterFloat* outputParam = nullptr;

    std::vector<Preset> factoryPresets;
    int currentProgramIndex = 0;

    void initialiseFactoryPresets();
    void applyPreset(const Preset& preset);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UnravelProcessor)
};
