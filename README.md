# Threadbare Unravel

A dreamy reverb plugin with WebView UI, built with JUCE 8 and C++20.

## Build Instructions

### Prerequisites
- CMake 3.22+
- C++20 compiler (Clang 14+, GCC 12+, or MSVC 2022)
- Node.js 18+ (for frontend bundling)

### Full Build

```bash
# 1. Build frontend (required after editing src/ files)
cd Source/UI/frontend
npm install
npm run build

# 2. Build plugin
cd ../../..
cmake -B build
cmake --build build --target ThreadbareUnravel_All -j8
```

Plugins are installed to:
- **AU:** `~/Library/Audio/Plug-Ins/Components/ThreadbareUnravel.component`
- **VST3:** `~/Library/Audio/Plug-Ins/VST3/ThreadbareUnravel.vst3`

### Frontend Development

The UI is built with vanilla JS and bundled by Vite into a single HTML file.

**Source files:** `Source/UI/frontend/src/`
- `style.css` - All styles
- `orb.js` - Animated orb visualization
- `controls.js` - Puck, sliders, freeze button
- `presets.js` - Preset dropdown
- `main.js` - App initialization

**After editing any frontend source files:**
```bash
cd Source/UI/frontend && npm run build
```
Then rebuild the plugin. The Vite build bundles everything into `dist/index.html`, which JUCE embeds as binary data.

### Quick Rebuild (no frontend changes)

```bash
cmake --build build --target ThreadbareUnravel_All -j8
```

### Clean Build

```bash
rm -rf build
cmake -B build
cmake --build build --target ThreadbareUnravel_All -j8
```

## Project Structure

```
plugins/unravel/
├── Source/
│   ├── DSP/                 # Pure C++ audio processing
│   │   └── UnravelReverb.h/cpp  # FDN reverb + ghost engine + disintegration looper
│   ├── Processors/          # JUCE AudioProcessor
│   │   └── UnravelProcessor.h/cpp
│   ├── UI/                  # WebView integration
│   │   ├── UnravelEditor.h/cpp
│   │   └── frontend/        # Web UI source
│   │       ├── src/         # Edit these files
│   │       └── dist/        # Bundled output (don't edit)
│   ├── UnravelTuning.h      # All magic numbers / tuning constants
│   └── UnravelGeneratedParams.h  # Auto-generated from params.json
├── config/
│   └── params.json          # Parameter definitions (source of truth)
└── assets/
    └── app-icon.png

shared/                      # Shared code across plugins
├── core/                    # ProcessorBase, WebViewBridge
├── scripts/                 # generate_params.js
└── ui/shell/                # Shared UI components

docs/                        # Design specs and implementation guides
```

## Documentation

- `docs/unravel_spec.md` - Product requirements
- `docs/unravel_tuning_cheatsheet.md` - DSP tuning reference
- `docs/webview_integration_guide.md` - JUCE WebView setup

