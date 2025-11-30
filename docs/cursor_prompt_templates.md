# **CURSOR_PROMPTS.md**

### Prompt Templates for Working on Threadbare with Cursor

**Fray · Weave · Unravel**

These templates are designed to:

-   Keep Cursor inside the guardrails defined in `THREADBARE.md`
-   Avoid project-corrupting changes
-   Get tight, focused help instead of "refactor the universe" chaos

Copy / adjust the text in each section as needed for specific tasks.

------------------------------------------------------------------------

## 1. DSP Change (Safe, Scoped)

**Use this when you're changing DSP behavior without touching UI.**

``` text
You are editing a JUCE 8 audio plugin for the Threadbare suite (Fray, Weave, Unravel).
Follow these hard constraints:

- Do NOT edit the APVTS parameter declarations or IDs.
- Do NOT allocate memory in processBlock().
- Do NOT touch any UI, WebView, or JS code.
- Do NOT add locks, mutexes, or file I/O to the audio thread.
- Respect the existing threading model and lifecycle (prepareToPlay, reset, releaseResources).

Task:
[Describe the exact change you want, e.g.
"Implement SmoothedValue-based smoothing (10ms ramp) for the Weave `amount` parameter in WeaveEngine.cpp.
Do not change routing, buffer sizes, or voice counts."]

Requirements:
- Use std::atomic<float>* parameter pointers for parameter reads (already set up via APVTS).
- Use juce::SmoothedValue or LinearSmoothedValue for parameter smoothing, updated per-sample.
- Keep all new buffer or state allocations in prepareToPlay() or constructor.
- If you add any new member state, show the header AND implementation changes.

Finally:
- Explain the algorithm in plain English first.
- Call out potential failure modes and CPU impact.
- Provide a small test harness or pseudocode test (identity + sanity tests) I can adapt.
```

------------------------------------------------------------------------

## 2. UI / WebView Change (No DSP Touching)

**Use this when you only want to adjust the JS/HTML/CSS side.**

``` text
You are editing only the WebView-based UI for Threadbare.
You MUST NOT touch any C++ DSP, APVTS, or audio-thread code.

Constraints:
- No changes to C++ files or JUCE AudioProcessor code.
- All changes must be in JS/HTML/CSS/WebView integration files only.
- Do NOT introduce any blocking calls that depend on audio state.
- UI may read parameter values and send changes via existing messaging only.

Task:
[Describe the UI change, e.g.
"Add a subtle animated highlight ring around the main XY control when Fray's depth is above 0.7.
Use existing parameter bridge; do NOT invent a new protocol."]

Requirements:
- Clearly show which files change and which functions are added/modified.
- Keep logic modular and easy to maintain (no 300-line functions).
- Explain how the UI listens to parameter changes and how it writes back.
- Confirm there is no code that assumes it runs on the audio thread.

Finally:
- Summarize the UX change in 3–4 bullets.
- Note any accessibility or performance concerns.
```

------------------------------------------------------------------------

## 3. Bug Fix (Focused on a Known Symptom)

**Use this when you know what's broken (clicks, noise, crashes) and want
a surgical fix.**

``` text
You are debugging a JUCE 8 audio plugin for Threadbare.

Constraints (non-negotiable):
- Do NOT change APVTS parameter definitions or IDs.
- Do NOT allocate memory in processBlock().
- Do NOT touch .jucer, .pbxproj, AAX, or build system files.
- Do NOT mix UI logic into DSP.
- Keep changes as small and localized as possible.

Bug description:
[Describe the symptom as concretely as possible, e.g.
"When adjusting Weave `amount` quickly during playback, I hear clicking at transition points.
WeaveEngine rebuilds voices or changes loop length at thresholds 0.2, 0.6, 0.9."]

Task:
- Identify likely root causes based on the existing code structure (do not invent a new architecture).
- Propose a minimal fix that:
  - avoids discontinuities,
  - uses per-sample smoothing where needed,
  - preserves existing behavior where possible.

Requirements:
- Show only the changed code sections, with enough context to apply the patch.
- Explain the fix in detail: what it changes, and why it removes the bug.
- State explicitly how the fix respects real-time safety (no allocations, no locks).

Finally:
- Suggest a simple test plan:
  - which parameters to sweep,
  - what to listen for,
  - how to verify no regressions in CPU or sound.
```

------------------------------------------------------------------------

## 4. New Feature (Small & Isolated)

``` text
You are adding a small, isolated feature to the Threadbare plugin DSP.

Constraints:
- Do NOT edit APVTS parameter IDs or remove existing parameters.
- Do NOT modify unrelated classes or architectures.
- No memory allocation in processBlock().
- No UI changes in this request.

Feature:
[Describe the feature, e.g.
"Add a subtle high-frequency damping option (`HF Damping`) inside Unravel's FDN reverb feedback loop."]

Requirements:
- Describe the algorithm and where it lives in the signal chain before writing code.
- Add any new parameters using the current APVTS pattern, but:
  - choose sensible default values,
  - define the range and units clearly,
  - mention where in the processor constructor they are registered.
- Implement lifecycle support in prepareToPlay(), reset(), and releaseResources() if needed.
- Use SmoothedValue for any continuous parameter controlling audio.

Code expectations:
- Show header and implementation changes.
- No global variables.
- No extra threads.

Finally:
- Summarize:
  - sonic impact,
  - CPU cost,
  - expected edge cases.
- Provide a short “WHAT CHANGED.md” entry for this feature.
```

------------------------------------------------------------------------

## 5. Refactor (Tightest Possible Scope)

``` text
You are refactoring code in the Threadbare DSP codebase for clarity and maintainability.
This is a behavior-preserving refactor.

Constraints:
- Behavior must remain identical: same inputs → same outputs (within float tolerance).
- Do NOT change APVTS parameter structures.
- Do NOT move logic between UI and DSP layers.
- Do NOT introduce memory allocation in processBlock().
- No architectural re-writes (no "let’s redesign the whole engine").

Refactor target:
[Describe the target.]

Requirements:
- Explain the refactor plan in 3–5 bullets before you write any code.
- Keep public APIs stable unless there is a strong reason.
- Add comments where behavior is subtle.
- Confirm control-rate vs audio-rate logic remains unchanged.

Finally:
- Provide a diff-style summary.
- Suggest a sanity test plan.
```

------------------------------------------------------------------------

## 6. Explain Before You Code

``` text
I am designing a new DSP feature for Threadbare and do NOT want code yet.

Task:
[Describe idea.]

Deliverables:
- High-level description.
- Pseudocode only.
- Notes on memory, threading, update rates, CPU.

After agreement, convert to JUCE 8 C++.
```

------------------------------------------------------------------------

## 7. WHAT CHANGED Generator

``` text
I have just made the following change:
[Paste diff]

Generate WHAT CHANGED.md entry:

## What changed
## Why
## Risks
## Rollback
```

------------------------------------------------------------------------

## 8. UI + DSP Two-Step

### Step 1 --- DSP only

### Step 2 --- UI only

------------------------------------------------------------------------

## 9. Safety Audit

``` text
Review code for:
- allocations in processBlock
- non-atomic parameter reads
- missing smoothing
- UI/DSP thread mixing
- global mutable state
- CPU foot-guns

Output:
- bullet list of violations
- minimal safe fixes
```
