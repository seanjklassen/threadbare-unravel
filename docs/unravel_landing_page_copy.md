# threadbare -- unravel

---

## reverb with a memory problem.

I kept reaching for a reverb that sounded more like the way I remember a song. Like the feeling of it coming back to me later, softer and not quite right. I wanted one that simulated recollection.

So I built it. Unravel holds onto what you played and replays fragments of it. Sometimes detuned and sometimes reversed, and feeds them back into the reverb engine. So, the tail drifts and degrades over time, carrying traces of your recent performance the way memory carries traces of a conversation you had last week. It's recognizable but a little unreliable.

---

## one main control. you drag it.

**vivid to hazy.** 
Left emphasizes presence and focus. 

Right adds fog and stereo width. 

**recent to distant.** 

Down sounds short and close. 

Up adds long tails, ghostly activity, and pitch warping.

The thing I didn't expect: you can play the puck while the music runs. Drag it slowly from vivid to hazy during a chord and the whole reverb turns inside out. It can shift under your hand like you're reshaping the air around the sound. I've spent entire evenings just moving it, making weird sounds I couldn't have dialed in on purpose.

---

### disintegration looper
Record up to 60 seconds of your audio and loop it. Each pass degrades the signal. Filters converge, saturation builds, random dropouts appear where the oxide has shed. Pitch wanders downward. Left and right channels drift apart. Wow and flutter creep in, and a noise floor rises underneath until there's nothing left. The idea came from listening to William Basinski's Disintegration Loops and wanting that process as something you could control in real time.

**spectral to diffuse.** 
Left emphasizes spectral thinning, like a photograph fading in the sun. 

Right emphasizes diffuse smearing, like watching something through rain. 

**lingering to fleeting.** 

Down can loop for thousands of iterations, barely changing. 

Up reaches broken entropy in as short as two passes. 

---

## okay, fine

The puck is the instrument. The drawer is the workbench. The puck maps across every parameter simultaneously and for most people, most of the time, it's the only control you need. But if you want to push the ghost engine harder than the puck allows, or you've decided at midnight that the decay needs to be exactly 7.2 seconds and you can't explain why, you can make that tweak. Open the drawer when you want precision. The puck keeps working either way, scaling relative to whatever you've set manually.

### ghost engine
Continuously records your input and spawns up to eight ghostly fragments at any time -- some shimmer, some play backwards, and all of it feeds into the reverb rather than sitting on top. The ghost material doesn't decorate the tail. It is the tail.

### scatter
Four voices that trigger off your transients, grab ghost fragments, pitch-shift them, and fling them across the stereo field. At low amounts it adds texture, a glittering edge. Turn it up and the reverb sounds like it is coming apart at the seams.

### decay
How long the tail rings out. The puck's Y-axis scales this by roughly 3x in either direction.

### distance
Low values put the reverb on top of your signal. Higher values push it back. Useful for keeping vocals intelligible in a long wash.

### size
Perceived space. Small is tight and close. Large is open and diffuse. The puck's Y-axis nudges this too, adding subtle pitch movement to the tail.

### brightness
How much high-end the reverb keeps. Left is muffled and warm. Right has air. Center is neutral.

### blend
Dry/wet mix. Most presets sit between 35% and 75%. On a send, set it to 100%.

### loudness
Output gain, -24 dB to +12 dB. Level compensation between presets. If the wet signal is too loud or too quiet relative to your session, this is where you fix it.

### ducking
Reverb pulls back while you play and swells when you stop, which is the difference between mud and breathing room on anything rhythmic.

### drift
Wandering modulation that keeps the tail from going static on sustained input. Subtle on the left side of the puck, increasingly woozy as you move right.

### true stereo sound
Left and right sides of the reverb develop independently -- not a mono reverb with widening applied after the fact. It costs more CPU, but the stereo image is real.

---

## what it can sound like

