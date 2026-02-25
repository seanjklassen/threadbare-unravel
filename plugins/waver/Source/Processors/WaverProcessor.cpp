#include "WaverProcessor.h"
#include "../UI/WaverEditor.h"

WaverProcessor::WaverProcessor()
    : ProcessorBase(
          BusesProperties()
              .withOutput("Output", juce::AudioChannelSet::stereo(), true),
          createParameterLayout())
{
    auto& rng = juce::Random::getSystemRandom();
    determinismState.globalSeed = static_cast<std::uint64_t>(rng.nextInt64());
    if (determinismState.globalSeed == 0)
        determinismState.globalSeed = 0xDEADBEEF42u;

    initialiseFactoryPresets();
    if (!factoryPresets.empty())
    {
        const auto& preset = factoryPresets.front();
        presetPuckX.store(preset.puckX, std::memory_order_relaxed);
        presetPuckY.store(preset.puckY, std::memory_order_relaxed);
        morphPuckX.store(preset.puckX, std::memory_order_relaxed);
        morphPuckY.store(preset.puckY, std::memory_order_relaxed);
    }
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
    engine.prepare(spec, static_cast<std::uint32_t>(determinismState.globalSeed & 0xFFFFFFFFu));
    outputGainSmoothed.reset(rateDependent.sampleRate, 0.02);
    outputGainSmoothed.setCurrentAndTargetValue(
        juce::Decibels::decibelsToGain(apvts.getRawParameterValue("outputGain")->load()));
    applyQualityMode(qualityMode);
    lastQualityModeParam = static_cast<int>(qualityMode);
    transitionFade.reset(rateDependent.sampleRate, 0.30);
    transitionFade.setCurrentAndTargetValue(1.0f);
    transitionPhase = TransitionPhase::idle;
    arpLatchGain.reset(rateDependent.sampleRate, 0.05);
    arpLatchGain.setCurrentAndTargetValue(1.0f);
    arpLatchPhase = ArpLatchPhase::idle;
    pendingPresetIndex.store(-1, std::memory_order_relaxed);
    stateQueue.reset();
    uiEventQueue.reset();
}

void WaverProcessor::releaseResources() {}

void WaverProcessor::reset()
{
    engine.reset();
    outputGainSmoothed.setCurrentAndTargetValue(
        juce::Decibels::decibelsToGain(apvts.getRawParameterValue("outputGain")->load()));
    transitionFade.setCurrentAndTargetValue(1.0f);
    transitionPhase = TransitionPhase::idle;
    arpLatchGain.setCurrentAndTargetValue(1.0f);
    arpLatchPhase = ArpLatchPhase::idle;
    stateQueue.reset();
    uiEventQueue.reset();
}

void WaverProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    drainUiEvents();

    latestState.puckX = morphPuckX.load(std::memory_order_relaxed);
    latestState.puckY = morphPuckY.load(std::memory_order_relaxed);
    latestState.presetPuckX = presetPuckX.load(std::memory_order_relaxed);
    latestState.presetPuckY = presetPuckY.load(std::memory_order_relaxed);
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
    const int qualityModeParam = static_cast<int>(apvts.getRawParameterValue("qualityMode")->load());
    const float macroShape = apvts.getRawParameterValue("macroShape")->load();
    const float lfoToPwm = apvts.getRawParameterValue("lfoToPwm")->load();
    const float driftAmt = apvts.getRawParameterValue("driftAmount")->load();
    const float puckY = morphPuckY.load(std::memory_order_relaxed);
    const float dcoSubLvl = apvts.getRawParameterValue("dcoSubLevel")->load();
    const float noiseLvl = apvts.getRawParameterValue("noiseLevel")->load();
    const float lfoRateHz = apvts.getRawParameterValue("lfoRate")->load();
    const int lfoShapeIdx = static_cast<int>(apvts.getRawParameterValue("lfoShape")->load());
    const float lfoVibrato = apvts.getRawParameterValue("lfoToVibrato")->load();
    const float toyIdx = apvts.getRawParameterValue("toyIndex")->load();
    const float toyRat = apvts.getRawParameterValue("toyRatio")->load();
    const float layDco = apvts.getRawParameterValue("layerDco")->load();
    const float layToy = apvts.getRawParameterValue("layerToy")->load();
    const float envA = apvts.getRawParameterValue("envAttack")->load();
    const float envD = apvts.getRawParameterValue("envDecay")->load();
    const float envS = apvts.getRawParameterValue("envSustain")->load();
    const float envR = apvts.getRawParameterValue("envRelease")->load();

    const float ageNorm = (puckY + 1.0f) * 0.5f;
    bool isPlaying = false;
    bool isRecording = false;
    double hostBpm = 0.0;
    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
        {
            isPlaying = pos->getIsPlaying();
            isRecording = pos->getIsRecording();
            if (auto bpm = pos->getBpm())
                hostBpm = *bpm;
        }
    }
    const bool transportActive = isPlaying || isRecording;
    latestState.isPlaying = isPlaying;
    latestState.isRecording = isRecording;
    latestState.transportActive = transportActive;

    const bool requestedArpOn = apvts.getRawParameterValue("arpEnabled")->load() > 0.5f;
    const bool arpOn = transportActive ? prevArpOn : requestedArpOn;
    const int clampedQualityMode = juce::jlimit(0, 2, qualityModeParam);
    if (clampedQualityMode != lastQualityModeParam)
    {
        applyQualityMode(static_cast<QualityMode>(clampedQualityMode));
        lastQualityModeParam = clampedQualityMode;
    }
    const bool arpStateChanged = arpOn != prevArpOn;
    if (arpOn && !prevArpOn)
        frozenAgeNorm = ageNorm;
    if (arpStateChanged)
    {
        arpLatchPhase = ArpLatchPhase::dip;
        arpLatchGain.reset(rateDependent.sampleRate, 0.075);
        arpLatchGain.setCurrentAndTargetValue(1.0f);
        arpLatchGain.setTargetValue(0.90f);
    }
    prevArpOn = arpOn;
    latestState.arpEnabled = arpOn;

    const int pending = pendingPresetIndex.load(std::memory_order_acquire);
    if (pending >= 0)
    {
        if (transitionPhase == TransitionPhase::idle)
        {
            transitionPhase = TransitionPhase::fadeOut;
            transitionFade.setTargetValue(0.0f);
            engine.setTransitionDelay(25.0f);
        }
        else if (transitionPhase == TransitionPhase::fadeIn)
        {
            transitionPhase = TransitionPhase::fadeOut;
            transitionFade.setTargetValue(0.0f);
            engine.setTransitionDelay(25.0f);
        }
    }
    if (transitionPhase == TransitionPhase::fadeOut && transitionFade.getCurrentValue() < 0.001f)
    {
        const int idx = pendingPresetIndex.exchange(-1, std::memory_order_acq_rel);
        if (idx >= 0 && idx < static_cast<int>(factoryPresets.size()))
            applyPreset(factoryPresets[static_cast<size_t>(idx)]);
        transitionPhase = TransitionPhase::fadeIn;
        transitionFade.setTargetValue(1.0f);
        engine.setTransitionDelay(0.0f);
    }
    if (transitionPhase == TransitionPhase::fadeIn && transitionFade.getCurrentValue() > 0.999f)
    {
        transitionPhase = TransitionPhase::idle;
        transitionFade.setCurrentAndTargetValue(1.0f);
    }

    engine.setPortamento(portaTimeMs, portaMode == 1);
    engine.setChorusMode(chorusMode);
    engine.setFilter(filterCutoff, filterRes, filterMode == 1);
    engine.setWaveBlend(macroShape);
    engine.setLfoToPwm(lfoToPwm);
    engine.setDriftAmount(driftAmt);
    engine.setAge(arpOn ? frozenAgeNorm : ageNorm);
    engine.setSubLevel(dcoSubLvl);
    engine.setNoiseLevel(noiseLvl);
    engine.setLfoRate(lfoRateHz);
    engine.setLfoShape(lfoShapeIdx);
    engine.setLfoToVibrato(lfoVibrato);
    const float layOrgan = apvts.getRawParameterValue("layerOrgan")->load();
    const float org16 = apvts.getRawParameterValue("organ16")->load();
    const float org8 = apvts.getRawParameterValue("organ8")->load();
    const float org4 = apvts.getRawParameterValue("organ4")->load();
    const float orgMix = apvts.getRawParameterValue("organMix")->load();
    const float driveGn = apvts.getRawParameterValue("driveGain")->load();
    const float tapeSt = apvts.getRawParameterValue("tapeSat")->load();
    const float wowDp = apvts.getRawParameterValue("wowDepth")->load();
    const float flutDp = apvts.getRawParameterValue("flutterDepth")->load();
    const float hissLv = apvts.getRawParameterValue("hissLevel")->load();
    const int humIdx = static_cast<int>(apvts.getRawParameterValue("humFreq")->load());
    const float printMx = apvts.getRawParameterValue("printMix")->load();
    const float humHz = humIdx == 0 ? 50.0f : 60.0f;

    const float filterKeyTrk = apvts.getRawParameterValue("filterKeyTrack")->load();
    const float envToFilt = apvts.getRawParameterValue("envToFilter")->load();
    const float noiseClr = apvts.getRawParameterValue("noiseColor")->load();
    const float stereoWd = apvts.getRawParameterValue("stereoWidth")->load();
    const int subOctChoice = static_cast<int>(apvts.getRawParameterValue("dcoSubOctave")->load());

    engine.setToyParams(toyIdx, toyRat, 0.0f);
    engine.setLayerLevels(layDco, layToy);
    engine.setEnvelopeParams(envA, envD, envS, envR);
    engine.setFilterKeyTrack(filterKeyTrk);
    engine.setEnvToFilter(envToFilt);
    engine.setNoiseColor(noiseClr);
    engine.setStereoWidth(stereoWd);
    engine.setSubOctave(subOctChoice);
    engine.setOrganDrawbars(org16, org8, org4, orgMix);
    engine.setOrganLevel(layOrgan);
    engine.setPrintParams(driveGn, tapeSt, wowDp, flutDp, hissLv, humHz, printMx);

    engine.setArpEnabled(arpOn);
    if (arpOn)
        engine.setArpPuck(latestState.puckX, latestState.puckY);
    engine.setArpHostTempo(hostBpm);

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
            engine.arpNoteOn(message.getNoteNumber(), message.getFloatVelocity());
        else if (message.isNoteOff())
            engine.arpNoteOff(message.getNoteNumber(), message.getFloatVelocity());
        else if (message.isController())
        {
            const int cc = message.getControllerNumber();
            if (cc == 64)
                engine.setSustainPedal(message.getControllerValue() >= 64);
            else if (cc == 123 || cc == 120)
                engine.arpAllNotesOff();
        }
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

    outputGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(outputGainDb));
    const bool transitioning = transitionPhase != TransitionPhase::idle;
    float sumSq = 0.0f;
    float peak = 0.0f;
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float gain = outputGainSmoothed.getNextValue();
        if (transitioning)
        {
            const float t = transitionFade.getNextValue();
            gain *= t * t * (3.0f - 2.0f * t);
        }
        const float latchGain = arpLatchGain.getNextValue();
        gain *= latchGain;
        if (arpLatchPhase == ArpLatchPhase::dip && latchGain <= 0.901f)
        {
            arpLatchPhase = ArpLatchPhase::recover;
            arpLatchGain.reset(rateDependent.sampleRate, 0.22);
            arpLatchGain.setTargetValue(1.0f);
        }
        else if (arpLatchPhase == ArpLatchPhase::recover && latchGain >= 0.999f)
        {
            arpLatchPhase = ArpLatchPhase::idle;
            arpLatchGain.setCurrentAndTargetValue(1.0f);
        }
        left[sample] *= gain;
        right[sample] *= gain;
        const float mono = (left[sample] + right[sample]) * 0.5f;
        sumSq += mono * mono;
        const float absMono = std::abs(mono);
        if (absMono > peak) peak = absMono;
    }
    latestState.rmsLevel = numSamples > 0 ? std::sqrt(sumSq / static_cast<float>(numSamples)) : 0.0f;
    latestState.peakLevel = peak;
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
    const auto& preset = factoryPresets[static_cast<size_t>(currentProgramIndex)];
    presetPuckX.store(preset.puckX, std::memory_order_relaxed);
    presetPuckY.store(preset.puckY, std::memory_order_relaxed);
    morphPuckX.store(preset.puckX, std::memory_order_relaxed);
    morphPuckY.store(preset.puckY, std::memory_order_relaxed);
    pendingPresetIndex.store(currentProgramIndex, std::memory_order_release);
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

        // --- AMBER (warm pads, lush, breathing) ---
        {"A.M. Pad", {
            {"macroShape", 0.15f}, {"layerDco", 0.85f}, {"layerToy", 0.0f}, {"layerOrgan", 0.0f},
            {"filterCutoff", 3200.0f}, {"filterRes", 0.2f}, {"filterMode", 0.0f},
            {"dcoSubLevel", 0.3f}, {"noiseLevel", 0.02f},
            {"lfoRate", 2.5f}, {"lfoToPwm", 0.15f}, {"lfoToVibrato", 3.0f},
            {"chorusMode", 1.0f}, {"driftAmount", 0.25f},
            {"envAttack", 0.4f}, {"envDecay", 1.5f}, {"envSustain", 0.75f}, {"envRelease", 2.5f},
            {"driveGain", 0.05f}, {"tapeSat", 0.15f}, {"wowDepth", 0.08f}, {"flutterDepth", 0.03f},
            {"hissLevel", 0.05f}, {"printMix", 0.6f}, {"outputGain", 0.0f}
        }, -0.35f, -0.15f},

        {"Summer Here Pad", {
            {"macroShape", 0.35f}, {"layerDco", 0.7f}, {"layerToy", 0.15f}, {"layerOrgan", 0.1f},
            {"filterCutoff", 4500.0f}, {"filterRes", 0.25f}, {"filterMode", 0.0f},
            {"dcoSubLevel", 0.2f}, {"noiseLevel", 0.04f},
            {"lfoRate", 1.8f}, {"lfoToPwm", 0.2f}, {"lfoToVibrato", 5.0f},
            {"chorusMode", 2.0f}, {"driftAmount", 0.35f},
            {"envAttack", 0.6f}, {"envDecay", 2.0f}, {"envSustain", 0.65f}, {"envRelease", 4.0f},
            {"driveGain", 0.1f}, {"tapeSat", 0.25f}, {"wowDepth", 0.12f}, {"flutterDepth", 0.05f},
            {"hissLevel", 0.08f}, {"printMix", 0.7f}, {"outputGain", -1.0f}
        }, 0.25f, 0.0f},

        {"Dial-A-View Focus", {
            {"macroShape", 0.05f}, {"layerDco", 0.9f}, {"layerToy", 0.0f}, {"layerOrgan", 0.15f},
            {"filterCutoff", 2000.0f}, {"filterRes", 0.1f}, {"filterMode", 1.0f},
            {"dcoSubLevel", 0.45f}, {"noiseLevel", 0.01f},
            {"lfoRate", 0.8f}, {"lfoToPwm", 0.0f}, {"lfoToVibrato", 2.0f},
            {"chorusMode", 3.0f}, {"driftAmount", 0.2f},
            {"envAttack", 1.2f}, {"envDecay", 3.0f}, {"envSustain", 0.8f}, {"envRelease", 5.0f},
            {"driveGain", 0.0f}, {"tapeSat", 0.1f}, {"wowDepth", 0.06f}, {"flutterDepth", 0.02f},
            {"hissLevel", 0.03f}, {"printMix", 0.5f}, {"outputGain", 0.0f}
        }, -0.45f, 0.35f},

        // --- DRIFT (movement, modulation, evolving) ---
        {"Stray Dog Drift", {
            {"macroShape", 0.25f}, {"layerDco", 0.8f}, {"layerToy", 0.0f}, {"layerOrgan", 0.0f},
            {"filterCutoff", 5000.0f}, {"filterRes", 0.35f}, {"filterMode", 0.0f},
            {"dcoSubLevel", 0.15f}, {"noiseLevel", 0.03f},
            {"lfoRate", 0.4f}, {"lfoToPwm", 0.3f}, {"lfoToVibrato", 8.0f},
            {"chorusMode", 1.0f}, {"driftAmount", 0.7f},
            {"envAttack", 0.8f}, {"envDecay", 2.0f}, {"envSustain", 0.6f}, {"envRelease", 3.5f},
            {"driveGain", 0.05f}, {"tapeSat", 0.2f}, {"wowDepth", 0.2f}, {"flutterDepth", 0.08f},
            {"hissLevel", 0.06f}, {"printMix", 0.65f}, {"outputGain", 0.0f}
        }, -0.2f, 0.2f},

        {"Wandering Pilot", {
            {"macroShape", 0.5f}, {"layerDco", 0.6f}, {"layerToy", 0.2f}, {"layerOrgan", 0.0f},
            {"filterCutoff", 6000.0f}, {"filterRes", 0.3f}, {"filterMode", 0.0f},
            {"dcoSubLevel", 0.1f}, {"noiseLevel", 0.05f},
            {"lfoRate", 3.5f}, {"lfoToPwm", 0.4f}, {"lfoToVibrato", 12.0f},
            {"chorusMode", 2.0f}, {"driftAmount", 0.6f},
            {"portaTime", 350.0f}, {"portaMode", 0.0f},
            {"envAttack", 0.3f}, {"envDecay", 1.0f}, {"envSustain", 0.5f}, {"envRelease", 2.0f},
            {"driveGain", 0.09f}, {"tapeSat", 0.26f}, {"wowDepth", 0.15f}, {"flutterDepth", 0.06f},
            {"hissLevel", 0.07f}, {"printMix", 0.66f}, {"outputGain", -1.0f}
        }, 0.45f, -0.05f},

        {"Jed's Memory", {
            {"macroShape", 0.4f}, {"layerDco", 0.75f}, {"layerToy", 0.1f}, {"layerOrgan", 0.05f},
            {"filterCutoff", 3500.0f}, {"filterRes", 0.4f}, {"filterMode", 0.0f},
            {"dcoSubLevel", 0.25f}, {"noiseLevel", 0.06f},
            {"lfoRate", 1.2f}, {"lfoToPwm", 0.25f}, {"lfoToVibrato", 15.0f},
            {"chorusMode", 3.0f}, {"driftAmount", 0.85f},
            {"envAttack", 0.5f}, {"envDecay", 2.5f}, {"envSustain", 0.55f}, {"envRelease", 4.0f},
            {"driveGain", 0.08f}, {"tapeSat", 0.27f}, {"wowDepth", 0.16f}, {"flutterDepth", 0.06f},
            {"hissLevel", 0.1f}, {"printMix", 0.74f}, {"outputGain", -2.0f}
        }, 0.1f, 0.55f},

        // --- HUSH (quiet, intimate, barely there) ---
        {"Hewlett's Keys", {
            {"macroShape", 0.0f}, {"layerDco", 0.5f}, {"layerToy", 0.0f}, {"layerOrgan", 0.0f},
            {"filterCutoff", 1800.0f}, {"filterRes", 0.05f}, {"filterMode", 1.0f},
            {"dcoSubLevel", 0.5f}, {"noiseLevel", 0.0f},
            {"lfoRate", 0.5f}, {"lfoToPwm", 0.0f}, {"lfoToVibrato", 1.5f},
            {"chorusMode", 0.0f}, {"driftAmount", 0.15f},
            {"envAttack", 0.08f}, {"envDecay", 0.8f}, {"envSustain", 0.3f}, {"envRelease", 1.5f},
            {"driveGain", 0.0f}, {"tapeSat", 0.05f}, {"wowDepth", 0.03f}, {"flutterDepth", 0.01f},
            {"hissLevel", 0.02f}, {"printMix", 0.4f}, {"outputGain", -3.0f}
        }, -0.45f, -0.55f},

        {"Nature Anthem", {
            {"macroShape", 0.0f}, {"layerDco", 0.3f}, {"layerToy", 0.0f}, {"layerOrgan", 0.6f},
            {"organ16", 3.0f}, {"organ8", 5.0f}, {"organ4", 1.0f}, {"organMix", 2.0f},
            {"filterCutoff", 2500.0f}, {"filterRes", 0.1f}, {"filterMode", 0.0f},
            {"dcoSubLevel", 0.0f}, {"noiseLevel", 0.0f},
            {"lfoRate", 1.0f}, {"lfoToPwm", 0.0f}, {"lfoToVibrato", 2.0f},
            {"chorusMode", 1.0f}, {"driftAmount", 0.2f},
            {"envAttack", 0.8f}, {"envDecay", 3.0f}, {"envSustain", 0.7f}, {"envRelease", 6.0f},
            {"driveGain", 0.0f}, {"tapeSat", 0.1f}, {"wowDepth", 0.05f}, {"flutterDepth", 0.02f},
            {"hissLevel", 0.04f}, {"printMix", 0.55f}, {"outputGain", -2.0f}
        }, -0.2f, 0.1f},

        {"Willow Tape", {
            {"macroShape", 0.1f}, {"layerDco", 0.65f}, {"layerToy", 0.0f}, {"layerOrgan", 0.2f},
            {"organ16", 4.0f}, {"organ8", 3.0f}, {"organ4", 0.0f}, {"organMix", 1.0f},
            {"filterCutoff", 1500.0f}, {"filterRes", 0.15f}, {"filterMode", 1.0f},
            {"dcoSubLevel", 0.4f}, {"noiseLevel", 0.02f},
            {"lfoRate", 0.3f}, {"lfoToPwm", 0.0f}, {"lfoToVibrato", 3.0f},
            {"chorusMode", 1.0f}, {"driftAmount", 0.3f},
            {"envAttack", 1.5f}, {"envDecay", 4.0f}, {"envSustain", 0.6f}, {"envRelease", 8.0f},
            {"driveGain", 0.0f}, {"tapeSat", 0.2f}, {"wowDepth", 0.15f}, {"flutterDepth", 0.04f},
            {"hissLevel", 0.08f}, {"printMix", 0.75f}, {"outputGain", -1.0f}
        }, -0.35f, 0.65f},

        // --- SIGNAL (brighter, present, cutting through) ---
        {"Elson's Pocket Toy", {
            {"macroShape", 0.6f}, {"layerDco", 0.2f}, {"layerToy", 0.8f}, {"layerOrgan", 0.0f},
            {"toyIndex", 0.5f}, {"toyRatio", 0.3f},
            {"filterCutoff", 10000.0f}, {"filterRes", 0.15f}, {"filterMode", 0.0f},
            {"dcoSubLevel", 0.0f}, {"noiseLevel", 0.0f},
            {"lfoRate", 5.0f}, {"lfoToPwm", 0.0f}, {"lfoToVibrato", 0.0f},
            {"chorusMode", 0.0f}, {"driftAmount", 0.1f},
            {"envAttack", 0.005f}, {"envDecay", 0.4f}, {"envSustain", 0.2f}, {"envRelease", 0.3f},
            {"driveGain", 0.05f}, {"tapeSat", 0.15f}, {"wowDepth", 0.04f}, {"flutterDepth", 0.02f},
            {"hissLevel", 0.03f}, {"printMix", 0.5f}, {"outputGain", 0.0f}
        }, 0.35f, -0.55f},

        {"Broken Appliance Bells", {
            {"macroShape", 0.7f}, {"layerDco", 0.15f}, {"layerToy", 0.7f}, {"layerOrgan", 0.0f},
            {"toyIndex", 0.7f}, {"toyRatio", 0.6f},
            {"filterCutoff", 8000.0f}, {"filterRes", 0.2f}, {"filterMode", 0.0f},
            {"dcoSubLevel", 0.0f}, {"noiseLevel", 0.02f},
            {"lfoRate", 4.0f}, {"lfoToPwm", 0.0f}, {"lfoToVibrato", 4.0f},
            {"chorusMode", 1.0f}, {"driftAmount", 0.2f},
            {"envAttack", 0.001f}, {"envDecay", 1.2f}, {"envSustain", 0.0f}, {"envRelease", 1.0f},
            {"driveGain", 0.1f}, {"tapeSat", 0.2f}, {"wowDepth", 0.06f}, {"flutterDepth", 0.03f},
            {"hissLevel", 0.04f}, {"printMix", 0.6f}, {"outputGain", 0.0f}
        }, 0.1f, -0.3f},

        {"Humanoid Pulse", {
            {"macroShape", 0.85f}, {"layerDco", 0.9f}, {"layerToy", 0.0f}, {"layerOrgan", 0.0f},
            {"filterCutoff", 6500.0f}, {"filterRes", 0.45f}, {"filterMode", 0.0f},
            {"dcoSubLevel", 0.35f}, {"noiseLevel", 0.0f},
            {"lfoRate", 6.0f}, {"lfoToPwm", 0.35f}, {"lfoToVibrato", 6.0f},
            {"chorusMode", 0.0f}, {"driftAmount", 0.15f},
            {"portaTime", 250.0f}, {"portaMode", 0.0f},
            {"envAttack", 0.01f}, {"envDecay", 0.5f}, {"envSustain", 0.6f}, {"envRelease", 0.4f},
            {"driveGain", 0.12f}, {"tapeSat", 0.1f}, {"wowDepth", 0.03f}, {"flutterDepth", 0.01f},
            {"hissLevel", 0.02f}, {"printMix", 0.45f}, {"outputGain", 1.0f}
        }, 0.65f, -0.35f},

        // --- WEIGHT (deep, heavy, grounding) ---
        {"Vacant Foundation", {
            {"macroShape", 0.0f}, {"layerDco", 0.7f}, {"layerToy", 0.0f}, {"layerOrgan", 0.5f},
            {"organ16", 8.0f}, {"organ8", 6.0f}, {"organ4", 2.0f}, {"organMix", 4.0f},
            {"filterCutoff", 1200.0f}, {"filterRes", 0.3f}, {"filterMode", 1.0f},
            {"dcoSubLevel", 0.6f}, {"noiseLevel", 0.0f},
            {"lfoRate", 0.6f}, {"lfoToPwm", 0.0f}, {"lfoToVibrato", 1.0f},
            {"chorusMode", 1.0f}, {"driftAmount", 0.3f},
            {"envAttack", 0.15f}, {"envDecay", 1.0f}, {"envSustain", 0.85f}, {"envRelease", 2.0f},
            {"driveGain", 0.12f}, {"tapeSat", 0.3f}, {"wowDepth", 0.1f}, {"flutterDepth", 0.04f},
            {"hissLevel", 0.06f}, {"printMix", 0.7f}, {"outputGain", -1.0f}
        }, -0.55f, -0.35f},

        {"Saddest Organ", {
            {"macroShape", 0.0f}, {"layerDco", 0.0f}, {"layerToy", 0.0f}, {"layerOrgan", 0.9f},
            {"organ16", 6.0f}, {"organ8", 8.0f}, {"organ4", 4.0f}, {"organMix", 5.0f},
            {"filterCutoff", 4000.0f}, {"filterRes", 0.1f}, {"filterMode", 0.0f},
            {"dcoSubLevel", 0.0f}, {"noiseLevel", 0.0f},
            {"lfoRate", 1.0f}, {"lfoToPwm", 0.0f}, {"lfoToVibrato", 0.0f},
            {"chorusMode", 2.0f}, {"driftAmount", 0.4f},
            {"envAttack", 0.01f}, {"envDecay", 0.5f}, {"envSustain", 0.9f}, {"envRelease", 0.5f},
            {"driveGain", 0.07f}, {"tapeSat", 0.28f}, {"wowDepth", 0.16f}, {"flutterDepth", 0.06f},
            {"hissLevel", 0.12f}, {"printMix", 0.75f}, {"outputGain", -2.0f}
        }, 0.0f, 0.35f},

        {"Fambly Tape", {
            {"macroShape", 0.2f}, {"layerDco", 0.6f}, {"layerToy", 0.1f}, {"layerOrgan", 0.3f},
            {"organ16", 5.0f}, {"organ8", 4.0f}, {"organ4", 2.0f}, {"organMix", 3.0f},
            {"filterCutoff", 2800.0f}, {"filterRes", 0.2f}, {"filterMode", 0.0f},
            {"dcoSubLevel", 0.4f}, {"noiseLevel", 0.08f},
            {"lfoRate", 2.0f}, {"lfoToPwm", 0.1f}, {"lfoToVibrato", 4.0f},
            {"chorusMode", 3.0f}, {"driftAmount", 0.5f},
            {"envAttack", 0.1f}, {"envDecay", 1.5f}, {"envSustain", 0.65f}, {"envRelease", 3.0f},
            {"driveGain", 0.08f}, {"tapeSat", 0.26f}, {"wowDepth", 0.16f}, {"flutterDepth", 0.06f},
            {"hissLevel", 0.15f}, {"printMix", 0.74f}, {"outputGain", -3.0f}
        }, -0.2f, 0.7f},

        // --- SHOWCASE / HYBRID ---
        {"Sophtware Pad", {
            {"macroShape", 0.3f}, {"layerDco", 0.65f}, {"layerToy", 0.15f}, {"layerOrgan", 0.2f},
            {"organ16", 4.0f}, {"organ8", 5.0f}, {"organ4", 2.0f}, {"organMix", 3.0f},
            {"toyIndex", 0.2f}, {"toyRatio", 0.4f},
            {"filterCutoff", 3800.0f}, {"filterRes", 0.25f}, {"filterMode", 0.0f},
            {"dcoSubLevel", 0.3f}, {"noiseLevel", 0.04f},
            {"lfoRate", 2.0f}, {"lfoToPwm", 0.15f}, {"lfoToVibrato", 5.0f},
            {"chorusMode", 3.0f}, {"driftAmount", 0.4f},
            {"envAttack", 0.5f}, {"envDecay", 2.0f}, {"envSustain", 0.7f}, {"envRelease", 4.5f},
            {"driveGain", 0.1f}, {"tapeSat", 0.3f}, {"wowDepth", 0.15f}, {"flutterDepth", 0.05f},
            {"hissLevel", 0.08f}, {"printMix", 0.75f}, {"outputGain", -1.0f}
        }, 0.0f, 0.1f},

        {"Jed's PSS", {
            {"macroShape", 0.5f}, {"layerDco", 0.1f}, {"layerToy", 0.85f}, {"layerOrgan", 0.0f},
            {"toyIndex", 0.4f}, {"toyRatio", 0.5f},
            {"filterCutoff", 7000.0f}, {"filterRes", 0.1f}, {"filterMode", 0.0f},
            {"dcoSubLevel", 0.0f}, {"noiseLevel", 0.03f},
            {"lfoRate", 3.0f}, {"lfoToPwm", 0.0f}, {"lfoToVibrato", 3.0f},
            {"chorusMode", 1.0f}, {"driftAmount", 0.25f},
            {"envAttack", 0.005f}, {"envDecay", 0.6f}, {"envSustain", 0.35f}, {"envRelease", 0.5f},
            {"driveGain", 0.08f}, {"tapeSat", 0.2f}, {"wowDepth", 0.08f}, {"flutterDepth", 0.03f},
            {"hissLevel", 0.05f}, {"printMix", 0.6f}, {"outputGain", 0.0f}
        }, 0.4f, -0.25f},

        {"Pilot's Ghost", {
            {"macroShape", 0.2f}, {"layerDco", 1.0f}, {"layerToy", 0.0f}, {"layerOrgan", 0.0f},
            {"filterCutoff", 4000.0f}, {"filterRes", 0.3f}, {"filterMode", 0.0f},
            {"dcoSubLevel", 0.2f}, {"noiseLevel", 0.01f},
            {"lfoRate", 1.5f}, {"lfoToPwm", 0.2f}, {"lfoToVibrato", 4.0f},
            {"chorusMode", 3.0f}, {"driftAmount", 0.5f},
            {"envAttack", 0.3f}, {"envDecay", 1.5f}, {"envSustain", 0.6f}, {"envRelease", 3.0f},
            {"driveGain", 0.05f}, {"tapeSat", 0.15f}, {"wowDepth", 0.1f}, {"flutterDepth", 0.04f},
            {"hissLevel", 0.05f}, {"printMix", 0.65f}, {"outputGain", 0.0f}
        }, 0.3f, 0.25f},

        {"Skyward Glide", {
            {"macroShape", 0.45f}, {"layerDco", 0.7f}, {"layerToy", 0.2f}, {"layerOrgan", 0.1f},
            {"organ16", 3.0f}, {"organ8", 4.0f}, {"organ4", 1.0f}, {"organMix", 2.0f},
            {"toyIndex", 0.3f}, {"toyRatio", 0.4f},
            {"filterCutoff", 5500.0f}, {"filterRes", 0.2f}, {"filterMode", 0.0f},
            {"dcoSubLevel", 0.2f}, {"noiseLevel", 0.02f},
            {"lfoRate", 2.5f}, {"lfoToPwm", 0.2f}, {"lfoToVibrato", 6.0f},
            {"chorusMode", 2.0f}, {"driftAmount", 0.35f},
            {"portaTime", 300.0f}, {"portaMode", 0.0f},
            {"envAttack", 0.2f}, {"envDecay", 1.0f}, {"envSustain", 0.55f}, {"envRelease", 2.5f},
            {"driveGain", 0.08f}, {"tapeSat", 0.2f}, {"wowDepth", 0.1f}, {"flutterDepth", 0.04f},
            {"hissLevel", 0.05f}, {"printMix", 0.65f}, {"outputGain", 0.0f}
        }, 0.55f, 0.15f},

    };
}

void WaverProcessor::applyPreset(const Preset& preset)
{
    for (const auto& [id, value] : preset.parameters)
    {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WaverProcessor();
}
