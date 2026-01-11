#include "UnravelEditor.h"
#include "../Processors/UnravelProcessor.h"
#include "WebViewBridge.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>

// Include generated binary resources
#include "BinaryData.h"

namespace
{
    constexpr int kEditorWidth = 420;
    constexpr int kEditorHeight = 700;

    // Resource provider for Unravel's embedded UI assets
    std::optional<juce::WebBrowserComponent::Resource> getUnravelResource(const juce::String& url)
    {
        using Bridge = threadbare::core::WebViewBridge;
        
        // Clean the URL path
        juce::String path = Bridge::cleanURLPath(url, "UnravelResources");
        juce::String originalPath = path;
        
        // Find in UnravelResources
        int dataSize = 0;
        const char* data = nullptr;
        
        // Build list of mangled names to try
        juce::StringArray namesToTry;
        
        // Primary: simple underscore mangling (JUCE's format)
        juce::String mangled = path.replaceCharacter('.', '_')
                                   .replaceCharacter('-', '_')
                                   .replaceCharacter('/', '_');
        namesToTry.add(mangled);
        
        // Full alphanumeric mangling
        namesToTry.add(Bridge::toResourceName(path));
        
        // With "dist_" prefix
        namesToTry.add("dist_" + mangled);
        
        // Filename only if path has slashes
        if (path.contains("/"))
        {
            juce::String filename = path.fromLastOccurrenceOf("/", false, false);
            namesToTry.add(filename.replaceCharacter('.', '_').replaceCharacter('-', '_'));
        }
        
        namesToTry.removeDuplicates(false);
        
        // Try each name
        for (const auto& name : namesToTry)
        {
            data = UnravelResources::getNamedResource(name.toRawUTF8(), dataSize);
            if (data != nullptr && dataSize > 0)
                break;
        }
        
        // Fallback: search by original filename
        if (data == nullptr)
        {
            for (int i = 0; i < UnravelResources::namedResourceListSize; ++i)
            {
                const char* resName = UnravelResources::namedResourceList[i];
                const char* origFilename = UnravelResources::getNamedResourceOriginalFilename(resName);
                
                if (origFilename != nullptr)
                {
                    juce::String origStr(origFilename);
                    if (origStr.endsWithIgnoreCase(path) || path.endsWithIgnoreCase(origStr))
                    {
                        data = UnravelResources::getNamedResource(resName, dataSize);
                        if (data != nullptr && dataSize > 0)
                            break;
                    }
                }
            }
        }
        
        // Return resource
        if (data != nullptr && dataSize > 0)
        {
            return juce::WebBrowserComponent::Resource {
                std::vector<std::byte>(reinterpret_cast<const std::byte*>(data), 
                                      reinterpret_cast<const std::byte*>(data) + dataSize),
                Bridge::getMimeType(originalPath)
            };
        }
        
        // Log failure for debugging
        DBG("UnravelEditor: Resource not found: " << url);
        return std::nullopt;
    }

    // Build native function map for Unravel
    threadbare::core::NativeFunctionMap createNativeFunctions(UnravelProcessor& processor)
    {
        auto* processorPtr = &processor;
        
        return {
            // setParameter: Set a plugin parameter from JS
            { "setParameter", 
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
              }
            },
            
            // getPresetList: Get list of factory presets
            { "getPresetList",
              [processorPtr](const juce::Array<juce::var>&,
                            juce::WebBrowserComponent::NativeFunctionCompletion completion)
              {
                  juce::Array<juce::var> presets;
                  for (int i = 0; i < processorPtr->getNumPrograms(); ++i)
                  {
                      presets.add(processorPtr->getProgramName(i));
                  }
                  completion(presets);
              }
            },
            
            // loadPreset: Load a preset by index
            { "loadPreset",
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
              }
            }
        };
    }

    juce::WebBrowserComponent::Options makeBrowserOptions(UnravelProcessor& processor)
    {
        return threadbare::core::WebViewBridge::createOptions(
            getUnravelResource,
            createNativeFunctions(processor),
            "ThreadbareUnravel"
        );
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
    
    // Tempo from DAW
    obj->setProperty("tempo", state.tempo);
    
    // === DISINTEGRATION LOOPER STATE ===
    // looperState: 0 = Idle, 1 = Recording, 2 = Looping
    obj->setProperty("looperState", static_cast<int>(state.looperState));
    obj->setProperty("loopProgress", state.loopProgress);
    obj->setProperty("entropy", state.entropy);
    
    // Current preset
    obj->setProperty("currentPreset", processorRef.getCurrentProgram());

    // juce::JSON::toString takes a pointer or reference depending on version, 
    // wrapping it in a var ensures safety.
    const auto jsonString = juce::JSON::toString(juce::var(obj));
    
    webView.emitEventIfBrowserIsVisible("updateState", jsonString);
}

void UnravelEditor::loadInitialURL()
{
    const auto resourceUrl = threadbare::core::WebViewBridge::getInitialURL();
    DBG("UnravelEditor: Loading UI from " << resourceUrl);
    webView.goToURL(resourceUrl);
}
