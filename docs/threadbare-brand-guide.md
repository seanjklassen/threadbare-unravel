# Threadbare — Brand Style Guide

*Reference for visual identity, voice, and design language across all Threadbare products. Version 1.0 — February 2026.*

---

## 1. Brand Identity

### 1.1 Name

**threadbare** — always lowercase in running text, logos, and product UI. The name is the aesthetic: stripped down, worn in, honest. Nothing unnecessary.

### 1.2 Positioning Statement

Threadbare makes audio tools for people who care more about how music feels than how many features are on the box. Every product has a point of view.

### 1.3 Brand Personality

| Trait | Expression |
|---|---|
| **Understated** | Quiet confidence. No exclamation marks, no superlatives, no hype. Let the work speak. |
| **Warm** | Approachable without being casual. First-person is fine. Humor is dry, never loud. |
| **Honest** | If a feature has a trade-off, say so. If the CPU spikes at max settings, acknowledge it. |
| **Craft-oriented** | Respect for the process. Language that sounds like it was written by someone who makes music, not someone who markets software. |
| **Nostalgic** | Not retro kitsch. The kind of nostalgia that aches slightly — memory, impermanence, the beauty of things fading. |

### 1.4 Brand Values (Design Decisions Flow From These)

1. **Character over neutrality.** Every product has a sonic identity.
2. **Simplicity over flexibility.** Fewer controls, deeper expression.
3. **Real-time safety.** No compromises in the audio path.
4. **Emotional resonance.** Names, presets, and copy evoke feelings.
5. **Craftsmanship.** Fewer products, each one excellent.

---

## 2. Voice & Tone

### 2.1 Writing Voice

Write like a musician explaining their favorite pedal to a friend. First person is fine. Technical accuracy matters, but it should never feel like a spec sheet unless it literally is one.

**Good:**
> I kept reaching for a reverb that sounded more like the way I remember a song. Like the feeling of it coming back to me later, softer and not quite right.

**Bad:**
> Introducing our revolutionary new reverb plugin featuring cutting-edge granular synthesis technology and an innovative 8x8 FDN architecture!

### 2.2 Tone Spectrum

| Context | Tone | Example |
|---|---|---|
| Landing page / marketing | Personal, evocative, unhurried | "Reverb with a memory problem." |
| Feature descriptions | Clear, direct, visual | "Left emphasizes presence and focus. Right adds fog and stereo width." |
| Technical docs | Precise but readable | "8 delay lines with prime-ish base times to avoid metallic ringing." |
| Preset names | Emotional, sensory, concise | "bloom", "mist", "halation" — not "Large Hall" or "Guitar Verb 2" |
| Error messages / UI text | Calm, helpful, brief | Not cute, not corporate. |
| Store listing (technical) | Accurate, dense, keyword-rich | Full DSP spec for the audience that wants it. |

### 2.3 Language Rules

**Do:**
- Use first person when it adds warmth ("I built it", "I've spent entire evenings").
- Use concrete sensory language ("fog", "dissolving", "fading photograph").
- Be specific about what something sounds like, not what it technically does.
- Use short sentences. Let white space breathe.
- Acknowledge trade-offs honestly.

**Don't:**
- Use superlatives ("best", "revolutionary", "game-changing").
- Use exclamation marks in marketing copy.
- Describe features with jargon the audience doesn't care about (unless in a dedicated technical section).
- Use emojis in product copy.
- Use ALL CAPS for emphasis. Use *italics* or sentence structure.
- Reference competitors by name (position through quality, not comparison).

### 2.4 Naming Conventions

**Product Names**
- Single evocative English words: *unravel*, *weave*, etc.
- Lowercase in all contexts.
- Metaphorical — should suggest a process, texture, or feeling.
- Avoid: technical terms, gear references, version numbers in the display name.

