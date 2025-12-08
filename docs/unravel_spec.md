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
* At any time, up to 8 grains active (configurable via `kMaxGrains`):
    * Length ~50–300 ms (tunable range).
    * Hann window.
    * Start positions influenced by puck X (see 3.6.3).
    * Detuned ± small amount (cents), plus occasional octave-up shimmer grains at low level.
* Sum grains and feed into FDN input.
* Ghost slider + puck Y control overall ghost gain (within safe dB bounds).

#### 3.6.1 Reverse Memory Playback
**Concept:** At high ghost amounts, grains can play backwards through the history buffer, evoking the sensation of memories replaying in reverse.

**Implementation:**
1. **Spawn Decision:**
    * When spawning a grain, generate random float `r ∈ [0, 1]`.
    * If `r < kReverseProbability × ghostAmount²`, set grain to reverse mode.
    * Squaring ghost amount means reverse grains become prominent only at `ghost > 0.7`.

2. **Playback Direction:**
    * Normal grains: `speed > 0` (forward through history buffer).
    * Reverse grains: `speed < 0` (backward through history buffer).
    * Magnitude of speed still controlled by detune/shimmer logic.

3. **Buffer Position Management:**
    * Normal grain: starts at random position, advances toward write head.
    * Reverse grain: starts at random position, retreats toward older samples.
    * Both wrap/clamp within history buffer bounds.

4. **Interpolation:**
    * Use existing Catmull-Rom/cubic interpolator.
    * Works identically in reverse; just negate the position increment.

**Tuning Constants (add to `UnravelTuning.h::Ghost`):**
```cpp
// Probability of reverse grains at ghost=1.0
// At ghost=0.5, probability = 0.25 × 0.25 = 6.25%
// At ghost=1.0, probability = 25%
static constexpr float kReverseProbability = 0.25f;

// Gain reduction for reverse grains (to reduce muddiness on percussive sources)
// 0.75 = -2.5dB, helps reverse grains sit "behind" forward grains
static constexpr float kReverseGainReduction = 0.75f;
```

#### 3.6.2 Spectral Freezing of Grains
**Concept:** When Freeze is active, the ghost engine "locks" onto specific moments in the history buffer and replays them indefinitely with variation, rather than continuing to record new material.

**Implementation:**
1. **Freeze Transition (OFF → ON):**
    * When freeze button pressed:
        * **Snapshot current history buffer** positions for all active grains.
        * Mark these positions as "frozen spawn points" (store in fixed array, max 8 positions).
        * If fewer than 4 grains active, add random positions from recent 500ms of buffer.
    * Stop advancing `ghostWriteHead` (or continue writing but ignore for grain spawning).

2. **Grain Spawning While Frozen:**
    * New grains spawn **only from the frozen spawn points** (not random positions).
    * Each new grain:
        * Picks one of the frozen positions (round-robin or random).
        * Applies **varied** detune/speed (still use existing randomization).
        * Applies **varied** grain length (still use existing randomization).
        * Applies **varied** stereo pan position.
    * Result: same source material, but constantly re-examined with different "lenses."

3. **Freeze Transition (ON → OFF):**
    * Resume normal random position spawning.
    * Clear frozen spawn points array.
    * Resume advancing `ghostWriteHead`.

**Data Structure (add to `UnravelReverb` class):**
```cpp
// Spectral freeze state
bool ghostFreezeActive = false;
std::array<float, 8> frozenSpawnPositions;
std::size_t numFrozenPositions = 0;
```

**Selection Strategy (Revised - Weighted Random):**
* Use **weighted random selection** instead of round-robin to avoid audible patterns.
* Each frozen position has equal probability (uniform distribution).
* This creates more organic, less predictable repetition.

**Freeze Shimmer Enhancement:**
* When frozen, increase shimmer probability to create more variation from limited source material:
    ```cpp
    float shimmerProb = ghostFreezeActive ? kFreezeShimmerProbability : kShimmerProbability;
    // 40% vs 25% - helps boring source material stay interesting when frozen
    ```

**Integration with Existing Freeze:**
* Freeze button controls **both**:
    * FDN feedback → near 1.0 (existing behavior).
    * Ghost engine spawn positions → frozen mode (new behavior).
* Both systems freeze simultaneously for unified effect.

**Tuning Constants (add to `UnravelTuning.h::Ghost`):**
```cpp
// Shimmer probability when frozen (higher for more variation)
static constexpr float kFreezeShimmerProbability = 0.40f; // vs 0.25f normally
```

