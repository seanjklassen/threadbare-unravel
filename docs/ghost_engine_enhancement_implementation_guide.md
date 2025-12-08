# Ghost Engine Enhancement Implementation Guide

This document provides step-by-step implementation guidance for the three new ghost engine features:
1. **Reverse Memory Playback**
2. **Spectral Freezing of Grains**
3. **Memory Proximity Modulation (Puck X Mapping)**
4. **Enhanced Stereo Positioning**

All features maintain real-time safety (no allocations, no locks, no exceptions).

---

## Feature 1: Reverse Memory Playback

### 1.1 Modify Grain Spawn Logic

**Location:** `UnravelReverb.cpp::trySpawnGrain()`

**Changes:**
1. After determining grain length, detune, and other parameters, add reverse decision logic:

```cpp
// Determine if this grain should play in reverse
bool isReverse = false;
if (ghostAmount > 0.5f) { // Only consider reverse at moderate-to-high ghost
    float reverseProb = threadbare::tuning::Ghost::kReverseProbability 
                        * ghostAmount * ghostAmount; // Squared scaling
    if (ghostRng.nextFloat() < reverseProb) {
        isReverse = true;
    }
}

// Set grain speed (pitch shift)
float speedRatio = std::pow(2.0f, detuneSemitones / 12.0f);
if (isReverse) {
    speedRatio = -speedRatio; // Negative speed = backward playback
}
grain.speed = speedRatio;
```

### 1.2 Modify Grain Processing

**Location:** `UnravelReverb.cpp::processGhostEngine()`

**Changes:**
1. When advancing grain position, handle negative speed:

```cpp
// Advance grain playback position
grain.pos += grain.speed; // Works for both positive and negative speeds

// Wrap/clamp position within history buffer
int historyLength = static_cast<int>(ghostHistory.size());
while (grain.pos < 0.0f) {
    grain.pos += static_cast<float>(historyLength);
}
while (grain.pos >= static_cast<float>(historyLength)) {
    grain.pos -= static_cast<float>(historyLength);
}
```

2. The existing `readGhostHistory()` interpolator already handles any position, so no changes needed there.

### 1.3 Testing
- Set ghost to 0.5: should hear occasional backward grains (~6% chance).
- Set ghost to 1.0: should hear frequent backward grains (25% chance).
- Verify no audio glitches, pops, or CPU spikes.

---

## Feature 2: Spectral Freezing of Grains

### 2.1 Freeze State Transitions

**Location:** `UnravelReverb.cpp::process()` (main process loop)

**Changes:**
1. Detect freeze state transitions:

```cpp
// At start of process() method, after parameter smoothing
bool freezeTransition = (state.freeze != ghostFreezeActive);
bool freezeActivating = (!ghostFreezeActive && state.freeze);
bool freezeDeactivating = (ghostFreezeActive && !state.freeze);

if (freezeActivating) {
    // Snapshot current grain positions as frozen spawn points
    numFrozenPositions = 0;
    
    // Capture positions of currently active grains
    for (const auto& grain : grainPool) {
        if (grain.active && numFrozenPositions < frozenSpawnPositions.size()) {
            frozenSpawnPositions[numFrozenPositions++] = grain.pos;
        }
    }
    
    // If fewer than 4 positions captured, add random recent positions
    while (numFrozenPositions < 4 && numFrozenPositions < frozenSpawnPositions.size()) {
        float recentMs = ghostRng.nextFloat() * 500.0f; // Random within last 500ms
        float sampleOffset = (recentMs * sampleRate) / 1000.0f;
        frozenSpawnPositions[numFrozenPositions++] = 
            static_cast<float>(ghostWriteHead) - sampleOffset;
    }
    
    frozenSpawnIndex = 0;
    ghostFreezeActive = true;
}

if (freezeDeactivating) {
    // Clear frozen state
    numFrozenPositions = 0;
    ghostFreezeActive = false;
}
```

### 2.2 Modify Grain Spawn Position Selection

**Location:** `UnravelReverb.cpp::trySpawnGrain()`

**Changes:**
1. At the point where spawn position is determined, add conditional logic:

```cpp
float spawnPosition;

if (ghostFreezeActive && numFrozenPositions > 0) {
    // FROZEN MODE: Pick from frozen positions (round-robin)
    int historyLength = static_cast<int>(ghostHistory.size());
    spawnPosition = frozenSpawnPositions[frozenSpawnIndex];
    frozenSpawnIndex = (frozenSpawnIndex + 1) % numFrozenPositions;
    
    // Ensure position is valid within current buffer
    while (spawnPosition < 0.0f) {
        spawnPosition += static_cast<float>(historyLength);
    }
    while (spawnPosition >= static_cast<float>(historyLength)) {
        spawnPosition -= static_cast<float>(historyLength);
    }
} else {
    // NORMAL MODE: Use proximity-based random position (see Feature 3)
    spawnPosition = calculateProximityBasedPosition(state.puckX);
}

grain.pos = spawnPosition;
```

