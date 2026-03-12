# threadbare: unravel

---

> [VISUAL] dark screen. one orb, breathing slowly. no controls yet.

## meet unravel.

---

> [VISUAL] orb pulse reacts to a single sustained note.

## reverb with a memory problem.

---

> [VISUAL] waveform fragments begin to appear and dissolve inside the orb.

I had reverbs I liked. They all gave me rooms. I kept reaching for something that felt less like architecture and more like recollection.

unravel holds onto what you play and replays fragments of it.

creating a tail that drifts over time, like a fading memory.

it is recognizable, but a little unreliable.

> [CAPTION] Some fragments shimmer. Some play backwards. All of it is woven into the decay.

---

> [VISUAL] the puck appears. the rest of the interface is still hidden.
> [INTERACTION] drag the puck on scroll-linked demo audio.

## one main control. 

**vivid to hazy**
left stays present and focused. right adds fog and stereo spread.

**recent to distant**
down stays short and close. up stretches time, memory, and movement.

The thing I did not expect: playing the puck while music runs feels like performing the reverb itself. Drag across a held chord and the whole tail turns inside out.

> [INTERACTION] feature plan: add a lightweight, browser-side puck interaction so visitors can drag and hear before watching a full demo.

---

> [VISUAL] looper button fades in. orb color shifts as entropy rises.
> [INTERACTION] press and hold to record, then release into loop playback.

## loops that disintegrate in real time.

Record up to 60 seconds. Let it loop. Each pass thins, wanders, and frays. Filters narrow. Saturation builds. Dropouts start appearing where the tape would have shed.

It came from obsessing over *The Disintegration Loops* and wanting that process under your hand, not frozen in a recording.

**spectral to diffuse**
left feels like fading detail. right feels like blurred weather.

**lingering to fleeting**
down can run for ages. up can fall apart in two passes.

---

> [VISUAL] drawer icon appears, then opens when section enters viewport.

## but wait, there is more.

The puck is the instrument. The drawer is the workbench. Most of the time, you will stay on the puck. But if you want the decay at exactly 7.2 seconds at midnight, the drawer is there.

### shape of the tail

- **memory:** how much of your recent playing gets folded back in.
- **scatter:** how much those fragments burst outward across stereo.
- **decay and size:** how long and how wide the space feels.

### clarity and placement

- **brightness:** warm and muffled to airy and open.
- **distance:** front-of-speaker to farther back in the mix.
- **ducking:** tail steps back while you play, then returns.

### movement and level

- **drift:** subtle wandering that keeps sustained audio alive.
- **blend:** dry and wet balance.
- **loudness:** final output trim.
- **true stereo:** left and right evolve independently, not widened after the fact.

---

> [VISUAL] preset list slides in.  
> [INTERACTION] click any preset name to audition one phrase.

## listen to what it can become.

1. **balanced** `unravel` smooth and centered. where most people start. [Play]
2. **intimate** `close` tight and close-mic, with texture but not extra size. [Play]
3. **grounded** `tether` follows your dynamics and stays out of the way. [Play]
4. **rhythmic** `pulse` breathes in the gaps between notes. [Play]
5. **expansive** `bloom` long, widening wash for pads and held chords. [Play]
6. **fogged** `mist` dark and distant, like sound through rain. [Play]
7. **surreal** `rewind` memory fragments step forward and time bends. [Play]
8. **luminous** `halation` bright, glassy shimmer with lift. [Play]
9. **erosive** `stasis` built for looper decay and long-form disintegration. [Play]
10. **unstable** `shiver` everything in motion, right at the edge. [Play]

---

> [VISUAL] minimal footer card.

## specs

**Formats:** VST3, AU, Standalone  
**Platforms:** macOS (Universal binary, arm64 + x86_64), Windows  
**Requirements:** macOS 10.13+ or Windows 10+, plus a DAW for plugin formats

At default settings, CPU is fine. If you max memory, scatter, and the looper together, your laptop may start negotiating terms.

---

## $45 [Buy Now]

*macOS 10.13+ or Windows 10+.*

---

## appendix: store page description

**Title:**  
threadbare: unravel

**Short Description:**  
Reverb with a memory problem. unravel holds onto fragments of what you just played and dissolves them into the tail. One puck reshapes the whole space. Add a disintegration looper for controlled entropy over time. For shoegaze, ambient, and dream pop.

**Full Description:**  
Most reverbs simulate a room. unravel simulates what happens to sound inside a memory.

The Ghost Engine maintains a circular buffer of your input and spawns up to 8 simultaneous granular voices (Hann-windowed, micro-detuned, with Catmull-Rom interpolation). At high settings, about 25% of grains play in reverse with mirrored stereo panning. Shimmer grains pitch up +12 semitones. Granular output feeds directly into the FDN input, not post-mixed on top.

The reverb core is a true stereo 8x8 feedback delay network with a Householder mixing matrix, frequency-dependent damping (variable LPF + HPF), and per-line sinusoidal LFO modulation (0.1 to 3.0 Hz, randomized rates). Delay times are prime-based (31 to 97 ms) with a user-controlled size multiplier (0.5x to 2.0x).

One 2D puck macro control maps across all parameters simultaneously. X-axis: Vivid/Hazy (ER gain, diffusion, ghost temporal depth 150 to 750 ms, drift depth 20 to 80 samples). Y-axis: Recent/Distant (decay multiplier about 0.3x to 3x, ghost amount, size modulation with Doppler pitch effect).

Disintegration Looper: input-gated recording up to 60 seconds. Entropy degradation model with ascending HPF (20 to 800 Hz), descending LPF (20 kHz to 2 kHz), resonant filter convergence, hysteresis tape saturation, stochastic oxide-shedding dropouts, Brownian pitch drag (up to +/-40 cents), azimuth drift (stereo decoupling), wow and flutter (0.5 and 6 Hz), and rising pink noise floor. Puck Y controls entropy rate (2 to 10,000+ iterations). Puck X controls degradation character (spectral thinning vs diffuse smearing). Transport-aware.

Additional features:

- Glitch Sparkle: 4-voice transient-reactive granular looper with pitch palette (root, fifth, octave, double octave), reverse grains (30%), ping-pong stereo with LFO panning, Haas micro-delays
- Ducking: envelope follower (10 ms attack, 250 ms release) on dry signal, minimum 15% wet floor
- Drift: per-line LFO modulation with randomized rates, puck-controlled depth
- Early reflections: 6-tap stereo multi-tap delay with pre-delay (0 to 100 ms)
- Decay range: 0.4 s to 50 s RT60
- 10 factory presets

Formats: VST3, AU, Standalone. Platforms: macOS (Universal binary, arm64 + x86_64), Windows. Built with JUCE 8, C++20.

**Keywords:**  
reverb, granular reverb, FDN reverb, feedback delay network, 8x8 FDN, Householder matrix, ghost engine, granular synthesis, reverse reverb, shimmer reverb, disintegration looper, tape degradation, tape decay, entropy, William Basinski, glitch sparkle, granular looper, ducking reverb, sidechain reverb, modulated reverb, drift modulation, true stereo, shoegaze, dream pop, ambient, lo-fi, indie, texture, drone, pad, VST3, AU, audio unit, standalone, macOS, Windows, JUCE, audio plugin, reverb plugin