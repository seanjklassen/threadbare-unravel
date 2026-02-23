#include "WaverProcessor.h"
#include "../UI/WaverEditor.h"

WaverProcessor::WaverProcessor()
    : ProcessorBase(
          BusesProperties()
              .withOutput("Output", juce::AudioChannelSet::stereo(), true),
          createParameterLayout())
{
    initialiseFactoryPresets();
}

void WaverProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    rateDependent = threadbare::tuning::waver::recalculate(sampleRate);
    preparedBlockSize = static_cast<std::uint32_t>(samplesPerBlock);
    preparedChannels = static_cast<std::uint32_t>(juce::jmax(1, getMainBusNumOutputChannels()));

    juce::dsp::ProcessSpec spec{
        rateDependent.sampleRate,
        preparedBlockSize,
        preparedChannels
    };
    configureOversampling(qualityMode);
    if (oversampling != nullptr)
    {
        const auto osFactor = static_cast<double>(oversampling->getOversamplingFactor());
        spec.sampleRate = rateDependent.sampleRate * osFactor;
        spec.maximumBlockSize = static_cast<juce::uint32>(preparedBlockSize * oversampling->getOversamplingFactor());
    }
    engine.prepare(spec);
    applyQualityMode(qualityMode);
    stateQueue.reset();
    uiEventQueue.reset();
}

void WaverProcessor::releaseResources() {}

void WaverProcessor::reset()
{
    engine.reset();
    stateQueue.reset();
    uiEventQueue.reset();
}

void WaverProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    drainUiEvents();

    latestState.puckX = morphPuckX.load(std::memory_order_relaxed);
    latestState.puckY = morphPuckY.load(std::memory_order_relaxed);
    latestState.mix = morphBlend.load(std::memory_order_relaxed);

    const auto numSamples = buffer.getNumSamples();
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : left;

    const float portaTimeMs = apvts.getRawParameterValue("portaTime")->load();
    const int portaMode = static_cast<int>(apvts.getRawParameterValue("portaMode")->load());
    const int chorusMode = static_cast<int>(apvts.getRawParameterValue("chorusMode")->load());
    const float filterCutoff = apvts.getRawParameterValue("filterCutoff")->load();
    const float filterRes = apvts.getRawParameterValue("filterRes")->load();
    const int filterMode = static_cast<int>(apvts.getRawParameterValue("filterMode")->load());
    const float outputGainDb = apvts.getRawParameterValue("outputGain")->load();
    const float macroShape = apvts.getRawParameterValue("macroShape")->load();
    const float lfoToPwm = apvts.getRawParameterValue("lfoToPwm")->load();
    engine.setPortamento(portaTimeMs, portaMode == 1);
    engine.setChorusMode(chorusMode);
    engine.setFilter(filterCutoff, filterRes, filterMode == 1);
    engine.setWaveBlend(macroShape);
    engine.setLfoToPwm(lfoToPwm);

    const auto renderRange = [&](int startSample, int endSample)
    {
        if (endSample <= startSample)
            return;

        const auto rangeSamples = static_cast<std::size_t>(endSample - startSample);
        if (oversampling != nullptr)
        {
            juce::dsp::AudioBlock<float> block(buffer);
            auto sub = block.getSubBlock(static_cast<std::size_t>(startSample), rangeSamples);
            auto up = oversampling->processSamplesUp(sub);
            auto* upLeft = up.getChannelPointer(0);
            auto* upRight = up.getNumChannels() > 1 ? up.getChannelPointer(1) : upLeft;
            engine.process(
                std::span<float>(upLeft, up.getNumSamples()),
                std::span<float>(upRight, up.getNumSamples()));
            oversampling->processSamplesDown(sub);
            return;
        }

        engine.process(
            std::span<float>(left + startSample, rangeSamples),
            std::span<float>(right + startSample, rangeSamples));
    };

    const auto handleMidiMessage = [&](const juce::MidiMessage& message)
    {
        if (message.isNoteOn())
            engine.noteOn(message.getNoteNumber(), message.getFloatVelocity());
        else if (message.isNoteOff())
            engine.noteOff(message.getNoteNumber(), message.getFloatVelocity());
        else if (message.isController() && message.getControllerNumber() == 64)
            engine.setSustainPedal(message.getControllerValue() >= 64);
    };

    int cursor = 0;
    for (const auto metadata : midiMessages)
    {
        const int eventSample = juce::jlimit(0, numSamples, metadata.samplePosition);
        renderRange(cursor, eventSample);
        handleMidiMessage(metadata.getMessage());
        cursor = eventSample;
    }
    renderRange(cursor, numSamples);
    midiMessages.clear();

    const float gain = juce::Decibels::decibelsToGain(outputGainDb);
    for (int sample = 0; sample < numSamples; ++sample)
    {
        left[sample] *= gain;
        right[sample] *= gain;
        latestState.scope[latestState.scopeWriteIndex] = left[sample];
        latestState.scopeWriteIndex = (latestState.scopeWriteIndex + 1u) % kScopeBufferSize;
    }

    latestState.inLevel = 0.0f;
    latestState.outLevel = 0.0f;
    stateQueue.push(latestState);
}

