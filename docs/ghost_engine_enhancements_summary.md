# Ghost Engine Enhancements: Summary

## Overview

This document summarizes the specifications for four new ghost engine features that enhance Unravel's "memory of sound" concept at extreme parameter levels.

**Status:** Fully specified, ready for implementation  
**Date:** December 7, 2025  
**Real-time safety:** All features maintain allocation-free, lock-free operation

---

## Features Specified

### 1. **Reverse Memory Playback**
- **Concept:** Grains occasionally play backwards through the history buffer
- **Trigger:** Probability increases with ghost amount (squared scaling)
- **At ghost=1.0:** 25% of grains play in reverse
- **Implementation:** Negative playback speed, works with existing interpolation
- **CPU cost:** ~0% (just a sign flip)

### 2. **Spectral Freezing of Grains**
- **Concept:** When freeze is active, grains lock onto 4-8 specific moments and replay them infinitely with variation
- **Trigger:** Activated by existing freeze button
- **Behavior:** Same source material, different pitch/length/pan each time
- **Implementation:** Capture spawn positions at freeze activation, use round-robin selection
- **CPU cost:** ~0% (same grain processing, different spawn logic)

### 3. **Memory Proximity Modulation**
- **Concept:** Puck X-axis controls temporal depth of memories
- **Left (Body):** Grains spawn from recent zone (0-200ms ago)
- **Right (Air):** Grains spawn from distant zone (500-750ms ago)
- **Integration:** Enhances existing Body/Air metaphor
- **Implementation:** Probabilistic zone selection based on puck position
- **CPU cost:** ~0% (same spawn cost, different position calculation)

### 4. **Enhanced Stereo Positioning**
- **Concept:** Grains distributed across stereo field based on ghost amount and puck position
- **Dynamic width:** Narrow at low ghost, wide at high ghost
- **Proximity correlation:** Recent memories narrower, distant memories wider
- **Reverse mirroring:** Reverse grains favor opposite stereo side (optional)
- **Implementation:** Constant-power panning, dynamic width calculation
- **CPU cost:** ~0% (minimal additional math)

---

## Files Modified

### 1. `/docs/unravel_spec.md`
**Changes:**
- Expanded section 3.6 (Ghost Engine) from 8 lines to ~200 lines
- Added subsections:
  - 3.6.1 Reverse Memory Playback
  - 3.6.2 Spectral Freezing of Grains
  - 3.6.3 Memory Proximity Modulation
  - 3.6.4 Enhanced Stereo Positioning
- Each subsection includes:
  - Conceptual explanation
  - Detailed implementation steps
  - Code examples
  - Tuning constant specifications

### 2. `/Source/UnravelTuning.h`
**Changes:**
- Added to `Ghost` struct:
  - `kReverseProbability` (0.25f)
  - `kRecentZoneMs` (200.0f)
  - `kDistantZoneStartMs` (500.0f)
  - `kDistantZoneEndMs` (750.0f)
  - `kProximityBiasPower` (1.0f)
  - `kMinPanWidth` (0.3f)
  - `kMaxPanWidth` (1.0f)
  - `kMirrorReverseGrains` (true)
- All constants include detailed comments explaining their purpose

### 3. `/Source/DSP/UnravelReverb.h`
**Changes:**
- Added to private members:
  - `ghostFreezeActive` (bool)
  - `frozenSpawnPositions` (array of 8 floats)
  - `numFrozenPositions` (size_t)
  - `frozenSpawnIndex` (size_t, round-robin selector)
- These support spectral freeze feature

---

## Documentation Created

### 1. `ghost_engine_enhancement_implementation_guide.md`
**Purpose:** Step-by-step implementation instructions for developers  
**Contents:**
- Detailed code examples for each feature
- Integration checklist with phases
- Real-time safety verification steps
- Tuning recommendations
- Testing procedures
- Performance impact assessment

**Key sections:**
- Feature 1: Reverse Playback (with code)
- Feature 2: Spectral Freezing (with state machine)
- Feature 3: Proximity Modulation (with helper method)
- Feature 4: Stereo Positioning (with panning law)
- Integration checklist (4 phases, 18 tasks)

### 2. `ghost_engine_memory_metaphor.md`
**Purpose:** Conceptual explanation of the "memory" design philosophy  
**Contents:**
- The core metaphor: remembering sound
- Feature map: concept to sound translation
- Combined experience scenarios
- Design philosophy rationale
- User-facing language guidelines
- Technical boundaries
- Neuroscience parallels (bonus)

**Key insight:**  
Traditional reverbs create **mechanical** memories (precise, consistent).  
Unravel creates **human** memories (fluid, emotional, sometimes unreliable).

---

## Design Principles

All features follow these principles:

### 1. **Metaphorical Coherence**
Each feature maps to a real aspect of human memory:
- Time fluidity → reverse playback
- Fixation → spectral freezing
- Temporal depth → proximity modulation
- Spatial perspective → stereo positioning

### 2. **Parametric Efficiency**
No new UI controls needed:
- Features scale with existing parameters (ghost, puck, freeze)
- Puck X does double-duty (body/air + recent/distant)
- Everything feels natural and discoverable

### 3. **Real-Time Safety**
Zero compromise on performance:
- No allocations in audio thread
- No locks or blocking operations
- No exceptions
- All buffers pre-allocated
- All CPU impact negligible (~0%)

### 4. **Threadbare Aesthetic**
Stays true to plugin identity:
- Nostalgic, bittersweet, weightless
- Nothing harsh or metallic
- Enhances indie/dream pop/ambient use cases

---

## Implementation Phases

