*waver* by
THREADBARE

Complete Architectural Specification

*A Character-Driven Soft Synthesizer in JUCE 8*

*"broken but beautiful synthesis."*

*Digital Signal Processing, Interface Design, and Development Methodology*

Version 6.0  —  February 2026

threadbare audio

Emotional targets: *Weathered · Tender · Breathing*

**Contents**

# **1\. Design Philosophy and Sonic Identity**

This specification defines the complete architecture for **waver**, a software synthesizer built under the threadbare brand identity. The instrument captures the lo-fi, nostalgic, and deeply emotive sonic signature associated with indie rock artists such as Jason Lytle of Grandaddy, where consumer-grade keyboards, tape machines, and aging analog circuits converge into something greater than the sum of their parts.

## **1.1 Brand Identity and Product Metadata**

**waver** — the name describes exactly what the instrument does. Oscillators waver. Tape speed wavers. Pitch drifts and pulls back. The word also captures the emotional register of the music this synthesizer exists to create: the constant hovering between hope and resignation, certainty and doubt.

The tagline: **"broken but beautiful synthesis."**

Three emotional targets serve as the north star for all design decisions: **Weathered** (the tape, the drift, the controlled imperfection), **Tender** (the emotional register is never aggressive, even when distorted), and **Breathing** (the sound is never static—OU drift, chorus modulation, and flutter keep everything alive).

| Element | Value |
| :---- | :---- |
| Product name | waver |
| Tagline | broken but beautiful synthesis. |
| Emotional targets | Weathered · Tender · Breathing |
| Accent color | \#C4A46C (dusty amber / warm ochre) |
| Foundation background | \#241D19 (vintage cassette shell) |
| Text / icons | \#D3C7BB (faded label / warm off-white) |
| Plugin dimensions | \~380–420 px wide, \~670–720 px tall |
| CMake target | ThreadbareWaver |
| Plugin code | Wavr |
| Bundle ID (VST3) | com.threadbare.waver.vst3 |
| Bundle ID (AU) | com.threadbare.waver.component |

## **1.2 Core Design Principles**

**Constraint as creative catalyst.** Every design decision should narrow the possibility space toward musicality rather than expand it toward complexity. The architecture revolves around one heroic interaction loop: Load preset → move puck → record automation → commit ("print"). Everything else must strictly support this loop, or remain hidden.

**Imperfection as identity.** The target sound is the cumulative result of specific analog architectures, cost-reduced digital synthesis chips, divide-down organ circuits, and budget home-recording artifacts. Age is treated as a first-class synthesis dimension, not just an effect knob.

**Playability over programmability.** The instrument must be immediately rewarding to a musician who has never opened a synthesizer manual. The primary interface is a 2D morphing controller, not a parameter grid. Deep editing is available but hidden.

**No reverb by design.** waver deliberately excludes built-in reverb. Spatial duties are handled by **unravel** or the user’s preferred reverb plugin. This keeps waver lean and reinforces the threadbare product ecosystem — each instrument has a focused purpose rather than duplicating functionality.

## **1.3 Sonic DNA: Deconstructing the Target Aesthetic**

The "Lytle sound" is identifiable by several converging characteristics:

* **Polyphonic pads with internal motion.** Sustained chords that shimmer and breathe, driven by BBD chorus artifacts and slow oscillator drift. The Juno-60 is the primary source, but the movement is never pristine—it passes through tape and room before reaching the listener.

* **Toy keyboard melodic lines.** Bright, nasal, slightly broken-sounding leads from consumer Yamaha FM keyboards (PSS-270, PSS-480). These sit on top of the pad layer with percussive attack and limited sustain.

* **Organ beds providing harmonic gravity.** Dense, static, fully polyphonic organ tones (Baldwin-style divide-down) that anchor everything below. These never modulate—they simply exist as harmonic infrastructure.

* **The "print" layer.** Everything passes through simulated budget recording gear: cassette saturation, wow and flutter, hiss, and the mid-focused compression of consumer tape. This is not optional post-processing—it is fundamental to the identity.

# **2\. The Tri-Layer Sound Engine**

The sound engine is structured as three distinct, blendable synthesis layers that run concurrently within the voice architecture. Each layer addresses a different sonic role and is modeled on specific hardware. The layers are mixed via a dedicated voice-level crossfader before entering the shared filter and effects chain.

## **2.1 The DCO Polyphonic Layer (Juno-60 Reference)**

The foundation layer is a digitally controlled oscillator bank inspired by the Roland Juno-60. The goal is not circuit-level emulation but behavioral replication: ultra-stable pitch generation combined with warm filtering, constrained modulation, and the expansive stereo width created by its bucket-brigade chorus.

**2.1.1 Oscillator Architecture**

The voice chain consists of a primary oscillator generating bandlimited sawtooth and variable-width pulse waves, a sub-oscillator (sine or triangle, one to two octaves below fundamental), and a dedicated noise generator with variable color (white through pink filtering).

The primary oscillator’s waveform is a **continuous blend of saw and pulse**: a single mix parameter (derived from the Shape macro and/or RBF) crossfades between the two. There is no discrete saw/pulse switch; the blend is smooth over the full range (0 \= saw, 1 \= pulse). Saw/pulse crossfade should use equal-power or linear mix so level stays even across the range.

Waveforms must be generated using **Polynomial Bandlimited Step (PolyBLEP)** for the sawtooth and **Polynomial Bandlimited Amplitude (PolyBLAM)** for the pulse wave. PolyBLAM corrects the amplitude-dependent discontinuities at both rising and falling edges of the pulse, which is essential for clean pulse-width modulation.

**2.1.2 Modulation Architecture**

A single LFO provides pitch modulation (vibrato) and pulse-width modulation. A single ADSR envelope is routable to both filter cutoff and VCA amplitude. The LFO offers triangle, sine, square, and sample-and-hold waveforms at 0.1–30 Hz.

**2.1.3 The BBD Ensemble Chorus**

The chorus is almost entirely responsible for the perceived warmth and stereo width of the Juno series. The digital implementation must model dual modulated delay lines:

| Chorus Mode | Target Rate | Delay Range | Phase | Character |
| :---- | :---- | :---- | :---- | :---- |
| Mode I | \~0.50 Hz | \~1.6–5.3 ms | 180° Inverted | Gentle, wide stereo |
| Mode II | \~0.83 Hz | \~1.6–5.3 ms | 180° Inverted | Faster, more obvious |
| Mode I+II | \~1.0–8.0 Hz | \~3.3 ms | In Phase | Vibrato/ensemble |

*These are starting targets for gray-box tuning against hardware recordings, not exact specifications.*

Implementation requirements beyond simple digital chorus:

* **BBD bandwidth limiting.** Low-pass filter in the feedback path simulates MN3009 high-frequency degradation (\~10 kHz usable bandwidth at typical clock rates).

* **Compander noise floor.** An envelope follower on the input modulates the amplitude of injected BBD hiss. As the Age (Y-axis) parameter increases, the compander noise breathes more heavily.

* **Sub-bass crossover.** Frequencies below \~150 Hz are kept strictly mono; spatial widening applies only to upper harmonics.

