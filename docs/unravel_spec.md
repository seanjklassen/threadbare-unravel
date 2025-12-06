# Threadbare – Unravel v1.1 Master Spec

## 0. Snapshot

* **Plugin:** Threadbare – Unravel
* **Role:** Lush, characterful “memory cloud” reverb for guitars, synths, and vocals.
* **Core DSP:**
    * 8×8 FDN (Feedback Delay Network) with damping + modulation.
    * Ghost engine: tiny granular smear feeding the FDN.
    * Early reflections, Freeze, Ducking.
* **Control Model:**
    * Single puck in a 2D macro space (Body ↔ Air, Near ↔ Distant).
    * Simple advanced controls in a drawer.
* **Tech Stack:**
    * DSP: C++ with JUCE 8.
    * UI: WebView (HTML/CSS/JS canvas; upgradeable to WebGL via PixiJS or similar).
    * Formats: VST3 + AU. No AAX in v1.

---

## 1. Concept, Emotional Targets & Scope

### 1.1 Concept
Unravel turns audio into a slowly dissolving, modulated cloud that remembers what you played a moment ago.

* Not a “do-everything” room reverb.
* It’s for:
    * Shoegaze / dream pop / ambient guitars.
    * Pads and synths.
    * Indie/ambient vocals.

### 1.2 Emotional Targets
Everything should lean toward:

* **Nostalgic** – like a memory, not a studio.
* **Bittersweet** – tension between comfort and ache.
* **Weightless** – hovering around the source, not glued to walls.
* **Soft** – no icepick highs or metallic zing in normal use.

*Use this as the north star when tuning constants and presets.*

### 1.3 Sound Goals
* Subjectively comparable to a good mode in Valhalla VintageVerb / TAL Reverb 4 in its lane.
* Strong on:
    * Smooth, non-metallic tails.
    * Frequency-dependent decay.
    * Gentle but persistent modulation.
    * Integrated ghost “memory” character.
    * Freeze + ducking that feel instantly usable.

---

## 2. UX / UI Spec

### 2.1 Layout
**Target inner UI size:**
* ~380–420 px wide
* ~670–720 px tall

**Structure:**
1.  **Preset bar**
    * Left: preset dropdown.
    * Right: small logo glyph.
2.  **Main canvas**
    * Evolving lissajous orb.
    * Puck 48x48.
3.  **Bottom strip**
    * Left: two compact readouts with a range of 0.00-11.00 (e.g. Decay + Size).
    * Right: settings icon → opens advanced drawer.
    * Right: freeze icon to the left of the sliders icon → activates 2.3.3 Freeze mode.

### 2.2 Look & Feel
* **Background:** `#26332A`
* **Text and icons:** `#EFDFBD`
* **Unravel accent:** `#AFD3E4` (puck center, sliders, active states, Freeze on background).
* **Lissajous Orb:** `#E1A6A6` line with soft anti-aliasing; faint “ghost” trail when Ghost > 0.

### 2.3 Controls

#### 2.3.1 Puck (Main Macro Control)
* **Output:**
    * `puckX` ∈ [-1, 1]
    * `puckY` ∈ [-1, 1]
* **Semantics:**
    * `puckX`: **Body ↔ Air**
        * Left = more body (stronger ER, more presence).
        * Right = more air (less body, more diffuse).
    * `puckY`: **Near ↔ Distant**
        * Down = nearer / shorter / subtler.
        * Up = distant / longer / ghostier / more drift.
* **Interaction:**
    * Click-drag to move.
    * Constrain to square region.
    * Double-click → reset to center (0,0).

#### 2.3.2 Blend & Output
* **Blend/Mix:** 0–100% wet.
* **Output Gain:** e.g. −24 dB to +12 dB.

#### 2.3.3 Freeze
* Small button near sliders icon (∞).
* **When ON:**
    * Tail “locks” and holds.
    * Dry still passes normally.

#### 2.3.4 Advanced Drawer
**Contains:**
1.  Decay (0.4–20 s).
2.  Pre-delay (0–150 ms).
3.  Size (0.5–2.0).
4.  Tone (Dark ↔ Bright, −1..1).
5.  Drift (0–1).
6.  Ghost Mix (0–1).
7.  Duck (0–1).
8.  Blend
9.  Output

**Defaults:** “great ambient guitar pad” right out of the box.

**Drawer Visual Language:**
* Each parameter uses a single horizontal slider with no numeric readout. The track shows two layers:
    * A neutral base layer that reflects the stored parameter value.
    * A secondary gold layer that indicates the puck-applied macro offset (if any) so users can see the effective value at a glance.
* A small “pupil” indicator to the right of each slider lights up (soft green) whenever the puck is contributing a macro value to that parameter.
* Sliders must remain keyboard accessible; optionally show a transient tooltip while dragging for precise values. Provide a taller invisible hit area (e.g. 3–4× the visible track height) so the control is easy to grab with mouse or touch.

