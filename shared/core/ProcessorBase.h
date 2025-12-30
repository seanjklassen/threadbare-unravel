#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace threadbare::core
{

/**
 * StateQueue: Lock-free SPSC queue for passing state from audio thread to UI.
 * 
 * Template parameter StateT must be trivially copyable for real-time safety.
 * Uses AbstractFifo internally for lock-free operation.
 */
template <typename StateT, int Capacity = 16>
class StateQueue
{
public:
    static_assert(std::is_trivially_copyable_v<StateT>, 
                  "StateT must be trivially copyable for real-time safety");

    StateQueue() = default;

    void reset() noexcept
    {
        fifo.reset();
    }

    /**
     * Push a state to the queue (audio thread).
     * If queue is full, discards oldest entry to make room.
     * @return true if push succeeded
     */
    bool push(const StateT& state) noexcept
    {
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        fifo.prepareToWrite(1, start1, size1, start2, size2);

        if (size1 == 0)
        {
            discardOldest();
            fifo.prepareToWrite(1, start1, size1, start2, size2);

            if (size1 == 0)
                return false;
        }

        buffer[static_cast<std::size_t>(start1)] = state;
        fifo.finishedWrite(1);
        return true;
    }

    /**
     * Pop a state from the queue (UI thread).
     * @return true if a state was dequeued
     */
    bool pop(StateT& state) noexcept
    {
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        fifo.prepareToRead(1, start1, size1, start2, size2);

        if (size1 == 0)
            return false;

        state = buffer[static_cast<std::size_t>(start1)];
        fifo.finishedRead(1);
        return true;
    }

private:
    void discardOldest() noexcept
    {
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        fifo.prepareToRead(1, start1, size1, start2, size2);

        if (size1 == 0)
            return;

        fifo.finishedRead(1);
    }

    std::array<StateT, Capacity> buffer{};
    juce::AbstractFifo fifo { Capacity };
};

/**
 * ProcessorBase: Common infrastructure for Threadbare plugin processors.
 * 
 * Provides:
 * - APVTS ownership and parameter access
 * - State persistence (getStateInformation/setStateInformation)
 * - Visual state queue for UI updates
 * 
 * Subclasses must implement:
 * - prepareToPlay, releaseResources, reset
 * - processBlock
 * - createEditor, getName
 * - createParameterLayout (static)
 */
class ProcessorBase : public juce::AudioProcessor
{
public:
    using BusesProperties = juce::AudioProcessor::BusesProperties;

    ProcessorBase(const BusesProperties& buses, 
                  juce::AudioProcessorValueTreeState::ParameterLayout layout)
        : juce::AudioProcessor(buses),
          apvts(*this, nullptr, "Params", std::move(layout))
    {
    }

    ~ProcessorBase() override = default;

    //==========================================================================
    // APVTS Access
    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return apvts; }
    const juce::AudioProcessorValueTreeState& getValueTreeState() const noexcept { return apvts; }

    //==========================================================================
    // State Persistence (default implementation using APVTS)
    void getStateInformation(juce::MemoryBlock& destData) override
    {
        auto state = apvts.copyState();
        onSaveState(state);  // Hook for subclass to add custom state
        juce::MemoryOutputStream stream(destData, false);
        state.writeToStream(stream);
    }

    void setStateInformation(const void* data, int sizeInBytes) override
    {
        auto tree = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
        if (tree.isValid())
        {
            onRestoreState(tree);  // Hook for subclass to read custom state
            apvts.replaceState(tree);
            onStateRestored();     // Hook for subclass post-restore actions
        }
    }

    //==========================================================================
    // Common defaults (can be overridden)
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==========================================================================
    // Buses support - default stereo in/out
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        const auto& main = layouts.getMainOutputChannelSet();
        return layouts.getMainInputChannelSet() == main 
            && (main == juce::AudioChannelSet::mono() 
                || main == juce::AudioChannelSet::stereo());
    }

protected:
    //==========================================================================
    // Hooks for state persistence customization
    virtual void onSaveState(juce::ValueTree& /*state*/) {}
    virtual void onRestoreState(const juce::ValueTree& /*tree*/) {}
    virtual void onStateRestored() {}

    juce::AudioProcessorValueTreeState apvts;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessorBase)
};

} // namespace threadbare::core

