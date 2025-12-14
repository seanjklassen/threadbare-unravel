# Orb Flicker - Deep Diagnosis Report

## Problem Description
The Orb visualizer flickers when dragging the puck, appearing to render two different states simultaneously (primary lines + ghost lines visible at once).

## Root Cause Analysis

### 1. ⚠️ **CRITICAL: ResizeObserver Firing During Puck Drag**

**Location:** `orb.js:126-147` (ResizeObserver callback)

**Issue:**
- ResizeObserver can fire during puck dragging, even when canvas dimensions haven't actually changed
- Possible triggers:
  - Puck CSS `transition: transform 0.1s ease-out` (line 78 in style.css) might cause layout recalculation
  - Parent container `.tb-canvas-shell` uses `position: absolute` with `top: 60px; bottom: 80px` - any parent resize triggers observer
  - Browser layout thrashing during drag operations

**Evidence:**
- Even with `scheduleResize()` deferring to next frame, ResizeObserver can fire multiple times rapidly
- Each call to `scheduleResize()` cancels previous, but there's a race window

**Impact:** HIGH - This is the most likely cause of flicker

---

### 2. ⚠️ **Race Condition: Resize Between Draw Frames**

**Location:** `orb.js:163-170` (scheduleResize) and `orb.js:250-254` (draw)

**Issue:**
The timing sequence can be:
1. Frame N: `draw()` starts, calls `clearRect()`, begins drawing
2. **During Frame N:** ResizeObserver fires → `scheduleResize()` → schedules for next frame
3. Frame N+1: Scheduled `resize()` runs → sets `canvas.width` → **clears canvas context**
4. Frame N+1: `draw()` runs → calls `clearRect()` again → draws

**Problem:**
- If `resize()` runs at the START of a frame (before `draw()`), the canvas is cleared
- Then `draw()` clears again and draws
- But if there's any delay or if `draw()` was already mid-execution, we see partial renders

**Impact:** HIGH - This explains the "two renders" visual artifact

---

### 3. ⚠️ **Multiple Resize Sources**

**Location:** 
- `orb.js:126-147` (ResizeObserver)
- `main.js:135-144` (window 'resize' event)

**Issue:**
- Both ResizeObserver AND window 'resize' event call `scheduleResize()`
- If both fire simultaneously (e.g., window resize triggers both), we get duplicate schedules
- Current implementation cancels previous, but timing can still cause issues

**Impact:** MEDIUM - Could compound with issue #1

---

### 4. ⚠️ **Context State Reset During Resize**

**Location:** `orb.js:226-237` (resize method)

**Issue:**
When `canvas.width` is set:
- Canvas context is **completely reset** (all state cleared)
- We restore `lineWidth` and transforms, but:
  - `globalAlpha` is reset to 1.0
  - `strokeStyle` is reset
  - `lineJoin`, `lineCap` are reset
  - Any other context state is lost

**Current Code:**
```javascript
this.canvas.width = Math.floor(width * dpr)  // ← Clears context
this.canvas.height = Math.floor(height * dpr) // ← Clears context
this.ctx.setTransform(1, 0, 0, 1, 0, 0)
this.ctx.scale(dpr, dpr)
this.ctx.lineWidth = CONFIG.stroke.baseWidth || 2.5
// ❌ Missing: globalAlpha, strokeStyle, lineJoin, lineCap, etc.
```

**Impact:** MEDIUM - If `draw()` is using stale context state, could cause rendering issues

---

### 5. ⚠️ **Guard Clause Timing Issue**

**Location:** `orb.js:203-205` (resize guard clause)

**Issue:**
The guard checks dimensions BEFORE updating `lastLogicalWidth/Height`:
```javascript
if (width === this.lastLogicalWidth && height === this.lastLogicalHeight) {
  return
}
// ... then later ...
this.lastLogicalWidth = width
this.lastLogicalHeight = height
```

**Problem:**
- If ResizeObserver fires twice with same dimensions in rapid succession:
  - First call: guard passes, resize runs, dimensions stored
  - Second call: guard catches it, returns early ✓
- BUT: If ResizeObserver fires with slightly different dimensions (due to rounding or sub-pixel), guard doesn't catch it
- Even with `Math.floor()`, browser might report slightly different values

**Impact:** LOW - Guard should work, but sub-pixel issues could bypass it

---

### 6. ⚠️ **Double ClearRect in Same Frame**

**Location:** `orb.js:254` (draw method)

**Issue:**
Every `draw()` call starts with:
```javascript
ctx.clearRect(0, 0, this.width, this.height)
```

If `resize()` runs in the same frame (before `draw()`):
1. `resize()` sets `canvas.width` → clears canvas
2. `draw()` calls `clearRect()` → clears again (redundant but harmless)

**However:** If `resize()` runs AFTER `draw()` starts but before it completes:
- Partial render visible
- Canvas cleared mid-draw
- Next frame draws on cleared canvas
- **Result: Flicker**

