# Frontend Settings Drawer Implementation Plan

## Overview
Add a collapsible "Settings Drawer" to the Unravel UI that reveals advanced DSP controls while keeping the main interface minimal and performance-focused.

---

## 1. Visual Design

### Location & Behavior
- **Position:** Right side of the plugin UI (below freeze button)
- **Toggle:** Click sliders icon → drawer slides in from right
- **Width:** 280-320px
- **Animation:** Smooth 200ms ease-out slide transition
- **Overlay:** Semi-transparent backdrop when open (click to close)

### Visual Language (From Spec)
```
Background:     #26332A (main)
Text/Icons:     #EFDFBD
Accent:         #AFD3E4 (puck center, active states)
Orb:            #E1A6A6
```

### Slider Design (Per Spec Section 2.3)
Each parameter uses a **single horizontal slider** with **no numeric readout**:

1. **Two-Layer Track:**
   - **Base layer (neutral):** Shows stored parameter value
   - **Gold layer:** Indicates puck-applied macro offset (if any)
   - This visualizes "what you set" vs "what the puck is doing"

2. **Pupil Indicator:**
   - Small soft-green indicator to the right of each slider
   - Lights up when puck is contributing a macro value
   - Example: ER Pre-Delay has no puck interaction → no pupil

3. **Interaction:**
   - Tall invisible hit area (3-4× visible track height)
   - Keyboard accessible (arrow keys, modifier for fine control)
   - Optional transient tooltip while dragging (shows value)

---

## 2. Parameters to Include

### Section A: Core Reverb
```
┌─ REVERB ────────────────────────┐
│ Decay       [====·········]     │ ← Puck Y affects (pupil lit)
│ Size        [=====·········]    │ ← No puck (pupil off)
│ Tone        [······=======]     │ ← Puck X affects (pupil lit)
│ Mix         [=====·········]    │ ← No puck (pupil off)
└──────────────────────────────────┘
```

### Section B: Character
```
┌─ CHARACTER ─────────────────────┐
│ Drift       [===···········]    │ ← Puck X+Y affects (pupil lit)
│ Ghost       [··············]    │ ← Puck X affects (pupil lit)
│ Duck        [··············]    │ ← No puck (pupil off)
└──────────────────────────────────┘
```

### Section C: Early Reflections (NEW!)
```
┌─ REFLECTIONS ───────────────────┐
│ Pre-Delay   [··············]    │ ← No puck (pupil off)
│                                  │
│ ℹ️ Tip: Use Puck X for ER mix   │
│   Left = Physical (ERs loud)    │
│   Right = Ethereal (ERs silent) │
└──────────────────────────────────┘
```

### Section D: Output
```
┌─ OUTPUT ────────────────────────┐
│ Level       [=====·········]    │ ← No puck (pupil off)
└──────────────────────────────────┘
```

---

## 3. Implementation Steps

### Phase 1: Create Settings Drawer Component
**File:** `Source/UI/frontend/src/settings-drawer.js`

```javascript
export class SettingsDrawer {
  constructor({ 
    onParamChange,    // Callback when slider changes
    getCurrentState   // Get current parameter values
  }) {
    this.container = this.createDrawer();
    this.sliders = {};
    this.isOpen = false;
  }

  createDrawer() {
    // Create drawer DOM structure
    // Return container element
  }

  createSlider(config) {
    // config: { id, label, min, max, default, hasPuckMacro }
    // Returns slider element with two-layer track
    // Adds pupil indicator if hasPuckMacro = true
  }

  open() {
    // Slide in animation
    // Show backdrop
  }

  close() {
    // Slide out animation
    // Hide backdrop
  }

  updateFromState(state) {
    // Update all sliders from C++ state
    // Highlight gold layer for macro offsets
  }
}
```

### Phase 2: Update Main UI
**File:** `Source/UI/frontend/src/main.js`

```javascript
import { SettingsDrawer } from './settings-drawer.js'

// Initialize drawer
const drawer = new SettingsDrawer({
  onParamChange: (paramId, value) => {
    // Send to C++ via window.sendParameterChange()
    currentState[paramId] = value
  },
  getCurrentState: () => currentState
})

// Add drawer to DOM
document.querySelector('.tb-canvas-shell').appendChild(drawer.container)

// Hook up sliders icon button
document.querySelector('.btn-sliders').addEventListener('click', () => {
  drawer.isOpen ? drawer.close() : drawer.open()
})
```

### Phase 3: C++ ↔ JavaScript Communication
**File:** `Source/UI/UnravelEditor.cpp`

Current state updates already send all parameters via JSON:
```cpp
{
  "puckX": 0.0,
  "puckY": 0.0,
  "mix": 0.5,
  "size": 1.0,
  "decaySeconds": 5.0,
  "tone": 0.0,
  "drift": 0.2,
  "ghost": 0.0,
  "duck": 0.0,
  "erPreDelay": 0.0,  // ← Already included!
  "freeze": false,
  "inLevel": 0.35,
  "tailLevel": 0.4
}
```

**Add bidirectional communication:**
```cpp
// In UnravelEditor constructor, register callback:
webView.bind("sendParameterChange", [this](const juce::var& args) {
    auto paramId = args["id"].toString();
    auto value = (float)args["value"];
    
    if (auto* param = apvts.getParameter(paramId))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost(param->convertTo0to1(value));
        param->endChangeGesture();
    }
});
```

### Phase 4: Styling
**File:** `Source/UI/frontend/src/style.css`