**Preset Names**
- Single lowercase words.
- Evoke emotion, sensation, visual imagery, or physical experience.
- Source domains: memory, light, weather, texture, time, physicality.
- Never: "Big Room", "Guitar 1", "Vocal Plate", or any technical descriptor.

| Good | Bad |
|---|---|
| bloom | Large Ambient |
| halation | Bright Shimmer FX |
| stasis | Freeze Loop Drone |
| shiver | MAX EVERYTHING |
| mist | Dark Foggy Reverb |
| close | Small Room Short |

**Parameter Display Names**
- Semantic rather than technical where possible.
- "Brightness" not "Tone HF Cutoff". "Distance" not "ER Pre-Delay". "Drift" not "Mod Depth".
- Single words preferred. Two-word max.

---

## 3. Visual Identity

### 3.1 Color System

The palette is warm, muted, and slightly desaturated. It should feel like looking at something through a window at dusk.

#### Palette Principles

- **No pure white or pure black.** Everything has warmth.
- **Low contrast by default.** Controls recede until interacted with.
- **Single accent color per product.** Unravel = lime. Future products get their own accent but share the neutral foundation.
- **State changes use color, not layout.** The looper button changes color, not position.

#### Primary Palette *for Unravel*

**Core tokens** (CSS custom properties on `:root`):

| Role | Hex | Swatch | CSS Variable |
|---|---|---|---|
| Background | `#31312b` | Warm charcoal | `--bg` |
| Text / Icons | `#C8C7B8` | Soft beige | `--text` |
| Accent | `#E0E993` | Lime | `--accent` |
| Accent hover | `#E8B8A8` | Dusty coral | `--accent-hover` |

**Component colors** (used directly in CSS, derived from the core tokens):

| Role | Hex | Swatch | Usage |
|---|---|---|---|
| Puck pupil (highlighted) | `#C5CC7A` | Olive green | Macro activity indicator, looper active button |
| Puck pupil (active) | `#9EAE5A` | Dark olive | Puck engaged / pressed |
| Puck pupil (inactive) | `#969588` | Warm grey | Default puck state |
| Orb (idle) | `#E8B8A8` | Dusty coral | Lissajous visualization, idle state |
| Orb (recording) | `#CC8A7E` | Darker coral | Orb during looper recording |
| Recording | `#CC8A7E` | Darker coral | Looper recording button pulse, puck recording state |
| Looping (orb start) | `rgb(255,200,140)` | Amber | Orb color at entropy 0% |
| Looping (orb end) | `#A8C0D4` | Dusty blue | Orb color at entropy 100% |
| Settings overlay | `rgba(49,49,43,0.85)` | — | Background at 85% opacity + 4px backdrop blur |

### 3.2 Typography

**UI Font:** System sans-serif stack defined on `:root`:
```
-apple-system, BlinkMacSystemFont, 'SF Pro Text', 'SF Pro', 'Inter', sans-serif
```
- macOS resolves to SF Pro via `-apple-system`.
- Windows falls through to Inter or the platform sans-serif (Segoe UI).

**Weight:** Regular (400) for all text. No bold in the plugin UI. `letter-spacing: -0.01em` and `font-variant-numeric: tabular-nums` are set globally for tighter, monospaced-numeric rendering.

**Size Scale:**
| Element | Size | Usage |
|---|---|---|
| Preset name / navigation icons | 21px | Preset bar, gear icon, looper button |
| Labels & readouts | 12px | Parameter names, X/Y coordinates, secondary text |

**Case:** Lowercase for all product-facing text (preset names, parameter labels, axis labels). Sentence case for longer UI strings and documentation.

### 3.3 Spacing & Layout

**Plugin Dimensions:** ~380-420 px wide, ~670-720 px tall. Compact and vertical — fits alongside DAW mixers without dominating.

**Spacing Scale:** Base unit of 4px. Common increments: 4, 8, 12, 16, 24, 32.