### 2.3 Maintain History Buffer During Freeze

**Important:** The history buffer should **continue writing** during freeze (don't stop `ghostWriteHead`). This allows smooth transition back to normal mode. The frozen positions are stored as absolute buffer indices and will naturally "age" as the buffer continues to wrap.

### 2.4 Testing
- Play audio, activate freeze: grains should lock onto specific moments.
- Each new grain spawned should replay one of the locked positions with different pitch/length.
- Deactivate freeze: should smoothly return to normal random spawning.
- Verify no audio artifacts at freeze transitions.

---

## Feature 3: Memory Proximity Modulation (Puck X Mapping)

### 3.1 Create Proximity Position Calculator

**Location:** `UnravelReverb.cpp` (new helper method)

**Add to class:**
```cpp
private:
    float calculateProximityBasedPosition(float puckX) noexcept;
```

**Implementation:**
```cpp
float UnravelReverb::calculateProximityBasedPosition(float puckX) noexcept
{
    using namespace threadbare::tuning::Ghost;
    
    // Calculate zone biases (0.0 to 1.0)
    // puckX = -1.0 (left/body) → recentBias=1.0, distantBias=0.0
    // puckX = +1.0 (right/air) → recentBias=0.0, distantBias=1.0
    float recentBias = (1.0f - puckX) * 0.5f;
    float distantBias = (1.0f + puckX) * 0.5f;
    
    // Apply power curve for more/less sensitivity
    recentBias = std::pow(recentBias, kProximityBiasPower);
    distantBias = std::pow(distantBias, kProximityBiasPower);
    
    // Re-normalize so they sum to 1.0
    float total = recentBias + distantBias;
    if (total > 0.0f) {
        recentBias /= total;
        distantBias /= total;
    }
    
    // Randomly choose zone weighted by bias
    float spawnPosMs;
    if (ghostRng.nextFloat() < recentBias) {
        // Spawn in recent zone (0-200ms ago)
        spawnPosMs = ghostRng.nextFloat() * kRecentZoneMs;
    } else {
        // Spawn in distant zone (500-750ms ago)
        float rangeMs = kDistantZoneEndMs - kDistantZoneStartMs;
        spawnPosMs = kDistantZoneStartMs + (ghostRng.nextFloat() * rangeMs);
    }
    
    // Convert milliseconds to sample offset from write head
    float sampleOffset = (spawnPosMs * sampleRate) / 1000.0f;
    float spawnPos = static_cast<float>(ghostWriteHead) - sampleOffset;
    
    // Wrap within buffer bounds
    int historyLength = static_cast<int>(ghostHistory.size());
    while (spawnPos < 0.0f) {
        spawnPos += static_cast<float>(historyLength);
    }
    while (spawnPos >= static_cast<float>(historyLength)) {
        spawnPos -= static_cast<float>(historyLength);
    }
    
    return spawnPos;
}
```

### 3.2 Integrate with Spawn Logic

**Location:** `UnravelReverb.cpp::trySpawnGrain()`

**Changes:**
1. Replace any existing random position generation with call to proximity calculator:

```cpp
// Use proximity-based position (unless frozen - see Feature 2)
if (!ghostFreezeActive) {
    grain.pos = calculateProximityBasedPosition(state.puckX);
}
```

### 3.3 Testing
- Move puck to far left (body): grains should mostly replay very recent audio (0-200ms ago).
- Move puck to far right (air): grains should mostly replay distant audio (500-750ms ago).
- Move puck to center: should get even mix of recent and distant.
- Listen for the qualitative difference: left=immediate/present, right=distant/memory-like.

---

## Feature 4: Enhanced Stereo Positioning

### 4.1 Dynamic Pan Width

**Location:** `UnravelReverb.cpp::trySpawnGrain()`

**Changes:**
1. When assigning pan position, make width dynamic based on ghost amount and puck X:

```cpp
using namespace threadbare::tuning::Ghost;

// Calculate pan width based on ghost amount and proximity
float distantBias = (1.0f + state.puckX) * 0.5f; // 0.0 at left, 1.0 at right
float panWidth = kMinPanWidth + (ghostAmount * distantBias * (kMaxPanWidth - kMinPanWidth));

// Randomize pan within calculated width
float panOffset = (ghostRng.nextFloat() - 0.5f) * panWidth; // -width/2 to +width/2
grain.pan = 0.5f + panOffset; // Center ± offset
grain.pan = std::clamp(grain.pan, 0.0f, 1.0f);
```

### 4.2 Mirror Reverse Grains (Optional)

**Location:** `UnravelReverb.cpp::trySpawnGrain()` (after pan assignment)

**Changes:**
1. If grain is playing in reverse, optionally mirror its pan position:

```cpp
// Mirror reverse grains in stereo field for spatial separation
if (isReverse && kMirrorReverseGrains) {
    grain.pan = 1.0f - grain.pan;
}
```

### 4.3 Constant-Power Panning Output

**Location:** `UnravelReverb.cpp::processGhostEngine()`

**Changes:**
1. When outputting grain samples, use constant-power panning law:

```cpp
// Apply constant-power panning (equal energy across stereo field)
float panAngle = grain.pan * (juce::MathConstants<float>::halfPi); // 0 to π/2
float gainL = std::cos(panAngle);
float gainR = std::sin(panAngle);

// Apply to sample and accumulate
outL += grainSample * gainL;
outR += grainSample * gainR;
```

### 4.4 Testing
- Set ghost to 0.0: stereo width should be narrow (more focused).
- Set ghost to 1.0 with puck right: stereo width should be very wide (spacious).
- Set ghost to 1.0 with puck left: stereo width should be moderate (wide-ish but not extreme).
- Enable reverse grains: check that forward and reverse grains occupy different stereo regions.
- Verify constant loudness as grains pan from L to R.

---

## Integration Checklist

### Phase 1: Reverse Playback (Lowest Risk)
- [ ] Add reverse probability calculation to `trySpawnGrain()`
- [ ] Support negative `grain.speed` in position advancement
- [ ] Add tuning constants to `UnravelTuning.h` ✓ (already done)
- [ ] Test at ghost=0.5 and ghost=1.0
- [ ] Verify no CPU spikes or audio glitches

### Phase 2: Stereo Enhancement (Low Risk)
- [ ] Implement dynamic pan width calculation
- [ ] Implement constant-power panning output
- [ ] Add pan/reverse mirroring (optional)
- [ ] Test panning behavior across ghost and puckX ranges
- [ ] Verify stereo image coherence

### Phase 3: Proximity Modulation (Medium Risk)
- [ ] Implement `calculateProximityBasedPosition()` helper
- [ ] Integrate with spawn logic
- [ ] Add tuning constants ✓ (already done)
- [ ] Test puck X behavior: left=recent, right=distant
- [ ] Tune zone ranges for musical results

### Phase 4: Spectral Freeze (Highest Risk)
- [ ] Add freeze state tracking variables ✓ (already done)
- [ ] Implement freeze transition logic
- [ ] Capture frozen spawn positions
- [ ] Modify spawn logic to use frozen positions
- [ ] Test freeze activation/deactivation smoothness
- [ ] Test interaction with existing FDN freeze
- [ ] Verify no buffer overruns or invalid positions

---

## Real-Time Safety Verification

For each feature, verify:
1. **No allocations:** All data structures pre-allocated in `prepare()`.
2. **No locks:** All state is single-threaded or uses atomics.
3. **No exceptions:** All math operations safe; all array accesses bounds-checked.
4. **Denormal handling:** All feedback paths use `ScopedNoDenormals`.

---

## Tuning Recommendations

After implementation, iterate on these values for best musicality:

### Reverse Probability
- Start with `kReverseProbability = 0.25f` (25% at ghost=1.0).
- If too subtle, increase to 0.35f.
- If too distracting, decrease to 0.15f.

### Memory Zones
- Start with Recent=200ms, Distant=500-750ms.
- If proximity effect is too subtle, widen gap (Recent=150ms, Distant=600-750ms).
- If too extreme, narrow gap (Recent=300ms, Distant=400-750ms).

### Stereo Width
- Start with Min=0.3, Max=1.0.
- If too wide at high ghost, reduce Max to 0.8.
- If not wide enough, reduce Min to 0.2.

### Frozen Spawn Points
- Currently captures up to 8 positions, pads to minimum 4.
- If too repetitive, reduce to 6 max and 3 min.
- If too chaotic, increase to 12 max and 6 min.

---

## Visual Feedback (UI Integration)

Consider exposing these features in the Lissajous orb visualization:

1. **Reverse Grains:** Flash a subtle indicator or change orb color when reverse grains are active.
2. **Proximity:** Subtle hue shift based on puckX (warmer=recent, cooler=distant).
3. **Frozen State:** Orb "locks" its form when spectral freeze is active.
4. **Stereo Width:** Orb width/ellipticity reflects current stereo spread.

These are non-critical but enhance the "memory" metaphor visually.

---

## Performance Impact Assessment

**Expected CPU Impact:**
- **Reverse playback:** ~0% (just sign flip on speed).
- **Stereo positioning:** ~0% (replacing simple with slightly more complex math).
- **Proximity modulation:** ~0% (same spawn cost, different position calculation).
- **Spectral freeze:** ~0% (no extra processing, just different spawn logic).

**Total:** Negligible CPU increase. All features are essentially "free" in terms of DSP cost.

---

## Conclusion

These four features transform the ghost engine from a simple granular texture generator into a sophisticated "memory playback" system that evokes the emotional quality of remembering sound. The implementation maintains strict real-time safety while adding significant expressive depth.

**Key insight:** By mapping abstract concepts (memory proximity, temporal direction, spatial distribution) to concrete DSP parameters, we create a plugin that feels psychologically resonant rather than just technically impressive.