juce::AudioProcessorEditor* WaverProcessor::createEditor()
{
    return new WaverEditor(*this);
}

int WaverProcessor::getNumPrograms()
{
    return static_cast<int>(factoryPresets.size());
}

int WaverProcessor::getCurrentProgram()
{
    return currentProgramIndex;
}

void WaverProcessor::setCurrentProgram(int index)
{
    if (factoryPresets.empty())
        return;

    currentProgramIndex = juce::jlimit(0, getNumPrograms() - 1, index);
    applyPreset(factoryPresets[static_cast<size_t>(currentProgramIndex)]);
}

const juce::String WaverProcessor::getProgramName(int index)
{
    if (index >= 0 && index < getNumPrograms())
        return factoryPresets[static_cast<size_t>(index)].name;
    return {};
}

void WaverProcessor::changeProgramName(int, const juce::String&) {}

juce::AudioProcessorValueTreeState::ParameterLayout WaverProcessor::createParameterLayout()
{
    return threadbare::waver::WaverGeneratedParams::createParameterLayout();
}

void WaverProcessor::onSaveState(juce::ValueTree& state)
{
    state.setProperty("currentPreset", currentProgramIndex, nullptr);
    state.setProperty("determinismGlobalSeed", static_cast<juce::int64>(determinismState.globalSeed), nullptr);
    state.setProperty("determinismPrngCounter", static_cast<juce::int64>(determinismState.prngCounter), nullptr);

    for (std::size_t i = 0; i < determinismState.voiceSeeds.size(); ++i)
    {
        state.setProperty("determinismVoiceSeed" + juce::String(static_cast<int>(i)),
                          static_cast<juce::int64>(determinismState.voiceSeeds[i]),
                          nullptr);
        state.setProperty("determinismOuState" + juce::String(static_cast<int>(i)),
                          determinismState.ouStates[i],
                          nullptr);
    }
}

void WaverProcessor::onRestoreState(const juce::ValueTree& tree)
{
    if (tree.hasProperty("currentPreset"))
        currentProgramIndex = static_cast<int>(tree.getProperty("currentPreset"));

    if (tree.hasProperty("determinismGlobalSeed"))
        determinismState.globalSeed = static_cast<std::uint64_t>(static_cast<juce::int64>(tree.getProperty("determinismGlobalSeed")));

    if (tree.hasProperty("determinismPrngCounter"))
        determinismState.prngCounter = static_cast<std::uint64_t>(static_cast<juce::int64>(tree.getProperty("determinismPrngCounter")));

    for (std::size_t i = 0; i < determinismState.voiceSeeds.size(); ++i)
    {
        const auto seedKey = "determinismVoiceSeed" + juce::String(static_cast<int>(i));
        const auto ouKey = "determinismOuState" + juce::String(static_cast<int>(i));

        if (tree.hasProperty(seedKey))
            determinismState.voiceSeeds[i] = static_cast<std::uint64_t>(static_cast<juce::int64>(tree.getProperty(seedKey)));

        if (tree.hasProperty(ouKey))
            determinismState.ouStates[i] = static_cast<float>(tree.getProperty(ouKey));
    }
}