* **Fractional delay interpolation.** Per Iron Laws: cubic Hermite or allpass interpolation only. Linear interpolation is forbidden.

## **2.2 The Toy Keys Layer (YM2413/OPLL Reference)**

The second layer introduces the lo-fi digital charm of consumer keyboards from the late 1980s, referencing the Yamaha PSS-270 and its YM2413 (OPLL) sound chip.

**2.2.1 Phase Modulation Architecture**

The OPLL implements phase modulation (PM), not true FM. The true "toy" character comes from operator limits, envelope stepping, limited ratios, and DAC quantization artifacts rather than sideband symmetry differences. The DSP implementation uses PM:

*output(t) \= sin(2π · fc · t \+ I · sin(2π · fm · t \+ feedback))*

**2.2.2 DAC Emulation and Artifacts**

* **Bit-depth quantization.** Output quantized to configurable bit depth (default 9-bit, matching YM2413 hardware).

* **Operator ratio quantization.** Ratios snap to discrete values available on the chip: 0.5, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 15\.

* **Envelope stepping (Age-coupled).** As the Age parameter increases, attack/decay/release curves exhibit increasingly obvious, degraded stairstepping.

## **2.3 The Organ Bed Layer (Baldwin Divide-Down Reference)**

The final synthesis layer provides static harmonic infrastructure. No voice stealing for the organ layer; however, the organ generates only active notes, preserving divide-down identity and perfect phase-lock by deriving all octaves from shared pitch-class phase accumulators.

**Iron Law compliance:** All 12 pitch-class phase accumulators and all octave dividers must be **pre-allocated** at plugin initialization (e.g. a fixed std::array\<PhaseAccumulator, 128\> covering all MIDI notes). Note-on only activates an index; note-off deactivates it. No containers that grow (no push\_back, no new) are permitted in the audio path. This is the primary allocation risk for the organ layer.

**2.3.1 Drawbar Controls**

* **16′ Sub-fundamental** — one octave below the played note

* **8′ Fundamental** — the primary pitch

* **4′ Second harmonic** — one octave above, adding brightness

* **Mixture** — upper partials (5⅓′, 2⅔′, 2′) as a combined registration

Each drawbar is a 0–8 level control (Hammond convention). Output is processed through a gentle fixed-bandpass formant filter (center \~800–1200 Hz, Q \~0.7–1.2).

**2.3.2 Age Coupling**

A subtle mechanical key-click transient is generated on note-on. As the Age parameter (Y-axis) increases, both the key-click volume and subtle oscillator "leakage" (a faint continuous bleed of phase accumulators into the audio path) increase, making the organ feel physically older and less maintained.

# **3\. Voice Architecture and Polyphony Management**

## **3.1 Polyphony Strategy**

The synthesizer supports 8 polyphonic voices for the DCO and Toy layers (which share voice allocation), plus dynamic phase-accumulator pseudo-polyphony for the Organ layer. Eight voices represents the sweet spot: enough for rich chords, few enough for per-voice analogization without exceeding the CPU budget.

## **3.2 Voice Stealing Algorithm**

When all 8 voices are active and a new note-on arrives, the allocator uses a priority-weighted quiet-voice approach:

1. If any voice is in its release phase, steal the one with the lowest current amplitude.

2. If no voices are releasing, steal the voice with the longest elapsed time since note-on (LRU).

3. If multiple voices tie on elapsed time, steal the one with the lowest current amplitude.

4. Apply a fast crossfade (2–5 ms) to the stolen voice before reassigning to prevent click artifacts.

## **3.3 Portamento**

**Restored from V3 — this is essential for the Lytle sound.** Melodic toy-keyboard lines and expressive pad transitions require pitch glide.

Portamento is implemented as a one-pole low-pass filter on the target pitch value, with a glide time controllable from 0 ms to 2000 ms. Two modes are provided:

* **Legato:** Glide only occurs when a new note is played while a previous note is still held. Envelopes retrigger only on the first note of a legato phrase, allowing smooth melodic lines without percussive re-attacks.

* **Always:** Every note glides from the previous pitch regardless of note overlap.

The glide rate is constant in the log-frequency domain (equal time per octave), which produces musically natural portamento regardless of the interval size.

## **3.4 MIDI Implementation**

The following MIDI mappings are supported:

| MIDI Input | Destination | Range / Notes |
| :---- | :---- | :---- |
| Note On/Off | Voice allocation | Velocity maps to VCA gain and filter envelope depth |
| Pitch Bend | Oscillator pitch | ±2 semitones default (configurable ±1 to ±12) |
| Mod Wheel (CC 1\) | LFO depth | 0–127 maps to 0–100% LFO intensity |
| CC 11 (Expression) | Puck Y-axis | Allows hardware control of the Age axis |
| CC 74 (Brightness) | Puck X-axis | Allows hardware control of the Presence axis |
| Aftertouch (Channel) | Filter cutoff | Adds positive offset to current filter setting |
| Sustain (CC 64\) | Voice release | Standard sustain pedal behavior |

*CC 74 and CC 11 are chosen because they are MPE-compatible and commonly available on modern controllers.*

## **3.5 Arpeggiator Mode**

Arpeggiator mode is waver’s counterpart to Unravel’s disintegration looper: a single, distinctive bonus feature that extends the plugin without defining it. When enabled, held notes are arpeggiated in a pattern and rate derived from the **puck**, then fed into the existing voice allocator and print chain. The result is the same broken-but-beautiful character in a repeating, playable pattern—ideal for Lytle-style pad hooks and toy-keyboard figures. **No new sliders or drawer controls:** when the arp is on, the puck's X and Y axes drive arp parameters (rate, pattern, gate, swing) instead of—or in addition to—the usual RBF/presence/age mapping.

**Behavior**

* **Input:** MIDI note-on events add notes to a **held-note buffer**; note-off removes them. When the arpeggiator is off, notes pass through to the voice allocator as normal and the puck behaves as in Section 7.3.
* **When on:** A timing engine (rate or host-synced) advances a step. Each step selects one note from the held-note buffer according to the current **pattern**, triggers a note-on (or note-off-then-note-on) to the voice allocator, and holds it for a **gate** duration (percentage of step length). After gate, the note is released before the next step. Pattern, rate, gate, and swing are **puck-controlled** (see **Puck mapping when arp on** below).
* **Patterns:** Up, Down, Up-Down, Down-Up, Random (deterministic, PRNG seeded by Moment so bounce-in-place matches). Pattern order is computed from the current set of held notes (sorted by pitch where applicable). Pattern selection is derived from puck position when arp is on.
* **Rate:** Free-running (0.5–32 Hz) or host tempo sync (1/4, 1/8, 1/16, 1/32 note). Rate-dependent constants recalculated in prepareToPlay(sampleRate) per Section 4.1.1. Rate is derived from puck position when arp is on.
* **Gate:** 5–95% of step length. Gate is derived from puck position when arp is on.
* **Swing:** Even steps vs. swung (e.g. 60–75% delay on every other step). Subtle—never aggressive. Swing amount is derived from puck position when arp is on; higher Y can increase swing for a more “worn” feel, consistent with Age.
* **Iron Law compliance:** The held-note buffer is a fixed-size array (e.g. max 16 notes). No dynamic allocation. Step phase and pattern index are state variables updated sample-accurately or block-accurately from a deterministic clock. Any “humanize” (e.g. micro-timing jitter) must use the same PRNG/seed as the rest of the engine and be saved in DeterminismState if it affects audio.

