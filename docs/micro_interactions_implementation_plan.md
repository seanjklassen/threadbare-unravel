# Micro-Interactions Implementation Plan

**Version:** 1.0.0  
**Target:** Unravel Frontend (Vanilla JS/CSS)  
**Philosophy:** Sophisticated, timeless, performant animations that enhance without distracting

---

## Implementation Phases

### Phase 1: Foundation (Safe, High-Impact)
1. Staggered Cascade - Settings Drawer
2. Staggered Cascade - Preset Dropdown
4. Settings Drawer Spring Overshoot
6. Focus Ripple Effect

### Phase 2: Premium Polish
5. Parameter Value Morphing
7. Puck Memory Trace
8. Orb Breathing Animation

### Phase 3: Innovative Depth
9. Magnetic Cursor Glow
10. Gravity Well Labels
a. Orb Depth of Field
b. Settings Icon Shape Morph
c. Freeze Icon Loop Animation
d. App Load Fade-In
e. Orb Reactivity Zones

### Phase 4: Experimental
f. Haptic Feedback (Web Vibration API)
+ 10 Additional Innovative Concepts

---

## Detailed Implementation Plans

---

### 1. Staggered Cascade - Settings Drawer

**Complexity:** Low  
**Impact:** High  
**Performance:** Excellent (CSS-only)

**Implementation:**

```css
/* style.css - Add to settings section */

/* Base hidden state for control rows */
.settings-content .control-row {
  opacity: 0;
  transform: translateY(16px);
  transition: 
    opacity 0.4s cubic-bezier(0.22, 1, 0.36, 1),
    transform 0.4s cubic-bezier(0.22, 1, 0.36, 1);
}

/* Revealed state when settings open */
.tb-app.settings-open .settings-content .control-row {
  opacity: 1;
  transform: translateY(0);
}

/* Staggered delays - 30ms apart for waterfall effect */
.tb-app.settings-open .control-row:nth-child(1) { transition-delay: 0.06s; }
.tb-app.settings-open .control-row:nth-child(2) { transition-delay: 0.09s; }
.tb-app.settings-open .control-row:nth-child(3) { transition-delay: 0.12s; }
.tb-app.settings-open .control-row:nth-child(4) { transition-delay: 0.15s; }
.tb-app.settings-open .control-row:nth-child(5) { transition-delay: 0.18s; }
.tb-app.settings-open .control-row:nth-child(6) { transition-delay: 0.21s; }
.tb-app.settings-open .control-row:nth-child(7) { transition-delay: 0.24s; }
.tb-app.settings-open .control-row:nth-child(8) { transition-delay: 0.27s; }
.tb-app.settings-open .control-row:nth-child(9) { transition-delay: 0.30s; }

/* Reset delays when closing (all close together) */
.settings-content .control-row {
  transition-delay: 0s;
}
```

**Files to modify:** `style.css`

---

### 2. Staggered Cascade - Preset Dropdown

**Complexity:** Low  
**Impact:** High  
**Performance:** Excellent (CSS-only)

**Implementation:**

```css
/* style.css - Add to preset dropdown section */

.preset-option {
  opacity: 0;
  transform: translateX(-12px);
  transition: 
    opacity 0.3s cubic-bezier(0.22, 1, 0.36, 1),
    transform 0.3s cubic-bezier(0.22, 1, 0.36, 1),
    background 120ms ease;
}

.preset-pill.open .preset-option {
  opacity: 1;
  transform: translateX(0);
}

/* Staggered delays */
.preset-pill.open .preset-option:nth-child(1) { transition-delay: 0.02s; }
.preset-pill.open .preset-option:nth-child(2) { transition-delay: 0.05s; }
.preset-pill.open .preset-option:nth-child(3) { transition-delay: 0.08s; }
.preset-pill.open .preset-option:nth-child(4) { transition-delay: 0.11s; }
.preset-pill.open .preset-option:nth-child(5) { transition-delay: 0.14s; }
.preset-pill.open .preset-option:nth-child(6) { transition-delay: 0.17s; }
.preset-pill.open .preset-option:nth-child(7) { transition-delay: 0.20s; }
.preset-pill.open .preset-option:nth-child(8) { transition-delay: 0.23s; }

/* Reset on close */
.preset-option {
  transition-delay: 0s;
}
```

