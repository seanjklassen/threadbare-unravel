# threadbare -- waver

---

## broken but beautiful synthesis.

I kept trying to make modern synths sound like the keyboards on the records I love. Detuning things, adding noise, running the output through tape emulations. But it never quite landed because the imperfection wasn't coming from inside the instrument. It was always painted on after.

Waver is a synthesizer where the wear is built into the architecture. Three layers — an analog polysynth, a toy FM keyboard, and a divide-down organ — all running through a print chain that models cassette tape, not as an effect, but as a fundamental part of the sound. The drift is in the oscillators. The age is in the envelopes. The warmth comes from the same place it always did: things not working perfectly.

---

## one main control. you drag it.

**clear to weathered.**
Left is present, bright, defined.

Right adds tape hiss, chorus depth, and the creaking warmth of old circuits.

**young to old.**

Down sounds new and precise.

Up makes the oscillators drift, the envelopes step, the tape wow deepen, and the whole instrument start to feel like something you found in a closet.

Presence and age. Two axes. Every parameter moves at once. I've found more sounds dragging the puck slowly across one diagonal than I ever have opening a menu. The synth reshapes itself under your hand.

---

### arpeggiator

Hold a chord. Turn the arp on. The puck changes what it controls — X sweeps from slow arpeggios to fast scattered patterns, Y moves from tight staccato gates to long swung phrases. Same surface, different instrument. The waveform scope turns blue so you know you're in arp territory.

No extra knobs. No rate dropdown. No swing slider. Just the puck.

---

## okay, fine

The puck is the instrument. The drawer is the workbench. Most of the time the puck is all you need — it maps across every parameter simultaneously and the morph between presets is musical by design. But if you want the filter at exactly 1.4 kHz at midnight for reasons you can't explain, the drawer is there. Open it, make the tweak, close it. The puck keeps working either way, scaling relative to whatever you've set.

### three layers

The sound engine blends three synthesis layers simultaneously.

**analog.** A digitally-controlled oscillator modeled on the one you can find in a Juno-60. The chorus you know is here. You can feel it breathing harder as its age goes up. 

**toy.** A two-operator phase modulation engine modeled on the YM2413 chip. 9-bit DAC quantization, stepped envelopes, snapped operator ratios. It sounds like a $40 keyboard from 1988, which is exactly the point. 

**organ.** A divide-down organ modeled on a Baldwin spinet As age increases, you get key-click bleed and oscillator leakage, the sound of a church organ that hasn't been serviced in decades. 

### tone

Softer resonance than a Moog ladder, warmer self-oscillation, a singing quality at high Q instead of a scream. A Moog-inspired ladder is there too if you want it. Cutoff, resonance, key tracking, envelope depth.

### shape

One macro that sweeps from saw-heavy analog pads through nasal toy leads to organ beds. Or open the drawer and set each layer's level, the oscillator waveform blend, sub-oscillator octave, noise color, FM depth and ratio, and all four drawbars individually.

### motion

LFO with vibrato and pulse-width modulation. Chorus mode selection. Drift amount — how far the oscillators wander before they remember where they're supposed to be. Stereo width. Portamento with legato detection for the melodic lines that need it. Full ADSR envelope.

### etc.

Overdrive modeled on the SD-1 mid-push, cassette tape saturation with hysteresis, wow and flutter from a simulated transport mechanism, a noise floor made of tape hiss, mains hum, mechanical whir, and more buzzwords.

### moment

Press the moment button and waver re-rolls its internal tolerances. Every voice gets slightly different filter offsets, envelope timing, gain levels. The drift patterns change. The RBF morph surface shifts. It's the same patch, but today's version of it. Subtle, but you feel it. Like picking up a guitar that's been restrung.

### true stereo

The chorus develops left and right independently. The print chain processes both channels with their own drift characteristics. Stereo width is real, not a widening trick applied at the output.

---

## what it can sound like

1. **settle** — Warm Juno pad, slow chorus, light tape. Where most people start. [Play]
2. **glow** — Golden and sustained. Long attack, open filter, gentle drift. The sunset preset. [Play]
3. **worn** — Heavy print chain. Cassette compression, hiss floor, wow pulling at the pitch. Intimate and close. [Play]
4. **flicker** — Toy layer forward. Bright, nasal, slightly broken FM melody tones. [Play]
5. **anchor** — Organ bed, 16' and 8' drawbars, maximum gravity. Everything else sits on top of this. [Play]
6. **sway** — Chorus depth turned up, drift wandering. The pad moves under your hands. [Play]
7. **whisper** — Quiet, filtered, hiss-heavy. Like listening through a wall. [Play]
8. **spark** — Toy layer with fast attack, short decay. Percussive digital chime. Arp-ready. [Play]
9. **tide** — Slow arpeggio with long gates and heavy swing. Puck Y up. Patient and swung. [Play]
10. **dusk** — All three layers. Organ below, analog middle, toy on top. Age at 60%. The whole instrument breathing together. [Play]

---

## no reverb

On purpose. Waver makes the sound. Unravel makes the space. They're designed to work together — waver's print chain feeds beautifully into unravel's ghost engine, the tape artifacts dissolving into granular memory. But any reverb you like will work. Waver stays focused on what it does.

---

## specs

**Formats:** VST3, AU, Standalone.

