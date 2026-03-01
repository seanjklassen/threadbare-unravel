# Threadbare — Product Requirements Document

*Living reference for all Threadbare products. Version 1.0 — February 2026.*

**Document structure:** Sections 1-3 define global standards that apply to every Threadbare plugin (company identity, technical architecture, real-time safety rules, build system). Section 4 is the product spec for Unravel — every current and future product gets its own section here. Section 5 defines global quality standards. Section 6 is the step-by-step guide for building a new plugin. Section 7 is the documentation index.

---

## 1. Company Overview

**Threadbare** is an independent audio software studio building tools for musicians, producers, and sound designers who value character over feature count. Every product ships with a clear emotional point of view, a minimal control surface, and real-time-safe DSP that runs without compromise on modern hardware.

### 1.1 Mission

Make audio tools that feel like instruments — expressive, immediate, and sonically opinionated — rather than utility software.

### 1.2 Core Values

| Value | What It Means in Practice |
|---|---|
| **Character over neutrality** | Every product has a sonic identity. We don't make transparent processors. |
| **Simplicity over flexibility** | Fewer controls, deeper expression. One great macro beats ten mediocre knobs. |
| **Real-time safety** | No allocations, no locks, no exceptions on the audio thread. Ever. |
| **Emotional resonance** | Product names, presets, and copy evoke feelings, not technical specs. |
| **Craftsmanship** | Ship fewer products, make each one excellent. |

### 1.3 Target Audience

- Guitarists in shoegaze, dream pop, ambient, post-rock, and indie genres.
- Synth players and sound designers building pads, textures, and drones.
- Vocalists and producers looking for characterful, non-clinical processing.
- Hobbyists and professionals alike who prefer immediacy over deep menus.

### 1.4 Competitive Positioning

Threadbare occupies the space between boutique pedal makers (Chase Bliss, Meris) and plugin studios (Valhalla DSP, Soundtoys). Products should feel hand-built and personal while delivering studio-grade audio quality. Comparable in sonic quality to Valhalla VintageVerb, TAL Reverb 4, and similar benchmarks in their respective categories.

---

## 2. Product Line

### 2.1 Unravel (v0.7.0 — Current)

**Role:** Lush, characterful "memory cloud" reverb.
**Status:** In active development.
**Formats:** VST3, AU, Standalone.
**Platforms:** macOS (Universal binary arm64 + x86_64), Windows.
**Price:** $45.

### 2.2 Waver (v0.7.0 — Current)

**Role:** Character-driven soft synthesizer — "broken but beautiful synthesis."
**Status:** In active development.
**Formats:** VST3, AU, Standalone.
**Platforms:** macOS (Universal binary arm64 + x86_64), Windows.
**Price:** TBD.

*For the detailed spec, see `docs/waver_spec.md`.*

### 2.3 Future Products

The monorepo (`threadbare-unravel`) supports multiple plugins via `add_subdirectory`. Future products follow the same architecture, shared core, and brand guidelines documented here.

---

## 3. Technical Foundation

### 3.1 Stack

| Layer | Technology | Notes |
|---|---|---|
| DSP | C++20 | Real-time safe, no JUCE UI dependencies |
| Framework | JUCE 8.0.10 | Fetched via CMake FetchContent |
| UI | WebView (HTML/CSS/JS) | Single-file Vite bundle embedded as binary data |
| Build | CMake 3.22+ | Monorepo structure, auto-param generation |
| CI | GitHub Actions | macOS + Windows builds, signing, notarization |
| Frontend tooling | Vite + vite-plugin-singlefile | Node 18+ |

### 3.2 Monorepo Structure

```
threadbare-unravel/
├── plugins/
│   ├── unravel/            # Reverb plugin
│   │   ├── config/         # params.json (parameter source of truth)
│   │   ├── assets/         # Icons, branding
│   │   └── Source/
│   │       ├── DSP/        # Pure C++ signal processing
│   │       ├── Processors/ # JUCE AudioProcessor integration
│   │       └── UI/         # WebView editor + frontend/
│   └── waver/              # Synth plugin (same structure)
│       ├── config/
│       ├── assets/
│       └── Source/
│           ├── DSP/
│           ├── Processors/
│           └── UI/
├── shared/
│   ├── core/               # ProcessorBase, WebViewBridge, StateQueue
│   ├── scripts/            # generate_params.js, scaffold-plugin.js
│   └── ui/
│       ├── shell/          # Shared UI components (puck, sliders, presets, elastic-slider)
│       └── bridge/         # juce-bridge.js (JUCE 8 native function bridge)
├── installer/              # Platform installer resources
├── scripts/                # Build & packaging automation
├── docs/                   # Product specs, guides, this file
└── .github/workflows/      # CI pipelines
```

### 3.3 Architecture Patterns

All Threadbare plugins follow these patterns:

**Separation of Concerns**
- `Source/DSP/` — Pure C++ processing. No JUCE UI headers. Receives `std::span<float>` buffers. Pulls constants from a `Tuning.h` file. Testable in isolation.
- `Source/Processors/` — JUCE `AudioProcessor` wrapper. Owns APVTS, factory presets, and state persistence. Bridges DSP to DAW.
- `Source/UI/` — `WebBrowserComponent` editor. Loads embedded HTML. Communicates with C++ via native function protocol.

**Lock-Free Audio/UI Communication**
- Audio thread writes state structs to a `StateQueue` (FIFO-backed).
- UI thread pops state snapshots via `juce::VBlankAttachment` (screen refresh rate) and emits `updateState` to the WebView.
- UI-to-audio commands (e.g. looper triggers) go through a separate command queue.
- No `std::mutex`, no blocking, no shared mutable state.

**Parameter Generation**
- `params.json` is the single source of truth for parameter metadata.
- `generate_params.js` produces both a C++ header (`*GeneratedParams.h`) and a JS module (`generated/params.js`).
- Build system runs generation automatically via custom CMake command.