**Files to modify:** `style.css`

---

### 4. Settings Drawer Spring Overshoot

**Complexity:** Low  
**Impact:** Medium-High  
**Performance:** Excellent (CSS animation)

**Implementation:**

```css
/* style.css - Replace existing settings-view transition */

@keyframes drawer-spring-in {
  0% { 
    opacity: 0;
    transform: translateY(var(--settings-slide-distance));
  }
  70% { 
    opacity: 1;
    transform: translateY(-6px); /* Slight overshoot */
  }
  85% {
    transform: translateY(2px); /* Settle back */
  }
  100% { 
    transform: translateY(0);
  }
}

@keyframes drawer-spring-out {
  0% { 
    opacity: 1;
    transform: translateY(0);
  }
  100% { 
    opacity: 0;
    transform: translateY(var(--settings-slide-distance));
  }
}

.tb-app.settings-open .settings-view {
  opacity: 1;
  visibility: visible;
  pointer-events: auto;
  animation: drawer-spring-in 0.5s cubic-bezier(0.22, 1, 0.36, 1) forwards;
}

.settings-view.closing {
  animation: drawer-spring-out 0.3s cubic-bezier(0.22, 1, 0.36, 1) forwards;
}
```

**JS modification needed:** Add `.closing` class before removing `.settings-open`

```javascript
// controls.js - modify toggleSettings()
function closeSettings() {
  const settingsView = document.querySelector('.settings-view');
  settingsView.classList.add('closing');
  
  setTimeout(() => {
    document.querySelector('.tb-app').classList.remove('settings-open');
    settingsView.classList.remove('closing');
  }, 300);
}
```

**Files to modify:** `style.css`, `controls.js`

---

### 5. Parameter Value Morphing

**Complexity:** Medium  
**Impact:** Medium  
**Performance:** Good (requestAnimationFrame)

**Implementation:**

```javascript
// controls.js - Add value animation utility

class ValueAnimator {
  constructor(element, formatFn) {
    this.element = element;
    this.formatFn = formatFn || (v => v.toFixed(2));
    this.currentValue = 0;
    this.targetValue = 0;
    this.animating = false;
  }
  
  setValue(newValue, animate = true) {
    this.targetValue = newValue;
    
    if (!animate || Math.abs(this.targetValue - this.currentValue) < 0.001) {
      this.currentValue = newValue;
      this.element.textContent = this.formatFn(newValue);
      return;
    }
    
    if (!this.animating) {
      this.animating = true;
      this.animate();
    }
  }
  
  animate() {
    const diff = this.targetValue - this.currentValue;
    
    // Exponential ease-out
    this.currentValue += diff * 0.15;
    
    // Snap when close enough
    if (Math.abs(diff) < 0.001) {
      this.currentValue = this.targetValue;
      this.animating = false;
    }
    
    this.element.textContent = this.formatFn(this.currentValue);
    
    if (this.animating) {
      requestAnimationFrame(() => this.animate());
    }
  }
}

// Usage in Controls class:
// this.valueAnimators = new Map();
// For each param-value element, create a ValueAnimator
```

**Files to modify:** `controls.js`

---

### 6. Focus Ripple Effect

**Complexity:** Low  
**Impact:** Medium  
**Performance:** Excellent (CSS-only)

**Implementation:**

