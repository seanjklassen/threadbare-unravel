# Threadbare — Product Requirements Document

*Living reference for all Threadbare products. Version 1.0 — February 2026.*

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

### 2.2 Future Products

The monorepo (`threadbare-unravel`) supports multiple plugins via `add_subdirectory`. Future products (e.g. Weave) follow the same architecture, shared core, and brand guidelines documented here.

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
│   └── unravel/            # Each plugin is self-contained
│       ├── config/         # params.json (parameter source of truth)
│       ├── assets/         # Icons, branding
│       └── Source/
│           ├── DSP/        # Pure C++ signal processing
│           ├── Processors/ # JUCE AudioProcessor integration
│           └── UI/         # WebView editor + frontend/
├── shared/
│   ├── core/               # ProcessorBase, WebViewBridge, StateQueue
│   ├── scripts/            # generate_params.js
│   └── ui/shell/           # Shared UI components (future)
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
6. **Interpolation.** Cubic (Hermite or Catmull-Rom) for delay line reads. Linear interpolation is forbidden for FDN delay lines.
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

---

## 4. Product: Unravel

### 4.1 Concept

Unravel turns audio into a slowly dissolving, modulated cloud with memories of what you played a moment ago. It is not a room simulator. It is a memory simulator running at audio rate.

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

---

## 5. Quality Standards

### 5.1 Audio Quality

- No audible zipper noise on parameter changes.
- No clicks or pops on preset changes, looper transitions, or state changes.
- No metallic ringing or icy artifacts in reverb tails.
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

### 5.4 Testing Checklist (Per Release)

- [ ] UI loads in Standalone, VST3 (Reaper), AU (Logic).
- [ ] All parameters update when moving controls.
- [ ] Presets load and switch correctly.
- [ ] State persists across DAW save/load.
- [ ] Looper records, loops, degrades, and exits cleanly.
- [ ] No clicks/pops at any parameter setting.
- [ ] CPU stable under sustained max-setting use.
- [ ] Windows build works with WebView2.
- [ ] Installer runs clean on fresh macOS and Windows systems.

---

## 6. Documentation Index

| Document | Purpose |
|---|---|
| `docs/threadbare-prd.md` | This file. Comprehensive product requirements and reference. |
| `docs/threadbare-brand-guide.md` | Visual identity, voice, and design language. |
| `docs/unravel_spec.md` | Detailed Unravel product spec (v1.1). |
| `docs/unravel_tuning_cheatsheet.md` | Quick reference for `UnravelTuning.h` constants. |
| `docs/build_guide.md` | Build instructions and CI setup. |
| `docs/webview_integration_guide.md` | JUCE 8 WebView setup and troubleshooting. |
| `docs/ghost_engine_feature_map.md` | Ghost engine control-to-feature matrix. |
| `docs/ghost_engine_memory_metaphor.md` | Design philosophy behind the ghost engine. |
| `docs/glitch_sparkle_plan.md` | Glitch Sparkle design plan. |
| `docs/unravel_landing_page_copy.md` | Marketing copy for landing page and store listing. |
| `docs/installer_ux_completion_map.md` | Installer UX implementation status. |
| `docs/Plugin Installer UX Research.md` | Installer UX research and best practices. |
| `.cursorrules` | AI coding rules for real-time safety and project conventions. |
| `plugins/unravel/config/params.json` | Parameter definitions (source of truth). |
| `plugins/unravel/Source/UnravelTuning.h` | All DSP tuning constants. |

---

*This document consolidates product requirements, architecture, and standards for the Threadbare product line. It is the primary reference for new projects, onboarding, and AI-assisted development. Keep it current as the product evolves.*
