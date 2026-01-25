# Glitch Looper "Sparkle" Enhancement Plan

## Vision

Transform the glitch effect from subtle stutters into **shimmering, granular sparkles** - like light dancing on water or stars twinkling. Inspired by Phantom Limb's multi-voice architecture but tuned for beauty rather than chaos.

## Design Principles

1. **Layered shimmer** - Multiple quiet voices create richness, not volume
2. **Harmonic content** - Pitch shifts that are musical (octaves, fifths)
3. **Gentle transitions** - Soft envelopes, no harsh cuts
4. **Stereo width** - Voices scattered across the stereo field
5. **Memory depth** - Draw from anywhere in history, not just recent
6. **Graceful scaling** - Low glitch = occasional sparkle, high glitch = full shimmer

---

## Implementation Plan

### Phase 1: Multi-Voice Architecture

**Goal**: 4 simultaneous micro-loopers instead of 2 (tighter voice range)

```cpp
// In UnravelReverb.h
static constexpr std::size_t kSparkleVoices = 4;
std::array<GlitchSlice, kSparkleVoices> sparkleVoices;
```

**Changes**:
- Each voice runs independently with its own position, pitch, envelope
- Voices trigger asynchronously (staggered, not synchronized)
- Fewer total voices keeps sparkles audible vs smeared
- Sum all voices with per-voice gain of -12dB (4 voices at -12dB ≈ -6dB total)

### Phase 2: Harmonic Pitch Palette

**Goal**: Musical pitch relationships that create shimmer

| Pitch | Ratio | Character | Probability |
|-------|-------|-----------|-------------|
| Root | 1.0x | Grounding | 30% |
| Octave up | 2.0x | Sparkle | 25% |
| 2 octaves up | 4.0x | Twinkle | 15% |
| Fifth up | 1.5x | Ethereal | 15% |
| Octave down | 0.5x | Warmth | 10% |
| Micro-shimmer | 1.02-1.08x | Chorus | 5% |

**Key**: High-octave content (2x, 4x) creates the "sparkle" - these should be emphasized.

### Phase 3: Memory Scrubbing

**Goal**: Draw fragments from anywhere in the 2-second history buffer

```cpp
// Instead of just recent audio:
float randomDepth = glitchRng.nextFloat(); // 0-1
float scrubPosition = ghostWriteHead - (randomDepth * historySize * 0.8f);
// Wraps to valid range
```

**Effect**: Creates variety and unpredictability. Old memories resurface randomly.

### Phase 4: Granular Fragment Sizes

**Goal**: Shorter, more granular slices for texture

| Glitch Amount | Fragment Length | Character |
|---------------|-----------------|-----------|
| Low (0-30%) | 50-150ms | Occasional sparkle |
| Mid (30-70%) | 20-80ms | Shimmering texture |
| High (70-100%) | 5-30ms | Dense granular cloud |

**Note**: At very short lengths (5-30ms), pitch becomes timbral - creates "grain" texture.

### Phase 5: Stereo Scattering + Ping-Pong

**Goal**: Each voice positioned differently, with high glitch adding motion

```cpp
struct GlitchSlice {
    // ... existing fields ...
    float pan = 0.5f;  // 0=left, 1=right
};

// On trigger:
slice.pan = glitchRng.nextFloat();  // Random base position
slice.panDir = (glitchRng.nextFloat() < 0.5f) ? -1.0f : 1.0f; // L->R or R->L

// On render:
// At higher glitch, pan moves within the slice (ping-pong motion)
const float panMotion = glitchAmount * pingPongDepth; // 0..1
const float movingPan = juce::jlimit(0.0f, 1.0f, slice.pan + slice.panDir * panMotion * panLfo);
sliceOutL = sample * envelope * (1.0f - movingPan);
sliceOutR = sample * envelope * movingPan;
```

**Effect**: At low glitch, static scattered sparkle. At high glitch, sparkles ping-pong.

### Phase 6: Soft Envelopes

**Goal**: Longer crossfades for gentle onsets

```cpp
// Scale crossfade with inverse of slice length
// Short slices need proportionally longer fades
const float fadeRatio = 0.3f;  // 30% of slice is fade in/out
const int fadeLen = std::max(glitchCrossfadeSamples, 
    static_cast<int>(slice.lengthSamples * fadeRatio));
```

**Effect**: No clicks, no harsh attacks - everything blooms gently.

### Phase 7: Density & Triggering