```css
/* style.css - Add focus ripple */

@keyframes focus-ripple-expand {
  0% { 
    transform: scale(0.8);
    opacity: 0.6;
  }
  100% { 
    transform: scale(1.1);
    opacity: 0;
  }
}

/* For sliders */
.elastic-slider:focus-visible::after {
  content: '';
  position: absolute;
  inset: -4px;
  border-radius: 8px;
  border: 2px solid var(--accent);
  animation: focus-ripple-expand 0.6s cubic-bezier(0.22, 1, 0.36, 1) forwards;
  pointer-events: none;
}

/* For buttons */
.btn-freeze:focus-visible::after,
.btn-settings:focus-visible::after {
  content: '';
  position: absolute;
  inset: -2px;
  border-radius: 23px;
  border: 2px solid var(--accent);
  animation: focus-ripple-expand 0.6s cubic-bezier(0.22, 1, 0.36, 1) forwards;
  pointer-events: none;
}
```

**Files to modify:** `style.css`

---

### 7. Puck Memory Trace

**Complexity:** Medium  
**Impact:** High (unique brand identity)  
**Performance:** Good (Canvas-based, needs optimization)

**Implementation:**

```javascript
// orb.js - Add trail system

class PuckTrail {
  constructor(maxLength = 12) {
    this.points = [];
    this.maxLength = maxLength;
  }
  
  addPoint(x, y) {
    this.points.push({ x, y, age: 0 });
    
    if (this.points.length > this.maxLength) {
      this.points.shift();
    }
  }
  
  update() {
    // Age all points
    this.points.forEach(p => p.age++);
    
    // Remove old points
    this.points = this.points.filter(p => p.age < 30);
  }
  
  draw(ctx) {
    if (this.points.length < 2) return;
    
    this.points.forEach((point, i) => {
      const alpha = (1 - point.age / 30) * 0.25;
      const size = (1 - point.age / 30) * 6;
      
      ctx.beginPath();
      ctx.arc(point.x, point.y, size, 0, Math.PI * 2);
      ctx.fillStyle = `rgba(200, 199, 184, ${alpha})`;
      ctx.fill();
    });
  }
}

// In Orb class, add:
// this.puckTrail = new PuckTrail();
// 
// In render loop:
// this.puckTrail.addPoint(puckX, puckY);
// this.puckTrail.update();
// this.puckTrail.draw(this.ctx);
```

**Files to modify:** `orb.js`

---

### 8. Orb Breathing Animation

**Complexity:** Low-Medium  
**Impact:** High (ambient life)  
**Performance:** Good (CSS animation on canvas elements)

**Implementation:**

```javascript
// orb.js - Add breathing modulation

class OrbBreathing {
  constructor() {
    this.phase = 0;
    this.speed = 0.008; // Slow, meditative
  }
  
  update() {
    this.phase += this.speed;
    if (this.phase > Math.PI * 2) this.phase -= Math.PI * 2;
  }
  
  // Returns 0.85 - 1.0 opacity modulation
  getOpacity(ringIndex, totalRings) {
    const offset = (ringIndex / totalRings) * Math.PI * 0.5;
    return 0.85 + Math.sin(this.phase + offset) * 0.15;
  }
  
  // Returns subtle scale modulation (0.98 - 1.02)
  getScale(ringIndex, totalRings) {
    const offset = (ringIndex / totalRings) * Math.PI * 0.3;
    return 1 + Math.sin(this.phase + offset) * 0.015;
  }
}

// Apply in orb render:
// ctx.globalAlpha = this.breathing.getOpacity(i, rings.length);
// const scale = this.breathing.getScale(i, rings.length);
```

**Files to modify:** `orb.js`

---

### 9. Magnetic Cursor Glow

**Complexity:** Low  
**Impact:** Medium-High (ambient atmosphere)  
**Performance:** Good (CSS custom properties)

**Implementation:**

```html
<!-- index.html - Add after body open tag -->
<div class="cursor-glow" aria-hidden="true"></div>
```

```css
/* style.css */

.cursor-glow {
  position: fixed;
  inset: 0;
  pointer-events: none;
  z-index: 1000;
  opacity: 0;
  transition: opacity 0.3s ease;
  background: radial-gradient(
    300px circle at var(--cursor-x, 50%) var(--cursor-y, 50%),
    rgba(224, 233, 147, 0.04),
    transparent 70%
  );
}

.tb-app:hover .cursor-glow {
  opacity: 1;
}

/* Reduce effect when settings open (avoid distraction) */
.tb-app.settings-open .cursor-glow {
  opacity: 0.3;
}
```

