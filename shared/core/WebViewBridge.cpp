#include "WebViewBridge.h"

namespace threadbare::core
{

juce::WebBrowserComponent::Options WebViewBridge::createOptions(
    ResourceProvider resourceProvider,
    const NativeFunctionMap& nativeFunctions,
    const juce::String& windowsAppName)
{
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withBackend(juce::WebBrowserComponent::Options::Backend::defaultBackend);

#if JUCE_WINDOWS
    // Windows WebView2 workaround: use custom userDataFolder to prevent permissions issues
    options = options.withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
                     .withUserDataFolder(getWindowsUserDataFolder(windowsAppName));
#endif

    // Register native functions
    for (const auto& [name, handler] : nativeFunctions)
    {
        options = options.withNativeFunction(name.c_str(), handler);
    }

    // Set up resource provider
    if (resourceProvider)
    {
        options = options.withResourceProvider(
            [provider = std::move(resourceProvider)](const juce::String& url) 
                -> std::optional<juce::WebBrowserComponent::Resource>
            {
                return provider(url);
            });
    }

    return options;
}

juce::String WebViewBridge::getInitialURL(const juce::String& filename)
{
    // Use JUCE's resource provider root URL
    // Note: Using "juce-resource://..." does NOT work on macOS; must use getResourceProviderRoot()
    return juce::WebBrowserComponent::getResourceProviderRoot() + filename;
}

juce::String WebViewBridge::getMimeType(const juce::String& path)
{
    if (path.endsWithIgnoreCase(".html")) return "text/html";
    if (path.endsWithIgnoreCase(".js"))   return "text/javascript";
    if (path.endsWithIgnoreCase(".css"))  return "text/css";
    if (path.endsWithIgnoreCase(".png"))  return "image/png";
    if (path.endsWithIgnoreCase(".jpg") || path.endsWithIgnoreCase(".jpeg"))  return "image/jpeg";
    if (path.endsWithIgnoreCase(".gif"))  return "image/gif";
    if (path.endsWithIgnoreCase(".svg"))  return "image/svg+xml";
    if (path.endsWithIgnoreCase(".json")) return "application/json";
    if (path.endsWithIgnoreCase(".woff")) return "font/woff";
    if (path.endsWithIgnoreCase(".woff2")) return "font/woff2";
    if (path.endsWithIgnoreCase(".ttf"))  return "font/ttf";
    return "application/octet-stream";
}

juce::String WebViewBridge::cleanURLPath(const juce::String& url, const juce::String& resourceNamespace)
{
    juce::String path = url;

    // Strip any protocol prefixes
    if (path.startsWith("juce-resource://"))
    {
        path = path.substring(juce::String("juce-resource://").length());
        
        // Strip namespace prefix if provided
        if (resourceNamespace.isNotEmpty() && path.startsWith(resourceNamespace + "/"))
        {
            path = path.substring(resourceNamespace.length() + 1);
        }
    }

    // Remove leading slashes
    while (path.startsWith("/"))
        path = path.substring(1);

    // Remove query params
    int queryIndex = path.indexOfChar('?');
    if (queryIndex > -1)
        path = path.substring(0, queryIndex);

    // Default to index.html for empty path
    if (path.isEmpty())
        path = "index.html";

    return path;
}

juce::String WebViewBridge::toResourceName(const juce::String& path)
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

#if JUCE_WINDOWS
juce::File WebViewBridge::getWindowsUserDataFolder(const juce::String& appName)
{
    auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    auto folder = tempDir.getChildFile(appName + "WebView");
    folder.createDirectory();
    return folder;
}
#endif

} // namespace threadbare::core

