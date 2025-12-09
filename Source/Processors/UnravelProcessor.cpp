#include "UnravelProcessor.h"
#include "UI/UnravelEditor.h"
#include "../UnravelTuning.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <memory>
#include <vector>

namespace
{
constexpr int kStateFifoChunk = 1;
}

void UnravelProcessor::StateQueue::reset() noexcept
{
    fifo.reset();
}

bool UnravelProcessor::StateQueue::push(const threadbare::dsp::UnravelState& state) noexcept
{
    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    fifo.prepareToWrite(kStateFifoChunk, start1, size1, start2, size2);

    if (size1 == 0)
    {
        discardOldest();
        fifo.prepareToWrite(kStateFifoChunk, start1, size1, start2, size2);

        if (size1 == 0)
            return false;
    }

    buffer[static_cast<std::size_t>(start1)] = state;
    fifo.finishedWrite(kStateFifoChunk);
    return true;
}

bool UnravelProcessor::StateQueue::pop(threadbare::dsp::UnravelState& state) noexcept
{
    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    fifo.prepareToRead(kStateFifoChunk, start1, size1, start2, size2);

    if (size1 == 0)
        return false;

    state = buffer[static_cast<std::size_t>(start1)];
    fifo.finishedRead(kStateFifoChunk);
    return true;
}

void UnravelProcessor::StateQueue::discardOldest() noexcept
{
    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    fifo.prepareToRead(kStateFifoChunk, start1, size1, start2, size2);

    if (size1 == 0)
        return;

    fifo.finishedRead(kStateFifoChunk);
}

UnravelProcessor::UnravelProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Params", createParameterLayout())
{
    initialiseFactoryPresets();

    if (!factoryPresets.empty())
    {
        setCurrentProgram(0);
    }
}

//==============================================================================
void UnravelProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec{
        sampleRate,
        static_cast<juce::uint32>(samplesPerBlock),
        static_cast<juce::uint32>(juce::jmax(1, getMainBusNumOutputChannels()))};

    reverbEngine.prepare(spec);
    stateQueue.reset();

    const auto getFloat = [this](const juce::String& id)
    {
        return dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(id));
    };

    puckXParam = getFloat("puckX");
    puckYParam = getFloat("puckY");
    mixParam = getFloat("mix");
    sizeParam = getFloat("size");
    decayParam = getFloat("decay");
    toneParam = getFloat("tone");
    driftParam = getFloat("drift");
    ghostParam = getFloat("ghost");
    duckParam = getFloat("duck");
    erPreDelayParam = getFloat("erPreDelay");
    freezeParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("freeze"));
    outputParam = getFloat("output");
}

void UnravelProcessor::releaseResources() {}

void UnravelProcessor::reset()
{
    reverbEngine.reset();
    stateQueue.reset();
}

bool UnravelProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& main = layouts.getMainOutputChannelSet();
    return layouts.getMainInputChannelSet() == main && (main == juce::AudioChannelSet::mono()
                                                        || main == juce::AudioChannelSet::stereo());
}

void UnravelProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi);

    juce::ScopedNoDenormals noDenormals;

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    auto* leftPtr = buffer.getWritePointer(0);
    auto* rightPtr = numChannels > 1 ? buffer.getWritePointer(1) : leftPtr;

    auto leftSpan = std::span<float>(leftPtr, static_cast<std::size_t>(numSamples));
    auto rightSpan = std::span<float>(rightPtr, static_cast<std::size_t>(numSamples));

    if (numChannels > 2)
    {
        for (int ch = 2; ch < numChannels; ++ch)
            buffer.clear(ch, 0, numSamples);
    }

    const auto readParam = [](auto* param, float fallback) -> float
    {
        return param != nullptr ? param->get() : fallback;
    };

    const auto clamp = [](float value, float lo, float hi) { return juce::jlimit(lo, hi, value); };

    currentState.puckX = clamp(readParam(puckXParam, 0.0f), -1.0f, 1.0f);
    currentState.puckY = clamp(readParam(puckYParam, 0.0f), -1.0f, 1.0f);
    currentState.mix = clamp(readParam(mixParam, 0.5f), 0.0f, 1.0f);
    currentState.size = clamp(readParam(sizeParam, 1.0f), 
                             threadbare::tuning::Fdn::kSizeMin, 
                             threadbare::tuning::Fdn::kSizeMax);
    currentState.decaySeconds = clamp(readParam(decayParam, 5.0f), 
                                     threadbare::tuning::Decay::kT60Min, 
                                     threadbare::tuning::Decay::kT60Max);
    currentState.tone = clamp(readParam(toneParam, 0.0f), -1.0f, 1.0f);
    currentState.drift = clamp(readParam(driftParam, 0.2f), 0.0f, 1.0f);
    currentState.ghost = clamp(readParam(ghostParam, 0.0f), 0.0f, 1.0f);
    currentState.duck = clamp(readParam(duckParam, 0.0f), 0.0f, 1.0f);
    currentState.erPreDelay = clamp(readParam(erPreDelayParam, 0.0f), 
                                   0.0f, 
                                   threadbare::tuning::EarlyReflections::kMaxPreDelayMs);
    currentState.freeze = freezeParam != nullptr ? freezeParam->get() : false;

    // Get tempo from DAW host
    if (auto* playhead = getPlayHead())
    {
        if (auto posInfo = playhead->getPosition())
        {
            if (posInfo->getBpm().hasValue())
                currentState.tempo = static_cast<float>(*posInfo->getBpm());
        }
    }

    reverbEngine.process(leftSpan, rightSpan, currentState);

    const float outputGain = juce::Decibels::decibelsToGain(readParam(outputParam, 0.0f));
    buffer.applyGain(outputGain);

    stateQueue.push(currentState);
}

//==============================================================================
juce::AudioProcessorEditor* UnravelProcessor::createEditor()
{
    return new UnravelEditor(*this);
}

bool UnravelProcessor::hasEditor() const { return true; }

//==============================================================================
const juce::String UnravelProcessor::getName() const { return "UnravelProcessor"; }
bool UnravelProcessor::acceptsMidi() const { return false; }
bool UnravelProcessor::producesMidi() const { return false; }
bool UnravelProcessor::isMidiEffect() const { return false; }
double UnravelProcessor::getTailLengthSeconds() const { return 20.0; }

//==============================================================================
int UnravelProcessor::getNumPrograms()
{
    return static_cast<int>(factoryPresets.size());
}

int UnravelProcessor::getCurrentProgram() { return currentProgramIndex; }

void UnravelProcessor::setCurrentProgram(int index)
{
    if (factoryPresets.empty())
        return;

    const int safeIndex = juce::jlimit(0, getNumPrograms() - 1, index);
    currentProgramIndex = safeIndex;
    applyPreset(factoryPresets[static_cast<size_t>(safeIndex)]);
    
    // Force immediate UI update (for both DAW native and frontend preset changes)
    pushCurrentState();
}

const juce::String UnravelProcessor::getProgramName(int index)
{
    if (index >= 0 && index < getNumPrograms())
        return factoryPresets[static_cast<size_t>(index)].name;

    return {};
}

void UnravelProcessor::changeProgramName(int, const juce::String&) {}

//==============================================================================
void UnravelProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    const auto state = apvts.copyState();
    juce::MemoryOutputStream stream(destData, false);
    state.writeToStream(stream);
}

void UnravelProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
    if (tree.isValid())
        apvts.replaceState(tree);
}

bool UnravelProcessor::popVisualState(threadbare::dsp::UnravelState& state) noexcept
{
    threadbare::dsp::UnravelState latest{};
    bool popped = false;

    while (stateQueue.pop(latest))
    {
        state = latest;
        popped = true;
    }

    return popped;
}