```javascript
// main.js - Add cursor tracking

function initCursorGlow() {
  const glow = document.querySelector('.cursor-glow');
  if (!glow) return;
  
  let rafId = null;
  let targetX = 0, targetY = 0;
  let currentX = 0, currentY = 0;
  
  document.addEventListener('mousemove', (e) => {
    targetX = e.clientX;
    targetY = e.clientY;
    
    if (!rafId) {
      rafId = requestAnimationFrame(updateGlow);
    }
  });
  
  function updateGlow() {
    // Smooth follow
    currentX += (targetX - currentX) * 0.1;
    currentY += (targetY - currentY) * 0.1;
    
    glow.style.setProperty('--cursor-x', `${currentX}px`);
    glow.style.setProperty('--cursor-y', `${currentY}px`);
    
    if (Math.abs(targetX - currentX) > 0.1 || Math.abs(targetY - currentY) > 0.1) {
      rafId = requestAnimationFrame(updateGlow);
    } else {
      rafId = null;
    }
  }
}
```

**Files to modify:** `index.html`, `style.css`, `main.js`

---

### 10. Gravity Well Labels

**Complexity:** Medium  
**Impact:** Low-Medium (subtle spatial awareness)  
**Performance:** Moderate (needs throttling)

**Implementation:**

```javascript
// controls.js - Add gravity effect

class LabelGravity {
  constructor() {
    this.labels = document.querySelectorAll('.param-label');
    this.enabled = true;
    this.maxRotation = 1.5; // degrees
  }
  
  update(puckX, puckY) {
    if (!this.enabled) return;
    
    this.labels.forEach(label => {
      const rect = label.getBoundingClientRect();
      const centerX = rect.left + rect.width / 2;
      const centerY = rect.top + rect.height / 2;
      
      const dx = puckX - centerX;
      const dy = puckY - centerY;
      const distance = Math.hypot(dx, dy);
      
      // Influence falls off with distance
      const influence = Math.max(0, 1 - distance / 600);
      const angle = Math.atan2(dy, dx);
      
      // Subtle lean toward puck
      const rotation = angle * influence * this.maxRotation * (180 / Math.PI) * 0.01;
      const translateX = dx * influence * 0.01;
      
      label.style.transform = `translateX(${translateX}px) rotate(${rotation}deg)`;
    });
  }
  
  reset() {
    this.labels.forEach(label => {
      label.style.transform = '';
    });
  }
}
```

**Files to modify:** `controls.js`, integrate with puck position updates

---

## Additional Features (a-f)

### a. Orb Depth of Field

**Complexity:** Medium  
**Impact:** High (cinematic depth)  
**Performance:** Moderate (blur is expensive)

**Approach:** Use layered canvas or CSS blur on outer rings

```javascript
// orb.js - Depth of field based on puck distance from center

getBlurForRing(ringIndex, totalRings, puckDistanceFromCenter) {
  // Outer rings blur more when puck is centered
  // Inner rings blur when puck is at edge
  const ringPosition = ringIndex / totalRings;
  const normalizedPuckDist = puckDistanceFromCenter / this.maxRadius;
  
  // Focal plane follows puck
  const focalPlane = normalizedPuckDist;
  const distanceFromFocal = Math.abs(ringPosition - focalPlane);
  
  // Max 2px blur
  return distanceFromFocal * 2;
}

// Apply via CSS filter or separate canvas layers
```

**Alternative (cheaper):** Opacity-based depth instead of blur

---

### b. Settings Icon Shape Morph (SVG Path Animation)

**Complexity:** High  
**Impact:** High (delightful detail)  
**Performance:** Good (SMIL or JS path interpolation)

**Approach:** Use matching path point counts between icons