**WebView Integration**
- Frontend built with Vite into a single `index.html`.
- Embedded as JUCE binary data (`juce_add_binary_data`).
- Loaded via `WebBrowserComponent::getResourceProviderRoot()`.
- Native functions registered in `Options::withNativeFunction()`.
- JUCE 8 polyfill in `main.js` handles `getNativeFunction()`.
- Windows uses `Backend::webview2` with custom `userDataFolder` to avoid permissions crashes.

### 3.4 Real-Time Safety Rules (The Iron Laws)

These apply to `processBlock`, `process`, and any audio-thread logic across all products:

1. **No allocations.** Never `new`, `malloc`, `push_back`, or `std::string` creation.
2. **No locks.** Never `std::mutex`, `CriticalSection`, or blocking calls. Use `std::atomic` or `AbstractFIFO`.
3. **No exceptions.** Disable or guarantee they cannot be thrown on the audio thread.
4. **Numeric stability.** `juce::ScopedNoDenormals` at the start of every process block.
5. **Smoothing.** `juce::SmoothedValue` for all gain and mix parameters.
6. **Interpolation.** Cubic (Hermite or Catmull-Rom) for delay line reads. Linear interpolation is forbidden for any delay-based DSP.
7. **Per-sample updates.** Size and delay parameters update every sample (audio-rate smoothing), not once per block.

### 3.5 Build & Distribution

**Local Development**
```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

**Release Builds**
```
cmake -B build-release -DCMAKE_BUILD_TYPE=Release \
  -DTHREADBARE_COPY_PLUGIN_AFTER_BUILD=OFF \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build-release --config Release
```

**Installers**
- macOS: `scripts/build-installer.sh macos --sign` (flat .pkg with signing + notarization)
- Windows: `scripts/build-installer.ps1 -Sign` (Inno Setup; optional signing via `installer/windows/sign.bat`)

**CI Pipeline** (`.github/workflows/installer-build.yml`)
- macOS: certificate import, codesign, productbuild, notarytool, staple.
- Windows: gated behind `ENABLE_WINDOWS_INSTALLER_CI` repo variable.
- Frontend build runs as prerequisite for both jobs.

**Install Paths**

| Format | macOS | Windows |
|---|---|---|
| VST3 (installer) | `/Library/Audio/Plug-Ins/VST3/{product}.vst3` | `C:\Program Files\Common Files\VST3\{product}.vst3` |
| AU (installer) | `/Library/Audio/Plug-Ins/Components/{product}.component` | N/A |
| Support Data | `/Library/Application Support/{Product}` | `%PUBLIC%\Documents\{Product}` |

Note: local development builds may copy plugins into the user Library (see `docs/build_guide.md`). The macOS installer produced by `scripts/build-installer.sh` installs to system locations under `/Library/Audio/Plug-Ins/`.

**Versioning model**
- Root `CMakeLists.txt` `project(Threadbare VERSION x.y.z)` is the monorepo infrastructure version.
- `installer/product.json` `version` is the plugin release version used by installer output.
- Plugin `CMakeLists.txt` inherits the monorepo version for build orchestration; installer metadata controls shipped product versioning.

---

## 4. Product: Unravel

*Everything in this section is specific to Unravel. Future products get their own section following the same structure. For the detailed spec, see also `docs/unravel_spec.md`.*

### 4.1 Concept

I kept reaching for a reverb that sounded less like a room and more like something slipping away from me. Unravel holds onto a few seconds of what you just played and dissolves it into the tail — some pitched up, some reversed, all of it fading. The ghost material ends up woven into the reverb, which is what gives it that ache.

**What it's for:**
- Shoegaze / dream pop / ambient guitars.
- Pads and synths.
- Indie/ambient vocals.

### 4.2 Emotional Targets

Every design decision — tuning constants, preset names, UI color, copy — must lean toward:

| Target | Description |
|---|---|
| **Spectral** | Reverb feels like a fading memory, not a physical room. Transparent, weightless, hovering. |
| **Wistful** | Permanent tension between comfort and ache. Bittersweet bloom, emotionally evocative. |
| **Diffuse** | Soft and vaporous. No icepick highs or metallic zing. Cloud-like wash that evaporates gently. |

### 4.3 Sound Goals

- Smooth, non-metallic tails.
- Frequency-dependent decay.
- Gentle but persistent modulation.
- Integrated ghost "memory" character (not decorative — the ghost *is* the tail).
- Disintegration looper + ducking that feel instantly usable.
- Subjectively comparable to Valhalla VintageVerb / TAL Reverb 4 in its lane.

### 4.4 UI Specification

**Dimensions:** ~380-420 px wide, ~670-720 px tall.

**Layout (top to bottom):**
1. Preset bar — dropdown left.
2. Main canvas — Lissajous orb visualization + draggable puck (48x48 px).
3. Bottom strip — X/Y readouts (0.00-11.00), looper button (infinity symbol), settings gear.

**Advanced Drawer** — slides open from bottom strip. Contains horizontal sliders with no numeric readout, each with a "pupil" indicator that lights when the puck is contributing a macro value. Sliders are keyboard-accessible with tall invisible hit areas.

**Drawer Parameters:**
1. Decay (0.4-50 s)
2. Distance (0-100 ms)
3. Size (0.5-2.0)
4. Tone (Dark to Bright, -1..1)
5. Drift (0-1)
6. Ghost Mix (0-1)
7. Scatter / Glitch (0-1)
8. Duck (0-1)
9. Blend (0-100%)
10. Output (-24 to +12 dB)

### 4.5 Control Model

#### 4.5.1 Puck (Main Macro Control)

The puck is the primary instrument. It maps across every parameter simultaneously.

**X-axis: Vivid to Hazy** (`puckX` in [-1, 1])
- Left = vivid/physical: stronger ER, tighter focus, recent ghost memories (150ms lookback), narrow stereo, stable modulation (20 samples depth).
- Right = hazy/ethereal: more diffusion, distant ghost memories (750ms lookback), wider stereo, chaotic modulation (80 samples depth).

**Y-axis: Recent to Distant** (`puckY` in [-1, 1])
- Down = recent/near: shorter decay (/3), subtle, tight.
- Up = distant/far: longer decay (x3), more ghost, more drift, Doppler size modulation (0.92x-1.08x).

**Interaction:** Click-drag, constrained to square. Double-click resets to (0,0). When looper is active, axis labels shift to Spectral/Diffuse (X) and Fleeting/Lingering (Y).

#### 4.5.2 Disintegration Looper

William Basinski-inspired loop degradation. Three-state: Idle, Recording, Looping.

**Recording:** Time-based (up to 60s), DAW-agnostic, input-gated. Auto-cancels if no input detected within timeout. Button pulses during recording.

**Looping:** Plays back with entropy-based degradation:
- Ascension Filter: HPF sweeps 20-800 Hz, LPF sweeps 20k-2k Hz (converging).
- Saturation: warm tape saturation (0-0.6) with hysteresis model.
- Oxide Shedding: stochastic dropouts scaling with entropy.
- Motor Death: Brownian pitch drag (+-40 cents, downward bias).
- Azimuth Drift: L/R channels degrade at different rates.
- Wow & Flutter: 0.5 Hz / 6.0 Hz authentic transport wobble.
- Pink Noise Floor: ~-52 dB tape hiss fading in.

**Puck Mapping During Loop:**
- Y controls entropy rate: top = fast (~2 loop iterations), bottom = slow (~endless).
- X controls character: left (Ghost) = spectral thinning (HPF boost x4), right (Fog) = diffuse smearing (LPF reduction x0.25).
- When entropy reaches 1.0, loop fades gracefully back to normal reverb.

#### 4.5.3 Lissajous Orb

Canvas-based visualization. Visual only — reflects plugin state.

**Inputs:** inLevel, tailLevel, puckX, puckY, decay, size, ghost, drift, looperState, entropy, tempo, isPlaying.
**Mapping:** Radius grows with puckY/tailLevel/inLevel. Tangle grows with drift/ghost. Stroke follows tailLevel. Color shifts by looper state and entropy. Motion slows but never stops when transport is stopped.

### 4.6 DSP Architecture

#### 4.6.1 Signal Flow (True Stereo)

```
Input (Stereo)
  → Pre-delay (0-100 ms)
  → Early Reflections (6-tap stereo, asymmetric L/R)
  → Ghost History Buffer write (2.0s circular)
  → Ghost Engine reads history, spawns grains → FDN input
  → FDN Tail (8 delay lines, Householder matrix, per-line damping + modulation)
  → Glitch Sparkle (4-voice transient-reactive granular fragments)
  → Disintegration Looper (when active: capture, loop, degrade)
  → Ducking (envelope follower on dry → modulates wet gain)
  → Mix Dry/Wet
  → Output Gain
  → Output (Stereo)