**Puck mapping when arp on**

When the arpeggiator is enabled, the puck's two axes control the four arp dimensions (and retains where the sound settings were at in default mode) so the user shapes the arp from the same surface they use for the rest of the sound. The color of the waverform visualization changes to a blue to help indicate arp mode:

* **Puck X:** Drives **rate** (left = slower, right = faster) and **pattern** (e.g. left = Up, centre = Up-Down, right = Random; or a continuous blend between pattern types). Exact curve is defined in WaverTuning.h so one gesture sweeps both dimensions in a musical way.
* **Puck Y:** Drives **gate** (bottom = shorter gate, top = longer) and **swing** (bottom = straight timing, top = more swing). Keeps the "weathered" association of Y with imperfection and age.

Implementations may use the raw puck coordinates (puckX, puckY) or the same RBF-interpolated values that drive the rest of the engine; if RBF is used, the arp shares the preset's morph surface. In all cases, no extra arp controls are added to the Advanced Drawer.

**UI**

* **Toggle only:** An arpeggiator button (e.g. in the bottom strip or near the puck, analogous to Unravel’s looper button). State (on/off) is saved in the DAW session; it may be a non-automatable preference or an APVTS bool, per product choice. When on, the user moves the puck to change rate, pattern, gate, and swing—no additional knobs or sliders (see **Puck mapping when arp on** below).

**Determinism**

If the arp uses the same PRNG stream as the rest of waver (e.g. for Random pattern or humanize), its state and seed must be included in DeterminismState so that offline and realtime bounces remain bit-identical.

# **4\. The Analogization System: Deterministic Imperfection**

The analogization layer introduces compounded microscopic variances. To ensure Moment Mode works seamlessly in a DAW while passing rigorous bounce-in-place testing, these imperfections are entirely deterministic.

## **4.1 Deterministic Stochastic Pitch Drift (Ornstein-Uhlenbeck)**

Drift is modeled using the Ornstein-Uhlenbeck (OU) process, a stationary Gauss-Markov process that is uniquely mean-reverting. The continuous-time SDE is discretized using Euler-Maruyama as a leaky integrator with noise injection:

*x\[n\] \= a \* x\[n-1\] \+ b \* noise\[n\]*

**Determinism Rule:** The PRNG feeding the Wiener process must be seeded and advanced per-sample (not per-block), ensuring offline DAW renders perfectly match realtime playback. The PRNG seed and current OU state are saved in session state.

**Age Coupling:** As the Age (Y-axis) parameter increases, the OU correlation time changes—the drift becomes slower, stickier, and exhibits deeper excursions before mean-reverting.

**4.1.1 Sample-Rate Adaptation**

**Critical addition.** The OU coefficients, LFO rates, SmoothedValue ramp times, and all rate-dependent constants must be recalculated in prepareToPlay(sampleRate). The WaverTuning.h header exposes a recalculate(double sampleRate) method:

// WaverTuning.h — excerpt

namespace threadbare::tuning::waver {

  struct RateDependent {

    double ouAlpha;     // leaky integrator coeff (closer to 1.0 \= slower drift)

    double ouBeta;      // noise injection scale

    double smoothRampMs \= 15.0;  // SmoothedValue ramp time

    int    oversampleFactor \= 2; // 2x default, 4x in HQ mode

  };

  RateDependent recalculate(double sampleRate);

}

At 44.1 kHz: a ≈ 0.9997, b ≈ 0.0003 (drift time constant \~7 seconds). At 96 kHz these must be recalculated to produce the same perceptual drift rate: a ≈ 0.99986, b ≈ 0.00014. Without this, the instrument sounds different at different sample rates.

**4.1.2 Drift Routing**

Each voice’s independent OU generator outputs are routed to:

* **Oscillator pitch:** ±2 to ±8 cents (primary drift target)

* **Filter cutoff:** ±0.5% to ±2% of current cutoff value

* **Envelope time constants:** ±1% to ±4% variation on attack and release

* **Chorus delay time:** ±0.01 to ±0.05 ms (adds organic variation to BBD modulation)

## **4.2 Voice-to-Voice Component Tolerances**

Fixed randomized offsets are assigned per voice based on the patch’s saved PRNG seed:

| Parameter | Tolerance Range | Audible Effect |
| :---- | :---- | :---- |
| Filter Cutoff | ±1.0% to ±3.0% | Subtly different timbral color per voice |
| Filter Resonance | ±1.0% to ±2.0% | Resonance peaks vary slightly |
| VCA Gain | ±0.2 dB to ±0.8 dB | Voices not perfectly level-matched |
| Envelope Attack | ±2.0% to ±6.0% | Transients feel slightly "human" |
| Envelope Release | ±2.0% to ±6.0% | Decay tails vary naturally |

# **5\. The Print Chain: Tape Wear and Output Processing**

A built-in effects chain at the output stage "prints" the synthesized sound to a simulated physical medium. This chain operates post-voice-mixdown (stereo, not per-voice), keeping CPU costs manageable. **The print chain is not optional post-processing — it is fundamental to waver’s identity.** The chain processes in this fixed order: Overdrive → Cassette Saturation → Wow/Flutter → Noise Floor.

## **5.1 Overdrive and Mid-Push**

A distortion module modeled on asymmetric clipping circuits like the Boss SD-1 Super Overdrive. The SD-1 is characterized by deliberate mid-frequency emphasis: a wide bandwidth peak centered around 1000 Hz with gentle rolloff of highs and lows.

**5.1.1 Signal Flow**

1. **Pre-emphasis EQ.** A second-order bandpass filter (center \~1 kHz, Q \~0.8) shapes the frequency response before clipping. This creates the characteristic mid-push that helps the synth cut through a dense mix.

2. **Asymmetric waveshaping.** Different saturation curves for positive and negative signal excursions generate even-order harmonics (2nd, 4th, 6th), perceived as "warm" musical distortion. Implementation uses:

  positive: y \= tanh(gain \* x)

  negative: y \= tanh(gain \* x \* 0.7)  // reduced negative clipping

3. **Post-emphasis LPF.** A gentle 6 dB/octave low-pass at \~5 kHz tames the upper harmonics generated by the nonlinearity.

4. **Dry/wet mix.** The overdrive gain and mix are controlled by the "grit" parameter in the Print drawer group.

## **5.2 Cassette Saturation**

A soft tape saturation algorithm that dynamically compresses fast transients and introduces harmonic distortion based on program level, mimicking the limited headroom of iron oxide cassette tape (approximately 0 dBu before audible saturation on Type I tape).

The saturation curve models simplified tape hysteresis, where the magnetization of the oxide layer depends on the signal’s history, not just its current value. Implementation:

* **Hysteresis approximation.** A soft-clipping function with an asymmetric transfer curve and a state variable tracking the previous sample’s output (one-sample feedback). The feedback coefficient controls how "sticky" the tape saturation feels.

