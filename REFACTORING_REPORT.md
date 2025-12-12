# UnravelReverb Refactoring Report

**Generated:** Analysis of `Source/DSP/UnravelReverb.cpp` and `Source/DSP/UnravelReverb.h`

---

## 1. Unused Variables

### 1.1 Class Members Never Read in `process()`

**`frozenSpawnIndex` (Line 91 in header)**
- **Status:** Declared but never used
- **Location:** `UnravelReverb.h:91`
- **Description:** Comment says "Round-robin selector for frozen positions" but the code uses weighted random selection instead (see `trySpawnGrain()` line 268-270)
- **Impact:** Dead code - can be safely removed
- **Recommendation:** Remove this member variable

---

## 2. Redundant Math (Calculations That Could Be Moved Out of Sample Loop)

### 2.1 Constants Calculated Multiple Times

**`sixtyDb` constant (Lines 478 and 743)**
- **Status:** Duplicate calculation
- **Location:** 
  - Line 478: `constexpr float sixtyDb = -6.90775527898f; // ln(0.001)`
  - Line 743: `constexpr float sixtyDb = -6.90775527898f; // ln(0.001) for -60dB`
- **Impact:** Minor - constexpr is compile-time, but violates DRY principle
- **Recommendation:** Define once as a namespace-level constant or class static constexpr

### 2.2 Per-Block Calculations That Could Be Cached

**`baseDelayOffsets` array (Lines 589-594)**
- **Status:** Recalculated every `process()` call
- **Location:** `process()` function, before sample loop
- **Description:** Converts delay times from ms to samples. Only changes when `sampleRate` or `kBaseDelaysMs` changes (both are fixed after `prepare()`)
- **Impact:** Low CPU overhead, but unnecessary recalculation
- **Recommendation:** 
  - Move to class member: `std::array<float, kNumLines> baseDelayOffsetsSamples;`
  - Calculate once in `prepare()` when sample rate is set
  - Use directly in sample loop

**`avgDelayMs` constant (Line 476)**
- **Status:** Hardcoded constant calculated inline
- **Location:** `process()` function, line 476
- **Description:** `constexpr float avgDelayMs = 59.125f; // Average of [31,37,41,53,61,71,83,97]`
- **Impact:** Minor - constexpr is compile-time, but could be in tuning header
- **Recommendation:** Move to `UnravelTuning.h::Fdn` as `kAvgDelayMs` for consistency

**`preDelaySamples` (Line 644)**
- **Status:** Calculated per sample inside ER processing
- **Location:** Inside sample loop, line 644
- **Description:** `const float preDelaySamples = state.erPreDelay * 0.001f * sampleRate;`
- **Impact:** Low - only calculated when `erGain > 0.001f`, but `state.erPreDelay` changes infrequently
- **Recommendation:** 
  - Calculate once per block before sample loop
  - Only recalculate if `state.erPreDelay` changed (track previous value)

**`spawnInterval` (Line 691)**
- **Status:** Calculated every sample iteration
- **Location:** Inside sample loop, line 691
- **Description:** `const int spawnInterval = static_cast<int>(sampleRate * 0.015f);`
- **Impact:** Low - but `sampleRate` never changes during processing
- **Recommendation:** 
  - Calculate once in `prepare()` and store as class member
  - Use: `int grainSpawnInterval;` (set in `prepare()`)

### 2.3 Per-Sample Calculations That Could Be Optimized

**`toneCoef` calculation (Line 619)**
- **Status:** Calculated per sample
- **Location:** Inside sample loop, line 619
- **Description:** `const float toneCoef = juce::jmap(currentTone, -1.0f, 1.0f, 0.1f, 0.9f);`
- **Impact:** Low - simple linear mapping, but could use smoothed value directly
- **Recommendation:** Consider using a `SmoothedValue` for `toneCoef` itself, or pre-calculate the mapping coefficients

**`driftAmount` calculation (Line 623)**
- **Status:** Calculated per sample
- **Location:** Inside sample loop, line 623
- **Description:** `const float driftAmount = currentDrift * currentDriftDepth;`
- **Impact:** Low - single multiply, but both values are smoothed
- **Recommendation:** This is fine as-is (needs per-sample smoothing for tape warble effect)