```html
<!-- Both icons need same number of path points -->
<svg class="icon-morph" viewBox="0 0 24 24">
  <path class="morph-path" d="..." />
</svg>
```

```javascript
// Icon morph using path interpolation
const settingsPath = "M12,15.5A3.5,3.5..."; // Settings gear
const closePath = "M19,6.41L17.59,5...";    // X close

function morphIcon(progress) {
  // Interpolate between path data
  // Libraries: flubber, GSAP MorphSVG, or manual lerp
}
```

**Recommended library:** `flubber` for path morphing (lightweight)

---

### c. Freeze Icon Loop Animation

**Complexity:** Low  
**Impact:** Medium (status feedback)  
**Performance:** Excellent

```css
@keyframes infinity-trace {
  0% {
    stroke-dashoffset: 100;
  }
  100% {
    stroke-dashoffset: 0;
  }
}

.btn-freeze[aria-pressed="true"] svg path {
  stroke-dasharray: 100;
  animation: infinity-trace 2s linear infinite;
}
```

**Or particle effect around the icon:**

```css
@keyframes orbit-particle {
  0% { transform: rotate(0deg) translateX(20px) rotate(0deg); }
  100% { transform: rotate(360deg) translateX(20px) rotate(-360deg); }
}

.btn-freeze[aria-pressed="true"]::after {
  content: '';
  position: absolute;
  width: 4px;
  height: 4px;
  background: var(--accent);
  border-radius: 50%;
  animation: orbit-particle 3s linear infinite;
}
```

---

### d. App Load Fade-In

**Complexity:** Low  
**Impact:** High (first impression)  
**Performance:** Excellent

```css
/* style.css */

@keyframes app-reveal {
  0% {
    opacity: 0;
    transform: scale(0.98);
  }
  100% {
    opacity: 1;
    transform: scale(1);
  }
}

.tb-app {
  animation: app-reveal 0.6s cubic-bezier(0.22, 1, 0.36, 1) forwards;
}

/* Stagger child elements */
.tb-top-bar {
  opacity: 0;
  animation: app-reveal 0.5s cubic-bezier(0.22, 1, 0.36, 1) 0.1s forwards;
}

.tb-canvas-shell {
  opacity: 0;
  animation: app-reveal 0.6s cubic-bezier(0.22, 1, 0.36, 1) 0.2s forwards;
}

.tb-bottom-bar {
  opacity: 0;
  animation: app-reveal 0.5s cubic-bezier(0.22, 1, 0.36, 1) 0.3s forwards;
}
```

---

### e. Orb Reactivity Zones

**Complexity:** Medium-High  
**Impact:** High (interactive discovery)  
**Performance:** Moderate

```javascript
// orb.js - Define reactivity zones

const ZONES = {
  center: { radius: 0.2, effect: 'compress' },
  middle: { radius: 0.5, effect: 'ripple' },
  outer: { radius: 1.0, effect: 'expand' }
};

function getZoneEffect(puckNormalizedDistance) {
  if (puckNormalizedDistance < ZONES.center.radius) {
    // Orb rings compress toward center
    return { type: 'compress', intensity: 1 - puckNormalizedDistance / ZONES.center.radius };
  } else if (puckNormalizedDistance < ZONES.middle.radius) {
    // Rings ripple outward from puck
    return { type: 'ripple', intensity: 0.5 };
  } else {
    // Outer rings expand/breathe more
    return { type: 'expand', intensity: puckNormalizedDistance };
  }
}
```

---

### f. Haptic Feedback

**Complexity:** Low  
**Impact:** Medium (mobile/gamepad only)  
**Performance:** Excellent

```javascript
// utils.js - Haptic utility

const Haptics = {
  supported: 'vibrate' in navigator,
  
  light() {
    if (this.supported) navigator.vibrate(10);
  },
  
  medium() {
    if (this.supported) navigator.vibrate(25);
  },
  
  heavy() {
    if (this.supported) navigator.vibrate([30, 20, 30]);
  },
  
  success() {
    if (this.supported) navigator.vibrate([10, 50, 20]);
  }
};

// Usage:
// Haptics.light(); // On slider drag start
// Haptics.medium(); // On button press
// Haptics.success(); // On preset change
```