### 2.4 Lissajous Orb Behavior
Orb is visual only. It reflects state.

**Inputs from C++ to JS:**
* `inLevel` (dry level).
* `tailLevel` (wet level).
* `puckX`, `puckY`.
* `decay`, `size`, `ghost`, `drift`.
* `freeze` (bool).
* `time` (monotonic).

**Mapping:**
* Orb radius grows with `puckY` and `tailLevel`.
* Tangle / wobble grows with `drift` & `ghost`.
* Stroke thickness & opacity follow `tailLevel`.

**Implementation:**
* ~40 points around a center, drawn as a polyline on canvas.
* Optionally use PixiJS/WebGL if raw 2D canvas stutters on Retina.

---

## 3. DSP: Structure & Behavior
*This section is the logic; the actual numbers live in `UnravelTuning.h`.*

### 3.1 Signal Flow (True Stereo)
**Per block:**
1.  Read stereo input (L/R).
2.  Apply pre-delay.
3.  ER block.
4.  Write into:
    * FDN input (with ER content).
    * Ghost history buffer.
5.  Ghost engine reads from history buffer, generates grains, feeds into FDN input.
6.  FDN tail:
    * 8 delay lines + feedback matrix.
    * Per-line damping & modulation.
7.  Freeze logic:
    * Controls feedback gains & FDN input gain.
8.  Ducking:
    * Envelope follower on dry; modulates wet gain.
9.  Mix Dry/Wet.
10. Output gain.

### 3.2 FDN Core
* 8 delay lines.
* Delay times = base delays (in ms) × size scalar.
* Use a fixed, orthonormal 8×8 matrix (Hadamard/Householder-style).
* True stereo via input/output vectors:
    * Each line gets a mix of L/R.
    * Each output gets a mix of all 8 lines.

### 3.3 Decay & Damping
* Decay slider (with puck Y offset) defines a target RT60.
* **Per delay line:**
    * Compute feedback gains from RT60 formula.
    * Run through damping filters:
        * LPF (Tone mapped to cutoff).
        * HPF to prevent LF bloat.
        * Additional gentle tilt EQ on the wet output for extra Tone shaping.

### 3.4 Early Reflections
* Small ER multi-tap block before FDN.
* Tap times 5–60 ms.
* Per-tap gains decreasing.
* Slight stereo asymmetry.
* **Body/Air macro (puck X) controls:**
    * ER gain.
    * FDN input contribution (slightly counter-scaled).

### 3.5 Modulation (“Drift”)
* Each FDN line has a slow sine LFO:
    * Rate: 0.05–0.4 Hz (random per line).
    * Depth in samples scaled by Drift & puck Y (few samples max).
    * Modulate delay read position (fractional delay with interpolation).
* This breaks up static modes → smoother tails.

### 3.6 Ghost Engine
* Maintain a history buffer (~750 ms) after pre-delay.
* At any time, up to 2 grains active:
    * Length ~80–200 ms.
    * Hann window.
    * Start positions random within last 500 ms.
    * Detuned ± small amount (cents), plus occasional octave-up shimmer grains at low level.
* Sum grains and feed into FDN input.
* Ghost slider + puck Y control overall ghost gain (within safe dB bounds).

### 3.7 Freeze
* **When Freeze goes OFF → ON:**
    * Slowly ramp FDN feedback gains to near 1.0 (slightly <1).
    * Fade FDN input down to almost zero (ER + Ghost).
* **When ON:**
    * Tail effectively infinite.
    * No new content enters tank, dry still passes.
* **When ON → OFF:**
    * Fade feedback gains back to RT60-based values.
    * Fade FDN input back to normal.
* Ramps are short (tens of ms) and smoothed.

### 3.8 Ducking
* Envelope follower on mono-summed dry input.
* Fastish attack, slower release.
* Wet gain = `baseWet` × (1 − `duckAmount` × `env`).
* Optionally never go below a minimum wet factor so it doesn’t fully vanish.

### 3.9 Puck → Parameter Mapping (Conceptual)
* **Y (Near/Distant):**
    * Multiplies Decay (roughly /3 at bottom, ×3 at top).
    * Adds to Drift.
    * Adds to Ghost.
* **X (Body/Air):**
    * Scales ER gain & FDN input ratio (body vs wash).
* *In code, the exact multipliers are constants in `UnravelTuning::PuckMapping`.*

### 3.10 Smoothing & Denormals
* **Per-Sample Parameter Updates:** The `Size` and `Delay` parameters must be processed using **Audio-Rate Smoothing**. Do NOT update delay times once per block. Iterate through the buffer sample-by-sample and update the delay line read pointers for every sample. This ensures "tape-style" smooth warping without zipper noise.
* **Interpolation:** Use **Cubic (Hermite) Interpolation** for all FDN delay lines. Linear interpolation is forbidden for the delay lines as it causes volume drops and dulling during modulation.
* **Anti-denormal strategy:**
    * `ScopedNoDenormals`.
    * Tiny noise added to samples to keep CPU stable in long tails.