```css
/* Settings Drawer */
.settings-drawer {
  position: fixed;
  right: -320px; /* Hidden by default */
  top: 0;
  width: 320px;
  height: 100%;
  background: #26332A;
  border-left: 1px solid rgba(239, 223, 189, 0.1);
  transition: right 200ms ease-out;
  overflow-y: auto;
  z-index: 100;
}

.settings-drawer.open {
  right: 0;
}

/* Backdrop */
.settings-backdrop {
  position: fixed;
  inset: 0;
  background: rgba(0, 0, 0, 0.4);
  opacity: 0;
  pointer-events: none;
  transition: opacity 200ms;
  z-index: 99;
}

.settings-backdrop.visible {
  opacity: 1;
  pointer-events: auto;
}

/* Slider (Two-Layer Design) */
.param-slider {
  position: relative;
  height: 4px;
  margin: 12px 0;
}

.slider-track-base {
  /* Neutral base layer */
  background: #EFDFBD;
  opacity: 0.2;
  height: 100%;
  border-radius: 2px;
}

.slider-track-macro {
  /* Gold overlay for puck macro contribution */
  position: absolute;
  top: 0;
  left: 0;
  height: 100%;
  background: linear-gradient(90deg, #FFD700, #FFA500);
  opacity: 0;
  transition: opacity 200ms;
  border-radius: 2px;
}

.slider-track-macro.active {
  opacity: 0.5;
}

/* Pupil Indicator */
.param-pupil {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: #4CAF50;
  margin-left: 8px;
  opacity: 0;
  transition: opacity 200ms;
}

.param-pupil.lit {
  opacity: 1;
  box-shadow: 0 0 8px #4CAF50;
}

/* Section Headers */
.param-section {
  padding: 16px;
  border-bottom: 1px solid rgba(239, 223, 189, 0.05);
}

.param-section h3 {
  color: #AFD3E4;
  font-size: 11px;
  text-transform: uppercase;
  letter-spacing: 1px;
  margin-bottom: 12px;
}
```

---

## 4. Parameter Mapping

| Parameter | ID | Range | Default | Puck Macro? | Pupil |
|-----------|----|----|---------|-------------|-------|
| Decay | `decaySeconds` | 0.4-50s | 5.0 | Y (puckY ×3) | ✅ |
| Size | `size` | 0.5-2.0 | 1.0 | N | ❌ |
| Tone | `tone` | -1 to +1 | 0.0 | Y (puckX bias) | ✅ |
| Mix | `mix` | 0-1 | 0.5 | N | ❌ |
| Drift | `drift` | 0-1 | 0.2 | Y (puckX+Y) | ✅ |
| Ghost | `ghost` | 0-1 | 0.0 | Y (puckX) | ✅ |
| Duck | `duck` | 0-1 | 0.0 | N | ❌ |
| ER Pre-Delay | `erPreDelay` | 0-100ms | 0.0 | N | ❌ |
| Output | `output` | -24 to +12dB | 0.0 | N | ❌ |

---

## 5. User Experience Flow

1. **Default State:**
   - Drawer closed, only puck + orb visible
   - User focuses on performance/expression

2. **Open Drawer:**
   - Click sliders icon
   - Drawer slides in smoothly
   - Backdrop dims main UI slightly

3. **Adjust Parameters:**
   - Drag sliders to change values
   - Pupil indicators show which params are puck-affected
   - Gold layer shows how much puck is contributing
   - Tooltip shows exact value while dragging

4. **Close Drawer:**
   - Click sliders icon again, or click backdrop
   - Settings persist
   - Back to minimal performance view

---

## 6. Next Steps

### Immediate (Backend Ready ✅):
- [x] DSP: Add `erPreDelay` parameter
- [x] Processor: Wire up parameter
- [x] Build: Test parameter in DAW host

### Frontend (To Implement):
- [ ] Create `SettingsDrawer` component
- [ ] Implement two-layer slider design
- [ ] Add pupil indicators
- [ ] Style drawer with spec colors
- [ ] Add C++ → JS bidirectional binding
- [ ] Test parameter automation
- [ ] Add keyboard accessibility
- [ ] Polish animations

### Polish:
- [ ] Add preset recall to drawer
- [ ] Add "Reset to Default" per param
- [ ] Add tooltips with parameter descriptions
- [ ] Consider adding "Advanced" toggle for rarely-used params

---

## 7. Technical Notes

### Real-time Communication
- C++ sends state updates every ~16ms (60fps)
- JS debounces slider changes (50ms) to avoid flooding C++
- Use `requestAnimationFrame` for smooth UI updates

### Parameter Macro Visualization
```
Example: Tone parameter with PuckX at Right (+1.0)

Base Tone (manual): 0.0    ───────────●───────────
Macro Bias:        +0.7    ═══════════════════════▶
Effective Tone:    +0.7    ───────────────────●───

Base track:  Shows 0.0 (center)
Gold layer:  Extends to +0.7 (right)
Pupil:       Lit (green glow)
```

### Accessibility
- All sliders keyboard navigable
- Tab order: top to bottom, left to right
- Arrow keys: ±1% change
- Shift+Arrow: ±10% change (coarse)
- Alt+Arrow: ±0.1% change (fine)
- ARIA labels for screen readers

---

## 8. Future Enhancements

### Phase 2 (Post-MVP):
- Preset management in drawer
- A/B comparison toggle
- Parameter link/unlink
- MIDI learn indicators
- Undo/redo for parameter changes

### Phase 3 (Advanced):
- Modulation matrix view
- Spectral analyzer overlay
- Parameter automation curves
- Custom color themes

---

**Status:** DSP implementation complete ✅  
**Next:** Frontend UI implementation  
**Priority:** Medium (enhances UX but not blocking)