**Note:** Only works on mobile browsers and some gamepads. Desktop browsers don't support vibration.

---

## 10 Additional Innovative Concepts

### 11. **Temporal Echo Visualization**
When adjusting decay/delay parameters, show a visual "echo" of the orb fading out over time, previewing the reverb tail length.

### 12. **Parameter Constellations**
Connect related parameters with subtle lines (like constellations). When one changes, the connecting lines pulse.

### 13. **Ambient Sound Reactivity**
If the plugin receives audio, subtly modulate the orb's breathing speed/intensity based on input amplitude.

### 14. **Gesture Trails**
When dragging the puck quickly, leave a "comet tail" effect that shows velocity and direction.

### 15. **Preset Transition Morphing**
When changing presets, animate all parameter values simultaneously with staggered easing, creating a visual "morph" between states.

### 16. **Contextual Cursor**
Cursor changes based on what it's hovering: subtle glow near sliders, grabbing hand near puck, pointer near buttons.

### 17. **Micro-Parallax Layers**
Split the orb into depth layers that shift slightly based on cursor position, creating subtle 3D depth.

### 18. **Value Whispers**
When hovering over a parameter, show the value in a subtle, floating tooltip that follows the cursor.

### 19. **Undo/Redo Ripples**
When using keyboard shortcuts for undo/redo, show a subtle ripple effect emanating from the changed parameter.

### 20. **Session Memory Heatmap**
Subtly highlight areas of the orb where the puck has spent the most time in the current session (accumulative glow).

---

## Implementation Priority Matrix

| Feature | Impact | Effort | Priority |
|---------|--------|--------|----------|
| 1. Settings Cascade | High | Low | 🟢 P1 |
| 2. Preset Cascade | High | Low | 🟢 P1 |
| d. App Load Fade | High | Low | 🟢 P1 |
| 4. Drawer Spring | Medium-High | Low | 🟢 P1 |
| c. Freeze Loop | Medium | Low | 🟢 P1 |
| 6. Focus Ripple | Medium | Low | 🟡 P2 |
| 8. Orb Breathing | High | Medium | 🟡 P2 |
| 9. Cursor Glow | Medium-High | Low | 🟡 P2 |
| 7. Puck Trail | High | Medium | 🟡 P2 |
| 5. Value Morph | Medium | Medium | 🟡 P2 |
| 10. Gravity Labels | Low-Medium | Medium | 🟠 P3 |
| a. Depth of Field | High | High | 🟠 P3 |
| b. Icon Morph | High | High | 🟠 P3 |
| e. Reactivity Zones | High | High | 🟠 P3 |
| f. Haptics | Low | Low | 🔴 P4 |

---

## Technical Considerations

### Performance Budget
- Target: 60fps on mid-range devices
- Canvas operations: Batch draw calls
- CSS animations: Prefer `transform` and `opacity`
- Avoid: `filter: blur()` on large elements during animation

### Accessibility
- Respect `prefers-reduced-motion` media query
- Ensure animations don't interfere with screen readers
- Maintain keyboard navigation throughout

```css
@media (prefers-reduced-motion: reduce) {
  *, *::before, *::after {
    animation-duration: 0.01ms !important;
    transition-duration: 0.01ms !important;
  }
}
```

### Testing Strategy
1. Test on low-end devices (throttle CPU in DevTools)
2. Verify animations complete properly on rapid interactions
3. Check memory usage over time (no leaks from animation frames)

---

## Next Steps

1. **Implement P1 features** (cascade animations, app load, drawer spring)
2. **Test performance** on target devices
3. **Gather feedback** on animation timing/intensity
4. **Iterate** on P2 features based on P1 learnings
5. **Document** animation timing tokens for consistency