**Goal**: Natural, organic trigger timing

**Low glitch (0-30%)**:
- 1-2 voices active at a time
- Trigger every 200-500ms
- Long gaps between sparkles

**Mid glitch (30-70%)**:
- 2-3 voices active
- Trigger every 50-150ms
- Overlapping shimmer

**High glitch (70-100%)**:
- 3-4 voices active
- Trigger every 10-50ms
- Dense granular cloud with stronger ping-pong motion

### Phase 8: Gain Staging

**Goal**: Sparkle is present but not overwhelming

| Setting | Per-Voice Gain | Total (4 voices) |
|---------|----------------|------------------|
| Low glitch | -18dB | -12dB |
| Mid glitch | -15dB | -9dB |
| High glitch | -12dB | -6dB |

**Note**: At high settings, should be clearly audible but not clip.

---

## Tuning Constants

```cpp
struct GlitchSparkle {
    // Voice count
    static constexpr int kMaxVoices = 4;
    
    // Fragment sizes (ms)
    static constexpr float kMinFragmentMs = 5.0f;
    static constexpr float kMaxFragmentMs = 150.0f;
    
    // Memory scrub depth (0-1, portion of history buffer)
    static constexpr float kMinScrubDepth = 0.1f;   // Recent
    static constexpr float kMaxScrubDepth = 0.9f;   // Deep memory
    
    // Trigger intervals (ms)
    static constexpr float kMinTriggerMs = 10.0f;
    static constexpr float kMaxTriggerMs = 500.0f;
    
    // Per-voice gain
    static constexpr float kVoiceGainDb = -12.0f;
    
    // Ping-pong motion
    static constexpr float kPingPongDepthMin = 0.0f; // Low glitch: static pan
    static constexpr float kPingPongDepthMax = 0.8f; // High glitch: strong motion
    static constexpr float kPingPongRateHz = 3.0f;   // LFO rate for pan motion
    
    // Envelope
    static constexpr float kMinFadeRatio = 0.15f;
    static constexpr float kMaxFadeRatio = 0.4f;
    
    // Pitch probabilities
    static constexpr float kRootProb = 0.30f;
    static constexpr float kOctaveUpProb = 0.25f;
    static constexpr float kDoubleOctaveProb = 0.15f;
    static constexpr float kFifthProb = 0.15f;
    static constexpr float kOctaveDownProb = 0.10f;
    static constexpr float kMicroShimmerProb = 0.05f;
};
```

---

## Audio Examples (Conceptual)

**Low Glitch (10%)**:
> Playing a sustained guitar chord through the reverb. Every few seconds, a single high-pitched fragment sparkles briefly in the right channel, then fades. Like a distant wind chime.

**Mid Glitch (50%)**:
> The same chord now has a constant shimmer - multiple fragments at different pitches (octave up, fifth) dance across the stereo field. The reverb tail has gained a crystalline quality, like sunlight through a prism.

**High Glitch (90%)**:
> Dense granular cloud. The original chord is still recognizable but surrounded by a constellation of tiny pitched fragments. Almost like a chorus of angels or a thousand tiny bells. Beautiful, not chaotic.

---

## Implementation Order

1. **Quick wins first**:
   - Boost gain (-6dB → -3dB)
   - Add 4x pitch option
   - Enable stereo panning per slice

2. **Multi-voice**:
   - Refactor to use 6 independent voices
   - Staggered triggering
   - Per-voice gain

3. **Memory scrubbing**:
   - Random buffer position selection
   - Depth scaling with glitch amount

4. **Granular sizing**:
   - Much shorter fragments at high settings
   - Proportional fade ratios

5. **Polish**:
   - Fine-tune probabilities
   - Test with various sources
   - Ensure no clicks/artifacts

---

## Risks & Mitigations

| Risk | Mitigation |
|------|------------|
| CPU load with 6 voices | Voices are simple (just buffer read + envelope) |
| Clicking from short slices | Proportional fade ratio ensures smooth transitions |
| Overwhelming at high settings | Per-voice gain ensures headroom |
| Loss of original character | Low settings remain subtle and musical |

---

## Success Criteria

- [ ] Low glitch: Occasional, beautiful sparkle moments
- [ ] Mid glitch: Constant shimmering texture
- [ ] High glitch: Dense but beautiful granular cloud
- [ ] No clicks or artifacts at any setting
- [ ] CPU impact < 5% additional
- [ ] Works beautifully with pads, guitars, vocals