**Platforms:** macOS (Universal binary, arm64 + x86_64), Windows.

**Requirements:** macOS 10.13+ or Windows 10+. A DAW and a MIDI controller for the plugin formats (standalone works without a DAW).

**Polyphony:** 8 voices (DCO and Toy layers) plus full organ polyphony. Voice stealing is gentle — crossfaded, priority-weighted, quiet-voice-first.

**Quality modes:** Lite (no oversampling, for tracking), Standard (2x, recommended), HQ (4x, for final bounce). At Standard settings with a full chord ringing out, CPU is fine. If you run HQ mode with all three layers at max polyphony and the print chain cranked, your fans might have opinions.

---

## $XX [Buy Now]

*macOS 10.13+ or Windows 10+.*

---

## store page description

**Title:**
Threadbare -- Waver

**Short Description:**
Character-driven soft synthesizer with tri-layer architecture: Juno-60-modeled DCO with BBD chorus, YM2413-modeled toy FM, and Baldwin divide-down organ. One 2D puck macro control morphs across the entire parameter space. Built-in cassette print chain with hysteresis saturation, wow/flutter, and tape noise. Deterministic analog drift. Arpeggiator. For indie, dream pop, shoegaze, lo-fi.

**Full Description:**
Every soft synth tries to sound pristine. This one tries to sound like it's been left in a van for ten years.

Tri-layer sound engine: (1) DCO layer — PolyBLEP sawtooth + PolyBLAM pulse with continuous crossfade, sub-oscillator, noise generator, BBD ensemble chorus (3 modes with MN3009 bandwidth limiting, compander noise, 150 Hz sub-bass crossover mono). (2) Toy FM layer — 2-operator phase modulation referencing the YM2413/OPLL, 9-bit DAC quantization, 13 discrete operator ratios, age-coupled envelope stepping. (3) Organ layer — 12 pitch-class phase accumulators with octave division, 4 drawbars (16', 8', 4', mixture), formant bandpass (800-1200 Hz, Q 0.7-1.2), key-click transient, full polyphony with zero voice stealing.

Filter: IR3109 OTA 4-pole lowpass (TPT implementation) as default, Huovilainen Moog ladder as alternate. Per-stage polynomial soft-clip. Oversampled (2x standard, 4x HQ).

Analogization: Per-voice Ornstein-Uhlenbeck deterministic drift (sample-accurate PRNG, Euler-Maruyama discretization), component tolerances (filter cutoff ±3%, resonance ±2%, VCA gain ±0.8 dB, envelope timing ±6%), all seeded from Moment Mode for recallable imperfection.

Print chain (post-mixdown, stereo): SD-1-style asymmetric overdrive with 1 kHz mid-push, tape hysteresis saturation with one-pole 13 kHz head bandwidth, wow (0.5-2 Hz, 0.5-3 ms depth) + flutter (10-100 Hz, 0.02-0.15 ms) via cubic Hermite delay with OU-randomized rates, composite noise floor (tape hiss, 60/50 Hz mains hum, mechanical whir).

One 2D puck macro control. X-axis: Presence (waveform brightness, drive, FM index). Y-axis: Age (tape wear, BBD depth, envelope stepping, OU drift correlation, organ leakage). RBF interpolation across 8-16 preset landmarks in perceptual space (log Hz, log seconds, dB, equal-power mix). Moment Mode re-rolls PRNG seed for new component tolerances and sigma offsets.

Arpeggiator mode: Puck-controlled rate, pattern (Up/Down/Up-Down/Down-Up/Random), gate, and swing. Fixed-size held-note buffer. Deterministic for bounce-in-place. No additional controls — puck X drives rate and pattern, puck Y drives gate and swing.

8-voice polyphony with priority-weighted quiet-voice stealing and 2-5 ms crossfade. Portamento (legato/always, log-frequency domain). Full MIDI: velocity, pitch bend, mod wheel, CC 74 (puck X), CC 11 (puck Y), aftertouch, sustain. 46 automatable parameters. 40-60 factory presets across 5 categories (amber, drift, hush, signal, weight).

No built-in reverb by design — pairs with Threadbare Unravel or any reverb plugin.

Formats: VST3, AU, Standalone. Platforms: macOS (Universal binary, arm64 + x86_64), Windows. Built with JUCE 8, C++20. Quality modes: Lite (1x), Standard (2x OS), HQ (4x OS) with automatic latency reporting.

**Keywords:**
synthesizer, soft synth, analog modeling, virtual analog, Juno-60, Roland Juno, DCO, BBD chorus, bucket brigade, FM synthesis, phase modulation, YM2413, OPLL, toy keyboard, Yamaha PSS, divide-down organ, Baldwin organ, drawbar organ, tape saturation, cassette, wow flutter, lo-fi synth, lo-fi, analog drift, Ornstein-Uhlenbeck, deterministic, PolyBLEP, PolyBLAM, OTA filter, IR3109, Moog ladder, TPT filter, oversampling, 2D puck, morph, RBF interpolation, arpeggiator, dream pop, shoegaze, indie, ambient, Grandaddy, Jason Lytle, retro synth, vintage synth, warm pad, character synth, VST3, AU, audio unit, standalone, macOS, Windows, JUCE, audio plugin, synth plugin
