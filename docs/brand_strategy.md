# Threadbare: Strategic Brand Framework

*Positioning, purpose, and messaging hierarchy. Companion to the Brand Style Guide (visual identity, voice, and design language).*

*Version 3.0, March 2026*

---

## Brand Strategy

### Why

Most music software starts from the same place: a piece of hardware that already existed. The original was built by engineers solving engineering problems. Oscillator stability, filter tracking, noise floors. The software version faithfully reproduces all of it, including the part where you need to be an engineer to get the sound in your head out of the speakers.

There's a gap between feeling something and making something that sounds like how you feel. It's filled with parameters, menus, and jargon that have nothing to do with music.

Threadbare exists to make that gap as thin as possible.

### How

**Innovative, not emulative.** Classic instruments are material, not destinations. We build on their foundations (analog drift, tape warmth, organ harmonics) to create things that didn't exist before.

**Immediately understandable.** One primary control surface reshapes the entire sound in one gesture. You don't need to understand the architecture to play it. Depth is there when you want it, but it's never in the way.

**Alive by design.** Static, perfect sound creates distance between the tool and the person using it. Everything we build moves, breathes, and varies. Not because we're simulating decay, but because imperfection makes an instrument feel responsive. Responsive instruments invite play. That's where the joy lives.

**Modular.** Each product does one thing well. Like pedals on a board or modules in a rack, they combine in whatever way serves the musician.

### What

Tools for music creation, each one built around a single idea taken further than it's been taken before.

- **Instruments** generate sound with personality built in.
- **Effects** reshape sound in ways that respond to what you're actually playing.

The interaction model is the same across products: one surface that gets you 80% of the way, and architecture underneath for the other 20%.

---

## Product Positioning

### Waver

**Category:** Instrument
**Emotional targets:** Weathered, Tender, Breathing

**Position:** A tenderly alive polysynthesizer.

Most soft synths model a single instrument from a single era. Waver fuses three (analog poly, consumer FM, divide-down organ) into something that didn't exist in any of those decades. The result drifts, breathes, and never plays the same way twice.

**Proof:**

- Each voice breathes independently. Per-voice drift, component tolerances, envelope variation. Two instances of the same patch won't sound identical.
- The puck reshapes the entire instrument in one gesture (presence × age).
- Tape saturation, wow, flutter, and hiss aren't post-processing. They're part of the voice itself, shaping every note from the start.
- Every instance gets its own seeded personality (called Moment). Recallable, but unique each time you load a new one.
- The arpeggiator is controlled entirely from the puck. No separate controls, no menu diving.

**Lineage (for deeper copy, not the headline):** Analog polysynths gave it warmth. Consumer FM keyboards gave it that slightly wrong, slightly beautiful digital character. Divide-down organs gave it the breath. None of those instruments sounded like this on their own.

### Unravel

**Category:** Effect
**Emotional targets:** Spectral, Wistful, Diffuse

**Position:** Reverb with a memory problem.

Most reverbs simulate a space. Unravel simulates what happens to sound inside a memory. Fragments of what you just played replay at the wrong speed, details dissolve, and the whole thing drifts further from the original the longer you listen.

**Proof:**

- The reverb tail is built partly from fragments of your own performance, not just algorithmic reflections. Some shimmer, some play backwards, all of it woven into the decay rather than layered on top.
- A looper captures your sound and lets it physically degrade in real time. Tape hiss creeps in, filters narrow, pitch wobbles. You control the rate.
- The puck reshapes the entire space in one gesture (vivid/hazy × recent/distant).
- True stereo. Left and right channels develop independently, the way real memory is never perfectly symmetrical.

---

## Messaging Framework

### Hierarchy

Each level of messaging answers a different question. The order matters: purpose earns the right to promise, promise earns the right to prove, proof earns the right to get technical.


| Level               | Answers                         | Example (Waver)                                                                 |
| ------------------- | ------------------------------- | ------------------------------------------------------------------------------- |
| **Brand purpose**   | Why does Threadbare exist?      | The gap between feeling something and making it sound that way is too wide      |
| **Brand method**    | How do we close it?             | Innovative not emulative. Immediately understandable. Alive by design. Modular. |
| **Product promise** | What does this product deliver? | A synthesizer that sounds tender and alive                                      |
| **Product proof**   | Why should I believe that?      | Per-voice drift, the puck, seeded personality, the tape/saturation voice layer  |
| **Lineage**         | What's underneath?              | Analog polysynth, FM keyboard, divide-down organ: fused, not reproduced         |
| **Technical spec**  | What's the DSP?                 | PolyBLEP, OTA filter, Ornstein-Uhlenbeck drift, RBF interpolation, etc.         |


### Where Each Level Lives

**Brand purpose and method.** About page, interviews, brand documentation. Rarely in product marketing directly. The purpose should be *felt* in the copy, not stated. The moment you write "We believe..." you've lost it.