```

#### 4.6.2 FDN Core

- 8 delay lines with base delays: {31, 37, 41, 53, 61, 71, 83, 97} ms (prime-ish).
- Delay times = base x size scalar (0.5-2.0).
- Fixed orthonormal 8x8 Householder mixing matrix.
- True stereo via input/output vectors (each line gets L/R mix; each output gets mix of all 8).
- Per-line sinusoidal LFO modulation: 0.1-3.0 Hz, randomized rates, up to 100 samples depth.
- Cubic interpolation for delay reads (Hermite).

#### 4.6.3 Decay & Damping

- Target RT60 from Decay slider + puckY offset.
- Feedback gain is derived from RT60 using `g = exp(ln(0.001) * delaySec / T60)`. The implementation computes a global average target and also per-line targets (smoothed) for consistency across delay lengths.
- LPF (Tone-mapped cutoff: 400-16000 Hz) + HPF (100 Hz, prevents LF bloat).

#### 4.6.4 Ghost Engine

The ghost engine is what makes Unravel different. It maintains a 2.0-second history buffer, spawns up to 8 granular grains at any time, and feeds them into the FDN — not on top of it.

**Grain Parameters:**
- Duration: 50-300 ms, Hann window.
- Detune: +/-20 cents.
- Shimmer: 25% probability of +12 semitone (octave up) grains.
- Reverse: probability = 0.25 x ghost^2 (25% at ghost=1.0, ~6% at ghost=0.5).
- Reverse grains get -2.5 dB gain reduction and optional stereo mirroring.
- Gain range: -24 to -9 dB relative to FDN input.

**Memory Proximity (puckX):** Continuous interpolation from 150ms lookback (vivid) to 750ms (hazy).

**Stereo Positioning:** Pan width scales from 0.3 (narrow, low ghost) to 0.85 (wide, high ghost). Constant-power panning. Proximity-based narrowing (vivid = narrower, hazy = wider).

**Direct Mix:** Stereo ghost output also mixes directly into final output (max 0.3 at ghost=1.0, quadratic curve) to preserve granular character.

#### 4.6.5 Glitch Sparkle (Scatter)

4-voice transient-reactive granular fragments:
- Triggered by transient detection, staggered timing.
- Pitch palette: root (25%), fifth up (25%), octave up (20%), octave down (15%), double octave (8%), micro-shimmer (7%).
- 30% reverse grain probability.
- Fragment sizes: 60-400 ms with 12% fade ratio.
- Stereo scattering with ping-pong LFO at 2.5 Hz.
- Haas-effect micro-delays (0.3-1.5 ms) for width.
- Per-voice micro-detune (+/-3 cents).

#### 4.6.6 Ducking

- Envelope follower on mono-summed dry input.
- Attack: 10 ms, Release: 250 ms.
- Wet gain = baseWet x (1 - duckAmount x envelope).
- Minimum wet factor: 0.15 (never fully dries out).

### 4.7 Tuning Abstraction

All magic numbers live in `plugins/unravel/Source/UnravelTuning.h`. DSP code pulls from `threadbare::tuning`, never hardcodes. Each constant has a safe range and a comment describing what it controls in ear-language and what happens if you push it. Sections: `Fdn`, `Decay`, `Damping`, `EarlyReflections`, `Modulation`, `Ghost`, `Freeze`, `GlitchLooper`, `Disintegration`, `Ducking`, `PuckMapping`, `Metering`, `Safety`, `Debug`.

### 4.8 Parameters (Source of Truth: params.json)

| ID | Display Name | Type | Range | Default | Unit |
|---|---|---|---|---|---|
| `puckX` | Puck X | float | -1.0 to 1.0 | 0.0 | — |
| `puckY` | Puck Y | float | -1.0 to 1.0 | 0.0 | — |
| `mix` | Mix | float | 0.0 to 1.0 | 0.5 | — |
| `size` | Size | float | 0.5 to 2.0 | 1.0 | — |
| `decay` | Decay | float | 0.4 to 50.0 | 5.0 | s |
| `tone` | Brightness | float | -1.0 to 1.0 | 0.0 | — |
| `drift` | Drift | float | 0.0 to 1.0 | 0.2 | — |
| `ghost` | Ghost | float | 0.0 to 1.0 | 0.15 | — |
| `glitch` | Glitch | float | 0.0 to 1.0 | 0.0 | — |
| `duck` | Duck | float | 0.0 to 1.0 | 0.0 | — |
| `erPreDelay` | Distance | float | 0.0 to 100.0 | 0.0 | ms |
| `freeze` | Looper | bool | — | false | — |
| `output` | Output | float | -24.0 to 12.0 | 0.0 | dB |

### 4.9 Factory Presets

All names evoke emotion, sensation, or visual imagery. Never technical descriptions or gear references.

| # | Name | Puck | Character | Use Case |
|---|---|---|---|---|
| 1 | **unravel** | (0.0, 0.2) | Balanced ambient pad. The default. | Guitars, synths, vocals. |
| 2 | **close** | (-0.8, -0.6) | Tight, intimate, dark, minimal space. | Vocals, acoustic instruments. |
| 3 | **tether** | (-0.5, 0.1) | Vivid, warm, moderate ducking. | Fingerpicked guitar, intimate vocals. |
| 4 | **pulse** | (0.25, 0.1) | Rhythmic heavy ducking, reverb pumps. | Strummed guitars, rhythmic synths. |
| 5 | **bloom** | (0.4, 0.8) | Slowly expanding wash, long decay. | Pads, synth washes, ambient guitar. |
| 6 | **mist** | (0.9, 0.6) | Maximum haze, dark fog, distant. | Ambient drones, pads. |
| 7 | **rewind** | (0.3, 0.5) | Heavy ghost, high reverse, surreal. | Vocals, sustained sounds. |
| 8 | **halation** | (0.85, 0.7) | Bright, shimmery, overexposed light. | Synth pads, guitar swells. |
| 9 | **stasis** | (0.0, 0.3) | Looper-ready, max ghost, long decay. | Evolving drone pads. |
| 10 | **shiver** | (1.0, 1.0) | Maximum everything, dissolve completely. | Extreme ambient textures. |

### 4.10 Product Metadata

| Field | Value |
|---|---|
| Version | 0.7.0 |
| CMake target | `ThreadbareUnravel` |
| PRODUCT_NAME | `unravel` |
| Company | `threadbare` |
| Manufacturer code | `TrbR` |
| Plugin code | `Unrv` |
| Bundle ID (VST3) | `com.threadbare.unravel.vst3` |
| Bundle ID (AU) | `com.threadbare.unravel.component` |
| Formats | VST3, AU, Standalone |
| No AAX in v1 | Avoids PACE signing complexity |

### 4.11 Unravel-Specific Testing

In addition to the global checklist (section 5.4):

- [ ] Reverb tails are smooth and non-metallic across the full decay range.
- [ ] Ghost engine grains blend naturally into the tail at all mix levels.
- [ ] Looper records, loops, degrades, and exits cleanly.
- [ ] Looper puck mapping (entropy rate, spectral/diffuse character) responds correctly.
- [ ] Disintegration entropy reaches 1.0 and fades back to normal reverb gracefully.
- [ ] Orb visualization responds to audio state and looper transitions.

---

## 5. Quality Standards

These apply to every Threadbare plugin. Product-specific quality targets (e.g., Unravel's reverb tail character, looper behavior) belong in the product's own section.

### 5.1 Audio Quality

- No audible zipper noise on parameter changes.
- No clicks or pops on preset changes, state transitions, or feature toggles.
- No unpleasant artifacts in processing output (ringing, aliasing, harsh transients).
- CPU usage acceptable at default settings; manageable at max settings.
- Mono-compatible at reasonable settings.

### 5.2 Code Quality

- All DSP code passes real-time safety audit (no allocations, locks, or exceptions).
- Tuning constants centralized in a single header. DSP never hardcodes magic numbers.
- Parameter metadata generated from a single JSON source of truth.
- C++20 idioms: `std::span`, `std::unique_ptr`, `constexpr`, structured bindings.
- Namespaces: `threadbare::dsp`, `threadbare::ui`, `threadbare::core`, `threadbare::tuning`.

### 5.3 UI Quality

- UI loads correctly in Standalone, VST3 hosts, and AU hosts.
- Parameters update bidirectionally (UI to DSP, DSP to UI).
- Presets load and switch without artifacts.
- Accessible: keyboard navigation for all controls.
- No console errors in browser dev tools.
- Responsive within target dimensions.

### 5.4 Testing Checklist (Per Release — All Plugins)

- [ ] UI loads in Standalone, VST3 (Reaper), AU (Logic).
- [ ] All parameters update when moving controls.
- [ ] Presets load and switch correctly.
- [ ] State persists across DAW save/load.
- [ ] No clicks/pops at any parameter setting.
- [ ] CPU stable under sustained max-setting use.
- [ ] Windows build works with WebView2.
- [ ] Installer runs clean on fresh macOS and Windows systems.

Each product section should include its own supplemental testing checklist for product-specific features.

---

## 6. New Plugin Guide

This section is a step-by-step reference for adding a new plugin to the Threadbare monorepo. It covers every layer — CMake, DSP, processor, editor, frontend, params, assets, installers, and CI. Use Unravel as the working reference.

### 6.0 Scaffold Script (Recommended Starting Point)

Use the shared scaffold script to generate a complete plugin baseline:

```bash
node shared/scripts/scaffold-plugin.js {name} --type {effect|synth} --plugin-code {Xxxx}
```

- `--type` defaults to `effect`.
- `--plugin-code` must be a unique 4-char ASCII code (collision checks run against existing plugin CMake files).
- The script generates the directory tree from section 6.1, starter CMake, processor/editor/frontend stubs, and params scaffolding.
- The script does not update root `CMakeLists.txt` and does not run `npm install`.

In the checklist (section 6.12), steps 1-10 are mostly scaffolded and become verification/customization tasks.

### 6.1 Directory Structure

Create the following tree under `plugins/{name}/`:

```
plugins/{name}/
├── CMakeLists.txt
├── config/
│   └── params.json
├── assets/
│   └── app-icon.png
└── Source/
    ├── {Name}Tuning.h
    ├── {Name}GeneratedParams.h        (auto-generated)
    ├── DSP/
    │   ├── {Name}Engine.h
    │   └── {Name}Engine.cpp
    ├── Processors/
    │   ├── {Name}Processor.h
    │   └── {Name}Processor.cpp
    └── UI/
        ├── {Name}Editor.h
        ├── {Name}Editor.cpp
        └── frontend/
            ├── package.json
            ├── vite.config.js
            ├── index.html
            └── src/
                ├── main.js
                ├── viz.js               (plugin-specific visualization)
                └── generated/
                    └── params.js        (auto-generated)