---

## 3. Dead Code

### 3.1 Commented-Out Code Blocks
- **Status:** ✅ **NONE FOUND**
- **Analysis:** All comments are documentation-style, no commented-out implementation code

### 3.2 Unused Functions
- **Status:** ✅ **NONE FOUND**
- **Analysis:** All helper functions (`readDelayInterpolated`, `readGhostHistory`, `trySpawnGrain`, `processGhostEngine`) are called from `process()`

### 3.3 Unused Variables (See Section 1.1)
- `frozenSpawnIndex` - declared but never used

---

## 4. Safety Violations (Real-Time Safety)

### 4.1 Memory Allocations in `process()`
- **Status:** ✅ **NONE FOUND**
- **Analysis:** 
  - All `std::vector::resize()` calls are in `prepare()` (lines 80, 110, 125-126)
  - No `new`, `malloc`, `std::vector::push_back()`, or `std::string` creation in `process()`
  - All buffers are pre-allocated in `prepare()`

### 4.2 Locks or Blocking Operations
- **Status:** ✅ **NONE FOUND**
- **Analysis:** No mutexes, CriticalSections, or blocking calls in `process()`

### 4.3 Exception Safety
- **Status:** ✅ **SAFE**
- **Analysis:** `process()` is marked `noexcept`, and all operations are exception-safe

### 4.4 Denormal Handling
- **Status:** ✅ **PROPERLY HANDLED**
- **Analysis:** `juce::ScopedNoDenormals` used at start of `process()` (line 440)

---

## 5. Documentation and Specification Discrepancies

### 5.1 Tuning Constants Mismatches

**Decay Max Range:**
- **Spec (`unravel_spec.md` line 494):** `kT60Max = 20.0f`
- **Implementation (`UnravelTuning.h` line 26):** `kT60Max = 50.0f`
- **Impact:** Implementation allows longer decays than spec
- **Recommendation:** Align with spec (20.0f) or update spec to match implementation intent

**Modulation Rate Range:**
- **Spec (`unravel_spec.md` lines 516-517):** `kMinRateHz = 0.05f`, `kMaxRateHz = 0.4f`
- **Implementation (`UnravelTuning.h` lines 74-75):** `kMinRateHz = 0.1f`, `kMaxRateHz = 3.0f`
- **Impact:** Implementation has wider, faster modulation range than spec
- **Recommendation:** Document why range was expanded, or align with spec

**Modulation Depth:**
- **Spec (`unravel_spec.md` line 521):** `kMaxDepthSamples = 8.0f` (safe range ~2-8)
- **Implementation (`UnravelTuning.h` line 79):** `kMaxDepthSamples = 100.0f`
- **Impact:** Implementation allows extreme tape warble (100 samples vs 8)
- **Recommendation:** This appears intentional for "extreme tape warble/detune" effect - document this design decision

**Grain Duration Range:**
- **Spec (`unravel_spec.md` lines 529-530):** `kGrainMinSec = 0.08f`, `kGrainMaxSec = 0.20f`
- **Implementation (`UnravelTuning.h` lines 87-88):** `kGrainMinSec = 0.05f`, `kGrainMaxSec = 0.30f`
- **Impact:** Implementation has wider grain duration range
- **Recommendation:** Document or align with spec

**Ghost Gain Range:**
- **Spec (`unravel_spec.md` lines 540-541):** `kMinGainDb = -30.0f`, `kMaxGainDb = -12.0f`
- **Implementation (`UnravelTuning.h` lines 98-99):** `kMinGainDb = -24.0f`, `kMaxGainDb = -3.0f`
- **Impact:** Implementation has louder ghost presence (less attenuation)
- **Recommendation:** Document or align with spec

**Shimmer Probability:**
- **Spec (`unravel_spec.md` line 537):** `kShimmerProbability = 0.05f` (5%)
- **Implementation (`UnravelTuning.h` line 95):** `kShimmerProbability = 0.25f` (25%)
- **Impact:** Implementation has much more prominent shimmer effect
- **Recommendation:** Document or align with spec

### 5.2 Implementation Details Not in Spec

