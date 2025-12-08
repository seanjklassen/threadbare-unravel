# JUCE 8 WebView Integration Guide

This document describes how to properly set up WebBrowserComponent with embedded resources and native function integration in JUCE 8.

## Overview

The Threadbare Unravel plugin uses a WebView-based UI with:
- HTML/CSS/JS frontend built with Vite
- Binary-embedded resources via `juce_add_binary_data`
- Native C++ ↔ JavaScript communication via JUCE 8's native function protocol

---

## 1. CMake Setup for Binary Resources

### Adding Frontend Resources

```cmake
# Glob all files from the built frontend dist folder
file(GLOB_RECURSE UI_RESOURCES CONFIGURE_DEPENDS "${UI_DIR}/frontend/dist/*")

# Create binary data target with custom namespace
juce_add_binary_data(UnravelResources
    NAMESPACE UnravelResources
    SOURCES ${UI_RESOURCES}
)

# Link to your plugin
target_link_libraries(YourPlugin PRIVATE UnravelResources)
```

### Generated Files

JUCE generates:
- `BinaryData.h` - Header with resource declarations
- `BinaryData1.cpp`, `BinaryData2.cpp`, etc. - Resource data

**Important:** The header is always named `BinaryData.h` regardless of target name. The namespace inside matches what you specified (`UnravelResources`).

### Resource Name Mangling

JUCE mangles filenames by replacing special characters with underscores:
- `index.html` → `index_html`
- `style.css` → `style_css`
- `my-component.js` → `my_component_js`

---

## 2. C++ Resource Provider Setup

### Correct URL Scheme

**DO NOT USE:** `juce-resource://YourNamespace/index.html`

**USE:** `juce::WebBrowserComponent::getResourceProviderRoot() + "index.html"`

On macOS, this returns `juce://juce.backend/index.html`

### Resource Provider Implementation

```cpp
#include "BinaryData.h"  // Always this name

// Helper to get MIME type
juce::String getMimeType(const juce::String& path)
{
    if (path.endsWithIgnoreCase(".html")) return "text/html";
    if (path.endsWithIgnoreCase(".js"))   return "text/javascript";
    if (path.endsWithIgnoreCase(".css"))  return "text/css";
    if (path.endsWithIgnoreCase(".png"))  return "image/png";
    if (path.endsWithIgnoreCase(".svg"))  return "image/svg+xml";
    return "application/octet-stream";
}

// Resource provider function
std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url)
{
    // 1. Clean the URL path
    juce::String path = url;
    while (path.startsWith("/"))
        path = path.substring(1);
    
    int queryIndex = path.indexOfChar('?');
    if (queryIndex > -1)
        path = path.substring(0, queryIndex);
    
    if (path.isEmpty())
        path = "index.html";
    
    // 2. Mangle the name (replace . - / with _)
    juce::String mangled = path.replaceCharacter('.', '_')
                               .replaceCharacter('-', '_')
                               .replaceCharacter('/', '_');
    
    // 3. Look up resource
    int dataSize = 0;
    const char* data = UnravelResources::getNamedResource(mangled.toRawUTF8(), dataSize);
    
    // 4. Return with MIME type
    if (data != nullptr && dataSize > 0)
    {
        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte>(reinterpret_cast<const std::byte*>(data), 
                                  reinterpret_cast<const std::byte*>(data) + dataSize),
            getMimeType(path)
        };
    }
    
    return std::nullopt;
}
```

### Browser Options Setup

```cpp
juce::WebBrowserComponent::Options makeBrowserOptions()
{
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withBackend(juce::WebBrowserComponent::Options::Backend::defaultBackend);

#if JUCE_WINDOWS
    // Windows requires webview2 with custom user data folder
    options = options.withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
                     .withUserDataFolder(getTempUserDataFolder());
#endif

    // Register native functions
    options = options.withNativeFunction("setParameter", [](auto& args, auto completion) {
        // Handle parameter changes
        completion({});
    });

    // Attach resource provider
    options = options.withResourceProvider([](const juce::String& url) {
        return getResource(url);
    });

    return options;
}
```

### Loading the UI

```cpp
void loadInitialURL()
{
    // Use JUCE's resource provider root - NOT custom scheme
    const auto resourceUrl = juce::WebBrowserComponent::getResourceProviderRoot() + "index.html";
    webView.goToURL(resourceUrl);
}
```

---

## 3. JavaScript Native Function Integration

### The Problem

JUCE 8 provides native functions via `window.__JUCE__.backend`, but the helper `getNativeFunction()` from JUCE's JS module isn't automatically available when loading HTML via resource provider.

### The Solution: Polyfill

Add this to your main JavaScript entry point **BEFORE any other code**:

