# Threadbare

Audio tools with character — built with JUCE 8 and C++20.

**Products:**
- **Unravel** — a "memory cloud" reverb with ghost engine and disintegration looper.
- **Waver** — a character-driven soft synthesizer with tri-layer engine and arpeggiator.

## Build Instructions

### Prerequisites
- CMake 3.22+
- C++20 compiler (Clang 14+, GCC 12+, or MSVC 2022)
- Node.js 18+ (for frontend bundling)

### Full Build

```bash
# 1. Build frontends (required after editing frontend src/ files)
cd plugins/unravel/Source/UI/frontend && npm install && npm run build && cd -
cd plugins/waver/Source/UI/frontend && npm install && npm run build && cd -

# 2. Build all plugins
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Plugins are installed to:
- **AU:** `~/Library/Audio/Plug-Ins/Components/{product}.component`
- **VST3:** `~/Library/Audio/Plug-Ins/VST3/{product}.vst3`

### Frontend Development

Each plugin's UI is built with vanilla JS and bundled by Vite into a single HTML file. Shared controls (puck, sliders, presets) live in `shared/ui/shell/` and are aliased via `@threadbare/shell`. The JUCE native function bridge lives in `shared/ui/bridge/`.

**Plugin-specific frontend sources:**

| Plugin | Directory | Key files |
|--------|-----------|-----------|
| Unravel | `plugins/unravel/Source/UI/frontend/src/` | `main.js`, `orb.js` (Lissajous visualization) |
| Waver | `plugins/waver/Source/UI/frontend/src/` | `main.js`, `viz.js` (waveform scope), `palette.js`, `rbf.js`, `drawer.js`, `preset-surfaces.js`, `waver.css` |

**After editing any frontend source files:**
```bash
cd plugins/{product}/Source/UI/frontend && npm run build
```
Then rebuild the plugin. The Vite build bundles everything into `dist/index.html`, which JUCE embeds as binary data.

### Quick Rebuild (no frontend changes)

```bash
cmake --build build --config Release
```

### Clean Build

```bash
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Project Structure

```
threadbare-unravel/
├── plugins/
│   ├── unravel/                 # Reverb plugin
│   │   ├── Source/
│   │   │   ├── DSP/             # FDN reverb, ghost engine, disintegration looper
│   │   │   ├── Processors/      # UnravelProcessor (extends ProcessorBase)
│   │   │   ├── UI/              # UnravelEditor + frontend/
│   │   │   ├── UnravelTuning.h
│   │   │   └── UnravelGeneratedParams.h  (auto-generated)
│   │   ├── config/params.json   # Parameter definitions (source of truth)
│   │   └── assets/app-icon.png
│   └── waver/                   # Synth plugin
│       ├── Source/
│       │   ├── DSP/             # WaverEngine, voices, arp, organ, toy FM, BBD chorus, print chain
│       │   ├── Processors/      # WaverProcessor (extends ProcessorBase)
│       │   ├── UI/              # WaverEditor + frontend/
│       │   ├── WaverTuning.h
│       │   └── WaverGeneratedParams.h  (auto-generated)
│       ├── config/params.json
│       └── assets/app-icon.png
├── shared/
│   ├── core/                    # ProcessorBase, WebViewBridge, StateQueue
│   ├── scripts/                 # generate_params.js, scaffold-plugin.js
│   └── ui/
│       ├── shell/               # Shared UI: puck, sliders, presets, elastic-slider
│       └── bridge/              # juce-bridge.js (JUCE 8 native function bridge)
├── installer/                   # Platform installer resources
├── docs/                        # Specs, guides, brand docs
└── .github/workflows/           # CI pipelines
```

## Documentation

**Global:**
- `docs/threadbare-prd.md` — Product requirements, architecture, and standards
- `docs/threadbare-brand-guide.md` — Visual identity, voice, and design language
- `docs/build_guide.md` — Build instructions and CI setup
- `docs/webview_integration_guide.md` — JUCE 8 WebView setup

**Unravel:**
- `docs/unravel_spec.md` — Product spec
- `docs/unravel_tuning_cheatsheet.md` — DSP tuning reference

**Waver:**
- `docs/waver_spec.md` — Product spec
- `docs/waver_landing_page_copy.md` — Marketing copy