* **Tape head bandwidth.** A one-pole low-pass pre-filter at \~13 kHz simulates the finite bandwidth of a consumer cassette head, rolling off highs before they reach the saturation stage.

* **Level-dependent harmonics.** At low input levels, the tape is nearly transparent. As level increases, odd-order harmonics appear first (3rd), then even-order (2nd, 4th) at higher drive. This progression is characteristic of magnetic tape and differs from transistor clipping.

## **5.3 Wow and Flutter**

Algorithmic modeling of mechanical imperfections in the tape transport system. These are arguably the most emotionally evocative artifacts in the entire print chain.

**5.3.1 Implementation Detail**

Both wow and flutter are implemented as modulation of a single stereo fractional delay line. The total modulation signal is the sum of periodic and stochastic components:

* **Wow (0.5–2.0 Hz):** A low-frequency sine wave with added low-pass filtered noise (cutoff \~3 Hz). Modulation depth: 0.5–3.0 ms. This produces the slow pitch undulation of irregular capstan rotation.

* **Flutter (10–100 Hz):** A higher-frequency sine (typically \~25 Hz primary) plus filtered noise (cutoff \~120 Hz). Modulation depth: 0.02–0.15 ms. This produces the rapid, subtle pitch instability of tape scrape.

The delay line must use **cubic Hermite interpolation** (per Iron Laws — linear interpolation is forbidden). The periodic components should have slightly randomized frequencies that drift by ±5–10% over time (using a slow OU process) to prevent the modulation from sounding artificially regular.

**5.3.2 Age Coupling**

As the Age (Y-axis) parameter increases, wow and flutter depth increase non-linearly (an equal-power curve), the stochastic noise component becomes proportionally larger relative to the periodic component (the tape transport becomes less mechanically reliable), and occasional "dropout" events (brief amplitude dips of 1–3 dB lasting 20–80 ms) are introduced at a rate proportional to Age.

## **5.4 Aged Noise Floor**

A subtle, constant injection of shaped noise that ties the sound to physical reality. The noise floor is a composite of three sources:

* **Tape hiss.** Pink-filtered white noise at \-60 to \-48 dBFS (controllable via Print drawer). Level is subtly modulated by an envelope follower on the program signal, simulating compander behavior.

* **AC mains hum.** A 60 Hz tone (user-switchable to 50 Hz for European installations) with 2nd and 3rd harmonics, at approximately \-72 dBFS. Extremely subtle but ties the sound to a physical electrical reality.

* **Mechanical whir.** Low-level randomized tone in the 100–300 Hz range, simulating motor and transport mechanism noise. Amplitude modulated by a slow random process (\~0.3 Hz) to simulate irregular motor speed.

# **6\. High-Fidelity DSP Architecture in JUCE 8**

This section follows the threadbare Architecture Patterns and Iron Laws.

## **6.1 The Iron Laws of Real-Time Safety**

All code in Source/DSP/ is pure C++20, agnostic of JUCE UI headers. The following rules apply unconditionally to processBlock and all audio-thread logic:

1. **No allocations.** Never new, malloc, push\_back, or std::string creation.

2. **No locks.** Never std::mutex or CriticalSection. Use lock-free FIFO structures (StateQueue).

3. **No exceptions.** Disable or guarantee they cannot be thrown on the audio thread.

4. **Numeric stability.** juce::ScopedNoDenormals at the start of every process block.

5. **Smoothing.** juce::SmoothedValue for all gain, mix, and morphing parameters.

6. **Interpolation.** Cubic Hermite or allpass for all delay line reads. Linear interpolation is forbidden.

7. **Per-sample updates.** Pitch drift, delay time, and size parameters update every sample, not once per block.

## **6.2 Filter Lineage and Oversampling**

The primary DCO filter defaults to an **IR3109 OTA-based 4-pole lowpass** model, matching the Juno-60’s actual filter chip lineage. The Huovilainen Moog ladder filter is retained as a switchable alternate mode.

**6.2.1 IR3109 OTA Filter Implementation**

The IR3109 is an operational transconductance amplifier (OTA) based filter, not a transistor ladder. The topology is four cascaded OTA integrator stages in a Sallen-Key-like configuration with global negative feedback for resonance. Key behavioral differences from the Moog ladder:

* **Softer saturation.** OTA limiting is gentler than transistor ladder tanh saturation. The transfer function uses a polynomial soft-clip approximation rather than tanh, producing a rounder distortion at high drive levels.

* **Resonance character.** The IR3109 resonance is less aggressive and "screamy" than a Moog ladder. At high Q values it produces a warm, singing quality rather than a sharp, piercing whistle.

* **Self-oscillation behavior.** The filter can self-oscillate but the oscillation is softer and more sine-like than a Moog ladder, which tends toward a more triangular self-oscillation waveform.

The recommended DSP implementation uses the **Zavalishin Topology-Preserving Transform (TPT)** approach for the OTA integrator stages, with a custom soft-clip function applied at each stage output. This provides zero-delay feedback behavior without the Moog-specific tanh saturation:

// Per-stage processing (simplified)

v \= (input \- z1) \* g / (1.0 \+ g);    // TPT integrator

y \= softclip(v \+ z1);                 // OTA saturation

z1 \= y \+ v;                           // state update

Where g is the integrator gain coefficient derived from cutoff frequency, and softclip is a polynomial approximation that is cheaper than tanh while matching OTA character more closely.

**6.2.2 Oversampling Strategy**

Oversampling instances (juce::dsp::Oversampling) are wrapped strictly and locally around the nonlinear blocks: the OTA/Ladder filters and the tape saturation module. IIR polyphase filtering is used for minimum latency.

**Latency reporting (critical).** The total oversampling latency must be reported to the DAW via setLatencySamples() in prepareToPlay(). Without this, waver will be slightly ahead of every other track in the session. The latency is the sum of all active oversampling stages. **This must also be updated whenever the quality mode changes** (Lite/Standard/HQ), because each mode uses different oversampling factors. There is no existing pattern for this in the Unravel codebase (Unravel uses no oversampling and reports zero latency):

void prepareToPlay(double sampleRate, int samplesPerBlock) {

    filterOversampling.initProcessing(samplesPerBlock);

    tapeOversampling.initProcessing(samplesPerBlock);

    int totalLatency \= filterOversampling.getLatencyInSamples()

                     \+ tapeOversampling.getLatencyInSamples();

    setLatencySamples(totalLatency);

}

## **6.3 Tuning Abstraction**

No magic numbers in DSP code. All constants live in plugins/waver/Source/WaverTuning.h under the threadbare::tuning::waver namespace. Each constant includes a comment describing its audible effect and safe bounds. The recalculate(sampleRate) method is called from prepareToPlay() to adapt all rate-dependent values.

## **6.4 CPU Budget**

**Target: 8 voices at 44.1 kHz / 512-sample buffer under 15% of a single core on Apple M1.** This budget must be validated in the CI pipeline using JUCE’s PerformanceCounter. Breakdown:

