#include "UnravelProcessor.h"
#include "UI/UnravelEditor.h"
#include "../UnravelTuning.h"
#include "../UnravelGeneratedParams.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <memory>
#include <vector>

UnravelProcessor::UnravelProcessor()
    : threadbare::core::ProcessorBase(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true),
          createParameterLayout())
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

    // Get tempo and transport state from DAW host
    if (auto* playhead = getPlayHead())
    {
        if (auto posInfo = playhead->getPosition())
        {
            if (posInfo->getBpm().hasValue())
                currentState.tempo = static_cast<float>(*posInfo->getBpm());
            
            // Read transport playing state for auto-stop feature
            // getIsPlaying() returns bool directly
            currentState.isPlaying = posInfo->getIsPlaying();
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
// State persistence hooks (ProcessorBase)
void UnravelProcessor::onSaveState(juce::ValueTree& state)
{
    // Save current preset index alongside parameters
    state.setProperty("currentPreset", currentProgramIndex, nullptr);
}

void UnravelProcessor::onRestoreState(const juce::ValueTree& tree)
{
    // Restore preset index if saved
    if (tree.hasProperty("currentPreset"))
    {
        currentProgramIndex = static_cast<int>(tree.getProperty("currentPreset"));
    }
}

void UnravelProcessor::onStateRestored()
{
    // Push restored state to UI queue
    pushCurrentState();
}

//==============================================================================
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
    // Use generated parameter layout from params.json
    return threadbare::unravel::UnravelGeneratedParams::createParameterLayout();
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

        // 2. close
        preset("close",
               {{"puckX", -0.9f},
                {"puckY", -0.4f},
                {"decay", 1.1f},
                {"erPreDelay", 8.0f},
                {"size", 0.75f},
                {"tone", -0.45f},
                {"drift", 0.10f},
                {"ghost", 0.05f},
                {"duck", 0.15f},
                {"mix", 0.28f},
                {"output", 0.0f},
                {"freeze", 0.0f}}),

        // 3. tether
        preset("tether",
               {{"puckX", -0.7f},
                {"puckY", 0.0f},
                {"decay", 2.4f},
                {"erPreDelay", 18.0f},
                {"size", 0.95f},
                {"tone", -0.35f},
                {"drift", 0.20f},
                {"ghost", 0.15f},
                {"duck", 0.45f},
                {"mix", 0.35f},
                {"output", 0.0f},
                {"freeze", 0.0f}}),

        // 4. pulse
        preset("pulse",
               {{"puckX", 0.25f},
                {"puckY", 0.10f},
                {"decay", 4.5f},
                {"erPreDelay", 12.0f},
                {"size", 1.15f},
                {"tone", -0.15f},
                {"drift", 0.35f},
                {"ghost", 0.25f},
                {"duck", 0.85f},
                {"mix", 0.55f},
                {"output", -1.0f},
                {"freeze", 0.0f}}),

        // 5. bloom
        preset("bloom",
               {{"puckX", 0.40f},
                {"puckY", 0.80f},
                {"decay", 10.0f},
                {"erPreDelay", 40.0f},
                {"size", 1.70f},
                {"tone", 0.05f},
                {"drift", 0.50f},
                {"ghost", 0.55f},
                {"duck", 0.0f},
                {"mix", 0.60f},
                {"output", -2.0f},
                {"freeze", 0.0f}}),

        // 6. mist
        preset("mist",
               {{"puckX", 0.90f},
                {"puckY", 0.60f},
                {"decay", 14.0f},
                {"erPreDelay", 70.0f},
                {"size", 1.85f},
                {"tone", -0.60f},
                {"drift", 0.60f},
                {"ghost", 0.70f},
                {"duck", 0.0f},
                {"mix", 0.65f},
                {"output", -3.0f},
                {"freeze", 0.0f}}),

        // 7. rewind
        preset("rewind",
               {{"puckX", 0.10f},
                {"puckY", 0.75f},
                {"decay", 6.0f},
                {"erPreDelay", 20.0f},
                {"size", 1.25f},
                {"tone", -0.35f},
                {"drift", 0.55f},
                {"ghost", 0.95f},
                {"duck", 0.0f},
                {"mix", 0.50f},
                {"output", -1.0f},
                {"freeze", 0.0f}}),

        // 8. halation
        preset("halation",
               {{"puckX", 1.0f},
                {"puckY", 0.90f},
                {"decay", 9.0f},
                {"erPreDelay", 45.0f},
                {"size", 1.90f},
                {"tone", 0.60f},
                {"drift", 0.45f},
                {"ghost", 0.65f},
                {"duck", 0.0f},
                {"mix", 0.55f},
                {"output", -2.0f},
                {"freeze", 0.0f}}),

        // 9. stasis (freeze intentionally off; user can enable once audio is flowing)
        preset("stasis",
               {{"puckX", 0.0f},
                {"puckY", 0.30f},
                {"decay", 20.0f},
                {"erPreDelay", 0.0f},
                {"size", 1.50f},
                {"tone", -0.40f},
                {"drift", 0.60f},
                {"ghost", 1.0f},
                {"duck", 0.0f},
                {"mix", 0.75f},
                {"output", -3.0f},
                {"freeze", 0.0f}}),

        // 10. shiver (freeze intentionally off; user can enable once audio is flowing)
        preset("shiver",
               {{"puckX", 1.0f},
                {"puckY", 1.0f},
                {"decay", 30.0f},
                {"erPreDelay", 0.0f},
                {"size", 2.0f},
                {"tone", 0.45f},
                {"drift", 0.80f},
                {"ghost", 1.0f},
                {"duck", 0.0f},
                {"mix", 0.80f},
                {"output", -4.0f},
                {"freeze", 0.0f}})
    };

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
