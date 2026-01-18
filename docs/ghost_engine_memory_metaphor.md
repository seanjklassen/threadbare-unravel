# The Ghost Engine: A Memory Metaphor

## Conceptual Overview

The enhanced ghost engine transforms Unravel from a reverb plugin into a **memory machine**—a device that captures, distorts, and replays moments of sound as if they were being recalled from memory.

This document explains the psychological and sonic metaphor behind each feature.

---

## The Core Metaphor: Remembering Sound

When you remember a moment, several things happen that don't occur during the original experience:

1. **Time becomes fluid** – memories can play forward, backward, or seem stretched/compressed
2. **Details shift** – some aspects become clearer, others fade or distort
3. **Position changes** – you can "view" the memory from different perspectives
4. **Fixation occurs** – sometimes you replay the same moment over and over
5. **Distance matters** – recent memories feel different than distant ones

The ghost engine implements each of these as sonic phenomena.

---

## Feature Map: From Concept to Sound

### 1. Reverse Memory Playback → Time Fluidity

**The feeling:** Sometimes memories seem to run backwards. You find the ending first, then work back to the beginning. It's disorienting but also strangely beautiful.

**The sound:**
- Grains occasionally play backwards through the history buffer
- More common at high ghost levels (when memory is "active")
- Creates the sensation of time becoming unstuck
- Musically: adds subtle reverse cymbal swells, backward vocal textures, reversed attack transients

**User experience:**
- `ghost = 0.3`: Occasional backward grain (rare, surprising)
- `ghost = 0.7`: Frequent backward grains (6-7% of the time)
- `ghost = 1.0`: Heavy backward presence (25% of grains)

**Emotional resonance:** Nostalgia, time distortion, "rewinding" through a memory.

---

### 2. Disintegration Looper → Fixation, Rumination & Decay

**The feeling:** Sometimes you can't let go of a moment. Your mind replays it again and again, each time noticing something different—a slightly different angle, emphasis, or emotional coloring. But memories aren't static. Each time you revisit them, they change. They degrade. Eventually, they fade.

**The sound:**
- When the looper is activated, it records 4 bars of your reverb tail (tempo-synced)
- The loop plays back, but with each iteration it degrades:
  - Filters converge (highs thin, lows fade)
  - Pitch drifts downward (like a dying tape motor)
  - Stereo channels decouple (azimuth drift)
  - Random dropouts occur (oxide shedding)
- Meanwhile, the ghost engine locks onto specific moments, replaying them with variation
- Creates a "fading memory" effect: same material, gradually dissolving

**User experience:**
- Click the looper button to start recording during a guitar chord
- After 4 bars, the loop begins: the chord becomes a degrading memory that slowly evaporates
- Puck Y controls how fast it fades (top = fast, bottom = endless)
- Puck X controls the character: left (Ghost) = spectral thinning, right (Fog) = diffuse smearing
- When entropy reaches 1.0, the loop fades gracefully back to normal reverb

**Emotional resonance:** Obsession, meditation, impermanence, the bittersweet nature of memory. The loop doesn't just repeat—it degrades, like real memories do. Haunting on sustained sounds, hypnotic on rhythmic material.

---

### 3. Memory Proximity Modulation → Temporal Depth

**The feeling:** Recent memories feel immediate and vivid (you remember details). Distant memories feel hazy and ethereal (you remember the feeling, not the facts).

**The sound:**
- Puck X-axis controls WHERE in the history buffer grains spawn from:
  - **Left (Body):** Grains spawn from the last 0-200ms (recent zone)
  - **Right (Air):** Grains spawn from 500-750ms ago (distant zone)
  - **Center:** Even mix of both

**Sonic characteristics by position:**

| Puck Position | Memory Type | Sonic Quality |
|---------------|-------------|---------------|
| Far Left      | Immediate   | Tight, present, almost doubles the source |
| Center-Left   | Recent      | Slightly delayed echo/doubling |
| Center        | Mixed       | Blend of immediate and distant |
| Center-Right  | Fading      | Noticeably separate, dreamlike |
| Far Right     | Ancient     | Distant, ghostly, unmoored from source |