void UnravelProcessor::pushCurrentState() noexcept
{
    // Read current parameter values and push to state queue
    // This forces an immediate UI update (e.g., after preset load)
    const auto readParam = [](auto* param, float fallback) -> float
    {
        return param != nullptr ? param->get() : fallback;
    };

    const auto clamp = [](float value, float lo, float hi) { return juce::jlimit(lo, hi, value); };

    threadbare::dsp::UnravelState state{};
    state.puckX = clamp(readParam(puckXParam, 0.0f), -1.0f, 1.0f);
    state.puckY = clamp(readParam(puckYParam, 0.0f), -1.0f, 1.0f);
    state.mix = clamp(readParam(mixParam, 0.5f), 0.0f, 1.0f);
    state.size = clamp(readParam(sizeParam, 1.0f), 
                       threadbare::tuning::Fdn::kSizeMin, 
                       threadbare::tuning::Fdn::kSizeMax);
    state.decaySeconds = clamp(readParam(decayParam, 5.0f), 
                               threadbare::tuning::Decay::kT60Min, 
                               threadbare::tuning::Decay::kT60Max);
    state.tone = clamp(readParam(toneParam, 0.0f), -1.0f, 1.0f);
    state.drift = clamp(readParam(driftParam, 0.2f), 0.0f, 1.0f);
    state.ghost = clamp(readParam(ghostParam, 0.0f), 0.0f, 1.0f);
    state.duck = clamp(readParam(duckParam, 0.0f), 0.0f, 1.0f);
    state.erPreDelay = clamp(readParam(erPreDelayParam, 0.0f), 
                             0.0f, 
                             threadbare::tuning::EarlyReflections::kMaxPreDelayMs);
    state.freeze = freezeParam != nullptr ? freezeParam->get() : false;
    state.tempo = currentState.tempo;  // Keep current tempo from audio thread

    stateQueue.push(state);
}

