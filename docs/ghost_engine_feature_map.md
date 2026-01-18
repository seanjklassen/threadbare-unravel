# Ghost Engine Feature Map

A visual reference showing how user controls map to the four enhanced ghost engine features.

---

## Control → Feature Matrix

```
┌─────────────────────────────────────────────────────────────────────┐
│                     USER CONTROLS                                    │
├─────────────────────────────────────────────────────────────────────┤
│                                                                       │
│  PUCK X (-1.0 to +1.0)          PUCK Y (-1.0 to +1.0)              │
│  ├─ Body/Air (existing)         ├─ Decay multiplier (existing)      │
│  ├─ ER gain (existing)          ├─ Drift bonus (existing)           │
│  ├─ Memory Proximity (NEW)      └─ Ghost bonus (existing)           │
│  └─ Stereo Width (NEW)                                               │
│                                                                       │
│  GHOST SLIDER (0.0 to 1.0)      LOOPER BUTTON (3-state)            │
│  ├─ Grain volume (existing)     ├─ Idle → Recording → Looping       │
│  ├─ Reverse probability (NEW)   ├─ Loop degradation (entropy)       │
│  └─ Stereo width (NEW)          └─ Grain position locking           │
│                                                                       │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Feature Dependencies

```
                    ┌──────────────────────┐
                    │   GRAIN SPAWNING     │
                    │   (every ~50-200ms)  │
                    └──────────┬───────────┘
                               │
                ┌──────────────┼──────────────┐
                │              │              │
                ▼              ▼              ▼
        ┌───────────┐  ┌──────────┐  ┌──────────────┐
        │  POSITION │  │ PLAYBACK │  │    STEREO    │
        │  SELECT   │  │ DIRECTION│  │   POSITION   │
        └─────┬─────┘  └────┬─────┘  └──────┬───────┘
              │             │                │
              │             │                │
    ┌─────────▼──────┐      │        ┌──────▼───────┐
    │ Looper active? │      │        │  Ghost amt   │
    ├────────────────┤      │        │  Puck X      │
    │ YES: Use locked│      │        │  Reverse?    │
    │      positions │      │        └──────────────┘
    │  NO: Calculate │      │
    │      proximity │      │
    └────────────────┘      │
                            │
                    ┌───────▼────────┐
                    │ Ghost amt > 0.5│
                    ├────────────────┤
                    │ Random < prob? │
                    │ YES: Reverse   │
                    │ NO:  Forward   │
                    └────────────────┘
```

---

## Parameter Influence Chart

### Reverse Playback Probability

```
Ghost    Actual Reverse
Amount   Probability
─────────────────────
  0.0       0.0%
  0.3       2.3%
  0.5       6.3%
  0.7      12.3%
  0.8      16.0%
  0.9      20.3%
  1.0      25.0%
```

Formula: `prob = 0.25 × ghost²` (only when ghost > 0.5)

---

### Memory Proximity (Spawn Position)

```
Puck X    Recent Zone    Distant Zone
          (0-200ms)      (500-750ms)
──────────────────────────────────────
 -1.0       100%            0%        ◀─ Body
 -0.5        75%           25%
  0.0        50%           50%        ◀─ Center
 +0.5        25%           75%
 +1.0         0%          100%        ◀─ Air
```

Linear interpolation (can adjust with `kProximityBiasPower`)

---

### Stereo Width

```
                    Puck X = -1.0           Puck X = +1.0
                    (Body/Recent)           (Air/Distant)
                    
Ghost = 0.0         |-----|                 |-----|
                    30% width               30% width
                    
Ghost = 0.5         |----------|            |-------------|
                    30% width               65% width
                    
Ghost = 1.0         |-----------|           |-----------------|
                    30% width               100% width
