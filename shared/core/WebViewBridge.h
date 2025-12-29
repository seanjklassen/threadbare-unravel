#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>
#include <optional>
#include <string>
#include <map>

namespace threadbare::core
{

/**
 * ResourceProvider: Callback type for serving embedded UI resources.
 * 
 * @param url The requested URL (may include protocol prefix)
 * @return Resource data and MIME type, or nullopt if not found
 */
using ResourceProvider = std::function<std::optional<juce::WebBrowserComponent::Resource>(const juce::String&)>;

/**
 * NativeFunctionHandler: Callback type for native function calls from JS.
 */
using NativeFunctionHandler = std::function<void(
    const juce::Array<juce::var>&, 
    juce::WebBrowserComponent::NativeFunctionCompletion)>;

/**
 * NativeFunctionMap: Map of function names to handlers.
 */
using NativeFunctionMap = std::map<std::string, NativeFunctionHandler>;

/**
 * WebViewBridge: Shared WebView setup infrastructure for Threadbare plugins.
 * 
 * Provides:
 * - Platform-aware WebBrowserComponent::Options configuration
 * - Windows WebView2 workaround (custom userDataFolder)
 * - Generic native function registration
 * - Resource provider setup
 */
class WebViewBridge
{
public:
    /**
     * Create WebBrowserComponent::Options with Threadbare defaults.
     * 
     * @param resourceProvider Callback to serve embedded UI assets
     * @param nativeFunctions Map of native function names to handlers
     * @param windowsAppName App name for Windows userDataFolder (defaults to "ThreadbarePlugin")
     * @return Configured Options suitable for WebBrowserComponent constructor
     */
    static juce::WebBrowserComponent::Options createOptions(
        ResourceProvider resourceProvider,
        const NativeFunctionMap& nativeFunctions = {},
        const juce::String& windowsAppName = "ThreadbarePlugin");

    /**
     * Get the initial URL for loading the embedded UI.
     * Uses JUCE's resource provider root URL.
     * 
     * @param filename The HTML file to load (default: "index.html")
     * @return URL string suitable for WebBrowserComponent::goToURL()
     */
    static juce::String getInitialURL(const juce::String& filename = "index.html");

    //==========================================================================
    // Helper utilities for resource providers

    /**
     * Get MIME type from file extension.
     * Supports common web file types (html, js, css, png, svg, etc.)
     */
    static juce::String getMimeType(const juce::String& path);

    /**
     * Clean a URL path for resource lookup.
     * Strips protocol prefixes, leading slashes, and query params.
     */
    static juce::String cleanURLPath(const juce::String& url, const juce::String& resourceNamespace = "");

    /**
     * Convert a file path to JUCE binary resource name format.
     * Replaces all non-alphanumeric characters with underscores.
     */
    static juce::String toResourceName(const juce::String& path);

private:
#if JUCE_WINDOWS
    static juce::File getWindowsUserDataFolder(const juce::String& appName);
#endif

    WebViewBridge() = delete;  // Static-only class
};

} // namespace threadbare::core