```

### 6.2 Root CMake Registration

Add one line to the root `CMakeLists.txt`:

```cmake
# PLUGINS
add_subdirectory(plugins/unravel)
add_subdirectory(plugins/{name})
```

JUCE and `shared/core` are already available — every plugin shares them.

### 6.3 Plugin CMakeLists.txt

Follow the Unravel pattern (`plugins/unravel/CMakeLists.txt`). The key sections:

**Synth vs effect target flags**
- Effects: `IS_SYNTH FALSE`, `NEEDS_MIDI_INPUT FALSE`, stereo input + output.
- Synths: `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_MIDI_OUTPUT FALSE`, output-only audio bus.
- `PLUGIN_CODE` must be unique across the monorepo (4-char ASCII). Validate before adding a new plugin.

**Parameter generation** — wire `params.json` through the shared generator:

```cmake
set(PARAMS_JSON ${CMAKE_CURRENT_SOURCE_DIR}/config/params.json)
set(PARAMS_CPP_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Source/{Name}GeneratedParams.h)
set(PARAMS_JS_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Source/UI/frontend/src/generated/params.js)
set(PARAMS_GENERATOR ${CMAKE_SOURCE_DIR}/shared/scripts/generate_params.js)

find_program(NODE_EXECUTABLE node HINTS /usr/local/bin /opt/homebrew/bin)
add_custom_command(
    OUTPUT ${PARAMS_CPP_OUTPUT} ${PARAMS_JS_OUTPUT}
    COMMAND ${NODE_EXECUTABLE} ${PARAMS_GENERATOR}
            ${PARAMS_JSON} ${PARAMS_CPP_OUTPUT} ${PARAMS_JS_OUTPUT}
    DEPENDS ${PARAMS_JSON} ${PARAMS_GENERATOR}
    COMMENT "Generating parameter definitions from params.json"
)
add_custom_target({Name}GenerateParams DEPENDS ${PARAMS_CPP_OUTPUT} ${PARAMS_JS_OUTPUT})
```

**DSP library** — static library, no JUCE UI dependencies:

```cmake
add_library({name}_dsp STATIC ${DSP_SOURCES})
target_link_libraries({name}_dsp PUBLIC juce::juce_dsp)
target_compile_features({name}_dsp PUBLIC cxx_std_20)
```

**Frontend resources** — embedded as binary data:

```cmake
file(GLOB_RECURSE UI_RESOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/Source/UI/frontend/dist/*")
juce_add_binary_data({Name}Resources NAMESPACE {Name}Resources SOURCES ${UI_RESOURCES})
```

**Plugin target** — register with JUCE:

```cmake
juce_add_plugin(Threadbare{Name}
    COMPANY_NAME "threadbare"
    PLUGIN_MANUFACTURER_CODE TrbR          # Same for all Threadbare plugins
    PLUGIN_CODE {Xxxx}                     # Unique 4-char code
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "{name}"                  # Lowercase
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/assets/app-icon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/assets/app-icon.png"
)

target_link_libraries(Threadbare{Name}
    PRIVATE
        {name}_dsp
        {Name}Resources
        threadbare_core                    # ProcessorBase, WebViewBridge, StateQueue
        juce::juce_audio_utils
        juce::juce_gui_extra
)
```

**Artefact output** — required for installer scripts to find built plugins:

```cmake
if(DEFINED THREADBARE_ARTEFACTS_OUT)
    # Write JSON with vst3 and au artefact paths
    # (copy the block from plugins/unravel/CMakeLists.txt lines 125-143)
endif()
```

### 6.4 Parameter Definition (params.json)

The single source of truth for all parameters. `shared/scripts/generate_params.js` produces both a C++ header and a JS module from this file.

**Schema:**

```json
{
  "plugin": "{name}",
  "parameters": [
    {
      "id": "paramId",
      "name": "Display Name",
      "type": "float",
      "min": 0.0,
      "max": 1.0,
      "default": 0.5,
      "skewCentre": 0.25,
      "unit": "ms"
    },
    {
      "id": "toggle",
      "name": "Toggle",
      "type": "bool",
      "default": false
    }
  ]
}
```

**Constraints:**
- `id` must be a valid camelCase identifier (used as C++ constant and JS key).
- `type` is `"float"` or `"bool"`.
- `skewCentre` is optional — sets the midpoint for logarithmic controls (e.g., decay).
- `unit` is optional — `"s"`, `"ms"`, `"dB"`, or omit for dimensionless.

**Generated C++ header** (`{Name}GeneratedParams.h`):
- Namespace: `threadbare::{name}`
- `createParameterLayout()` — returns `juce::AudioProcessorValueTreeState::ParameterLayout`.
- `IDs` struct — string constants (e.g., `IDs::PARAM_ID`).
- `Meta` struct — `k`-prefixed range constants (e.g., `Meta::kPARAM_ID_MIN`, `kPARAM_ID_MAX`, `kPARAM_ID_DEFAULT`).

**Generated JS module** (`generated/params.js`):
- `PARAMS` — object mapping IDs to `{ name, type, min, max, default, ... }`.
- `PARAM_IDS` — array of IDs in definition order.
- `getParam(id)` — lookup helper.

### 6.5 DSP Layer (Source/DSP/)

Pure C++ signal processing. No JUCE UI headers.

**Rules:**
- Pull constants from `{Name}Tuning.h` (namespace `threadbare::tuning`). Never hardcode magic numbers.
- Accept `std::span<float>` buffers. Avoid passing `juce::AudioBuffer` into inner DSP classes.
- Follow the Iron Laws (section 3.4): no allocations, no locks, no exceptions, `ScopedNoDenormals`, `SmoothedValue`, cubic interpolation, per-sample updates.
- The DSP library links only against `juce::juce_dsp` — no `juce_gui_*` dependencies.

**Tuning header pattern** (`{Name}Tuning.h`):

```cpp
#pragma once
namespace threadbare::tuning {
namespace {Name} {

    // Section: Core
    inline constexpr float kSomeParam = 0.5f;   // What it controls [safe: 0.1-0.9]

} // namespace {Name}
} // namespace threadbare::tuning
```

Group constants by section. Each constant gets a comment describing what it controls in ear-language and what happens if you push it, plus a safe range.

### 6.6 Processor Layer (Source/Processors/)

Inherits from `threadbare::core::ProcessorBase`, which provides APVTS ownership, state persistence, and default audio bus configuration.

**Required implementation:**

```cpp
class {Name}Processor final : public threadbare::core::ProcessorBase
{
public:
    {Name}Processor()
        : ProcessorBase(
              BusesProperties()
                  .withInput("Input", juce::AudioChannelSet::stereo(), true)
                  .withOutput("Output", juce::AudioChannelSet::stereo(), true),
              createParameterLayout())
    {
        initialiseFactoryPresets();
    }

    // --- Required overrides ---
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override { return "{name}"; }

    // --- Parameter layout (uses generated header) ---
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // --- Audio→UI state ---
    bool popVisualState({Name}State& out) { return stateQueue.pop(out); }

private:
    threadbare::core::StateQueue<{Name}State> stateQueue;
    // ... DSP engine, cached parameter pointers, presets
};
```

**State struct** — must be trivially copyable (no strings, no pointers, no containers):

```cpp
struct {Name}State
{
    float paramA = 0.0f;
    float paramB = 0.0f;
    float inLevel = 0.0f;
    float tailLevel = 0.0f;
    // ... all fields the UI needs for visualization and display
};
```

**processBlock pattern:**
1. `juce::ScopedNoDenormals noDenormals;`
2. Read parameters from cached `std::atomic<float>*` pointers.
3. Run DSP.
4. Build state struct, push to `stateQueue`.

**State persistence hooks** (optional, provided by ProcessorBase):
- `onSaveState(juce::ValueTree&)` — save custom state (e.g., current preset index).
- `onRestoreState(juce::ValueTree&)` — restore custom state.
- `onStateRestored()` — post-restore actions (e.g., push state to UI).

**Presets:**
- Store as a vector of structs (param values + name).
- Implement `getNumPrograms()`, `getCurrentProgram()`, `setCurrentProgram()`, `getProgramName()`.
- Preset names follow brand guide section 2.4: single lowercase words, evocative.

**Preset struct + apply pattern (Unravel reference):**

```cpp
struct Preset {
    juce::String name;
    std::map<juce::String, float> parameters;
};

void applyPreset(const Preset& preset)
{
    for (const auto& [id, value] : preset.parameters)
    {
        if (auto* parameter = apvts.getParameter(id))
        {
            const float normalised = parameter->convertTo0to1(value);
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(normalised);
            parameter->endChangeGesture();
        }
    }
}
```

Persist preset index using `onSaveState` / `onRestoreState` hooks when presets are part of UX state.

**Synth-specific `ProcessorBase` overrides**
- `acceptsMidi() const override { return true; }`
- Override `isBusesLayoutSupported()` to accept output-only mono/stereo layouts.
- Keep constructor buses and overrides aligned; mismatched settings lead to host layout rejection.

**State struct guidance**
- Include all UI-visible parameter values.
- Include metering fields (`inLevel`, `tailLevel`, etc.).
- Include feature flags (transport/looper/voice count) needed by visualization and controls.
- Keep the struct flat and trivially copyable (no strings, pointers, dynamic containers).

### 6.7 Editor Layer (Source/UI/)

The editor owns the `WebBrowserComponent` and bridges C++ state to the JS frontend.

**Pattern:**

```cpp
class {Name}Editor final : public juce::AudioProcessorEditor
{
public:
    {Name}Editor({Name}Processor& p);
    void resized() override { webView.setBounds(getLocalBounds()); }

private:
    {Name}Processor& processorRef;
    juce::WebBrowserComponent webView;
    juce::VBlankAttachment vblank;

    // Resource provider: serves embedded frontend files
    auto getResource(const juce::String& url)
        -> std::optional<juce::WebBrowserComponent::Resource>;

    // Native function map: JS → C++ bridge
    auto createNativeFunctions()
        -> std::map<juce::String, std::function</*...*/>>;

    // Called at screen refresh rate by VBlankAttachment
    void handleUpdate();
};
```

**Constructor wiring:**

```cpp
{Name}Editor::{Name}Editor({Name}Processor& p)
    : AudioProcessorEditor(p),
      processorRef(p),
      webView(makeBrowserOptions()),
      vblank(this, [this] { handleUpdate(); })
{
    setSize(kEditorWidth, kEditorHeight);
    addAndMakeVisible(webView);
    webView.goToURL(WebViewBridge::getInitialURL());
}
```

**WebViewBridge integration:**

```cpp
auto {Name}Editor::makeBrowserOptions()
{
    return WebViewBridge::createOptions(
        createNativeFunctions(),
        [this](const auto& url) { return getResource(url); }
    );
}
```

`WebViewBridge::createOptions()` handles:
- Windows WebView2 backend with safe `userDataFolder`.
- Native function registration via `withNativeFunction()`.
- Resource provider callback for serving embedded files.

**Resource provider lookup pattern (BinaryData)**
- Clean incoming URL (`cleanURLPath`) and strip namespace prefixes.
- Try standard mangled names (`.`/`-`/`/` -> `_`).
- Try alphanumeric mangling (`toResourceName`), `dist_`-prefixed variants, and filename-only fallbacks.
- If needed, fallback by comparing original filenames from binary data metadata.
- Log misses with `DBG()` for fast diagnosis.

**handleUpdate** — called at screen refresh rate via `VBlankAttachment`:

```cpp
void {Name}Editor::handleUpdate()
{
    {Name}State state;
    if (!processorRef.popVisualState(state)) return;

    auto json = juce::DynamicObject::Ptr(new juce::DynamicObject());
    json->setProperty("paramA", state.paramA);
    json->setProperty("inLevel", state.inLevel);
    // ... all state fields

    webView.emitEventIfBrowserIsVisible(
        "updateState", juce::JSON::toString(json.get()));
}
```

**Native functions** — the JS→C++ bridge. At minimum:

| Function | Purpose |
|---|---|
| `setParameter(id, value)` | Set any APVTS parameter from UI |
| `getPresetList()` | Return preset names array |
| `loadPreset(index)` | Load a preset by index |

Add plugin-specific functions as needed (e.g., looper triggers).

**WebView debugging**
- Standalone builds: right-click in the WebView area to access inspector/tools (platform-dependent).
- macOS: `defaults write com.threadbare.{name} WebKitDeveloperExtras -bool true` may be required.
- Windows (WebView2): set `WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS=--remote-debugging-port=9222`.
- Use resource-provider `DBG()` logging when frontend files fail to resolve.

### 6.8 Frontend Setup

Each plugin has its own Vite project under `Source/UI/frontend/`.

**package.json** (minimal):

```json
{
  "private": true,
  "scripts": {
    "dev": "vite",
    "build": "vite build"
  },
  "devDependencies": {
    "vite": "^7.0.0",
    "vite-plugin-singlefile": "^2.0.0"
  }
}
```

**vite.config.js:**

```javascript
import { defineConfig } from 'vite'
import { viteSingleFile } from 'vite-plugin-singlefile'
import path from 'path'