```

Formula: `width = minWidth + (ghost × distantBias × widthRange)`
- `distantBias = (1 + puckX) / 2`
- `widthRange = maxWidth - minWidth = 0.7`

---

## Interaction Scenarios

### Scenario A: "Tight Body"
```
Settings:
  Puck X: -1.0 (far left)
  Puck Y:  0.0 (center)
  Ghost:   0.3 (low)
  Looper:  OFF

Effects:
  ✓ Recent memories (0-200ms)
  ✓ Narrow stereo (30% width)
  ✓ Rare reverse grains (2%)
  ✓ Strong ER (existing)
  
Sound: Tight, present doubling/echo
```

### Scenario B: "Distant Cloud"
```
Settings:
  Puck X: +1.0 (far right)
  Puck Y: +1.0 (top)
  Ghost:   1.0 (max)
  Looper:  OFF

Effects:
  ✓ Distant memories (500-750ms)
  ✓ Wide stereo (100% width)
  ✓ Frequent reverse grains (25%)
  ✓ Weak ER, strong diffuse (existing)
  
Sound: Ethereal, time-warped wash
```

### Scenario C: "Frozen Obsession"
```
Settings:
  Puck X:  0.0 (center)
  Puck Y: +0.5 (upper)
  Ghost:   0.7 (high)
  Looper:  ON

Effects:
  ✓ Locked spawn positions (4-8 points)
  ✓ Mixed stereo (65% width)
  ✓ Moderate reverse grains (12%)
  ✓ Infinite FDN (existing)
  
Sound: Meditative repetition with variation
```

### Scenario D: "Extreme Memory Flux"
```
Settings:
  Puck X: +0.7 (right-ish)
  Puck Y: +0.8 (near top)
  Ghost:   0.9 (very high)
  Drift:   0.8 (high)
  Looper:  OFF

Effects:
  ✓ Mostly distant memories (85%)
  ✓ Very wide stereo (90% width)
  ✓ Heavy reverse grains (20%)
  ✓ Heavy modulation (existing)
  
Sound: Chaotic, beautiful memory overload
```

---

## Feature Toggle Reference

For debugging/testing, these features can be independently disabled:

```cpp
// In UnravelTuning.h::Ghost

// Disable reverse playback:
static constexpr float kReverseProbability = 0.0f;

// Disable proximity (always use recent zone):
static constexpr float kDistantZoneStartMs = 0.0f;
static constexpr float kDistantZoneEndMs = 200.0f;

// Disable stereo width variation (always narrow):
static constexpr float kMinPanWidth = 0.3f;
static constexpr float kMaxPanWidth = 0.3f;

// Disable reverse mirroring:
static constexpr bool kMirrorReverseGrains = false;
```

For disabling grain locking, simply don't transition `ghostFreezeActive` when looper button pressed.

---

## Grain Lifecycle with All Features

```
1. SPAWN TRIGGER
   └─ Every N samples (based on ghost amount)

2. SPAWN DECISION
   ├─ Find inactive grain in pool
   └─ If none available, steal oldest

3. POSITION SELECTION
   ├─ Is looper active?
   │  ├─ YES: Use locked position (round-robin)
   │  └─ NO:  Calculate proximity position
   │         ├─ Get puck X bias
   │         ├─ Choose recent or distant zone
   │         └─ Random position within zone

4. DIRECTION SELECTION
   ├─ Is ghost > 0.5?
   │  ├─ YES: Roll for reverse (prob = 0.25 × ghost²)
   │  └─ NO:  Always forward
   └─ Set speed (positive or negative)

5. STEREO SELECTION
   ├─ Calculate pan width (ghost × distantBias)
   ├─ Random pan within width
   └─ If reverse: optionally mirror

6. OTHER PARAMETERS
   ├─ Grain length (random 50-300ms)
   ├─ Detune (random ±20 cents)
   ├─ Shimmer check (25% chance +12 semitones)
   └─ Amplitude envelope (Hann window)

7. PLAYBACK
   ├─ Read from history buffer
   ├─ Apply window
   ├─ Apply pan (constant-power)
   └─ Sum to output

