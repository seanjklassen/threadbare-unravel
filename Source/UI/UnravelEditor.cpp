#include "UnravelEditor.h"
#include "../Processors/UnravelProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>

// Include generated binary resources
#include "BinaryData.h"

namespace
{
    constexpr int kEditorWidth = 420;
    constexpr int kEditorHeight = 700;

#if JUCE_WINDOWS
    juce::File getUserDataFolder()
    {
        auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
        auto folder = tempDir.getChildFile("ThreadbareUnravelWebView");
        folder.createDirectory();
        return folder;
    }
#endif

    juce::WebBrowserComponent::Options makeBrowserOptions(UnravelProcessor& processor)
    {
        auto options = juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withBackend(juce::WebBrowserComponent::Options::Backend::defaultBackend);

#if JUCE_WINDOWS
        options = options.withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
                         .withUserDataFolder(getUserDataFolder());
#endif

        auto* processorPtr = &processor;
        
        // Native function: setParameter
        options = options.withNativeFunction(
            "setParameter",
            [processorPtr](const juce::Array<juce::var>& args,
                           juce::WebBrowserComponent::NativeFunctionCompletion completion)
            {
                if (args.size() < 2)
                {
                    completion({});
                    return;
                }

                const auto paramId = args[0].toString();
                const auto value = static_cast<float>(args[1]);

                if (auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*>(
                        processorPtr->getValueTreeState().getParameter(paramId)))
                {
                    const auto normalised = rangedParam->convertTo0to1(value);
                    rangedParam->beginChangeGesture();
                    rangedParam->setValueNotifyingHost(normalised);
                    rangedParam->endChangeGesture();
                }

                completion({});
            });
        
        // Native function: getPresetList
        options = options.withNativeFunction(
            "getPresetList",
            [processorPtr](const juce::Array<juce::var>&,
                           juce::WebBrowserComponent::NativeFunctionCompletion completion)
            {
                juce::Array<juce::var> presets;
                for (int i = 0; i < processorPtr->getNumPrograms(); ++i)
                {
                    presets.add(processorPtr->getProgramName(i));
                }
                completion(presets);
            });
        
        // Native function: loadPreset
        options = options.withNativeFunction(
            "loadPreset",
            [processorPtr](const juce::Array<juce::var>& args,
                           juce::WebBrowserComponent::NativeFunctionCompletion completion)
            {
                if (args.size() < 1)
                {
                    completion(false);
                    return;
                }
                
                const int index = static_cast<int>(args[0]);
                if (index >= 0 && index < processorPtr->getNumPrograms())
                {
                    processorPtr->setCurrentProgram(index);
                    completion(true);
                }
                else
                {
                    completion(false);
                }
            });
        
        return options;
    }
} // namespace

UnravelEditor::UnravelEditor(UnravelProcessor& proc)
    : juce::AudioProcessorEditor(proc),
      processorRef(proc),
      webView(makeBrowserOptions(proc))
{
    setSize(kEditorWidth, kEditorHeight);

    addAndMakeVisible(webView);
    loadInitialURL();

    // Drive UI updates via VBlank (screen refresh rate)
    vblankAttachment = std::make_unique<juce::VBlankAttachment>(
        &webView,
        [this]
        {
            handleUpdate();
        });
}

void UnravelEditor::resized()
{
    webView.setBounds(getLocalBounds());
}

void UnravelEditor::handleUpdate()
{
    // C++ -> JS: Pull the latest snapshot from the FIFO and push to the browser
    threadbare::dsp::UnravelState dequeued{};
    if (processorRef.popVisualState(dequeued))
    {
        cachedVisualState = dequeued;
        hasCachedVisualState = true;
    }

    if (! hasCachedVisualState)
        return;

    const auto& state = cachedVisualState;

    auto* obj = new juce::DynamicObject();
    obj->setProperty("puckX", state.puckX);
    obj->setProperty("puckY", state.puckY);
    obj->setProperty("mix", state.mix);
    obj->setProperty("size", state.size);
    obj->setProperty("decaySeconds", state.decaySeconds);
    obj->setProperty("tone", state.tone);
    obj->setProperty("drift", state.drift);
    obj->setProperty("ghost", state.ghost);
    obj->setProperty("duck", state.duck);
    obj->setProperty("freeze", state.freeze);
    
    // Metering
    obj->setProperty("inLevel", state.inLevel);
    obj->setProperty("tailLevel", state.tailLevel);
    
    // Current preset
    obj->setProperty("currentPreset", processorRef.getCurrentProgram());

    // juce::JSON::toString takes a pointer or reference depending on version, 
    // wrapping it in a var ensures safety.
    const auto jsonString = juce::JSON::toString(juce::var(obj));
    
    webView.emitEventIfBrowserIsVisible("updateState", jsonString);
}

void UnravelEditor::loadInitialURL()
{
    // Load HTML directly from embedded binary data as a data URL
    // This enables native integration while avoiding file:// CORS issues
    
    juce::String htmlContent(UnravelResources::index_html, UnravelResources::index_htmlSize);
    
    // Encode as data URL
    juce::String dataUrl = "data:text/html;charset=utf-8;base64," + juce::Base64::toBase64(htmlContent);
    
    webView.goToURL(dataUrl);
    juce::Logger::writeToLog("Loading UI from embedded data URL");
}