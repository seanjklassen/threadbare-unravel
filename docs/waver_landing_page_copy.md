# threadbare: waver

---

> [VISUAL] dark screen. one moving waveform scope. no controls.

## meet waver.

---

> [VISUAL] scope motion gets slightly unstable as a held chord sustains.

## a broken but beautiful polysynthesizer.

---

> [VISUAL] subtle detune and phase spread appear in the waveform.

I kept trying to make modern synths feel like the keyboards on records I love. I could add drift later. I could add hiss later. It still felt like makeup.

waver is a synthesizer where the life comes from inside.

the drift is in the oscillators. the breathing is in the chorus.

every voice moves independently.

---

> [VISUAL] puck appears. surrounding UI is still hidden.
> [INTERACTION] drag puck over one sustained chord and one rhythmic phrase.

## one main control. 

**focused to warm**  
left stays present and defined. right introduces chorus depth, tape tone, and softness.

**steady to restless**  
down stays controlled. up adds drift, wow, movement, and looseness.

I have found more sounds by dragging one diagonal slowly than by opening ten menus.

---

> [VISUAL] three sonic states crossfade as the puck moves: warm pad, digital chime, organ bed.

## three lineages. one instrument.

Its voice draws on analog polysynths, consumer FM keyboards, and divide-down organs. Not as separate modes. Fused into one continuous surface you can move through in real time.

> [CAPTION] warmth, edge, and weight from different decades. one gesture to move between them.

---

> [VISUAL] arp mode toggles on. scope tint shifts blue.
> [INTERACTION] hold chord, then drag puck to change rhythm and gate.

## hold a chord. turn arp on.

Same puck, different job. X moves from slower patterns to scattered motion. Y moves from short gates to longer swung phrases.

No extra knobs. No rate dropdown. No swing menu. it automatically adjusts based on the settings in your DAW.

---

> [VISUAL] seed value icon rolls, tiny per-voice offsets appear as animated micro-variance.

## every instance has its own personality.

Moment seeds internal tolerances so two fresh loads of the same patch are not identical. Subtle filter offsets. Slight envelope differences. Different drift paths.

Like two takes from the same player on different days. Related. Not cloned.

Save your session and it recalls exactly. Start a new one and it breathes a little differently.

---

> [VISUAL] drawer reveals in one clean panel.

## but wait, there is more.

The puck is the instrument. The drawer is the workbench. If you need exact values, it is there. If you do not, close it and keep moving.

### tone and character

- **filter:** cutoff, resonance, key tracking, envelope depth.
- **drive and tape:** overdrive, saturation, hiss, wow, flutter.
- **stereo life:** chorus depth and width that move per channel.

### harmonic shape

- **layer blend:** analog body, digital edge, organ weight.
- **oscillator detail:** waveform blend, sub octave, noise color.
- **fm color:** depth and ratio for toy-like or glassy motion.

### movement and response

- **drift:** how far voices wander before they settle.
- **envelopes:** full ADSR for contour and punch.
- **portamento:** glide behavior with legato support.

---

> [VISUAL] preset list appears with persistent play controls.
> [INTERACTION] click name to audition phrase loops.

## listen to it.

1. **foundation** `settle` warm pad, slow chorus, light tape. [Play]
2. **glow** `glow` sustained, golden, and patient. [Play]
3. **wear** `worn` compressed, hissy, gently warped pitch. [Play]
4. **edge** `flicker` bright, nasal, slightly wrong in the best way. [Play]
5. **weight** `anchor` deep harmonic bed that everything else can sit on. [Play]
6. **motion** `sway` wandering chorus and drifting center. [Play]
7. **distance** `whisper` filtered and quiet, like sound through a wall. [Play]
8. **attack** `spark` fast, percussive, chime-forward. [Play]
9. **pattern** `tide` slow arp with longer gates and swing. [Play]
10. **whole** `dusk` all layers breathing together at once. [Play]

---

> [VISUAL] simple pairing card with two product marks.

## no reverb. on purpose.

waver focuses on making the sound. Space is a separate choice. Pair it with unravel if you want the full threadbare lane, or use any reverb you already trust.

---

## specs

**Formats:** VST3, AU, Standalone  
**Platforms:** macOS (Universal binary, arm64 + x86_64), Windows  
**Requirements:** macOS 10.13+ or Windows 10+; DAW for plugin formats (standalone works without one)  
**Polyphony:** 8 voices (DCO and Toy layers) plus full organ polyphony with gentle voice stealing  
**Quality modes:** Lite (1x), Standard (2x), HQ (4x)

At Standard settings, CPU is fine. If you run HQ with every layer wide open, your fans may file a formal complaint.

---

## $XX [Buy Now]

*macOS 10.13+ or Windows 10+.*

---

## appendix: store page description

**Title:**  
threadbare: waver

**Short Description:**  
A polysynthesizer that sounds tenderly alive. One 2D puck reshapes the entire instrument, from warm analog pads to bright digital chimes to deep harmonic beds, all with independent drift and built-in analog character. Includes puck-controlled arpeggiator. For indie, dream pop, shoegaze, and lo-fi.

**Full Description:**  
Most soft synths optimize for perfect repeatability. waver optimizes for movement and personality.

The engine fuses three layers: (1) DCO layer with PolyBLEP sawtooth, PolyBLAM pulse, sub oscillator, noise source, and BBD ensemble chorus. (2) Toy FM layer with 2-operator phase modulation inspired by YM2413/OPLL behavior, 9-bit DAC quantization, and stepped character. (3) Organ layer with divide-down topology, drawbar control, and key-click transient behavior.

Filter section: IR3109 OTA 4-pole lowpass (TPT) as the default voice, plus Huovilainen Moog ladder as an alternate model. Per-stage polynomial soft clip. Oversampled at 2x Standard and 4x HQ.

Analogization: per-voice Ornstein-Uhlenbeck deterministic drift with component tolerances (filter cutoff, resonance, VCA gain, envelope timing), seeded through Moment for recallable imperfection.

Print chain (post mix, stereo): SD-1-style asymmetric overdrive, tape hysteresis saturation, wow and flutter via cubic Hermite delay with randomized rates, and a composite noise floor (tape hiss, mains hum, mechanical whir).

One 2D puck macro control. X-axis: Presence (brightness, drive, FM index). Y-axis: Age (tape wear, chorus depth, drift character, leakage). RBF interpolation across preset landmarks in perceptual space.

Arpeggiator mode is also puck-controlled: rate, pattern behavior, gate, and swing without separate controls.

Polyphony and control: 8-voice polyphony with priority-weighted quiet-voice stealing and short crossfade, portamento (legato/always), full MIDI expression support, and 47 automatable parameters.

No built-in reverb by design. Pair with threadbare unravel or any reverb chain you prefer.

Formats: VST3, AU, Standalone. Platforms: macOS (Universal binary, arm64 + x86_64), Windows. Built with JUCE 8, C++20.

**Keywords:**  
synthesizer, soft synth, analog modeling, virtual analog, DCO, BBD chorus, FM synthesis, phase modulation, YM2413, OPLL, divide-down organ, drawbar organ, tape saturation, cassette tone, wow flutter, lo-fi synth, analog drift, Ornstein-Uhlenbeck, PolyBLEP, PolyBLAM, OTA filter, IR3109, Moog ladder, TPT filter, oversampling, 2D puck, morph, RBF interpolation, arpeggiator, dream pop, shoegaze, indie, ambient, retro synth, warm pad, character synth, VST3, AU, audio unit, standalone, macOS, Windows, JUCE, audio plugin, synth plugin