#include "UnravelProcessor.h"
#include "UI/UnravelEditor.h"
#include "../UnravelTuning.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <memory>
#include <vector>

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
    freezeParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("freeze"));
    outputParam = getFloat("output");
}

void UnravelProcessor::releaseResources() {}

void UnravelProcessor::reset()
{
    reverbEngine.reset();
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
    currentState.freeze = freezeParam != nullptr ? freezeParam->get() : false;

    reverbEngine.process(leftSpan, rightSpan, currentState);

    const float outputGain = juce::Decibels::decibelsToGain(readParam(outputParam, 0.0f));
    buffer.applyGain(outputGain);
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
        preset("Init - Memory Cloud",
               {{"decay", 5.0f},
                {"size", 1.0f},
                {"ghost", 0.2f},
                {"mix", 0.5f},
                {"puckX", 0.0f},
                {"puckY", 0.0f},
                {"drift", 0.2f},
                {"tone", 0.0f},
                {"freeze", 0.0f}}),
        preset("Nebula",
               {{"decay", 12.0f},
                {"size", 1.5f},
                {"ghost", 0.6f},
                {"drift", 0.4f},
                {"mix", 0.65f},
                {"puckY", 0.6f}}),
        preset("Tight Room",
               {{"decay", 0.8f},
                {"size", 0.6f},
                {"tone", -0.5f},
                {"mix", 0.35f},
                {"puckX", -0.5f},
                {"puckY", -0.2f}}),
        preset("Tape Warble",
               {{"decay", 3.0f},
                {"drift", 0.8f},
                {"tone", -0.8f},
                {"ghost", 0.0f},
                {"mix", 0.45f},
                {"puckX", -0.25f},
                {"puckY", 0.15f}}),
        preset("Infinite Freeze",
               {{"freeze", 1.0f},
                {"mix", 1.0f},
                {"drift", 0.5f},
                {"decay", 20.0f},
                {"ghost", 0.4f},
                {"puckX", 0.0f},
                {"puckY", 0.0f}})};
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