| Component | Budget | Notes |
| :---- | :---- | :---- |
| DCO oscillators (8 voices) | \~2% | PolyBLEP/BLAM is very cheap |
| Toy FM engine (8 voices) | \~2% | Two operators per voice, minimal |
| Organ layer (all notes) | \~1% | Phase accumulators, no filtering per-voice |
| OTA filter (8 voices, 2x OS) | \~4% | Most expensive per-voice component |
| OU drift (8 generators) | \<1% | Trivial arithmetic per sample |
| Print chain (stereo, 2x OS) | \~3% | Post-mixdown, not per-voice |
| RBF interpolation | \<1% | Runs at UI rate, not audio rate |
| Headroom | \~2% | Buffer for DAW overhead and spikes |

At 96 kHz the budget roughly doubles. At 4x oversampling, filter and tape costs approximately double again. The 15% target at 2x oversampling is ambitious; to ensure the plugin remains usable on a wider range of hardware, waver must ship with explicit quality modes:

| Mode | Filter OS | Tape OS | CPU Target | Notes |
| :---- | :---- | :---- | :---- | :---- |
| Lite | None (1x) | None (1x) | \~8% | For tracking / low-latency monitoring |
| Standard (default) | 2x | 2x | \~15% | Recommended for production |
| HQ | 4x | 4x | \~25% | For final bounce / offline render |

Mode is selectable in a settings popover (not the drawer — it’s a global preference, not a sound design parameter). Changing mode triggers prepareToPlay() to reinitialize oversampling stages and update setLatencySamples(). The current mode is **not** saved in the preset — it is a user preference stored in the plugin’s global config.

# **7\. User Interface: The Puck and Lovable Design**

## **7.1 WebView Architecture**

The UI is built with standard web technologies via JUCE 8’s WebBrowserComponent. The frontend is built with Vite into a single index.html, embedded as binary data (WaverResources), and loaded via WebBrowserComponent::getResourceProviderRoot().

**Lock-free audio/UI communication:**

* **Audio to UI:** The DSP thread constructs a trivially-copyable WaverState struct (puck position, output metering, waveform scope data) and pushes it to StateQueue. The UI thread uses juce::VBlankAttachment to pop this struct and emit an updateState event to the WebView.

* **UI to Audio:** Standard parameter changes are handled via APVTS. Commands (Moment Mode, preset load) route through a dedicated command queue.

**Frontend stack (per threadbare monorepo standards):** Vite 7 with vite-plugin-singlefile (bundles into a single index.html for JUCE embedding), vanilla JavaScript (ES modules), Motion for UI animation, and the shared @threadbare/shell package for common controls, presets UI, and elastic sliders. System sans-serif font stack (SF Pro on macOS, Segoe UI on Windows). No framework (React, Vue, etc.) — the shell provides all necessary UI primitives. Host communication uses JUCE 8’s native window.\_\_JUCE\_\_ protocol (setParameter, \_\_juce\_\_invoke / \_\_juce\_\_complete).

| Layer | Tech |
| :---- | :---- |
| Build | Vite 7 \+ vite-plugin-singlefile |
| UI | Vanilla JS (ES modules) |
| Animation | Motion (framework-agnostic successor to Framer Motion) |
| Shared shell | @threadbare/shell (controls, presets, elastic sliders) |
| Visualization | Canvas 2D (waveform scope) |
| Host IPC | JUCE 8 WebBrowserComponent (window.\_\_JUCE\_\_ protocol) |
| Windows | WebView2 with custom userDataFolder |
| Node | 18+ (frontend tooling only) |

## **7.2 The Waveform Scope**