**Impact:** MEDIUM - Timing-dependent, but possible

---

### 7. ⚠️ **Puck Animation Loop Interference**

**Location:** `controls.js:387-408` (animatePuck)

**Issue:**
- `animatePuck()` runs in its own `requestAnimationFrame` loop
- Calls `this.onPuckChange({ puckX, puckY })` every frame during animation
- This triggers `orb.update(currentState)` in `main.js:108`
- If puck animation is running while user drags, we have:
  - Main animation loop: `orb.update()` + `orb.draw()`
  - Puck animation loop: `onPuckChange()` → `orb.update()` again
  - Multiple updates per frame could cause state thrashing

**Impact:** LOW - Updates are idempotent, but could cause visual jitter

---

### 8. ⚠️ **CSS Transitions on Puck**

**Location:** `style.css:78` (puck transition)

**Issue:**
```css
#puck {
  transition: transform 0.1s ease-out, box-shadow 0.1s ease-out;
}
```

**Problem:**
- CSS transitions can trigger layout recalculations
- Layout changes can trigger ResizeObserver
- Even if canvas size doesn't change, parent layout might shift slightly

**Impact:** LOW-MEDIUM - Could trigger unnecessary ResizeObserver events

---

## Most Likely Root Cause

**Combination of Issues #1 and #2:**

1. **ResizeObserver fires during puck drag** (triggered by layout recalculation or CSS transitions)
2. **`scheduleResize()` defers to next frame**, but:
   - If ResizeObserver fires multiple times rapidly, we get multiple scheduled resizes
   - Even with cancellation, there's a race window
3. **Resize runs at start of frame** (before or during `draw()`)
4. **Canvas is cleared mid-render** → partial render visible → flicker

## Recommended Fixes (Priority Order)

### Fix 1: **Debounce ResizeObserver** (CRITICAL)
- Add debouncing to ResizeObserver callback
- Only schedule resize if dimensions actually changed AND stable for ~16ms
- Prevents rapid-fire resize events during drag

### Fix 2: **Synchronize Resize with Draw Loop** (CRITICAL)
- Instead of `requestAnimationFrame` in `scheduleResize()`, use a flag
- Check flag at START of `draw()` method
- If resize pending, do it BEFORE `clearRect()`
- Ensures resize happens at predictable time (start of draw cycle)

### Fix 3: **Disable ResizeObserver During Drag** (HIGH)
- Track drag state in Orb class
- Temporarily disconnect ResizeObserver when drag starts
- Reconnect when drag ends
- Prevents resize events during user interaction

### Fix 4: **Restore Full Context State After Resize** (MEDIUM)
- After setting `canvas.width`, restore ALL context properties used in `draw()`
- Include: `globalAlpha`, `strokeStyle`, `lineJoin`, `lineCap`, etc.
- Prevents rendering artifacts from stale state

### Fix 5: **Remove Window Resize Handler** (LOW)
- ResizeObserver should handle all resize events
- Window 'resize' event is redundant and can cause duplicate schedules
- Remove `resizeCanvas()` call from window 'resize' listener

### Fix 6: **Add Resize Lock During Draw** (MEDIUM)
- Add a flag `isDrawing` set at start of `draw()`, cleared at end
- In `resize()`, check flag and defer if drawing
- Prevents resize from interrupting active draw

---

## Testing Strategy

1. **Add console logging:**
   - Log every ResizeObserver callback with dimensions
   - Log every `scheduleResize()` call
   - Log every `resize()` execution
   - Log every `draw()` start/end

2. **Monitor during puck drag:**
   - Count ResizeObserver events
   - Check if dimensions actually change
   - Verify timing of resize vs draw

3. **Test with CSS transitions disabled:**
   - Temporarily remove `transition` from `#puck`
   - See if flicker persists
   - Confirms if CSS is triggering ResizeObserver

---

## Code Locations to Modify

1. `Source/UI/frontend/src/orb.js`:
   - `setupResizeObserver()` - Add debouncing
   - `scheduleResize()` - Change to flag-based approach
   - `resize()` - Add draw lock check, restore full context state
   - `draw()` - Check resize flag at start

2. `Source/UI/frontend/src/main.js`:
   - `resizeCanvas()` - Consider removing or making it no-op
   - `initApp()` - Remove window 'resize' listener if ResizeObserver handles all

3. `Source/UI/frontend/src/controls.js`:
   - `handlePointerDown()` - Notify Orb that drag started
   - `handlePointerUp()` - Notify Orb that drag ended
   - Orb can temporarily disable ResizeObserver during drag

---

## Conclusion

The flicker is most likely caused by **ResizeObserver firing during puck drag** and **resize operations interrupting the draw loop**. The combination of rapid resize events and timing-sensitive canvas clearing creates the "two renders" visual artifact.

**Primary fix:** Debounce ResizeObserver and synchronize resize with draw loop.
**Secondary fix:** Disable ResizeObserver during active drag operations.