#### 3.6.3 Memory Proximity Modulation (Puck X Mapping)
**Concept:** The puck's X-axis not only controls Body vs Air in the reverb, but also controls the **temporal depth** of ghost memories—whether grains replay recent or distant moments.

**Implementation (Revised - Continuous Range):**
1. **Spawn Position Calculation:**
    * Use a **continuous range** from recent to distant (no discrete zones).
    * Map `puckX ∈ [-1, 1]` to maximum lookback time:
        * `puckX = -1` (Body): Max lookback = 150ms (very recent).
        * `puckX = 0` (Center): Max lookback = 400ms (medium).
        * `puckX = +1` (Air): Max lookback = 750ms (distant).

2. **Spawning Logic:**
    * When spawning a grain (in non-frozen mode):
        ```cpp
        // Calculate max lookback based on puckX
        // distantBias: 0.0 at left, 1.0 at right
        float distantBias = (1.0f + puckX) * 0.5f;
        
        // Interpolate between min and max lookback
        float maxLookbackMs = kMinLookbackMs + (distantBias * (kMaxLookbackMs - kMinLookbackMs));
        
        // Random position within [0, maxLookback]
        float spawnPosMs = random.nextFloat() * maxLookbackMs;
        
        // Convert ms to sample offset from write head
        float sampleOffset = (spawnPosMs * sampleRate) / 1000.0f;
        spawnPos = ghostWriteHead - sampleOffset;
        
        // Wrap within buffer bounds
        while (spawnPos < 0.0f) spawnPos += historyLength;
        while (spawnPos >= historyLength) spawnPos -= historyLength;
        ```

3. **Benefits of Continuous Range:**
    * More predictable behavior at center puck position.
    * Smooth transitions across the X-axis.
    * No "gap" between zones.
    * Simpler code, easier to tune.

4. **Interaction with Existing Puck X Behavior:**
    * **Maintain existing** ER gain / FDN input scaling.
    * **Add** ghost memory proximity on top.
    * Both behaviors run in parallel—they're complementary:
        * Body (left): Strong presence (high ER) + recent memories.
        * Air (right): Diffuse wash (low ER) + distant memories.

**Tuning Constants (add to `UnravelTuning.h::Ghost`):**
```cpp
// Memory proximity continuous range (ms relative to write head)
// Minimum lookback at puckX=-1 (body/recent)
static constexpr float kMinLookbackMs = 150.0f;
// Maximum lookback at puckX=+1 (air/distant)
static constexpr float kMaxLookbackMs = 750.0f;
```

#### 3.6.4 Enhanced Stereo Positioning
**Current State:** Grains already have a `pan` field (0.0 = hard L, 0.5 = center, 1.0 = hard R).

**Enhancements:**
1. **Dynamic Pan Assignment:**
    * Each grain gets randomized pan position at spawn:
        ```cpp
        grain.pan = 0.2f + random.nextFloat() * 0.6f; // Bias toward center, avoid hard panning
        ```
    * At higher ghost amounts, widen the pan range:
        ```cpp
        float panWidth = 0.3f + (ghostAmount * 0.7f); // 0.3–1.0 range
        grain.pan = 0.5f + (random.nextFloat() - 0.5f) * panWidth;
        grain.pan = std::clamp(grain.pan, 0.0f, 1.0f);
        ```

2. **Correlation with Reverse Grains:**
    * Reverse grains could favor **opposite stereo field** from forward grains:
        ```cpp
        if (isReverse) {
            grain.pan = 1.0f - grain.pan; // Mirror across center
        }
        ```
    * Creates spatial separation between forward/reverse memories.

3. **Proximity-Based Panning:**
    * Recent memories (puck left) → narrower stereo image (more mono/focused).
    * Distant memories (puck right) → wider stereo image (more spacious).
    * Implementation:
        ```cpp
        float stereoWidth = 0.3f + distantBias * 0.7f;
        grain.pan = 0.5f + (random.nextFloat() - 0.5f) * stereoWidth;
        ```

4. **Output Amplitude Calculation:**
    * Use **constant-power panning** for smooth energy distribution:
        ```cpp
        float panAngle = grain.pan * (M_PI / 2.0f);
        float gainL = std::cos(panAngle) * grainSample;
        float gainR = std::sin(panAngle) * grainSample;
        outL += gainL;
        outR += gainR;
        ```