**Product promise.** Landing page hero, social media bios, one-line descriptions, store short descriptions. The sentence you say when someone asks "what is it?" One sentence. No hedging.

**Product proof.** Landing page body, feature sections, demo videos. The proof makes the promise concrete. This is where you show the thing doing what you said it does, without becoming a spec sheet.

**Lineage.** Deeper landing page copy, feature descriptions, blog posts, press material. What instruments influenced the sound, what circuits inspired the design. Context for the curious. Never the headline.

**Technical spec.** Store full descriptions, documentation, forum posts, review guides. Dense, accurate, keyword-rich. This audience respects specificity; give it to them. This is where "PolyBLEP sawtooth," "8×8 FDN," and "Ornstein-Uhlenbeck drift" live. But only because the earlier levels gave those terms something to mean.

### Copy Principles

The voice is technically precise but emotionally alive. The underlying tension between the broken and the beautiful, the digital and the human, isn't a marketing angle. It's the actual worldview. These principles govern all Threadbare copy.

**Lead with the tension or the problem, not the conclusion.**
Start with what's missing, what's wrong, what's unresolved. The product is the answer, but only after the reader feels the question.

*Example: "I had reverbs I liked. They were all simulating spaces. I kept reaching for one that sounded less like a room and more like something slipping away from me."*

**"Why" before "what": earn the technical depth before going there.**
Context first, mechanism second. If a reader hasn't been given a reason to care about the PolyBLEP oscillator, the PolyBLEP oscillator is noise.

*Example: "Waver's voices wander apart over time, each note aging slightly differently, the way a held chord shifts on a real instrument. Underneath, drift models give each oscillator its own trajectory."*

**Ground abstract ideas in something specific.**
A reference, a detail, a named thing. Specificity builds trust. Vagueness signals insecurity.

*Example: "The kind of pad you hear on Talk Talk's Spirit of Eden" over "lush, cinematic pads."*

**Short sentences land weight. Use them after longer ones, not instead of them.**
Rhythm matters. A paragraph of nothing but short sentences reads like a list. A long sentence that builds toward a short one reads like intent.

**Self-awareness is a default, not a device.**
Name the limitation before someone else does. Musicians respect tools that know what they are.

*Example: "Waver's filter is warm but it won't self-oscillate. That's a deliberate choice. We optimized for musical range over extremes."*

**Humor lives in structure, not punchlines.**
Build it into the sentence so it rewards attention. Never forced, never loud.

**First person when it adds warmth.**
"We" and "our" in brand-level communication. "I" is acceptable in founder-voice contexts (blog posts, interviews, launch announcements). Never in product UI or spec copy.

*Example (brand): "We build on foundations, not replicas." Example (founder): "I wanted Waver to sound like an amalgamation of the keyboards I grew up playing."*

**The product name does more work than any descriptor. Trust it.**
"Waver" already implies instability and tenderness. "Unravel" already implies decay and transformation. Don't overwrite what the name is doing.

**What to avoid:**

- Declarative enthusiasm ("We believe...", "We're passionate about...")
- Finishing thoughts too cleanly. Leave some of the tension intact.
- Surface-level "human" gestures (warmth as finish coat, not intent)
- Superlatives, exclamation marks, hype. The work speaks. If it doesn't, louder copy won't fix it.

### Describing the Sound

The core juxtaposition: old craft techniques meeting new technology. Broken things that are still beautiful. This isn't a tension to resolve. It's the point. But when describing the *sound*, lead with life, not damage.


| Prefer                                 | Over                              |
| -------------------------------------- | --------------------------------- |
| alive, breathing, responsive           | aged, worn, degraded              |
| drifts, wanders, moves                 | decays, deteriorates, breaks down |
| tender, gentle, warm                   | broken, damaged, lo-fi            |
| never quite the same twice, unreliable | imprecise, inaccurate             |
| reactive, evocative                    | emulates, models, simulates       |


"Broken but beautiful" works as a tagline precisely because it names the tension directly. But it's the exception, not the template. In most copy, the beauty should be the subject and the brokenness should be the texture underneath.

---

## Changelog

**v3.0** Rewrote Why/How/What and product positioning to follow updated voice guidelines. Revised copy principles to lead with tension, earn depth, and avoid declarative enthusiasm. Added explicit avoid list. Removed em dashes. Rewrote proof bullets to lead with what the listener hears, not internal feature names.

**v2.0** Expanded "What" section to bridge brand purpose into product categories. Revised Waver position line. Added internal note on "broken but beautiful" usage. Expanded copy principles with inline examples.

---

*This framework defines why Threadbare exists and how that purpose flows into product positioning and messaging. The Brand Style Guide (threadbare-brand-guide.md) handles execution: visual identity, voice rules, naming conventions, and design language.*