waver’s primary visualization is a **slowly evolving waveform scope** tinted in the dusty amber accent color (\#C4A46C). This replaces unravel’s Lissajous orb as the product’s visual identity.

**7.2.1 Specification**

* **Data source.** The audio thread writes a rolling buffer of \~256–512 post-mixdown samples into the WaverState struct. The scope reads from this buffer at the display refresh rate.

* **Rendering.** Canvas 2D polyline with anti-aliased strokes. The waveform line uses \#C4A46C at \~70% opacity, with a subtle glow/bloom effect (achieved via a second, blurred, lower-opacity copy of the polyline offset by 1–2px).

* **Behavior.** The scope is not a traditional oscilloscope — it does not attempt zero-crossing trigger sync. Instead, it displays a continuous, free-running waveform that drifts and evolves, reinforcing the "breathing" emotional target. When no audio is playing, the scope shows a faint, slow sine-like idle animation rather than going blank.

* **Age coupling.** As the Y-axis increases, the scope’s visual treatment becomes more degraded: the line becomes slightly thicker and more diffuse, a subtle noise texture overlays the waveform, and the amber tint shifts slightly warmer (toward copper).

* **Reduced motion.** When prefers-reduced-motion is active, the scope displays a static waveform snapshot updated once per second rather than continuous animation.

**7.2.2 Shell Integration**

The @threadbare/shell expects a VizClass and a canvas element (it looks for \#orb in the DOM). For waver:

* **VizClass:** Pass a WaverScope class that implements the same interface as Unravel’s orb viz. The shell doesn’t care what it draws; it just calls update() on each frame with the current state.

* **Canvas reuse:** Keep id="orb" for the canvas element the shell manages. No structural change to the shell DOM is required — just a different viz class.

* **Theme tokens:** Apply waver’s accent color and palette via the shell’s themeTokens configuration (e.g. { accent: "\#C4A46C", bg: "\#241D19", text: "\#D3C7BB" }). The scope and all shell controls (sliders, preset browser, drawer) inherit these tokens automatically.

* **No shell modifications required.** The shell is designed for pluggable viz and theming. waver’s frontend work is limited to: implementing the WaverScope viz class, writing a main.js entry that imports from @threadbare/shell and passes waver-specific config, and creating the params.js (auto-generated from params.json).

## **7.3 The Puck: RBF Interpolation in Perceptual Space**

The puck traverses a continuous sound map populated by preset landmark states. RBF interpolation is calculated in perceptual domains (log Hz for cutoff, log seconds for envelope times, dB for gains, equal-power for mixes) to prevent dead zones and parameter jumps.

*w\_i \= exp(-||p \- p\_i||² / (2σ²))*

*V\_final \= Σ(w\_i / W) · v\_i,  where W \= Σ w\_i*

**Practical σ guidance:** For 8–16 landmarks in a unit square, σ of 0.25–0.40 works well. The "blend" parameter in the params.json allows the preset designer to tune σ per preset.

**7.3.1 Axis Mapping**

* **X-Axis (Harmonic Presence):** Moves DCO toward brighter waveforms, increases drive, raises Toy FM modulation index.

* **Y-Axis (Nostalgia/Age):** Deeply coupled to the engine’s core. Moving up increases tape wear and BBD depth, forces OPLL envelope stepping to become harsher, increases Organ key-click/leakage, causes compander noise to breathe aggressively, and makes OU drift stickier and slower.

## **7.4 Moment Mode (Deep Recalibration)**

Pressing the Moment button re-rolls the PRNG seed, causing new voice-level component tolerances, slight ± shifts in RBF landmark radii (σ), and micro-changes to macro-to-parameter mapping curves. This gives "today’s waver" a different personality while remaining entirely deterministic and recallable once saved.

Visual feedback: a subtle warmth shift in the dusty amber accent elements and a gentle ripple on the waveform scope. Transition uses ease-out timing (\~200 ms), snapping without animation when prefers-reduced-motion is active.

## **7.5 The Advanced Drawer**

A slide-up panel (ease-out \~200 ms) exposes underlying parameters organized into four groups. These map directly to the params.json definitions in Section 8\. When Arpeggiator mode (Section 3.5) is on, rate, pattern, gate, and swing are controlled by the puck; no Arp subsection or extra controls in the drawer.

**Tone**

Controls the filter and harmonic character of the DCO and Toy layers.

| Parameter | Display Name | Range | Default |
| :---- | :---- | :---- | :---- |
| filterCutoff | cutoff | 20 Hz – 20 kHz (log) | 8 kHz |
| filterRes | resonance | 0.0 – 1.0 | 0.15 |
| filterMode | filter | OTA / Ladder | OTA |
| filterKeyTrack | tracking | 0% – 100% | 50% |
| envToFilter | envelope | \-100% – \+100% | \+30% |

**Shape**

Controls oscillator configuration, layer mixing, and the organ drawbars. Waveform (saw vs pulse) is a continuous crossfade over the Shape range, not a snap; the macro sweeps smoothly from saw to pulse.

macroShape acts as a macro: it drives a single non-linear curve that sets target values for the primary oscillator waveform mix (saw↔pulse), pulse width, layer balance (DCO/Toy/Organ), and organ drawbar levels. The granular Shape parameters (dcoSubLevel, layerDco, organ8, etc.) are derived from RBF interpolation of preset landmarks and/or direct drawer edits; they override or blend with the macro’s targets so that preset design and fine editing remain in control when used. When only the macro is moved (no preset change, no drawer override), the engine applies the macro curve so that one gesture sweeps the whole shape character in a musical way.

| Parameter | Display Name | Range | Notes |
| :---- | :---- | :---- | :---- |
| macroShape | shape | 0.0 – 1.0 | Macro: one curve drives waveform mix, pulse width, layers, drawbars; granular Shape params override or blend (see above). |
| dcoSubLevel | sub | 0.0 – 1.0 | Sub-oscillator level |
| dcoSubOctave | sub octave | \-1 / \-2 | Sub-oscillator octave offset |
| noiseLevel | noise | 0.0 – 1.0 | Noise generator level |
| noiseColor | color | White – Pink | Noise spectrum |
| toyIndex | fm depth | 0.0 – 1.0 | Toy layer modulation index |
| toyRatio | fm ratio | Quantized list | Modulator-to-carrier ratio |
| layerDco | analog | 0.0 – 1.0 | DCO layer level |
| layerToy | toy | 0.0 – 1.0 | Toy layer level |
| layerOrgan | organ | 0.0 – 1.0 | Organ layer level |
| organ16 | 16′ | 0 – 8 | 16′ drawbar |
| organ8 | 8′ | 0 – 8 | 8′ drawbar |
| organ4 | 4′ | 0 – 8 | 4′ drawbar |
| organMix | mixture | 0 – 8 | Mixture drawbar |

**Motion**

Controls modulation, drift, chorus, portamento, and stereo width.

| Parameter | Display Name | Range | Default |
| :---- | :---- | :---- | :---- |
| lfoRate | rate | 0.1 – 30 Hz | 3.0 Hz |
| lfoShape | shape | Tri / Sin / Sq / S\&H | Triangle |
| lfoToVibrato | vibrato | 0 – 50 cents | 0 |
| lfoToPwm | pwm | 0% – 100% | 0% |
| chorusMode | chorus | Off / I / II / I+II | I |
| driftAmount | drift | 0.0 – 1.0 | 0.3 |
| stereoWidth | width | 0% – 100% | 80% |
| portaTime | glide | 0 – 2000 ms | 0 ms |
| portaMode | glide mode | Legato / Always | Legato |
| envAttack | attack | 1 ms – 5 s (log) | 10 ms |
| envDecay | decay | 1 ms – 10 s (log) | 200 ms |
| envSustain | sustain | 0.0 – 1.0 | 0.7 |
| envRelease | release | 1 ms – 15 s (log) | 400 ms |

**Print**

Controls the tape wear and output processing chain.

| Parameter | Display Name | Range | Default |
| :---- | :---- | :---- | :---- |
| driveGain | grit | 0.0 – 1.0 | 0.2 |
| tapeSat | saturation | 0.0 – 1.0 | 0.3 |
| wowDepth | wow | 0.0 – 1.0 | 0.15 |
| flutterDepth | flutter | 0.0 – 1.0 | 0.10 |
| hissLevel | hiss | 0.0 – 1.0 | 0.1 |
| humFreq | mains | 50 Hz / 60 Hz | 60 Hz |
| printMix | print mix | 0% – 100% | 75% |
| outputGain | output | \-24 – \+12 dB | 0 dB |

# **8\. Parameters and Preset Architecture**

## **8.1 Single Source of Truth (params.json)**

Per threadbare monorepo standards, all plugin parameters are defined in plugins/waver/config/params.json. The build system uses generate\_params.js to output C++ WaverGeneratedParams.h and the frontend params.js. The base parameters (puck, blend, moment, output) plus all drawer parameters from Section 7.5 constitute the complete parameter set.

**8.1.1 Choice Parameter Extension (Required)**

**The shared generate\_params.js currently only supports bool and float types.** waver requires choice parameters for several controls:

* filterMode (OTA / Ladder)

* lfoShape (Tri / Sin / Sq / S\&H)

* chorusMode (Off / I / II / I+II)

* portaMode (Legato / Always)

* humFreq (50 Hz / 60 Hz)

* dcoSubOctave (-1 / \-2)

**Note:** Waveform (saw/pulse) is **not** a choice; it is a continuous blend derived from macroShape and preset landmarks (see Sections 7.5 and 2.1.1). Do not add a discrete saw/pulse switch.

The generator must be extended with a **choice** type that emits juce::AudioParameterChoice in C++ and an options array in JS. This is a one-time shared infrastructure change that benefits all future plugins. The params.json schema for a choice parameter:

{

  "id": "chorusMode",

  "name": "chorus",

  "type": "choice",

  "options": \["off", "I", "II", "I+II"\],

  "default": 1

}

The alternative—encoding choices as float (0.0, 1.0, 2.0, 3.0) and documenting the mapping—is explicitly rejected. Float-encoded enums produce nonsensical intermediate values during automation, lose semantic meaning in the DAW parameter list, and create a maintenance burden.

| ID | Display Name | Type | Range | Default | Unit |
| :---- | :---- | :---- | :---- | :---- | :---- |
| puckX | presence | float | \-1.0 to 1.0 | 0.0 | — |
| puckY | age | float | \-1.0 to 1.0 | 0.0 | — |
| blend | blend | float | 0.15 to 0.60 | 0.35 | — |
| momentSeed | moment | float | 0.0 to 1.0 | 0.5 | — |
| outputGain | output | float | \-24 to \+12 | 0.0 | dB |

*Plus the \~40 drawer parameters defined in Section 7.5 (Tone: 5, Shape: 15, Motion: 13, Print: 8). Total parameter count: \~46.*

## **8.2 Playable Surfaces**

Every factory preset ships as a JSON file defining a playable region with 8–16 intentional landmarks distributed across the X/Y axes. A preset is not a single point — it is a designed morphing surface.

**8.2.1 Preset JSON Schema**

{

  "name": "settle",

  "category": "amber",

  "author": "threadbare",

  "version": 1,

  "momentSeed": 0.42,

  "sigma": 0.32,

  "landmarks": \[

    {

      "x": \-0.6, "y": \-0.3,

      "label": "warm center",

      "params": {

        "filterCutoff": 2200.0,

        "filterRes": 0.12,

        "dcoWave": 0.0,

        "layerDco": 0.8,

        "layerToy": 0.1,

        "layerOrgan": 0.3,

        ...

      }

    },

    ...

  \],

  "tags": \["pad", "warm", "contemplative"\]

}

Landmark parameter values are stored in **perceptual space** (log Hz for frequencies, log seconds for times, dB for gains). For the primary oscillator, the waveform dimension must be stored as a float ("dcoWave": 0.0 \= saw, 1.0 \= pulse), which is interpolated by the RBF engine to allow presets to sit anywhere on the continuous saw↔pulse blend. The RBF engine interpolates in this space, then converts back to raw engine parameters before pushing to the audio thread.

## **8.3 Preset Categories**

Factory presets are organized by emotional/sensory intent:

| Category | Description | Sample Presets |
| :---- | :---- | :---- |
| amber | Warm, sunset-colored pads. Sustained, golden, unhurried. | glow, dusk, settle, porch |
| drift | Moving textures, chorus motion, slow evolution. | wander, sway, bloom, tide |
| hush | Lo-fi, compressed, intimate. Close-mic’d, cassette-worn. | close, worn, whisper, tape |
| signal | Bright leads, toy keyboard sparkle, transmissions. | flicker, spark, glint, beam |
| weight | Organ beds, drones, static harmonic foundations. | anchor, root, floor, mass |

Each category ships with 8–12 presets. Total factory library: 40–60 sounds.

## **8.4 DAW State Serialization**

**8.4.1 Required ProcessorBase Overrides**

WaverProcessor inherits from threadbare::core::ProcessorBase. Because waver is the **first instrument** in the monorepo (Unravel is an effect), several base defaults must be overridden:

* **acceptsMidi() → return true.** The base returns false. Without this override the DAW will not route MIDI to the plugin. Trivial but easy to forget.

* **getTailLengthSeconds() → return non-zero.** With envelope release times up to 15 s and the print chain’s wow/flutter delay, the plugin produces audio well after note-off. Returning 0 causes DAWs to cut rendering prematurely on bounce. Recommended: return maximum release time plus \~0.5 s of print chain tail.

* **onSaveState / onRestoreState.** Serialize all determinism-critical state into the DAW session ValueTree (see 8.4.2).

**8.4.2 Determinism State Struct**

To guarantee bit-identical offline and realtime renders, a single DeterminismState struct must contain **everything that affects the audio output** beyond the APVTS parameters. This struct is the sole payload of onSaveState / onRestoreState:

struct DeterminismState {

    uint64\_t prngSeed;           // Moment Mode seed

    uint64\_t prngCounter;         // current PRNG position (sample-accurate)

    std::array\<float, 8\> ouState; // OU drift value per voice

    float puckX;                  // current puck position

    float puckY;

    int activePresetIndex;         // which preset/landmark set is active

    float momentSigmaOffsets\[16\];  // per-landmark sigma jitter from Moment

};

**Rule: if it affects samples, it goes in this struct.** If any field is missing, offline vs realtime will diverge. Regression tests (Section 10.1) validate this by comparing bounces with identical DeterminismState.

# **9\. Development Toolchain and Monorepo Structure**

## **9.1 Project Structure**

threadbare-unravel/

└── plugins/

    └── waver/

        ├── CMakeLists.txt          \# ThreadbareWaver target

        ├── config/

        │   └── params.json           \# Parameter source of truth

        ├── presets/

        │   ├── amber/                \# Factory presets by category

        │   ├── drift/

        │   ├── hush/

        │   ├── signal/

        │   └── weight/

        ├── assets/

        │   └── app-icon.png

        └── Source/

            ├── WaverTuning.h           \# All DSP constants \+ recalculate()

            ├── WaverGeneratedParams.h  \# Auto-generated from params.json

            ├── DSP/                    \# Pure C++20, no UI deps

            ├── Processors/             \# Inherits ProcessorBase

            └── UI/

                ├── WaverEditor.h/cpp     \# Bridges WebView via StateQueue

                └── frontend/             \# Vite 7 \+ vanilla JS (ESM)

## **9.2 First Instrument: New Surface Area**

**waver is the first instrument in the threadbare monorepo.** Unravel is an effect (no MIDI, no voices, no polyphony). This means the following are being built from scratch with no existing shared infrastructure to rely on:

* Voice allocation and stealing (8 voices, priority-weighted algorithm)

* MIDI routing: note on/off, velocity, pitch bend, CC, aftertouch

* Per-voice state management (OU drift, envelope phase, component tolerances)

* Portamento (log-domain glide with legato detection)

* The entire organ layer (phase-accumulator polyphony with pre-allocated oscillators)

There is no shared "synth base" class. All of this must respect the Iron Laws (no alloc/lock in processBlock). This is the **primary source of implementation risk** and the area where AI agents are most likely to generate unsafe code. The .cursorrules file must explicitly prohibit common patterns that work in non-realtime contexts but fail here (e.g. std::vector\<Voice\> with dynamic sizing, std::map for note tracking).

**Mitigation:** Phase 1 of the roadmap focuses exclusively on getting a single voice producing audio in a DAW with correct MIDI handling and bounce-in-place passing. Polyphony, the toy/organ layers, and the print chain are all deferred to later phases. This surfaces the riskiest architectural decisions first.

## **9.3 AI-Assisted Development**

A strict .cursorrules file constrains AI agents:

* **Thread safety:** No std::vector, std::string, new, delete, or locks in processBlock.

* **State management:** APVTS and modern ParameterAttachments only. Slider::Listener is forbidden.

* **Architecture awareness:** Agents must use WaverTuning.h for constants, StateQueue for visualization, and parse params.json rather than manually writing parameter layouts.

**Claude Code** is recommended alongside Cursor for initial scaffolding, large-scale refactors, CI/CD pipeline setup, and documentation generation. Both tools share the same context files (.cursorrules / CLAUDE.md).

# **10\. Testing and Validation**

Audio DSP requires testing methodologies beyond standard unit tests. The deterministic PRNG architecture of waver makes regression testing especially viable.

## **10.1 Deterministic Regression Tests**

Process a fixed MIDI sequence through the engine at a fixed sample rate (44100 Hz) with a fixed PRNG seed and Moment state. Compare the output buffer bit-for-bit against a stored reference. Any change flags a regression. This is automated in CI.

## **10.2 Spectral Analysis Tests**

For each nonlinear block (OTA filter, Moog ladder, overdrive, tape saturation), feed a known sine wave (1 kHz at \-6 dBFS) and verify the harmonic content of the output using FFT. This catches aliasing regressions caused by changes to the oversampling configuration.

## **10.3 CPU Profiling**

Track per-voice and per-block CPU usage using JUCE’s PerformanceCounter. Regression thresholds are enforced in CI:

* No single voice exceeds 2% of a single core at 44.1 kHz / 512 buffer.

* Full 8-voice polyphony plus print chain does not exceed 15% of a single core.

* No individual processBlock call exceeds 60% of the available buffer time (to prevent audio dropout).

## **10.4 A/B Reference Comparison**

Record audio from the actual hardware references (Juno-60 for DCO, PSS-270 for Toy, Baldwin organ for Organ layer) and use spectral similarity metrics to tune DSP parameters. This is subjective tuning, not automated testing, but it must be systematic: reference recordings are stored in the repository and the comparison process is documented.

## **10.5 Bounce-in-Place Validation**

Load a session in the DAW with waver on an instrument track. Perform a realtime bounce and an offline bounce. The two resulting audio files must be bit-identical. This validates the deterministic PRNG, sample-rate-independent drift, and the absence of any timing-dependent behavior. This test is run manually before every release.

# **11\. Implementation Roadmap**

Each phase produces a playable, DAW-loadable build that can be evaluated musically.

## **Phase 1: Foundation (Weeks 1–3)**

* CMake scaffolding within threadbare-unravel monorepo

* params.json and generate\_params.js pipeline

* Basic SynthesiserVoice with single PolyBLEP sawtooth

* ADSR envelope on VCA, 8-voice polyphony with voice stealing

* Minimal WebView UI: single gain slider to verify C++/JS StateQueue bridge

* Audio output verified in DAW. Bounce-in-place test passes.

## **Phase 2: DCO Layer Complete (Weeks 4–6)**

* Full oscillator suite (sawtooth, pulse/PWM, sub, noise)

* IR3109 OTA filter with TPT implementation (plus Moog ladder alternate)

* LFO with vibrato and PWM routing

* BBD chorus (all three modes) with sub-bass crossover

* Portamento (legato and always modes)

* Local oversampling on filter. Latency reporting via setLatencySamples()

* Sample-rate adaptation validated at 44.1, 48, 88.2, 96 kHz

## **Phase 3: Analogization and Toy Layer (Weeks 7–9)**

* Per-voice deterministic OU drift generators

* Voice-to-voice component tolerances from PRNG seed

* Two-operator PM engine for the Toy layer

* Bit-crushing, sample-rate reduction, envelope stepping

* Layer mixing (DCO \+ Toy)

* CPU profiling pass: verify per-voice budget

## **Phase 4: Organ Layer and Print Chain (Weeks 10–12)**

* Divide-down organ with drawbar controls and formant filtering

* Key-click transient and Age-coupled leakage

* Full three-layer mixing

* Overdrive (SD-1 model), tape saturation, wow/flutter, noise floor

* Oversampling on print chain. Total latency reporting updated

* A/B comparison sessions against hardware references

## **Phase 5: UI and Morphing (Weeks 13–16)**

* WebView UI (vanilla JS via @threadbare/shell) with puck controller and dusty amber waveform scope

* RBF interpolation engine (perceptual space)

* Landmark preset system with JSON import/export

* Moment Mode with PRNG seed save/restore

* Advanced drawer with all four parameter groups (Tone, Shape, Motion, Print); arp is puck-controlled when on (no drawer controls)

* Arpeggiator mode (Section 3.5): toggle button only; rate, pattern, gate, swing driven by puck when arp on; deterministic for bounce-in-place

* MIDI CC mapping for puck axes

* Age coupling verified across all engine layers

## **Phase 6: Polish and Release (Weeks 17–20)**

* Factory preset library: 40–60 presets across 5 categories

* CPU optimization pass (SIMD where applicable, cache alignment)

* Automated regression test suite in CI

* Bounce-in-place validation at multiple sample rates

* Cross-platform validation (macOS AU/VST3, Windows VST3)

* Installer branding per threadbare style guide

* Landing page copy, store listing, documentation

# **12\. Corrections and Expansions from Prior Versions**

| Area | Prior State | V6 Change |
| :---- | :---- | :---- |
| Print Chain | Reduced to one-liners in V5 | Full implementation detail restored: overdrive waveshaping, hysteresis model, wow/flutter delay spec, noise composite |
| Portamento | Removed in V5 | Legato and Always modes restored with log-frequency glide |
| Sample-Rate Adapt. | Not addressed | recalculate(sampleRate) convention for WaverTuning.h, with concrete coefficient examples |
| Latency Reporting | Not addressed | setLatencySamples() requirement with code example |
| IR3109 OTA Filter | Named but not specified | TPT implementation approach, soft-clip function, behavioral comparison vs Moog ladder |
| Advanced Drawer | Referenced but empty | Four groups (Tone/Shape/Motion/Print) with \~41 parameters fully defined |
| Visualization | Not addressed | Dusty amber waveform scope specified: data source, rendering, age coupling, reduced motion |
| MIDI Implementation | Not addressed | Pitch bend, mod wheel, CC 74/11 for puck, aftertouch, sustain |
| CPU Budget | Not addressed | 15% single core M1 target with per-component breakdown |
| Testing | Removed in V5 | 5 test methodologies restored: regression, spectral, CPU, A/B reference, bounce validation |
| Roadmap | Removed in V5 | 20-week phased plan updated for V6 architecture |
| Reverb Decision | Ambiguous | Explicitly excluded. Unravel handles spatial duties (brand ecosystem decision) |
| Preset Schema | Not defined | Full JSON schema with perceptual-space landmark storage |
| Brand Identity | Established in V3 | V5 values preserved (name, tagline, targets, colors, bundle IDs) |
| Choice Parameters | Not addressed | generate\_params.js must be extended with a choice type for AudioParameterChoice. Float-as-enum explicitly rejected. |
| ProcessorBase Overrides | Not addressed | acceptsMidi() and getTailLengthSeconds() must return true/non-zero. First instrument in monorepo. |
| Determinism State | Mentioned conceptually | Explicit DeterminismState struct defined with all fields that affect audio output. |
| Organ Pre-allocation | Not addressed | All 128 phase accumulators pre-allocated at init. No containers that grow in audio path. |
| Quality Modes | HQ mentioned casually | Three explicit modes (Lite/Standard/HQ) with CPU targets and latency implications. |
| Shell Wiring | Not addressed | VizClass, \#orb canvas reuse, and themeTokens integration documented. |
| First Instrument Risk | Not addressed | New surface area acknowledged. No shared synth base exists. Phase 1 focuses on this risk. |
| Arpeggiator Mode | Not addressed | Bonus feature (waver’s analogue to Unravel’s disintegration looper): Section 3.5. When arp on, puck X/Y drive rate, pattern, gate, swing (no new drawer controls). Held-note buffer, Iron Law compliant, deterministic for bounce-in-place. |

This specification is a living document. waver is ultimately an instrument of feel and character—weathered, tender, breathing—and no specification can substitute for the iterative process of playing, listening, and tuning that transforms a technical architecture into a musical instrument.