**Integration with existing puck behavior:**
- Left already meant "more body" (strong early reflections)
- Right already meant "more air" (diffuse wash)
- Now it ALSO means recent vs. distant memories
- The metaphors reinforce each other:
  - **Body + Recent** = physical, present, almost in the room
  - **Air + Distant** = ethereal, memory-like, floating in time

**User experience:**
- Play a melody, move puck from left to right
- Left: immediate doubling/echo (still recognizable as "now")
- Right: distant, pitch-shifted fragments (recognizable as "then")

**Emotional resonance:** The passage of time, the fading of memory, distance vs. intimacy.

---

### 4. Enhanced Stereo Positioning → Spatial Perspective

**The feeling:** Memories aren't mono. They have spatial dimension. You remember sounds coming from different places, and that spatial quality is part of what makes them feel real (or surreal).

**The sound:**
- Each grain gets a randomized stereo position (pan)
- Pan width dynamically adjusts based on:
  - **Ghost amount:** Higher ghost = wider spread
  - **Puck X position:** Right (air/distant) = wider spread
  - **Reverse grains:** Optionally mirrored to opposite stereo side

**Stereo behavior matrix:**

| Ghost | Puck X | Stereo Width | Description |
|-------|--------|--------------|-------------|
| 0.0   | Any    | Narrow (30%) | Focused, almost mono |
| 0.5   | Left   | Moderate (45%) | Some spread, still centered |
| 0.5   | Right  | Wide (65%) | Noticeable stereo movement |
| 1.0   | Left   | Moderate-Wide (50%) | Presence with space |
| 1.0   | Right  | Very Wide (100%) | Full stereo field |

**Additional spatial features:**
- **Constant-power panning:** Grains maintain equal loudness across the stereo field (no center loudness spike)
- **Reverse grain mirroring:** Forward grains lean one direction, reverse grains lean the other (spatial separation between past and present)

**User experience:**
- Low ghost: grains cluster near center (focused memory)
- High ghost + right puck: grains scattered across entire stereo field (memory fragments coming from everywhere)
- Reverse grains appear from "the other side" spatially

**Emotional resonance:** Spatial awareness in memory, the feeling of sounds "surrounding" you in recollection, disorientation vs. clarity.

---

## The Combined Experience: All Features Together

When all four features are active simultaneously, you get:

### Scenario 1: "Near Body Memory" (Puck: Bottom-Left, Ghost: Medium)
- Recent memories (0-200ms)
- Mostly forward playback (few reverse grains)
- Narrow stereo field
- Strong early reflections (existing behavior)
- **Feeling:** Present, immediate, like you're still IN the moment
- **Use case:** Subtle doubling, room presence, "just happened" quality

### Scenario 2: "Distant Air Memory" (Puck: Top-Right, Ghost: High)
- Distant memories (500-750ms ago)
- Many reverse grains (25%)
- Wide stereo field
- Weak early reflections, strong diffuse wash
- **Feeling:** Ethereal, time-distorted, dreamlike
- **Use case:** Ambient washes, shoegaze textures, "fading memory" quality

### Scenario 3: "Fading Memory" (Looper: Active, Any Puck Position)
- 4 bars of reverb captured and looped
- Each iteration degrades: filters converge, pitch drifts, dropouts occur
- Ghost engine locked onto specific moments, replaying with variation
- Puck Y controls decay rate (top = fast fade, bottom = slow fade)
- **Feeling:** Obsessive, meditative, impermanent—a memory that won't stay still
- **Use case:** Evolving drones, ambient beds, William Basinski-style tape decay

