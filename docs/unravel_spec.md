# Threadbare – Unravel v1.1 Master Spec

## 0. Snapshot

* **Plugin:** Unravel by Threadbare
* **Role:** Lush, characterful “memory cloud” reverb for guitars, synths, and vocals.
* **Core DSP:**
    * 8×8 FDN (Feedback Delay Network) with damping + modulation.
    * Ghost engine: tiny granular smear feeding the FDN.
    * Early reflections, Disintegration Looper, Ducking.
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
Unravel turns audio into a slowly dissolving, modulated cloud with memories of what you played a moment ago.

* Not a “do-everything” room reverb.
* It’s for:
    * Shoegaze / dream pop / ambient guitars.
    * Pads and synths.
    * Indie/ambient vocals.

### 1.2 Emotional Targets
Everything should lean toward:

* **Spectral**
Reverb should feel like a fading memory, not a physical room. It is transparent and "weightless," hovering around the source rather than being anchored by early reflections or walls.

* **Wistful**
A permanent tension between comfort and ache. The decay should have a bittersweet "bloom" that feels emotionally evocative and haunting, rather than just a technical tail.

* **Diffuse**
The sound is soft and vaporous. There is no "icepick" high-end or metallic zing; reflections are so dense they become a uniform, cloud-like wash that evaporates gently.

*Use this as the north star when tuning constants and presets.*

### 1.3 Sound Goals
* Subjectively comparable to a good mode in Valhalla VintageVerb / TAL Reverb 4 in its lane.
* Strong on:
    * Smooth, non-metallic tails.
    * Frequency-dependent decay.
    * Gentle but persistent modulation.
    * Integrated ghost “memory” character.
    * Disintegration looper + ducking that feel instantly usable.

---

## 2. UX / UI Spec

### 2.1 Layout
**Target inner UI size:**
* ~380–420 px wide
* ~670–720 px tall

**Structure:**
1.  **Preset bar**
    * Left: preset dropdown.
2.  **Main canvas**
    * Evolving lissajous orb.
    * Puck 48x48.
3.  **Bottom strip**
    * Left: x,y compact readouts with a range of 0.00-11.00.
    * Right: settings icon → opens advanced drawer.
    * Right: looper icon to the left of the sliders icon → activates 2.3.3 Disintegration Looper.

### 2.2 Look & Feel
* **Background:** `#31312b` (warm gray)
* **Text and icons:** `#C8C7B8` (soft beige)
* **Unravel accent:** `#E0E993` (lime)
* **Puck pupil:** `#C5CC7A` (olive green)
* **Lissajous Orb:** `#E8B8A8` (dusty coral)

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

#### 2.3.3 Disintegration Looper
* Small button near sliders icon (∞).
* **Three-state operation:**
    1. **Idle:** Normal reverb operation. Button shows infinity symbol.
    2. **Recording:** Click to start. Captures dry+wet signal for N bars (default 4). Button pulses during recording.
    3. **Looping:** After recording completes, loop plays back with gradual degradation. Click again to stop and return to Idle.
* **Behavior:**
    * Loop plays back with entropy-based degradation (William Basinski-inspired tape decay).
    * Puck Y controls decay rate: top = fast disintegration (~2 loops), bottom = slow (~endless).
    * Puck X controls character: left (Ghost) = spectral thinning, right (Fog) = diffuse smearing.
    * As entropy increases: filters converge, saturation increases, dropouts occur, pitch wobbles.
    * When entropy reaches 1.0, loop fades gracefully back to normal reverb.

#### 2.3.4 Advanced Drawer
**Contains:**
1.  Decay (0.4–50 s).
2.  Distance (0–100 ms).
3.  Size (0.5–2.0).
4.  Tone (Dark ↔ Bright, −1..1).
5.  Drift (0–1).
6.  Ghost Mix (0–1).
7.  Duck (0–1).
8.  Blend
9.  Output

**Defaults:** “great ambient guitar pad” right out of the box.