export default defineConfig({
  plugins: [viteSingleFile()],
  resolve: {
    alias: {
      '@threadbare/shell': path.resolve(__dirname, '../../../../../shared/ui/shell/src'),
      '@threadbare/bridge': path.resolve(__dirname, '../../../../../shared/ui/bridge')
    }
  },
  build: {
    outDir: 'dist',
    minify: false,
    sourcemap: false
  }
})
```

The aliases `@threadbare/shell` and `@threadbare/bridge` resolve to shared frontend modules. The `viteSingleFile` plugin bundles everything into a single `index.html` that JUCE embeds as binary data.

**main.js** (entry point):

```javascript
import { initShell } from '@threadbare/shell/index.js'
import { createNativeFunctionBridge, createParamSender } from '@threadbare/bridge/juce-bridge.js'
import { {Name}Viz } from './viz.js'
import { PARAMS, PARAM_IDS } from './generated/params.js'

const THEME = {
  bg: '#......',
  text: '#......',
  accent: '#......',
  'accent-hover': '#......',
}

const getNativeFunction = createNativeFunctionBridge()
const sendParam = createParamSender(getNativeFunction)

const shell = initShell({
  VizClass: {Name}Viz,
  params: PARAMS,
  paramOrder: PARAM_IDS,
  themeTokens: THEME,
  getNativeFn: getNativeFunction,
  sendParam: sendParam,
})
```

**Shared UI shell** (`shared/ui/shell/src/`) provides:
- `initShell()` — main initializer, wires everything together.
- `Controls` — puck, settings overlay, elastic sliders.
- `Presets` — dropdown preset manager.
- `ElasticSlider` — spring-physics slider with rubber-band feel.
- `shell.css` — all shared styles, theme tokens, animations, reduced-motion support.

**Plugin-specific visualization** (`viz.js`):
- Each plugin provides its own `VizClass` (Unravel has the Lissajous orb).
- The class receives state updates from the shell and renders on a canvas.
- This is what gives each plugin its visual identity.

**Theme tokens:**
- Four CSS custom properties: `--bg`, `--text`, `--accent`, `--accent-hover`.
- Set via `themeTokens` in `initShell()`, applied to `:root`.
- Pick a unique accent color per plugin (see brand guide section 5.2).

**Build the frontend** before building the C++ plugin:

```bash
cd plugins/{name}/Source/UI/frontend
npm install && npm run build
```

The built `dist/index.html` is picked up by `juce_add_binary_data` in CMake.

### 6.9 Assets

| Asset | Location | Requirements |
|---|---|---|
| App icon | `plugins/{name}/assets/app-icon.png` | Works at 16x16 through 512x512. Monochromatic or limited palette. |
| macOS installer background | `installer/macos/resources/background.png` | Branded installer background. |
| Windows wizard image | `installer/windows/assets/wizard-image.png` | 240x459 px minimum. |
| Windows wizard small | `installer/windows/assets/wizard-small.png` | 55x58 px. |

### 6.10 Installer Setup

Installers currently live in a shared `installer/` directory with Unravel-specific content. For a second plugin, duplicate and adapt the plugin-specific files.

**Product metadata strategy**
- Current state (pre-migration): installer metadata source is `installer/product.json`.
- Future state (post-migration): per-plugin metadata is `plugins/{name}/installer/product.json`.
- During migration, scripts should support `--plugin-config` to make metadata source explicit.

**Product metadata** (`installer/product.json`):

```json
{
  "productName": "{Name}",
  "productId": "{name}",
  "companyName": "threadbare",
  "version": "0.1.0",
  "formats": ["VST3", "AU"],
  "bundleIds": {
    "vst3": "com.threadbare.{name}.vst3",
    "au": "com.threadbare.{name}.component"
  },
  "supportData": {
    "macos": "/Library/Application Support/{Name}",
    "windows": "%PUBLIC%\\Documents\\{Name}"
  }
}
```

**macOS installer** — adapt from `installer/macos/`:
- `Distribution.xml` — update bundle IDs and package references.
- `scripts/preinstall` — update plugin bundle names to remove.
- `scripts/postinstall` — update support directory name.
- `resources/welcome.html`, `conclusion.html` — update product name and copy.
- `resources/background.png` — plugin-specific branding.

**Windows installer** — adapt from `installer/windows/`:
- `Installer.iss` — update `AppName`, `AppVersion`, `OutputBaseFilename`, source paths, and bundle names.
- `assets/` — plugin-specific wizard images.
- `azure-metadata.json` — update certificate profile name if using separate signing identity.

**Build scripts** — `scripts/build-installer.sh` and `scripts/build-installer.ps1` currently hardcode `product_name="unravel"`. For a second plugin, either:
- Parameterize: accept a plugin name argument and read metadata from `installer/product.json`.
- Duplicate: create plugin-specific build scripts (simpler short-term, harder to maintain).

### 6.11 CI Pipeline

The GitHub Actions workflow (`.github/workflows/installer-build.yml`) currently hardcodes Unravel paths. For multi-plugin support:

**Frontend build directory:**
- Currently hardcoded to `plugins/unravel/Source/UI/frontend`.
- Parameterize with a workflow input or matrix strategy.

**Artifact names:**
- Currently `unravel-macos-installer` and `unravel-windows-installer`.
- Include plugin name in artifact names.

**Windows signing:**
- Certificate profile name is plugin-specific (`unravel-code-signing`).
- Each plugin may need its own profile or share a company-wide certificate.

**Required secrets** (shared across all plugins):
- macOS: `APPLE_DEVELOPER_ID_APP`, `APPLE_DEVELOPER_ID_INSTALLER`, `APPLE_ID`, `APPLE_APP_SPECIFIC_PASSWORD`, `APPLE_TEAM_ID`, `APPLE_CERTIFICATE_P12`, `APPLE_CERTIFICATE_PASSWORD`.
- Windows: `AZURE_TENANT_ID`, `AZURE_CLIENT_ID`, `AZURE_CLIENT_SECRET`.

**Recommended approach:** Add a `plugin` input to the workflow dispatch and use it to parameterize all paths. Alternatively, use a matrix strategy to build all plugins in one run.

### 6.12 Checklist

Summary of everything needed to go from zero to a building, installable new plugin:

Note: when using `shared/scripts/scaffold-plugin.js`, steps 1-10 are generated boilerplate and should be treated as review/customization items rather than greenfield implementation.

- [ ] Create `plugins/{name}/` directory tree (section 6.1)
- [ ] Write `config/params.json` with all parameters (section 6.4)
- [ ] Write `{Name}Tuning.h` with DSP constants (section 6.5)
- [ ] Implement DSP engine in `Source/DSP/` (section 6.5)
- [ ] Implement processor in `Source/Processors/` inheriting `ProcessorBase` (section 6.6)
- [ ] Define trivially-copyable state struct for audio→UI communication (section 6.6)
- [ ] Implement editor in `Source/UI/` with `WebViewBridge` and `VBlankAttachment` (section 6.7)
- [ ] Set up frontend with Vite, shell integration, theme tokens, and visualization (section 6.8)
- [ ] Build frontend (`npm install && npm run build`) (section 6.8)
- [ ] Write plugin `CMakeLists.txt` and register in root CMake (sections 6.2, 6.3)
- [ ] Create `app-icon.png` (section 6.9)
- [ ] Add `add_subdirectory(plugins/{name})` to root `CMakeLists.txt` (section 6.2)
- [ ] Verify local build: `cmake -B build && cmake --build build`
- [ ] Verify unique `PLUGIN_CODE` (4-char ASCII, no collisions) before first build
- [ ] Create installer metadata and scripts (section 6.10)
- [ ] Update CI workflow for new plugin (section 6.11)
- [ ] Define emotional targets and accent color (brand guide section 8)
- [ ] Name the presets (brand guide section 2.4)

---

## 7. Documentation Index

**Global (all plugins):**

| Document | Purpose |
|---|---|
| `docs/threadbare-prd.md` | This file. Product requirements, architecture, and standards. |
| `docs/threadbare-brand-guide.md` | Visual identity, voice, and design language. |
| `docs/brand_strategy.md` | Strategic brand framework — purpose, positioning, messaging hierarchy. |
| `docs/build_guide.md` | Build instructions and CI setup. |
| `docs/webview_integration_guide.md` | JUCE 8 WebView setup and troubleshooting. |
| `docs/installer_ux_completion_map.md` | Installer UX implementation status. |
| `docs/Plugin Installer UX Research.md` | Installer UX research and best practices. |
| `.cursorrules` | AI coding rules for real-time safety and project conventions. |

**Unravel-specific:**

| Document | Purpose |
|---|---|
| `docs/unravel_spec.md` | Detailed Unravel product spec (v1.1). |
| `docs/unravel_tuning_cheatsheet.md` | Quick reference for `UnravelTuning.h` constants. |
| `docs/ghost_engine_feature_map.md` | Ghost engine control-to-feature matrix. |
| `docs/ghost_engine_memory_metaphor.md` | Design philosophy behind the ghost engine. |
| `docs/glitch_sparkle_plan.md` | Glitch Sparkle design plan. |
| `docs/unravel_landing_page_copy.md` | Marketing copy for landing page and store listing. |
| `plugins/unravel/config/params.json` | Parameter definitions (source of truth). |
| `plugins/unravel/Source/UnravelTuning.h` | All DSP tuning constants. |

**Waver-specific:**

| Document | Purpose |
|---|---|
| `docs/waver_spec.md` | Detailed Waver product spec (v6.0). |
| `docs/waver_landing_page_copy.md` | Marketing copy for landing page and store listing. |
| `plugins/waver/config/params.json` | Parameter definitions (source of truth). |
| `plugins/waver/Source/WaverTuning.h` | DSP tuning constants. |

---

*This document consolidates product requirements, architecture, and standards for the Threadbare product line. It is the primary reference for new projects, onboarding, and AI-assisted development. Keep it current as the product evolves.*