Recommended implementation order (lowest to highest risk):

### Phase 1: Reverse Playback ✓ Specified
- Lowest risk, highest impact
- No new state, just logic change
- ~2 hours to implement
- Can be tested independently

### Phase 2: Stereo Enhancement ✓ Specified
- Low risk, good visual impact
- Minimal new code
- ~2 hours to implement
- Pairs well with reverse playback

### Phase 3: Proximity Modulation ✓ Specified
- Medium risk, requires new helper method
- ~3 hours to implement
- Should test extensively with puck positions

### Phase 4: Spectral Freeze ✓ Specified
- Highest risk, most new state
- ~4 hours to implement
- Requires careful testing of freeze transitions
- Should be last feature implemented

**Total estimated implementation time:** ~11 hours  
**Total estimated testing time:** ~5 hours  
**Total project time:** ~16 hours

---

## Testing Strategy

### For Each Feature:

1. **Isolation Testing**
   - Test feature alone with default settings
   - Verify no audio glitches or pops
   - Verify no CPU spikes

2. **Parameter Sweep**
   - Test feature across full parameter ranges
   - Verify smooth transitions
   - Verify expected sonic results

3. **Combination Testing**
   - Test feature with other features active
   - Verify no interaction bugs
   - Verify combined effect is musical

4. **Edge Case Testing**
   - Test with extreme settings
   - Test with rapid parameter changes
   - Test with very short/long grains
   - Test at different sample rates

5. **Real-World Testing**
   - Test with guitar, vocals, synths, drums
   - Test with extreme genres (ambient, metal, EDM)
   - Get user feedback on musicality

---

## Expected User Experience

### Subtle Use (Ghost < 0.5, Center Puck):
- Enhanced stereo presence (slightly wider than before)
- Occasional reverse grain (surprising, not distracting)
- Natural memory-like quality
- Most users won't consciously notice features, but sound feels "better"

### Moderate Use (Ghost 0.5-0.8, Puck Exploration):
- Clear reverse grains (present but not dominant)
- Noticeable difference between left/right puck positions
- Wide stereo field
- "This reverb sounds different" awareness

### Extreme Use (Ghost > 0.8, Far Right Puck):
- Heavy reverse presence (25% of grains)
- Distant, ghostly, time-distorted quality
- Very wide stereo field
- Obvious "memory playback" effect
- **This is where the features shine**

### With Freeze:
- Locked moment replaying infinitely
- Each repetition slightly different
- Meditative, obsessive quality
- Perfect for ambient pads and holds

---

## Success Metrics

How to know if the implementation succeeds:

### Technical Metrics:
- ✓ Zero allocations in process block (verified with profiler)
- ✓ Zero locks or blocking calls (verified with TSAN)
- ✓ CPU usage < 5% increase (measured with performance test)
- ✓ No audio glitches across parameter ranges (listening test)
- ✓ Stable at all sample rates (tested 44.1-192kHz)

### Musical Metrics:
- ✓ Features feel "natural" and discoverable
- ✓ Extreme settings are musical, not broken-sounding
- ✓ Puck positions have clear sonic identity
- ✓ Users describe sound as "memory-like" without prompting
- ✓ A/B tests show preference for enhanced version

### User Feedback Targets:
- "This reverb sounds alive"
- "I can't explain why, but it feels nostalgic"
- "The freeze mode is insane"
- "Moving the puck feels like traveling through time"

---

## Future Enhancements (Not in Scope)

Ideas that came up but are deferred:

1. **Grain density bursts** (memory flashes)
   - Too unpredictable for v1.0
   - Consider for v1.2

2. **Harmonic memory echoes** (P5, M3 intervals)
   - Interesting but adds complexity
   - Consider if users want "more harmony"

3. **Bit crushing for memory degradation**
   - Cool concept but might sound harsh
   - Consider for experimental preset pack

4. **User-triggered grain spawn**
   - Changes interaction model too much
   - Consider for v2.0 "performance mode"

5. **Grain density as parameter**
   - Currently auto-scaled by ghost
   - Could be exposed if users want more control

---

## Documentation References

**Core Specifications:**
- `/docs/unravel_spec.md` - Master spec (section 3.6 enhanced)
- `/Source/UnravelTuning.h` - All tuning constants

**Implementation Guides:**
- `/docs/ghost_engine_enhancement_implementation_guide.md` - Step-by-step coding instructions
- `/docs/ghost_engine_memory_metaphor.md` - Conceptual framework and UX language

**Original Design:**
- `/docs/unravel_spec.md` - Full plugin specification
- `/docs/unravel_tuning_cheatsheet.md` - Quick reference for magic numbers

---

## Conclusion

These four enhancements transform Unravel's ghost engine from a simple granular texture layer into a sophisticated memory playback system. By mapping human memory phenomena (time fluidity, fixation, temporal depth, spatial perspective) to concrete DSP behaviors, we create a plugin that feels psychologically resonant rather than just technically impressive.

**The features are "threadbare" in the best sense:**
- Strip away unnecessary complexity
- Focus on emotional resonance
- Maintain real-time safety
- Create beauty through simplicity

**Ready for implementation:** All specifications complete, all constants defined, all code patterns documented. Implementation can begin immediately.

---

## Sign-Off

**Specification Phase:** Complete ✓  
**Documentation Phase:** Complete ✓  
**Tuning Constants:** Defined ✓  
**Implementation Guide:** Written ✓  
**Conceptual Framework:** Documented ✓  

**Status:** Ready to ship to implementation phase.

---

*"The ghost engine doesn't just remember sound—it remembers the way we remember sound."*