---

## 4. Tuning Abstraction: UnravelTuning.h

This is the designer-facing “control panel” for all the underlying numbers.

### 4.1 Purpose
* All “magic numbers” live in one place.
* DSP code pulls from `UnravelTuning`, never hardcodes.
* Each constant has:
    * A safe range.
    * A short comment describing:
        * What it controls in ear-language.
        * What happens if you push it.
* You get to tweak this file without understanding the math.

### 4.2 Structure
**File:** `UnravelTuning.h`

```cpp
#pragma once

namespace UnravelTuning {

struct Fdn {
    // Number of delay lines in the FDN.
    // Evidence: 8–16 lines is common in high-quality algorithmic reverbs;
    // 8 is a sweet spot for musical use vs CPU.
    static constexpr int kNumLines = 8;

    // Base delay times in ms for Size = 1.0.
    // Incommensurate / prime-ish delays help avoid obvious metallic ringing.
    static constexpr float kBaseDelaysMs[kNumLines] = {
        31.0f, 37.0f, 41.0f, 53.0f, 61.0f, 71.0f, 83.0f, 97.0f
    };

    // Allowed range for Size scalar (tight ↔ huge).
    static constexpr float kSizeMin = 0.5f;
    static constexpr float kSizeMax = 2.0f;
};

struct Decay {
    // Global T60 bounds (seconds).
    // 0.4s keeps short settings usable; 20s covers ambient washes.
    static constexpr float kT60Min = 0.4f;
    static constexpr float kT60Max = 20.0f;

    // Puck Y decay multiplier (~ /3 to *3).
    static constexpr float kPuckYMultiplierMin = 1.0f / 3.0f;
    static constexpr float kPuckYMultiplierMax = 3.0f;
};

struct Damping {
    // Tone control → low-pass cutoff (Hz).
    // These set the darkest, neutral, and brightest edges.
    static constexpr float kLowCutoffHz  = 1500.0f;
    static constexpr float kMidCutoffHz  = 8000.0f;
    static constexpr float kHighCutoffHz = 16000.0f;

    // High-pass in loop to avoid boomy reverb.
    static constexpr float kLoopHighPassHz = 100.0f;
};

struct Modulation {
    // LFO frequency range for delay modulation (Hz).
    // Slow rates keep it "alive" without obvious chorus wobble.
    static constexpr float kMinRateHz = 0.05f;
    static constexpr float kMaxRateHz = 0.4f;

    // Max modulation depth in samples at drift=1, puckY=1.
    // Safe range ~2–8; higher = wobblier.
    static constexpr float kMaxDepthSamples = 8.0f;
};

struct Ghost {
    // How long the ghost remembers (seconds).
    static constexpr float kHistorySeconds = 0.75f;

    // Grain durations (seconds). Shorter = more granular; longer = smeary.
    static constexpr float kGrainMinSec = 0.08f;
    static constexpr float kGrainMaxSec = 0.20f;

    // Subtle detune range (in semitones) for most grains.
    static constexpr float kDetuneSemi = 0.2f; // ~20 cents

    // Rare shimmer grains at +12 semitones.
    static constexpr float kShimmerSemi = 12.0f;
    static constexpr float kShimmerProbability = 0.05f; // 5%

    // Ghost gain bounds relative to FDN input (dB).
    static constexpr float kMinGainDb = -30.0f; // subtle
    static constexpr float kMaxGainDb = -12.0f; // strong but not dominating
};

struct Freeze {
    // Feedback gain while frozen (slightly <1.0 to avoid blow-up).
    static constexpr float kFrozenFeedback = 0.999f;

    // Ramp time when entering/exiting freeze (seconds).
    static constexpr float kRampTimeSec = 0.05f;
};

struct Ducking {
    // Envelope times (seconds).
    static constexpr float kAttackSec  = 0.01f; // 10 ms: snappy enough
    static constexpr float kReleaseSec = 0.25f; // 250 ms: natural decay

    // Minimum wet proportion at full duck (0..1).
    // Keeps some ambience even when duck knob is maxed.
    static constexpr float kMinWetFactor = 0.15f;
};

struct PuckMapping {
    // Y influence on decay multiplier. 3.0 means ~ /3 to *3 across pad.
    static constexpr float kDecayYFactor = 3.0f;

    // Extra ghost added at top of pad (0..1).
    static constexpr float kGhostYBonus  = 0.3f;

    // Extra drift added at top of pad (0..1).
    static constexpr float kDriftYBonus  = 0.25f;
};

struct Safety {
    // Tiny noise to avoid denormal CPU spikes in FDN feedback.
    static constexpr float kAntiDenormal = 1.0e-18f;
};

} // namespace UnravelTuning