```javascript
// =============================================================================
// JUCE 8 Native Function Polyfill
// =============================================================================

class PromiseHandler {
  constructor() {
    this.lastPromiseId = 0
    this.promises = new Map()
    
    if (window.__JUCE__?.backend?.addEventListener) {
      window.__JUCE__.backend.addEventListener('__juce__complete', ({ promiseId, result }) => {
        if (this.promises.has(promiseId)) {
          this.promises.get(promiseId).resolve(result)
          this.promises.delete(promiseId)
        }
      })
    }
  }
  
  createPromise() {
    const promiseId = this.lastPromiseId++
    const result = new Promise((resolve, reject) => {
      this.promises.set(promiseId, { resolve, reject })
    })
    return [promiseId, result]
  }
}

let promiseHandler = null
if (window.__JUCE__?.backend) {
  promiseHandler = new PromiseHandler()
}

const getNativeFunction = (name) => {
  const registeredFunctions = window.__JUCE__?.initialisationData?.__juce__functions || []
  
  if (!registeredFunctions.includes(name)) return null
  if (!promiseHandler || !window.__JUCE__?.backend?.emitEvent) return null
  
  return function() {
    const [promiseId, result] = promiseHandler.createPromise()
    
    window.__JUCE__.backend.emitEvent('__juce__invoke', {
      name: name,
      params: Array.prototype.slice.call(arguments),
      resultId: promiseId
    })
    
    return result
  }
}

// Make globally available
window.__getNativeFunction = getNativeFunction
```

### Using Native Functions

```javascript
// Get function reference (cache it for performance)
let setParameter = null

const sendParam = (id, value) => {
  if (!setParameter && window.__getNativeFunction) {
    setParameter = window.__getNativeFunction('setParameter')
  }
  
  if (typeof setParameter === 'function') {
    setParameter(id, value)
  }
}

// For async functions that return values
const loadPresets = async () => {
  const getPresetList = window.__getNativeFunction('getPresetList')
  if (getPresetList) {
    const presets = await getPresetList()
    console.log('Presets:', presets)
  }
}
```

---

## 4. Vite Build Configuration

### vite-plugin-singlefile

We use `vite-plugin-singlefile` to bundle everything into one HTML file for easy embedding.

```javascript
// vite.config.js
import { defineConfig } from 'vite'
import { viteSingleFile } from 'vite-plugin-singlefile'

export default defineConfig({
  plugins: [viteSingleFile()],
  build: {
    outDir: 'dist',
    assetsInlineLimit: 100000000,  // Inline all assets
    cssCodeSplit: false,
    rollupOptions: {
      output: {
        inlineDynamicImports: true
      }
    }
  }
})
```

### Critical: DOMContentLoaded

When using singlefile, the script ends up in `<head>` and runs before `<body>` is parsed. **Always wrap app initialization:**

```javascript
const initApp = () => {
  const canvas = document.getElementById('orb')
  // ... rest of initialization
}

// Wait for DOM
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', initApp)
} else {
  initApp()
}
```

---

## 5. Build & Deploy Workflow

### Build Steps

```bash
# 1. Build frontend
cd Source/UI/frontend
npm run build

# 2. Rebuild C++ (picks up new binary resources)
cd ../../../build
ninja  # or cmake --build .

# Plugin is now at:
# - build/ThreadbareUnravel_artefacts/Debug/Standalone/
# - build/ThreadbareUnravel_artefacts/Debug/VST3/
# - build/ThreadbareUnravel_artefacts/Debug/AU/
```

### Debugging Tips

1. **Resource not found?** Check the generated `BinaryData.h` for actual resource names
2. **Native functions undefined?** Check `window.__JUCE__.initialisationData.__juce__functions`
3. **UI not rendering?** Check `document.readyState` and wrap in DOMContentLoaded
4. **Puck/controls missing?** Elements might exist but have no position - check bounds/dimensions

### Console Debugging

```javascript
// Check JUCE backend availability
console.log('JUCE backend:', window.__JUCE__?.backend)
console.log('Registered functions:', window.__JUCE__?.initialisationData?.__juce__functions)
console.log('Backend methods:', Object.keys(window.__JUCE__?.backend || {}))
```

---

## 6. Common Pitfalls

| Issue | Cause | Solution |
|-------|-------|----------|
| "Unsupported URL" | Wrong URL scheme | Use `getResourceProviderRoot()` not custom scheme |
| Resource returns null | Name mismatch | Check mangling: `index.html` → `index_html` |
| Native function undefined | JS helpers not loaded | Use the polyfill above |
| UI doesn't render | Script runs before DOM | Wrap in `DOMContentLoaded` check |
| Parameters don't update | Wrong function call | Use `emitEvent('__juce__invoke', ...)` |

---

## 7. File Structure Reference

```
Source/
├── UI/
│   ├── UnravelEditor.cpp      # WebView setup, resource provider
│   ├── UnravelEditor.h
│   └── frontend/
│       ├── dist/              # Built output (embedded in binary)
│       │   └── index.html     # Single-file bundle
│       ├── src/
│       │   ├── main.js        # Entry point with JUCE polyfill
│       │   ├── controls.js    # UI controls
│       │   └── ...
│       ├── index.html         # Dev template
│       ├── package.json
│       └── vite.config.js
└── Processors/
    └── UnravelProcessor.cpp   # Audio processing
```

---

## 8. Testing Checklist

Before release, verify:

- [ ] UI loads in Standalone app
- [ ] UI loads in VST3 host (e.g., Reaper)
- [ ] UI loads in AU host (e.g., Logic)
- [ ] Parameters update when moving controls
- [ ] Presets load and switch correctly
- [ ] State updates flow from C++ to JS (`updateState` events)
- [ ] No console errors in browser dev tools
- [ ] Windows build works with WebView2

---

*Last updated: December 2024*
*JUCE version: 8.0.10*