**PuckX Macro Drift Depth:**
- **Status:** Implementation detail not documented in spec
- **Location:** `process()` lines 576-580
- **Description:** PuckX macro overrides standard `kMaxDepthSamples` with 20-80 sample range
- **Impact:** Spec doesn't mention this puckX→drift mapping
- **Recommendation:** Document in spec section 3.9 (Puck → Parameter Mapping)

**PuckY Size Modulation (Doppler Effect):**
- **Status:** Implementation detail not in spec
- **Location:** `process()` lines 448-457
- **Description:** PuckY maps to size multiplier (0.92x to 1.08x) for subtle pitch warp
- **Impact:** Spec doesn't mention this "subtle doppler" effect
- **Recommendation:** Document in spec section 3.9

**Per-Line Feedback Calculation:**
- **Status:** Implementation uses per-line feedback (line 750), spec suggests single feedback
- **Location:** `process()` lines 747-751
- **Description:** Each delay line gets its own feedback gain based on its delay time
- **Impact:** More accurate decay than spec's "simplified approach: single feedback gain"
- **Recommendation:** Update spec section 3.3 to reflect per-line feedback implementation

**Early Reflections Pre-Delay:**
- **Status:** Spec mentions pre-delay (section 3.1) but doesn't detail ER pre-delay separately
- **Location:** `process()` line 644, `UnravelState` line 24
- **Description:** `erPreDelay` is a separate parameter (0-100ms) that shifts ER taps
- **Impact:** Spec doesn't clearly document this as a user-facing parameter
- **Recommendation:** Clarify in spec section 3.4 that ER pre-delay is adjustable

### 5.3 Spec Requirements Not Implemented

**Anti-Denormal Noise:**
- **Status:** Spec mentions adding "tiny noise" (section 3.10, `UnravelTuning.h::Safety::kAntiDenormal`)
- **Location:** Spec line 448, `UnravelTuning.h` line 169
- **Description:** `kAntiDenormal = 1.0e-18f` is defined but never used in code
- **Impact:** Minor - `ScopedNoDenormals` handles most cases, but spec suggests additional noise
- **Recommendation:** Either implement noise injection or remove from spec/tuning

**Freeze Ramp Time:**
- **Status:** Spec mentions freeze ramps (section 3.7, `UnravelTuning.h::Freeze::kRampTimeSec`)
- **Location:** Spec lines 418-426, `UnravelTuning.h` line 137
- **Description:** `kRampTimeSec = 0.05f` is defined but implementation uses instant feedback switching
- **Impact:** Freeze transitions may be abrupt rather than smoothed
- **Recommendation:** Implement smoothed freeze transitions or document instant behavior

**Tilt EQ on Wet Output:**
- **Status:** Spec mentions "additional gentle tilt EQ on the wet output for extra Tone shaping" (section 3.3)
- **Location:** Spec line 186
- **Description:** Not implemented in current code
- **Impact:** Missing feature from spec
- **Recommendation:** Implement or remove from spec

---

## Summary of Priority Issues

### High Priority
1. **Remove unused `frozenSpawnIndex` variable** (Section 1.1)
2. **Cache `baseDelayOffsets` calculation** (Section 2.2) - reduces per-block overhead
3. **Cache `spawnInterval` calculation** (Section 2.2) - simple optimization

### Medium Priority
4. **Consolidate `sixtyDb` constant** (Section 2.1) - code cleanliness
5. **Move `avgDelayMs` to tuning header** (Section 2.2) - consistency
6. **Calculate `preDelaySamples` once per block** (Section 2.2) - minor optimization

### Low Priority / Documentation
7. **Align tuning constants with spec or update spec** (Section 5.1)
8. **Document implementation details missing from spec** (Section 5.2)
9. **Implement or remove spec features** (Section 5.3)

---

## Code Quality Notes

✅ **Excellent real-time safety** - No allocations, locks, or exceptions in audio thread
✅ **Proper denormal handling** - `ScopedNoDenormals` used correctly
✅ **Good use of `noexcept`** - All audio-thread functions properly marked
✅ **Clean separation** - DSP logic properly namespaced
✅ **Modern C++** - Uses `std::span`, `std::array`, proper RAII

⚠️ **Minor optimization opportunities** - Some per-block calculations could be cached
⚠️ **Documentation gaps** - Some implementation details not in spec, some spec features not implemented