**Tuning Constants (add to `UnravelTuning.h::Ghost`):**
```cpp
// Stereo pan width at ghost=0 (narrower = more focused)
static constexpr float kMinPanWidth = 0.3f;
// Stereo pan width at ghost=1 (wider = more spacious)
static constexpr float kMaxPanWidth = 1.0f;

// Whether to mirror reverse grains in stereo field
static constexpr bool kMirrorReverseGrains = true;
```

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
```

---

## 5. Factory Presets

Each preset is designed to showcase specific features while hitting the emotional targets of **nostalgic, bittersweet, weightless, soft**. Names evoke memory and emotion, not technical specs.

### 5.1 Preset List

#### 1. **unravel** [INIT/DEFAULT]
*"Great ambient guitar pad right out of the box."*
- **Puck:** X=0.0, Y=0.2
- **Decay:** 3.2s
- **Pre-delay:** 25ms
- **Size:** 1.1
- **Tone:** -0.2 (slightly warm)
- **Drift:** 0.35
- **Ghost:** 0.4
- **Duck:** 0.0
- **Blend:** 45%
- **Output:** 0dB
- **Use case:** Balanced starting point for guitars, synths, vocals. Gentle ghost presence, smooth decay.

---

#### 2. **fading polaroid**
*Showcases Body (left puck) + strong early reflections for intimate presence.*
- **Puck:** X=-0.7, Y=0.0
- **Decay:** 2.8s
- **Pre-delay:** 15ms
- **Size:** 0.8
- **Tone:** -0.3 (darker, warmer)
- **Drift:** 0.2
- **Ghost:** 0.25
- **Duck:** 0.3
- **Blend:** 40%
- **Output:** 0dB
- **Use case:** Fingerpicked guitar, intimate vocals. Strong body, short tail, stays close. Ducking keeps it articulate.

---

#### 3. **dissolving into mist**
*Showcases Air (right puck) + diffuse wash for weightless ambience.*
- **Puck:** X=+0.8, Y=0.6
- **Decay:** 8.5s
- **Pre-delay:** 60ms
- **Size:** 1.6
- **Tone:** +0.1 (slightly bright)
- **Drift:** 0.5
- **Ghost:** 0.6 (distant memories)
- **Duck:** 0.0
- **Blend:** 55%
- **Output:** -1dB
- **Use case:** Pads, synth washes, ambient guitars. Minimal body, maximum diffusion. Ghost pulls from distant memories.

---

#### 4. **rewind the moment**
*Heavy Ghost + recent memory (left puck X) for granular, tape-like smear.*
- **Puck:** X=-0.5, Y=0.7
- **Decay:** 5.0s
- **Pre-delay:** 10ms
- **Size:** 1.2
- **Tone:** -0.4 (dark, lo-fi)
- **Drift:** 0.6
- **Ghost:** 0.85 (high = more reverse grains)
- **Duck:** 0.0
- **Blend:** 50%
- **Output:** 0dB
- **Use case:** Showcases Ghost engine at high levels with reverse memory playback. Grains replay recent moments (puck left). Great for textural guitar loops.

---

#### 5. **echoes you can't quite place**
*Heavy Ghost + distant memory (right puck X) for ethereal, detuned fragments.*
- **Puck:** X=+0.6, Y=0.8
- **Decay:** 7.0s
- **Pre-delay:** 40ms
- **Size:** 1.5
- **Tone:** +0.2 (airy)
- **Drift:** 0.7
- **Ghost:** 0.9 (max ghost, pulls from 750ms back)
- **Duck:** 0.0
- **Blend:** 60%
- **Output:** -2dB
- **Use case:** Maximum ghost character pulling from distant memory buffer. Shimmer grains likely. Perfect for "what is that sound?" moments on synths/vocals.

---

#### 6. **held breath**
*Optimized for Freeze mode—long decay, high feedback, ready to lock.*
- **Puck:** X=0.0, Y=0.5
- **Decay:** 12.0s
- **Pre-delay:** 20ms
- **Size:** 1.4
- **Tone:** -0.1 (neutral-dark)
- **Drift:** 0.4
- **Ghost:** 0.5
- **Duck:** 0.0
- **Blend:** 50%
- **Output:** 0dB
- **Use case:** Hit Freeze to lock the tail into infinite sustain. Ghost engine will replay frozen spawn points with variation. Perfect for creating instant pads underneath guitars.

---

#### 7. **sunfaded memory**
*Soft, nostalgic, bittersweet. Maximum emotional target alignment.*
- **Puck:** X=-0.2, Y=0.3
- **Decay:** 4.5s
- **Pre-delay:** 30ms
- **Size:** 1.0
- **Tone:** -0.5 (warm, rolled-off highs)
- **Drift:** 0.3
- **Ghost:** 0.35
- **Duck:** 0.2
- **Blend:** 42%
- **Output:** 0dB
- **Use case:** Indie vocals, clean guitars. Soft, warm, never harsh. Feels like a memory fading at the edges.

---

#### 8. **shimmer and sway**
*Longer tail + higher drift for evolving, modulated spaces.*
- **Puck:** X=+0.3, Y=0.7
- **Decay:** 9.0s
- **Pre-delay:** 45ms
- **Size:** 1.7
- **Tone:** +0.3 (bright, shimmery)
- **Drift:** 0.8 (high modulation)
- **Ghost:** 0.7
- **Duck:** 0.0
- **Blend:** 58%
- **Output:** -1dB
- **Use case:** Synth pads, ambient guitar swells. Emphasizes modulation and shimmer grains. Tail constantly shifts and breathes.

---

#### 9. **between the notes**
*Short, tight, heavily ducked—reverb only in the gaps.*
- **Puck:** X=-0.4, Y=-0.3
- **Decay:** 1.8s
- **Pre-delay:** 5ms
- **Size:** 0.7
- **Tone:** -0.2
- **Drift:** 0.15
- **Ghost:** 0.2
- **Duck:** 0.75 (heavy ducking)
- **Blend:** 38%
- **Output:** +1dB
- **Use case:** Strummed guitars, percussion, rhythmic synths. Reverb blooms between notes but stays out of the way during playing.

---

#### 10. **weightless ascent**
*Maximum Y (distant) + long decay for hovering, infinite spaces.*
- **Puck:** X=0.1, Y=+0.9
- **Decay:** 15.0s (×3 from puck Y)
- **Pre-delay:** 80ms
- **Size:** 1.8
- **Tone:** 0.0 (neutral)
- **Drift:** 0.6
- **Ghost:** 0.8
- **Duck:** 0.0
- **Blend:** 65%
- **Output:** -2dB
- **Use case:** Ambient washes, drones, "lost in space" textures. Puck at top = maximum decay multiplier, maximum drift/ghost bonus. Feels like floating.

---

#### 11. **twilight reversal**
*Showcases reverse grain feature—memories played backwards.*
- **Puck:** X=0.0, Y=0.75
- **Decay:** 6.0s
- **Pre-delay:** 35ms
- **Size:** 1.3
- **Tone:** -0.3
- **Drift:** 0.5
- **Ghost:** 0.9 (high ghost = higher reverse probability)
- **Duck:** 0.0
- **Blend:** 52%
- **Output:** 0dB
- **Use case:** At ghost > 0.7, reverse grains become prominent. Showcases the "rewinding memory" effect. Surreal on vocals and sustained synths.

---

#### 12. **whispering distance**
*Vocal-optimized: clear, soft, never muddy.*
- **Puck:** X=-0.3, Y=0.2
- **Decay:** 3.0s
- **Pre-delay:** 40ms
- **Size:** 1.0
- **Tone:** -0.1
- **Drift:** 0.25
- **Ghost:** 0.3
- **Duck:** 0.4
- **Blend:** 35%
- **Output:** 0dB
- **Use case:** Indie/ambient vocals. Pre-delay separates dry from wet. Slight body bias, gentle ghost, ducking prevents wash-out. Stays intimate but spacious.

---

### 5.2 Preset Design Philosophy

**Naming Convention:**
- Evoke **emotion, sensation, or visual imagery** (not technical descriptions).
- Fit the "memory/nostalgia/dissolving" aesthetic.
- Avoid gear references or technical jargon.

**Coverage Goals:**
- ✅ Body vs Air extremes (#2, #3)
- ✅ Near vs Distant extremes (#9, #10)
- ✅ Ghost engine showcase (recent: #4, distant: #5, reverse: #11)
- ✅ Freeze-ready (#6)
- ✅ Ducking showcase (#2, #9, #12)
- ✅ Drift/modulation showcase (#8)
- ✅ Source-specific (guitar: #2, vocals: #12, synths: #3, #8)
- ✅ Emotional targets (#7 hits all four)

**Implementation Notes:**
- Store presets as JSON or XML in plugin state.
- Preset dropdown in UI (section 2.1) loads these on selection.
- Init state = "Fading Polaroid" (#1)