juce::AudioProcessorValueTreeState::ParameterLayout UnravelProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("puckX", "Puck X", -1.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("puckY", "Puck Y", -1.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("mix", "Mix", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("size", "Size", 
                                                                  threadbare::tuning::Fdn::kSizeMin, 
                                                                  threadbare::tuning::Fdn::kSizeMax, 
                                                                  1.0f));

    auto decayRange = juce::NormalisableRange<float>(threadbare::tuning::Decay::kT60Min, 
                                                      threadbare::tuning::Decay::kT60Max);
    decayRange.setSkewForCentre(2.0f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>("decay", "Decay", decayRange, 5.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("tone", "Tone", -1.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("drift", "Drift", 0.0f, 1.0f, 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ghost", "Ghost", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("duck", "Duck", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("erPreDelay", "ER Pre-Delay", 
                                                                  0.0f, 
                                                                  threadbare::tuning::EarlyReflections::kMaxPreDelayMs, 
                                                                  0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("freeze", "Freeze", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("output", "Output", -24.0f, 12.0f, 0.0f));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new UnravelProcessor();
}

void UnravelProcessor::initialiseFactoryPresets()
{
    auto preset = [](juce::String name, std::map<juce::String, float> params)
    {
        return Preset{std::move(name), std::move(params)};
    };

    factoryPresets = {
        // 1. unravel [INIT/DEFAULT]
        preset("unravel",
               {{"puckX", 0.0f},
                {"puckY", 0.2f},
                {"decay", 3.2f},
                {"erPreDelay", 25.0f},
                {"size", 1.1f},
                {"tone", -0.2f},
                {"drift", 0.35f},
                {"ghost", 0.4f},
                {"duck", 0.0f},
                {"mix", 0.45f},
                {"output", 0.0f},
                {"freeze", 0.0f}}),

        // 2. fading polaroid
        preset("fading polaroid",
               {{"puckX", -0.7f},
                {"puckY", 0.0f},
                {"decay", 2.8f},
                {"erPreDelay", 15.0f},
                {"size", 0.8f},
                {"tone", -0.3f},
                {"drift", 0.2f},
                {"ghost", 0.25f},
                {"duck", 0.3f},
                {"mix", 0.40f},
                {"output", 0.0f},
                {"freeze", 0.0f}}),

        // 3. dissolving into mist
        preset("dissolving into mist",
               {{"puckX", 0.8f},
                {"puckY", 0.6f},
                {"decay", 8.5f},
                {"erPreDelay", 60.0f},
                {"size", 1.6f},
                {"tone", 0.1f},
                {"drift", 0.5f},
                {"ghost", 0.6f},
                {"duck", 0.0f},
                {"mix", 0.55f},
                {"output", -1.0f},
                {"freeze", 0.0f}}),

        // 4. rewind the moment
        preset("rewind the moment",
               {{"puckX", -0.5f},
                {"puckY", 0.7f},
                {"decay", 5.0f},
                {"erPreDelay", 10.0f},
                {"size", 1.2f},
                {"tone", -0.4f},
                {"drift", 0.6f},
                {"ghost", 0.85f},
                {"duck", 0.0f},
                {"mix", 0.50f},
                {"output", 0.0f},
                {"freeze", 0.0f}}),

        // 5. echoes you can't quite place
        preset("echoes you can't quite place",
               {{"puckX", 0.6f},
                {"puckY", 0.8f},
                {"decay", 7.0f},
                {"erPreDelay", 40.0f},
                {"size", 1.5f},
                {"tone", 0.2f},
                {"drift", 0.7f},
                {"ghost", 0.9f},
                {"duck", 0.0f},
                {"mix", 0.60f},
                {"output", -2.0f},
                {"freeze", 0.0f}}),

        // 6. held breath
        preset("held breath",
               {{"puckX", 0.0f},
                {"puckY", 0.5f},
                {"decay", 12.0f},
                {"erPreDelay", 20.0f},
                {"size", 1.4f},
                {"tone", -0.1f},
                {"drift", 0.4f},
                {"ghost", 0.5f},
                {"duck", 0.0f},
                {"mix", 0.50f},
                {"output", 0.0f},
                {"freeze", 0.0f}}),

        // 7. sunfaded memory
        preset("sunfaded memory",
               {{"puckX", -0.2f},
                {"puckY", 0.3f},
                {"decay", 4.5f},
                {"erPreDelay", 30.0f},
                {"size", 1.0f},
                {"tone", -0.5f},
                {"drift", 0.3f},
                {"ghost", 0.35f},
                {"duck", 0.2f},
                {"mix", 0.42f},
                {"output", 0.0f},
                {"freeze", 0.0f}}),

        // 8. shimmer and sway
        preset("shimmer and sway",
               {{"puckX", 0.3f},
                {"puckY", 0.7f},
                {"decay", 9.0f},
                {"erPreDelay", 45.0f},
                {"size", 1.7f},
                {"tone", 0.3f},
                {"drift", 0.8f},
                {"ghost", 0.7f},
                {"duck", 0.0f},
                {"mix", 0.58f},
                {"output", -1.0f},
                {"freeze", 0.0f}}),

        // 9. between the notes
        preset("between the notes",
               {{"puckX", -0.4f},
                {"puckY", -0.3f},
                {"decay", 1.8f},
                {"erPreDelay", 5.0f},
                {"size", 0.7f},
                {"tone", -0.2f},
                {"drift", 0.15f},
                {"ghost", 0.2f},
                {"duck", 0.75f},
                {"mix", 0.38f},
                {"output", 1.0f},
                {"freeze", 0.0f}}),

        // 10. weightless ascent
        preset("weightless ascent",
               {{"puckX", 0.1f},
                {"puckY", 0.9f},
                {"decay", 15.0f},
                {"erPreDelay", 80.0f},
                {"size", 1.8f},
                {"tone", 0.0f},
                {"drift", 0.6f},
                {"ghost", 0.8f},
                {"duck", 0.0f},
                {"mix", 0.65f},
                {"output", -2.0f},
                {"freeze", 0.0f}}),

        // 11. twilight reversal
        preset("twilight reversal",
               {{"puckX", 0.0f},
                {"puckY", 0.75f},
                {"decay", 6.0f},
                {"erPreDelay", 35.0f},
                {"size", 1.3f},
                {"tone", -0.3f},
                {"drift", 0.5f},
                {"ghost", 0.9f},
                {"duck", 0.0f},
                {"mix", 0.52f},
                {"output", 0.0f},
                {"freeze", 0.0f}}),

        // 12. whispering distance
        preset("whispering distance",
               {{"puckX", -0.3f},
                {"puckY", 0.2f},
                {"decay", 3.0f},
                {"erPreDelay", 40.0f},
                {"size", 1.0f},
                {"tone", -0.1f},
                {"drift", 0.25f},
                {"ghost", 0.3f},
                {"duck", 0.4f},
                {"mix", 0.35f},
                {"output", 0.0f},
                {"freeze", 0.0f}})};

}

void UnravelProcessor::applyPreset(const Preset& preset)
{
    for (const auto& entry : preset.parameters)
    {
        if (auto* parameter = apvts.getParameter(entry.first))
        {
            const float normalised = parameter->convertTo0to1(entry.second);
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(normalised);
            parameter->endChangeGesture();
        }
    }
}

