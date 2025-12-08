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

    // Helper to get MIME type from file extension
    juce::String getMimeType(const juce::String& path)
    {
        if (path.endsWithIgnoreCase(".html")) return "text/html";
        if (path.endsWithIgnoreCase(".js"))   return "text/javascript";
        if (path.endsWithIgnoreCase(".css"))  return "text/css";
        if (path.endsWithIgnoreCase(".png"))  return "image/png";
        if (path.endsWithIgnoreCase(".svg"))  return "image/svg+xml";
        return "application/octet-stream";
    }

    // Helper to convert a path to JUCE binary resource name format
    juce::String toResourceName(const juce::String& path)
    {
        // JUCE mangles: replace all non-alphanumeric chars with underscore
        juce::String result;
        for (int i = 0; i < path.length(); ++i)
        {
            auto c = path[i];
            if (juce::CharacterFunctions::isLetterOrDigit(c))
                result += c;
            else
                result += '_';
        }
        return result;
    }

    // Helper to get resource with proper path normalization and MIME types
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url)
    {
        // 1. Clean the URL path
        juce::String path = url;
        
        // Strip any protocol prefixes
        if (path.startsWith("juce-resource://"))
        {
            path = path.substring(juce::String("juce-resource://").length());
            if (path.startsWith("UnravelResources/"))
                path = path.substring(juce::String("UnravelResources/").length());
        }
        
        // Remove leading slashes and query params
        while (path.startsWith("/"))
            path = path.substring(1);
        
        int queryIndex = path.indexOfChar('?');
        if (queryIndex > -1)
            path = path.substring(0, queryIndex);
        
        // Default to index.html for empty path
        if (path.isEmpty())
            path = "index.html";
        
        // 2. Find in UnravelResources
        int dataSize = 0;
        const char* data = nullptr;
        juce::String originalPath = path;
        
        // Build list of mangled names to try
        juce::StringArray namesToTry;
        
        // Primary: simple underscore mangling (JUCE's format)
        juce::String mangled = path.replaceCharacter('.', '_')
                                   .replaceCharacter('-', '_')
                                   .replaceCharacter('/', '_');
        namesToTry.add(mangled);
        
        // Full alphanumeric mangling
        namesToTry.add(toResourceName(path));
        
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
                getMimeType(originalPath)
            };
        }
        
        // Log failure for debugging
        DBG("UnravelEditor: Resource not found: " << url);
        return std::nullopt;
    }

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
        
        // Resource provider for juce-resource:// URLs with proper path normalization
        options = options.withResourceProvider(
            [](const juce::String& url) -> std::optional<juce::WebBrowserComponent::Resource>
            {
                return getResource(url);
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
    // Use JUCE's resource provider root URL - this triggers the resource provider callback
    // Note: Using "juce-resource://..." does NOT work on macOS; must use getResourceProviderRoot()
    const auto resourceUrl = juce::WebBrowserComponent::getResourceProviderRoot() + "index.html";
    
    DBG("UnravelEditor: Loading UI from " << resourceUrl);
    webView.goToURL(resourceUrl);
}