8. TERMINATION
   └─ When window phase reaches 1.0
```

---

## CPU Cost Breakdown

```
Feature                  CPU Impact    Notes
─────────────────────────────────────────────────────
Reverse Playback           ~0.0%      Just sign flip
Grain Position Locking     ~0.0%      Array lookup vs random
Memory Proximity           ~0.0%      Same spawn cost
Stereo Width Calculation   ~0.0%      Simple math
Constant-Power Panning     ~0.0%      Sin/cos (might use LUT)
─────────────────────────────────────────────────────
TOTAL OVERHEAD             ~0.0%      Unmeasurable
```

All features essentially "free" in terms of CPU.

---

## Testing Checklist

```
□ Reverse Playback
  □ Spawns at correct probability
  □ Sounds smooth (no clicks)
  □ Works at all sample rates
  □ Probability scales with ghost²
  
□ Grain Position Locking (Looper)
  □ Captures positions on looper activation
  □ Uses locked positions while looping
  □ Returns to normal on deactivation
  □ No buffer overruns
  □ Round-robin selection works
  
□ Memory Proximity
  □ Left puck favors recent zone
  □ Right puck favors distant zone
  □ Center puck is even mix
  □ Zones are musically meaningful
  
□ Stereo Width
  □ Narrow at low ghost
  □ Wide at high ghost + right puck
  □ Constant-power law maintains loudness
  □ Reverse mirroring works (if enabled)
  
□ Combined
  □ All features work simultaneously
  □ No interaction bugs
  □ Musically useful results
  □ CPU stable under all conditions
```

---

## Quick Reference: Magic Numbers

```cpp
// Reverse
kReverseProbability = 0.25f;  // 25% at ghost=1.0

// Proximity
kRecentZoneMs = 200.0f;       // 0-200ms
kDistantZoneStartMs = 500.0f; // 500-750ms
kDistantZoneEndMs = 750.0f;

// Stereo
kMinPanWidth = 0.3f;          // Narrow
kMaxPanWidth = 1.0f;          // Full width
kMirrorReverseGrains = true;  // Flip reverse grains

// Looper (grain locking)
// (uses existing kNumLines = 8 for locked position array)
```

All values tunable at runtime by editing `UnravelTuning.h`.

---

## Architecture Summary

```
┌─────────────────────────────────────────────────┐
│           USER INTERFACE                        │
│  Puck XY, Ghost Slider, Looper Button          │
└────────────────┬────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────┐
│          UNRAVEL STATE                          │
│  puckX, puckY, ghost, looperState, etc.        │
└────────────────┬────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────┐
│      UNRAVEL REVERB (DSP Core)                 │
│                                                 │
│  ┌───────────────────────────────────────┐    │
│  │     GHOST ENGINE (Enhanced)            │    │
│  │                                         │    │
│  │  History Buffer (750ms)                │    │
│  │  Grain Pool (8 grains)                 │    │
│  │  Frozen Positions (8 slots)            │    │
│  │                                         │    │
│  │  Features:                              │    │
│  │  • Reverse Playback (NEW)              │    │
│  │  • Grain Position Locking (NEW)        │    │
│  │  • Memory Proximity (NEW)              │    │
│  │  • Stereo Positioning (NEW)            │    │
│  └───────────────────────────────────────┘    │
│                                                 │
│  ┌───────────────────────────────────────┐    │
│  │     FDN CORE (Existing)                │    │
│  │  8 delay lines + feedback matrix       │    │
│  └───────────────────────────────────────┘    │
│                                                 │
└────────────────┬────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────┐
│           AUDIO OUTPUT                          │
│  Stereo signal (L/R)                           │
└─────────────────────────────────────────────────┘
```

---

*This feature map provides a quick visual reference for understanding how the ghost engine enhancements integrate with the existing Unravel architecture.*