**General Principles:**
- Generous padding around the main canvas. The orb and puck need room to breathe.
- Tight spacing in the drawer. Information density is fine for "workbench" controls.
- No visible borders or boxes around controls. Use spacing and subtle color shifts to create groupings.
- Rounded corners: 21px radius (pill shape) on buttons and preset pills. 4px on slider tracks. `50%` for circular elements (puck pupil, ripples).

### 3.4 Iconography

- Minimal line icons, 1.5px stroke, in `#C8C7B8` (text color).
- Infinity symbol (looper), gear (settings), and chevrons (drawer) are the only icons.
- Icons change color to reflect state (e.g., accent lime when active).
- No pictorial icons, no custom icon font. SVG inline or unicode glyphs.

### 3.5 Motion & Animation *for Unravel*

- **Orb:** Continuous Lissajous figure, ~60 points, polyline on canvas. Motion slows but never stops. Radius and tangle respond to audio state. Color lerps smoothly between states. Transitions are always smooth.
- **Settings overlay:** Spectral blur materialization (~600-680ms, `cubic-bezier(0.4, 0, 0.1, 1)`). Opens from 36px blur → clear with scale and opacity. Content rows stagger in with organic delays (80-272ms). Close reverses with a slow fade back into blur.
- **Puck:** Snaps to finger/cursor position immediately. No easing on drag. Supports stretch/squash and inertia for a physical feel. Subtle idle breathing animation when not being touched.
- **Looper button:** Expanding glow pulse during recording (`box-shadow` animation, 1.2s cycle). No harsh flash. Puck pupil shifts to `#CC8A7E` coral with its own synchronized pulse.
- **Preset dropdown:** Staggered reveal with organic delay curve (40-220ms per option). Same spectral materialization feel as settings overlay.
- **Reduced motion:** Respect `prefers-reduced-motion`. All `animation-duration` set to near-zero, `transition-duration` set to near-zero, `scroll-behavior` set to auto.

---

## 4. UI Design Language

### 4.1 Control Philosophy

The puck is the instrument. The drawer is the workbench.

Most users should be able to get a great sound using only the puck. The drawer exists for precision and edge cases. This hierarchy should be reflected in visual weight: the puck and orb dominate; the drawer is secondary.

### 4.2 Slider Design

- Single horizontal track, no numeric readout visible by default.
- Neutral base layer reflects stored value.
- "Pupil" indicator to the right of each slider: lights up (soft accent color) when the puck is contributing a macro offset to that parameter.
- Tall invisible hit area (3-4x visible track height) for easy grabbing.
- Transient tooltip on drag for precise values.
- Keyboard accessible: arrow keys for fine control.

### 4.3 State Communication

| State | Visual Signal |
|---|---|
| Parameter at default | Slider fill at neutral position |
| Puck contributing | Pupil indicator lit (accent color) |
| Looper idle | Infinity symbol in text color |
| Looper recording | Button glows `#CC8A7E` coral with expanding pulse, puck pupil matches |
| Looper active | Button solid `#C5CC7A` green, orb transitions amber → dusty blue with entropy |

### 4.4 Feedback Principles

- Every interaction should have immediate visual feedback.
- Audio-driven visual feedback (orb) keeps the plugin feeling alive even when not being touched.
- No loading spinners, no progress bars, no modal dialogs in the plugin UI.
- Error states are communicated through the orb (color shift, motion change), not through text popups.

---

## 5. Product-Specific Branding

### 5.1 Unravel

| Element | Value |
|---|---|
| Accent color | `#E0E993` (lime) |
| Emotional targets | Spectral, Wistful, Diffuse |
| Key metaphor | Memory — fading, dissolving, replaying |
| Orb color | `#E8B8A8` (dusty coral) |
| Sound words | dissolving, modulated, cloud, ghost, drift, bloom, haze, fog |

### 5.2 Future Products

Each new product:
- Gets its own accent color (contrasting with existing products).
- Shares the neutral foundation (`#31312b` background, `#C8C7B8` text).
- Has its own emotional target set and key metaphor.
- Follows the same layout philosophy (primary control surface + secondary drawer).
- Uses the same technical stack and architecture patterns.