bool WaverProcessor::popVisualState(WaverState& out) noexcept
{
    WaverState latest;
    bool popped = false;
    while (stateQueue.pop(latest))
    {
        out = latest;
        popped = true;
    }
    return popped;
}

void WaverProcessor::setMorphSnapshot(float puckX, float puckY, float blend) noexcept
{
    morphPuckX.store(juce::jlimit(-1.0f, 1.0f, puckX), std::memory_order_relaxed);
    morphPuckY.store(juce::jlimit(-1.0f, 1.0f, puckY), std::memory_order_relaxed);
    morphBlend.store(juce::jlimit(0.15f, 0.60f, blend), std::memory_order_relaxed);
}

void WaverProcessor::enqueueMomentTrigger() noexcept
{
    uiEventQueue.push(UiEvent { EventType::momentTrigger, 1.0f });
}

void WaverProcessor::enqueueArpToggle(bool enabled) noexcept
{
    uiEventQueue.push(UiEvent { EventType::arpToggle, enabled ? 1.0f : 0.0f });
}

void WaverProcessor::drainUiEvents() noexcept
{
    UiEvent event;
    while (uiEventQueue.pop(event))
    {
        switch (event.type)
        {
            case EventType::momentTrigger:
                ++determinismState.prngCounter;
                break;
            case EventType::arpToggle:
                if (auto* param = apvts.getRawParameterValue("arpEnabled"))
                    juce::ignoreUnused(param);
                break;
            default:
                break;
        }
    }
}

void WaverProcessor::applyQualityMode(QualityMode mode)
{
    qualityMode = mode;
    switch (qualityMode)
    {
        case QualityMode::lite:
            configureOversampling(qualityMode);
            setLatencySamples(0);
            break;
        case QualityMode::standard:
            configureOversampling(qualityMode);
            setLatencySamples(oversampling != nullptr ? static_cast<int>(oversampling->getLatencyInSamples()) : 0);
            break;
        case QualityMode::hq:
            configureOversampling(qualityMode);
            setLatencySamples(oversampling != nullptr ? static_cast<int>(oversampling->getLatencyInSamples()) : 0);
            break;
        default:
            setLatencySamples(0);
            break;
    }
}

void WaverProcessor::configureOversampling(QualityMode mode)
{
    if (preparedChannels == 0 || preparedBlockSize == 0)
        return;

    if (mode == QualityMode::lite)
    {
        oversampling.reset();
        return;
    }

    const std::size_t stages = mode == QualityMode::hq ? 2u : 1u;
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        preparedChannels,
        stages,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,
        false);
    oversampling->reset();
    oversampling->initProcessing(preparedBlockSize);
}

bool WaverProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (!layouts.getMainInputChannelSet().isDisabled())
        return false;
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono()
        || out == juce::AudioChannelSet::stereo();
}

void WaverProcessor::initialiseFactoryPresets()
{
    factoryPresets = {
        {"default", {{"puckX", 0.0f}, {"puckY", 0.0f}, {"blend", 0.35f}, {"outputGain", 0.0f}}}
    };
}

void WaverProcessor::applyPreset(const Preset& preset)
{
    for (const auto& [id, value] : preset.parameters)
    {
        if (auto* param = apvts.getParameter(id))
        {
            const auto normalised = param->convertTo0to1(value);
            param->beginChangeGesture();
            param->setValueNotifyingHost(normalised);
            param->endChangeGesture();
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WaverProcessor();
}