### Scenario 4: "Memory Overload" (Puck: Top-Center, Ghost: Max, Drift: High)
- Mix of recent and distant memories
- High reverse grain probability
- Wide stereo spread
- Heavy modulation (existing drift behavior)
- **Feeling:** Overwhelming, chaotic, memories flooding in
- **Use case:** Experimental textures, dense ambient clouds, "too much past at once"

---

## Design Philosophy: Why These Features Work

### 1. Metaphorical Coherence
Each feature maps to a real aspect of human memory. Users don't need to understand the DSP—they intuitively grasp "recent vs. distant" or "playing backwards" or "stuck on a moment."

### 2. Parametric Efficiency
Instead of adding 10 new knobs, we:
- Overload existing controls (puck X now does double-duty)
- Scale effects by existing parameters (ghost amount gates reverse probability)
- Tie to existing features (looper triggers spectral grain locking automatically)

Result: More expression, same interface complexity.

### 3. Emergent Complexity
Simple rules (reverse grains, proximity zones, pan width) combine to create complex, organic-feeling textures. The system feels "alive" rather than mechanical.

### 4. Real-Time Safety
Every feature is allocation-free, lock-free, and safe for audio-rate processing. No compromises on performance.

### 5. Threadbare Aesthetic
All features lean into the "Indie/Dream Pop/Ambient" sound goals:
- Nothing is harsh or metallic
- Everything trends toward nostalgic, soft, bittersweet
- The plugin helps you make beautiful, emotionally resonant music

---

## User-Facing Language

When describing these features in documentation or marketing:

### Don't say:
- "Grains spawn from different buffer positions based on puck X"
- "Reverse playback probability is squared-scaled by ghost amount"
- "The looper captures discrete spawn points for grain replay"

### Do say:
- "The puck lets you choose between recent memories (left) and distant memories (right)"
- "At high ghost levels, memories sometimes play backwards through time"
- "The looper captures a moment and replays it, gradually degrading with each pass"

The metaphor does the explaining. The DSP is just implementation.

---

## Technical Boundaries

These features push the "memory" metaphor without breaking the reverb:

### What we DON'T do:
- ❌ Actual time-stretching (too CPU-heavy, too "plasticky")
- ❌ FFT-based spectral manipulation (breaks real-time guarantee)
- ❌ ML/AI "memory" systems (out of scope, overkill)
- ❌ User-triggered grain spawn (keeps it automatic/emergent)

### What we DO:
- ✓ Pitch-shifting via playback speed (cheap, classic granular technique)
- ✓ Reverse playback (literally free—just a sign flip)
- ✓ Position-based spawning (same cost as random spawning)
- ✓ Pan randomization (negligible CPU)

All features stay within the "threadbare" philosophy: maximum effect, minimum overhead.

---

## Conclusion: Why "Memory" Matters

Reverb is already a form of memory—it's sound remembering its past interactions with space. But traditional reverbs are **mechanical** memories: precise, consistent, predictable.

The enhanced ghost engine creates **human** memories: fluid, distorted, emotional, sometimes unreliable but always evocative.

This is what separates Unravel from other reverbs. It's not trying to model a real space. It's trying to model how we **remember** sound—and that's a much more interesting (and musical) problem.

---

## Appendix: Neuroscience Parallels (For Fun)

| Memory Phenomenon | Neuroscience | Unravel Implementation |
|-------------------|--------------|------------------------|
| Memory consolidation | Recent memories more detailed | Proximity modulation (puckX) |
| Memory reconsolidation | Each recall slightly changes the memory | Disintegration looper degradation |
| Time distortion | Memories compress/expand temporal experience | Reverse playback, detune |
| Spatial memory | Hippocampal place cells | Stereo positioning |
| Intrusive thoughts | Repetitive unwanted memories | Frozen grain repetition |
| Memory decay | Older memories lose fidelity | Distant zone + damping |

Unravel is, in a very literal sense, a **memory simulator** running at audio rate. That's not a marketing phrase—it's an accurate technical description of the DSP architecture.

And that's why it sounds the way it does.