**Drawer Visual Language:**
* Each parameter uses a single horizontal slider with no numeric readout. The track shows:
    * A neutral base layer that reflects the stored parameter value.
* A small “pupil” indicator to the right of each slider lights up (soft green) whenever the puck is contributing a macro value to that parameter.
* Sliders must remain keyboard accessible; optionally show a transient tooltip while dragging for precise values. Provide a taller invisible hit area (e.g. 3–4× the visible track height) so the control is easy to grab with mouse or touch.

### 2.4 Lissajous Orb Behavior
Orb is visual only. It reflects state.

**Inputs from C++ to JS:**
* `inLevel` (dry level).
* `tailLevel` (wet level).
* `puckX`, `puckY`.
* `decay`, `size`, `ghost`, `drift`.
* `looperState` (0=Idle, 1=Recording, 2=Looping).
* `loopProgress` (0–1, recording progress indicator).
* `entropy` (0–1, disintegration amount).
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
7.  Disintegration Looper:
    * When active, captures and loops processed signal with degradation effects.
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
    * Compute feedback gains from RT60 formula: `g = exp(-6.9 * delayTime / T60)`
    * Each delay line gets its own feedback gain based on its individual delay time for accurate, even decay across all lines.
    * Run through damping filters:
        * LPF (Tone mapped to cutoff).
        * HPF to prevent LF bloat.
        * Additional gentle tilt EQ on the wet output for extra Tone shaping.

### 3.4 Early Reflections
* Small ER multi-tap block before FDN.
* Tap times 5–60 ms.
* Per-tap gains decreasing.
* Slight stereo asymmetry.
* **Pre-delay:** Adjustable parameter (0–100 ms) that shifts the entire ER cluster later in time. This is a user-facing control separate from the main pre-delay.
* **Body/Air macro (puck X) controls:**
    * ER gain.
    * FDN input contribution (slightly counter-scaled).

### 3.5 Modulation (“Drift”)
* Each FDN line has a sine LFO:
    * Rate: 0.1–3.0 Hz (random per line, wider range for more obvious modulation).
    * Depth in samples scaled by Drift & puck Y, with PuckX macro override (20–80 samples range).
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

#### 3.6.2 Grain Position Locking (During Looper)
**Concept:** When the Disintegration Looper is active (Looping state), the ghost engine "locks" onto specific moments in the history buffer and replays them with variation. This creates the "fixation" aspect of the memory metaphor—the same moments replaying with subtle differences.

**Implementation:**
1. **Looper Transition (Recording → Looping):**
    * When looper enters Looping state:
        * **Snapshot current history buffer** positions for all active grains.
        * Mark these positions as "locked spawn points" (store in fixed array, max 8 positions).
        * If fewer than 4 grains active, add random positions from recent 500ms of buffer.

2. **Grain Spawning While Looping:**
    * New grains spawn **only from the locked spawn points** (not random positions).
    * Each new grain:
        * Picks one of the locked positions (weighted random selection).
        * Applies **varied** detune/speed (still use existing randomization).
        * Applies **varied** grain length (still use existing randomization).
        * Applies **varied** stereo pan position.
    * Result: same source material, but constantly re-examined with different "lenses."

3. **Looper Transition (Looping → Idle):**
    * Resume normal random position spawning.
    * Clear locked spawn points array.

**Data Structure (in `UnravelReverb` class):**
```cpp
// Grain position locking state (used during looper)
bool ghostFreezeActive = false;  // true when looper is in Looping state
std::array<float, 8> frozenSpawnPositions;
std::size_t numFrozenPositions = 0;
```

**Selection Strategy (Weighted Random):**
* Use **weighted random selection** instead of round-robin to avoid audible patterns.
* Each locked position has equal probability (uniform distribution).
* This creates more organic, less predictable repetition.

**Shimmer Enhancement During Loop:**
* When looping, increase shimmer probability to create more variation:
    ```cpp
    float shimmerProb = ghostFreezeActive ? kFreezeShimmerProbability : kShimmerProbability;
    // 40% vs 25% - helps source material stay interesting during degradation
    ```

