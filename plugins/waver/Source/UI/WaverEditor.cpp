#include "WaverEditor.h"
#include "../Processors/WaverProcessor.h"
#include "WebViewBridge.h"

#include <juce_data_structures/juce_data_structures.h>
#include "BinaryData.h"

namespace
{
constexpr int kEditorWidth = 420;
constexpr int kEditorHeight = 700;
constexpr double kTransportStateStaleMs = 1000.0;

std::optional<juce::WebBrowserComponent::Resource> getWaverResource(const juce::String& url)
{
    using Bridge = threadbare::core::WebViewBridge;
    auto path = Bridge::cleanURLPath(url, "WaverResources");
    int dataSize = 0;
    const char* data = nullptr;
    const auto originalPath = path;

    juce::StringArray namesToTry;
    const auto mangled = path.replaceCharacter('.', '_')
                             .replaceCharacter('-', '_')
                             .replaceCharacter('/', '_');
    namesToTry.add(mangled);
    namesToTry.add(Bridge::toResourceName(path));
    namesToTry.add("dist_" + mangled);
    if (path.contains("/"))
    {
        const auto filename = path.fromLastOccurrenceOf("/", false, false);
        namesToTry.add(filename.replaceCharacter('.', '_').replaceCharacter('-', '_'));
    }
    namesToTry.removeDuplicates(false);

    for (const auto& name : namesToTry)
    {
        data = WaverResources::getNamedResource(name.toRawUTF8(), dataSize);
        if (data != nullptr && dataSize > 0)
            break;
    }

    if (data == nullptr)
    {
        for (int i = 0; i < WaverResources::namedResourceListSize; ++i)
        {
            const auto* resourceName = WaverResources::namedResourceList[i];
            const auto* originalFilename = WaverResources::getNamedResourceOriginalFilename(resourceName);
            if (originalFilename == nullptr)
                continue;

            const juce::String originalString(originalFilename);
            if (originalString.endsWithIgnoreCase(path) || path.endsWithIgnoreCase(originalString))
            {
                data = WaverResources::getNamedResource(resourceName, dataSize);
                if (data != nullptr && dataSize > 0)
                    break;
            }
        }
    }

    if (data == nullptr || dataSize <= 0)
    {
        DBG("WaverEditor: Resource not found: " << url);
        return std::nullopt;
    }

    return juce::WebBrowserComponent::Resource{
        std::vector<std::byte>(reinterpret_cast<const std::byte*>(data), reinterpret_cast<const std::byte*>(data) + dataSize),
        Bridge::getMimeType(originalPath)
    };
}

threadbare::core::NativeFunctionMap createNativeFunctions(WaverProcessor& processor)
{
    auto* processorPtr = &processor;
    return {
        {
            "setParameter",
            [processorPtr](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
            {
                if (args.size() >= 2)
                {
                    const auto paramId = args[0].toString();
                    const auto value = static_cast<float>(args[1]);
                    if (auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*>(
                            processorPtr->getValueTreeState().getParameter(paramId)))
                    {
                        rangedParam->setValueNotifyingHost(rangedParam->convertTo0to1(value));
                    }
                }
                completion({});
            }
        },
        {
            "setMorphSnapshot",
            [processorPtr](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
            {
                if (args.size() >= 3)
                {
                    processorPtr->setMorphSnapshot(static_cast<float>(args[0]),
                                                   static_cast<float>(args[1]),
                                                   static_cast<float>(args[2]));
                }
                completion({});
            }
        },
        {
            "enqueueUiEvent",
            [processorPtr](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
            {
                if (args.size() >= 1)
                {
                    const auto type = args[0].toString();
                    if (type == "moment")
                        processorPtr->enqueueMomentTrigger();
                }
                completion({});
            }
        },
        {
            "getPresetList",
            [processorPtr](const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion completion)
            {
                juce::Array<juce::var> presets;
                for (int i = 0; i < processorPtr->getNumPrograms(); ++i)
                    presets.add(processorPtr->getProgramName(i));
                completion(presets);
            }
        },
        {
            "loadPreset",
            [processorPtr](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
            {
                if (args.size() < 1)
                {
                    completion(false);
                    return;
                }
                const auto index = static_cast<int>(args[0]);
                if (index >= 0 && index < processorPtr->getNumPrograms())
                {
                    processorPtr->setCurrentProgram(index);
                    completion(true);
                    return;
                }
                completion(false);
            }
        }
    };
}
} // namespace

WaverEditor::WaverEditor(WaverProcessor& proc)
    : juce::AudioProcessorEditor(proc),
      processorRef(proc),
      webView(threadbare::core::WebViewBridge::createOptions(
          getWaverResource,
          createNativeFunctions(proc),
          "ThreadbareWaver"))
{
    setSize(kEditorWidth, kEditorHeight);
    addAndMakeVisible(webView);
    webView.goToURL(threadbare::core::WebViewBridge::getInitialURL());
    lastVisualStatePopMs = juce::Time::getMillisecondCounterHiRes();
    vblankAttachment = std::make_unique<juce::VBlankAttachment>(&webView, [this] { handleUpdate(); });
}

void WaverEditor::resized()
{
    webView.setBounds(getLocalBounds());
}

void WaverEditor::handleUpdate()
{
    const auto nowMs = juce::Time::getMillisecondCounterHiRes();
    WaverProcessor::WaverState dequeued{};
    if (processorRef.popVisualState(dequeued))
    {
        cachedVisualState = dequeued;
        hasCachedVisualState = true;
        lastVisualStatePopMs = nowMs;
    }

    if (!hasCachedVisualState)
        return;

    const auto& state = cachedVisualState;
    const bool transportStateIsStale = (nowMs - lastVisualStatePopMs) > kTransportStateStaleMs;
    auto* obj = new juce::DynamicObject();
    obj->setProperty("puckX", state.puckX);
    obj->setProperty("puckY", state.puckY);
    obj->setProperty("presetPuckX", state.presetPuckX);
    obj->setProperty("presetPuckY", state.presetPuckY);
    obj->setProperty("mix", state.mix);
    obj->setProperty("rms", state.rmsLevel);
    obj->setProperty("peak", state.peakLevel);
    obj->setProperty("arpEnabled", state.arpEnabled);
    obj->setProperty("noteActive", state.noteActive);
    // Note: `isPlaying` is used by the viz to gate animation. In many hosts (Logic),
    // live MIDI input happens while transport is stopped. We keep `transportActive`
    // authoritative for UI/ARP locking, but allow the viz to animate when notes are held.
    obj->setProperty("isPlaying",
                     (transportStateIsStale ? false : state.isPlaying) || state.noteActive);
    obj->setProperty("isRecording", transportStateIsStale ? false : state.isRecording);
    obj->setProperty("transportActive", transportStateIsStale ? false : state.transportActive);
    obj->setProperty("currentPreset", processorRef.getCurrentProgram());

    webView.emitEventIfBrowserIsVisible("updateState", juce::JSON::toString(juce::var(obj)));
}