1. **unravel** -- Balanced and smooth. Where most people start and a surprising number stay. [Play]
2. **close** -- Tight and intimate. Scattered fragments add texture without adding space. Good for vocals. [Play]
3. **tether** -- Moderate ducking, grounded. Reverb follows your dynamics. Finger-picked guitar, quiet piano. [Play]
4. **pulse** -- Heavy ducking. Reverb pumps in the gaps between notes. Strummed parts, rhythmic synths. [Play]
5. **bloom** -- Long decay, expanding wash, gentle scatter. The pad preset. [Play]
6. **mist** -- Dark fog. Maximum diffusion, ghost reaching into distant memory. [Play]
7. **rewind** -- Ghost engine turned up. High reverse probability, scattered fragments. Surreal on anything sustained. [Play]
8. **halation** -- Bright, shimmery. Shimmer grains emphasized, bright tail. Like overexposed light. [Play]
9. **stasis** -- Looper-ready. Long decay, max ghost. Hit the infinity button and let it degrade. [Play]
10. **shiver** -- Maximum everything. [Play]

---

## specs

**Formats:** VST3, AU, Standalone.

**Platforms:** macOS (Universal binary, arm64 + x86_64), Windows.

**Requirements:** macOS 10.13+ or Windows 10+. A DAW for the plugin formats.

At default settings, CPU is fine. If you max the ghost engine, scatter, and disintegration looper at the same time, which I do more often than I should, your laptop might let you know.

---

## $45 [Buy Now]

*macOS 10.13+ or Windows 10+.*

---

## store page description

**Title:**
Threadbare -- Unravel

**Short Description:**
Granular memory reverb with 8x8 FDN. Records the last 750ms of your performance, replays it as detuned grains with reverse playback and shimmer. One 2D puck macro control. Disintegration looper with tape-modeled entropy. For shoegaze, ambient, dream pop.

**Full Description:**
Every reverb plugin simulates a room. This one simulates memory.

The Ghost Engine maintains a circular buffer of your input and spawns up to 8 simultaneous granular voices (Hann-windowed, micro-detuned, with Catmull-Rom interpolation). At high settings, ~25% of grains play in reverse with mirrored stereo panning. Shimmer grains pitch up +12 semitones. All granular output feeds directly into the FDN input, not mixed in post.

The reverb core is a true stereo 8x8 feedback delay network with a Householder mixing matrix, frequency-dependent damping (variable LPF + HPF), and per-line sinusoidal LFO modulation (0.1-3.0 Hz, randomized rates). Delay times are prime-based (31-97ms) with a user-controlled size multiplier (0.5x-2.0x).

One 2D puck macro control maps across all parameters simultaneously. X-axis: Vivid/Hazy (ER gain, diffusion, ghost temporal depth 150-750ms, drift depth 20-80 samples). Y-axis: Recent/Distant (decay multiplier ~0.3x-3x, ghost amount, size modulation with Doppler pitch effect).

Disintegration Looper: input-gated recording up to 60 seconds. Entropy-based degradation model with ascending HPF (20-800 Hz), descending LPF (20k-2k Hz), resonant filter convergence, hysteresis tape saturation, stochastic oxide-shedding dropouts, Brownian pitch drag (±40 cents), azimuth drift (stereo decoupling), wow & flutter (0.5/6 Hz), and rising pink noise floor. Puck Y controls entropy rate (2 to 10,000+ iterations). Puck X controls degradation character (spectral thinning vs diffuse smearing). Transport-aware.

Additional features:
- Glitch Sparkle: 4-voice transient-reactive granular looper with pitch palette (root, fifth, octave, double octave), reverse grains (30%), ping-pong stereo with LFO panning, Haas-effect micro-delays
- Ducking: envelope follower (10ms attack, 250ms release) on dry signal, minimum 15% wet floor
- Drift: per-line LFO modulation, randomized rates, puck-controlled depth
- Early reflections: 6-tap stereo multi-tap delay with pre-delay (0-100ms)
- Decay: 0.4s to 50s RT60
- 10 factory presets

Formats: VST3, AU, Standalone. Platforms: macOS (Universal binary, arm64 + x86_64), Windows. Built with JUCE 8, C++20.

**Keywords:**
reverb, granular reverb, FDN reverb, feedback delay network, 8x8 FDN, Householder matrix, ghost engine, granular synthesis, reverse reverb, shimmer reverb, disintegration looper, tape degradation, tape decay, entropy, William Basinski, glitch sparkle, granular looper, ducking reverb, sidechain reverb, modulated reverb, drift modulation, true stereo, shoegaze, dream pop, ambient, lo-fi, indie, texture, drone, pad, VST3, AU, audio unit, standalone, macOS, Windows, JUCE, audio plugin, reverb plugin