**Integration with Disintegration Looper:**
* Looper button controls **both**:
    * Loop capture, playback, and degradation (Disintegration system).
    * Ghost engine spawn positions → locked mode (grain position locking).
* Both systems work together for the complete "fading memory" experience.

**Tuning Constants (in `UnravelTuning.h::Ghost`):**
```cpp
// Shimmer probability when looper active (higher for more variation)
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

### 3.7 Disintegration Looper
William Basinski-inspired loop degradation with "Ascension" filter model. The loop evaporates upward (HPF+LPF converge) with warm saturation.

#### 3.7.1 State Machine
* **Idle → Recording:** Button press starts capturing dry+wet mix into loop buffer. Records for N bars (synced to DAW tempo, default 4 bars).
* **Recording → Looping:** Recording completes, playback begins with crossfade. Entropy starts at 0.
* **Looping → Idle:** Button press fades loop out and returns to normal reverb.

#### 3.7.2 Degradation Effects (scaled by entropy 0→1)
* **Ascension Filter:** HPF sweeps 20→800Hz, LPF sweeps 20kHz→2kHz. Frequencies converge as entropy increases.
* **Saturation:** Warm tape saturation increases with entropy (0→0.6).
* **Oxide Shedding:** Stochastic dropouts (probability scales with entropy).
* **Motor Death:** Brownian pitch drag with downward bias (±40 cents max).
* **Azimuth Drift:** L/R channels degrade at slightly different rates.
* **Wow & Flutter:** Authentic tape transport wobble.
* **Noise Floor:** Pink noise fades in as loop degrades.

#### 3.7.3 Puck Mapping During Loop
* **Puck Y:** Controls entropy rate. Top = fast (~2 loops to full decay), Bottom = slow (~endless).
* **Puck X:** Controls degradation character:
    * Left (Ghost): Spectral thinning, emphasize highs (HPF boost ×4).
    * Right (Fog): Diffuse smearing, preserve mids (LPF reduction ×0.25).

#### 3.7.4 Tuning Constants
See `UnravelTuning.h::Disintegration` for all degradation parameters.

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
    * **Size Modulation (Doppler Effect):** Subtle pitch warp via size multiplier:
        * PuckY Down (-1.0): Size = 0.92x → subtle pitch up
        * PuckY Up (+1.0): Size = 1.08x → subtle pitch down
        * Adds "life" and movement without overwhelming the sound
* **X (Body/Air):**
    * Scales ER gain & FDN input ratio (body vs wash).
    * **Drift Depth Override:** PuckX macro overrides standard `kMaxDepthSamples` with a dynamic range:
        * Left (Physical/Stable): 20 samples depth
        * Right (Ethereal/Chaotic): 80 samples depth
        * This creates the "Stable → Seasick" macro behavior, with smoothed transitions to prevent clicks
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
    // 0.4s keeps short settings usable; 50s is near-infinite reverb.
    static constexpr float kT60Min = 0.4f;
    static constexpr float kT60Max = 50.0f;

    // Puck Y decay multiplier (~ /3 to *3).
    static constexpr float kPuckYMultiplierMin = 1.0f / 3.0f;
    static constexpr float kPuckYMultiplierMax = 3.0f;
};

struct Damping {
    // Tone control → low-pass cutoff (Hz).
    // Lower cutoff for more aggressive darkening (underwater effect).
    static constexpr float kLowCutoffHz  = 400.0f;
    static constexpr float kMidCutoffHz  = 8000.0f;
    static constexpr float kHighCutoffHz = 16000.0f;

    // High-pass in loop to avoid boomy reverb.
    static constexpr float kLoopHighPassHz = 100.0f;
};

struct Modulation {
    // LFO frequency range for delay modulation (Hz).
    // Wider range from slow to fast creates more obvious modulation.
    static constexpr float kMinRateHz = 0.1f;
    static constexpr float kMaxRateHz = 3.0f;

    // Max modulation depth in samples at drift=1, puckY=1.
    // 100 samples at 48kHz creates extreme tape warble/detune.
    static constexpr float kMaxDepthSamples = 100.0f;
};

struct Ghost {
    // How long the ghost remembers (seconds).
    // Extended to 1.2s for deeper memory recall while keeping lookback at 750ms.
    static constexpr float kHistorySeconds = 1.2f;

    // Grain durations (seconds). Wider range for more texture variety.
    static constexpr float kGrainMinSec = 0.05f;  // Shorter for more density
    static constexpr float kGrainMaxSec = 0.30f;  // Long for smooth texture

    // Subtle detune range (in semitones) for most grains.
    static constexpr float kDetuneSemi = 0.2f; // ~20 cents

    // Shimmer grains at +12 semitones (octave up).
    static constexpr float kShimmerSemi = 12.0f;
    static constexpr float kShimmerProbability = 0.25f; // 25% for obvious sparkle

    // Ghost gain bounds relative to FDN input (dB).
    static constexpr float kMinGainDb = -24.0f; // Louder minimum
    static constexpr float kMaxGainDb = -6.0f;  // Reduced from -3dB to prevent clipping
};

struct Freeze {
    // Multi-head loop buffer for smooth frozen pads
    static constexpr float kLoopBufferSeconds = 5.0f;
    static constexpr int kNumReadHeads = 6;
    static constexpr float kTransitionSeconds = 0.3f;
    
    // Per-head pitch modulation for subtle variation
    static constexpr float kHeadDetuneCents = 6.0f;
    static constexpr float kHeadModRateMin = 0.03f;
    static constexpr float kHeadModRateMax = 0.12f;
    
    // Feedback gain while frozen.
    static constexpr float kFrozenFeedback = 1.0f;

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

#### 2. **close**
*Intimate, close-mic'd feel with minimal space.*
- **Puck:** X=-0.9, Y=-0.4
- **Decay:** 1.1s
- **Pre-delay:** 8ms
- **Size:** 0.75
- **Tone:** -0.45 (dark, warm)
- **Drift:** 0.10
- **Ghost:** 0.05
- **Duck:** 0.15
- **Blend:** 28%
- **Output:** 0dB
- **Use case:** Tight, controlled ambience for vocals or acoustic instruments. Minimal ghost, stays very close.

---

#### 3. **tether**
*Body-heavy with ducking—reverb follows the playing.*
- **Puck:** X=-0.7, Y=0.0
- **Decay:** 2.4s
- **Pre-delay:** 18ms
- **Size:** 0.95
- **Tone:** -0.35 (warm)
- **Drift:** 0.20
- **Ghost:** 0.15
- **Duck:** 0.45 (moderate ducking)
- **Blend:** 35%
- **Output:** 0dB
- **Use case:** Fingerpicked guitar, intimate vocals. Strong body, ducking keeps it articulate and responsive.

---

#### 4. **pulse**
*Rhythmic ducking for strummed parts and synths.*
- **Puck:** X=0.25, Y=0.10
- **Decay:** 4.5s
- **Pre-delay:** 12ms
- **Size:** 1.15
- **Tone:** -0.15
- **Drift:** 0.35
- **Ghost:** 0.25
- **Duck:** 0.85 (heavy ducking)
- **Blend:** 55%
- **Output:** -1dB
- **Use case:** Strummed guitars, percussion, rhythmic synths. Reverb pumps with the input, blooms in gaps.

---

#### 5. **bloom**
*Slowly expanding wash with long decay and ghost presence.*
- **Puck:** X=0.40, Y=0.80
- **Decay:** 10.0s
- **Pre-delay:** 40ms
- **Size:** 1.7
- **Tone:** +0.05 (neutral-bright)
- **Drift:** 0.50
- **Ghost:** 0.55
- **Duck:** 0.0
- **Blend:** 60%
- **Output:** -2dB
- **Use case:** Pads, synth washes, ambient guitars. Long tail with ghost memory for evolving textures.

---

#### 6. **mist**
*Maximum Air—distant, diffuse, ethereal.*
- **Puck:** X=0.90, Y=0.60
- **Decay:** 14.0s
- **Pre-delay:** 70ms
- **Size:** 1.85
- **Tone:** -0.60 (dark, foggy)
- **Drift:** 0.60
- **Ghost:** 0.70 (distant memories)
- **Duck:** 0.0
- **Blend:** 65%
- **Output:** -3dB
- **Use case:** Ambient drones, pads. Maximum diffusion, ghost pulls from distant memory buffer. Enveloping fog.

---

#### 7. **rewind**
*Heavy Ghost with reverse grain character.*
- **Puck:** X=0.10, Y=0.75
- **Decay:** 6.0s
- **Pre-delay:** 20ms
- **Size:** 1.25
- **Tone:** -0.35 (warm, lo-fi)
- **Drift:** 0.55
- **Ghost:** 0.95 (max ghost = high reverse probability)
- **Duck:** 0.0
- **Blend:** 50%
- **Output:** -1dB
- **Use case:** Showcases Ghost engine at high levels with reverse memory playback. Surreal on vocals and sustained sounds.

---

#### 8. **halation**
*Bright, shimmery, lens-flare character.*
- **Puck:** X=1.0, Y=0.90
- **Decay:** 9.0s
- **Pre-delay:** 45ms
- **Size:** 1.9
- **Tone:** +0.60 (bright, shimmery)
- **Drift:** 0.45
- **Ghost:** 0.65
- **Duck:** 0.0
- **Blend:** 55%
- **Output:** -2dB
- **Use case:** Synth pads, ambient guitar swells. Emphasizes shimmer grains and bright tail. Feels like light overexposure.

---

#### 9. **stasis**
*Looper-optimized—long decay, max ghost, ready to capture.*
- **Puck:** X=0.0, Y=0.30
- **Decay:** 20.0s
- **Pre-delay:** 0ms
- **Size:** 1.5
- **Tone:** -0.40 (warm)
- **Drift:** 0.60
- **Ghost:** 1.0 (max ghost)
- **Duck:** 0.0
- **Blend:** 75%
- **Output:** -3dB
- **Use case:** Hit the looper to capture and gradually degrade. Perfect for creating evolving drone pads.

---

#### 10. **shiver**
*Extreme settings—maximum everything for otherworldly textures.*
- **Puck:** X=1.0, Y=1.0
- **Decay:** 30.0s
- **Pre-delay:** 0ms
- **Size:** 2.0 (max)
- **Tone:** +0.45 (bright)
- **Drift:** 0.80 (high modulation)
- **Ghost:** 1.0 (max ghost)
- **Duck:** 0.0
- **Blend:** 80%
- **Output:** -4dB
- **Use case:** Extreme ambient textures. Full distant, full ghost, max size. For when you want to dissolve completely.

---

### 5.2 Preset Design Philosophy

**Naming Convention:**
- Evoke **emotion, sensation, or visual imagery** (not technical descriptions).
- Fit the "memory/nostalgia/dissolving" aesthetic.
- Avoid gear references or technical jargon.

**Coverage Goals:**
- ✅ Body vs Air extremes (#2 close, #6 mist)
- ✅ Near vs Distant extremes (#2 close, #10 shiver)
- ✅ Ghost engine showcase (#7 rewind, #9 stasis)
- ✅ Looper-ready (#9 stasis)
- ✅ Ducking showcase (#3 tether, #4 pulse)
- ✅ Drift/modulation showcase (#8 halation, #10 shiver)
- ✅ Source-specific (guitar: #3, vocals: #2, synths: #5, #8)
- ✅ Emotional targets (#1 unravel hits the balanced sweet spot)

**Implementation Notes:**
- Presets defined in `UnravelProcessor::initialiseFactoryPresets()`.
- Preset dropdown in UI loads these on selection.
- Init state = "unravel" (#1)