---

## 6. Marketing & Copy

### 6.1 Tagline Format

Short, lowercase, slightly odd. A sentence fragment that makes you pause.

> reverb with a memory problem.

Not a value proposition. Not a feature list. A *feeling*.

### 6.2 Feature Description Format

**Structure:** Sensory description first, then mechanic, then use case.

> **ghost engine**
> Continuously records your input and spawns up to eight ghostly fragments at any time — some shimmer, some play backwards, and all of it feeds into the reverb rather than sitting on top. The ghost material doesn't decorate the tail. It *is* the tail.

**Pattern:**
1. What it feels/sounds like (one sentence, evocative).
2. What it does (one to two sentences, clear).
3. Why you'd use it or what makes it different (one sentence).

### 6.3 Store Listing Strategy

Two tiers of description:

**Short description:** One paragraph for casual browsers. Sensory, not technical. Ends with genre positioning.

**Full description (technical):** Dense, accurate, keyword-rich. This is for the people who search "8x8 FDN reverb" or "granular shimmer plugin." Use exact technical terms, parameter ranges, and architecture details. This audience respects specificity.

### 6.4 Words to Use

dissolving, modulated, cloud, ghost, drift, bloom, haze, fog, memory, fading, spectral, wistful, diffuse, vaporous, weightless, bittersweet, haunting, warm, soft, gentle, traces, moments, fragments, recollection, evaporates

### 6.5 Words to Avoid

revolutionary, game-changing, next-generation, best-in-class, powerful, unleash, supercharge, amazing, incredible, ultimate, state-of-the-art, cutting-edge, innovative (unless genuinely describing a novel technique in a technical context)

---

## 7. Installer & Packaging

### 7.1 Installer Branding

- macOS: .pkg with branded background image, welcome text, and license.
- Windows: Inno Setup with branded wizard images (240x459px large, 55x58px small).
- Both: Clean, professional, fast. The installer should be invisible — purchase to first reverb tail in seconds.

### 7.2 File Naming

| Platform | Format | Binary Name |
|---|---|---|
| macOS | AU | `unravel.component` |
| macOS | VST3 | `unravel.vst3` |
| macOS | Standalone | `unravel.app` |
| Windows | VST3 | `unravel.vst3` |
| macOS installer (current scripts) | .pkg | `Unravel-{version}.pkg` |
| Windows installer (current scripts) | .exe | `Unravel-Installer.exe` |

### 7.3 Icon

- Plugin icon at `plugins/{product}/assets/app-icon.png`.
- Should work at all sizes from 16x16 to 512x512.
- Monochromatic or very limited palette. Recognizable at small sizes.

---

## 8. Applying This Guide to New Products

When starting a new Threadbare product:

1. **Choose a name.** Single evocative word, lowercase. Test: does it suggest a process, texture, or feeling?
2. **Define emotional targets.** Three adjectives that serve as the north star for all design decisions.
3. **Pick an accent color.** Must utilize the same principles as unravel but be distinct.
4. **Write the tagline.** Short, lowercase, slightly odd. A sentence fragment.
5. **Design the primary control.** What is the "puck" of this product? One gesture that controls everything.
6. **Build the drawer.** What precision controls exist behind the primary interface?
7. **Name the presets.** Follow the naming conventions in section 2.4.
8. **Write the landing page copy.** Follow the voice rules in section 2. Personal, evocative, honest.
9. **Create the DSP in `Source/DSP/`.** Follow real-time safety rules. Pull constants from a tuning header.
10. **Follow the monorepo pattern.** `plugins/{name}/`, shared core, params.json, WebView UI.

---

*This guide defines the Threadbare brand across all touchpoints. It is a living document — update it as the brand evolves, but protect the core identity: understated, warm, honest, craft-oriented, and always in service of how music feels.*
