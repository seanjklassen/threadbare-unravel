// =============================================================================
// ELASTIC SLIDER - Shared UI component
// A spring-physics-based slider for audio parameter control
// =============================================================================

const clamp = (value, min = 0, max = 1) => Math.min(Math.max(value, min), max)

// SVG curve constants (viewBox is 0-1000 for precision, height 100)
const VB_WIDTH = 1000
const VB_HEIGHT = 100
const LINE_Y = 50             // Line is vertically centered
const THUMB_RADIUS_VB = 20    // 16px thumb in viewBox units
const CURVE_LIFT = 30         // Vertical lift above the line (gentle bump)
const CURVE_PEAK = LINE_Y - CURVE_LIFT
const CURVE_SPREAD = 90      // How wide the curve spreads horizontally (each side)
const CURVE_EXTEND = 180     // Extra straight line beyond edges for clipping
const RENDER_OVERSHOOT = 0.35 // Max render overshoot beyond bounds
const HIT_SLOP_PX = 8         // Extra horizontal hit area for thumb
const THUMB_RADIUS_PX = 8     // Matches 16px thumb size

const rubberBandDistance = (offset, dimension, constant) => {
  return (dimension * constant * offset) / (dimension + constant * offset)
}

const rubberBand = (value, min = 0, max = 1, constant = 0.35) => {
  if (value < min) {
    return min - rubberBandDistance(min - value, max - min, constant)
  }
  if (value > max) {
    return max + rubberBandDistance(value - max, max - min, constant)
  }
  return value
}

const rubberBandInverse = (value, min = 0, max = 1, constant = 0.35) => {
  if (value >= min && value <= max) return value

  const lower = value < min ? min - 2 : max
  const upper = value > max ? max + 2 : min
  let lo = lower
  let hi = upper

  for (let i = 0; i < 12; i += 1) {
    const mid = (lo + hi) * 0.5
    const mapped = rubberBand(mid, min, max, constant)
    if (value < min) {
      if (mapped < value) hi = mid
      else lo = mid
    } else {
      if (mapped > value) hi = mid
      else lo = mid
    }
  }

  return (lo + hi) * 0.5
}

export class ElasticSlider {
  constructor(element, options = {}) {
    this.element = element
    this.onChange = options.onChange || (() => {})
    
    // DOM elements
    this.track = element.querySelector('.elastic-slider__track')
    this.path = element.querySelector('.slider-path')
    this.thumbEl = element.querySelector('.slider-thumb')
    
    // State
    this.value = parseFloat(element.dataset.value) || 0
    this.targetValue = this.value
    this.displayValue = this.value  // Animated display value
    this.velocity = 0
    this.isDragging = false
    this.pointerId = null
    this.lastVisualValue = this.value
    this.isThumbDrag = false
    this.dragStartRaw = this.value
    this.allowSpringDuringDrag = false
    
    // Fine control (Shift key modifier)
    this.fineControlSensitivity = 0.1  // 10% of normal sensitivity when Shift held
    this.dragStartX = 0
    this.dragStartValue = 0
    
    // Spring physics parameters (refined for smoother feel)
    this.springStiffness = 150   // Snappier rebound
    this.springDamping = 16      // Keeps motion controlled
    this.mass = 1
    
    // Animation state
    this.animationFrame = null
    this.lastTime = 0
    
    // Default value for reset (stored from initial value)
    this.defaultValue = this.value
    
    // Bind methods
    this.handlePointerDown = this.handlePointerDown.bind(this)
    this.handlePointerMove = this.handlePointerMove.bind(this)
    this.handlePointerUp = this.handlePointerUp.bind(this)
    this.handleKeyDown = this.handleKeyDown.bind(this)
    this.handleDoubleClick = this.handleDoubleClick.bind(this)
    this.animate = this.animate.bind(this)
    
    // Initialize
    this.attachEvents()
    this.updateDisplay(this.value)
  }
  
  attachEvents() {
    this.element.addEventListener('pointerdown', this.handlePointerDown)
    this.element.addEventListener('keydown', this.handleKeyDown)
    this.element.addEventListener('dblclick', this.handleDoubleClick)
    
    // Global listeners for drag
    window.addEventListener('pointerup', this.handlePointerUp)
    window.addEventListener('pointercancel', this.handlePointerUp)
  }
  
  detachEvents() {
    this.element.removeEventListener('pointerdown', this.handlePointerDown)
    this.element.removeEventListener('keydown', this.handleKeyDown)
    this.element.removeEventListener('dblclick', this.handleDoubleClick)
    window.removeEventListener('pointerup', this.handlePointerUp)
    window.removeEventListener('pointercancel', this.handlePointerUp)
  }
  
  // Double-click to reset to default value
  handleDoubleClick(event) {
    event.preventDefault()
    event.stopPropagation()
    
    // Reset to default value with animation
    this.setValue(this.defaultValue, true)
    
    // Notify listener
    this.onChange(this.defaultValue)
  }
  
  // Set the default value (can be called externally for preset changes)
  setDefaultValue(value) {
    this.defaultValue = clamp(value)
  }
  
  handlePointerDown(event) {
    event.preventDefault()
    this.isDragging = true
    this.pointerId = event.pointerId
    this.element.classList.add('active')
    
    // Set pointer capture
    this.element.setPointerCapture?.(event.pointerId)
    
    const rect = this.track.getBoundingClientRect()
    const currentRenderValue = Math.max(
      -RENDER_OVERSHOOT,
      Math.min(1 + RENDER_OVERSHOOT, this.lastVisualValue ?? this.displayValue ?? this.value)
    )
    const thumbCenterX = rect.left + rect.width * currentRenderValue
    const isOnThumb = event.target === this.thumbEl
      || Math.abs(event.clientX - thumbCenterX) <= (THUMB_RADIUS_PX + HIT_SLOP_PX)
    
    let rawValue
    let clampedValue
    
    if (isOnThumb) {
      this.isThumbDrag = true
      this.allowSpringDuringDrag = false
      this.dragStartX = event.clientX
      this.dragStartRaw = rubberBandInverse(currentRenderValue)
      rawValue = this.dragStartRaw
      clampedValue = clamp(rawValue)
      this.lastVisualValue = currentRenderValue
    } else {
      this.isThumbDrag = false
      this.allowSpringDuringDrag = true
      const x = event.clientX - rect.left
      rawValue = x / Math.max(1, rect.width)
      clampedValue = clamp(rawValue)
      this.lastVisualValue = null
      this.dragStartX = event.clientX
      this.dragStartRaw = rawValue
    }
    
    // Store drag start position for fine control mode
    this.dragStartValue = clampedValue
    
    // Set target immediately (spring will animate)
    this.targetValue = clampedValue
    this.value = clampedValue
    this.displayValue = this.lastVisualValue ?? this.displayValue
    
    // Start animation if not already running
    if (!this.animationFrame) {
      this.lastTime = performance.now()
      this.animationFrame = requestAnimationFrame(this.animate)
    }
    
    // Immediate visual update (only when dragging the thumb)
    if (this.isThumbDrag) {
      this.updateDisplay(this.lastVisualValue)
    }
    
    // Attach move listener
    window.addEventListener('pointermove', this.handlePointerMove)
    
    // Trigger callback
    this.onChange(this.value)
  }
  
  handlePointerMove(event) {
    if (!this.isDragging) return
    if (this.pointerId !== null && event.pointerId !== this.pointerId) return
    
    this.allowSpringDuringDrag = false
    let newValue
    let rawValue  // Unclamped for elastic effect
    
    // Fine control mode: Shift key reduces sensitivity for precision
    if (event.shiftKey) {
      this.element.classList.add('fine-control')
      
      // Calculate delta from drag start, apply reduced sensitivity
      const deltaX = event.clientX - this.dragStartX
      const rect = this.track.getBoundingClientRect()
      const normalizedDelta = deltaX / Math.max(1, rect.width)
      const fineDelta = normalizedDelta * this.fineControlSensitivity
      rawValue = this.dragStartValue + fineDelta
      newValue = clamp(rawValue)
    } else {
      this.element.classList.remove('fine-control')
      
      const rect = this.track.getBoundingClientRect()
      
      if (this.isThumbDrag) {
        const deltaX = event.clientX - this.dragStartX
        rawValue = this.dragStartRaw + (deltaX / Math.max(1, rect.width))
        newValue = clamp(rawValue)
      } else {
        // Normal mode: direct positioning (allow overshoot for stretch)
        const x = event.clientX - rect.left
        rawValue = x / Math.max(1, rect.width)
        newValue = clamp(rawValue)
        
        // Update drag start for smooth transition if Shift is pressed mid-drag
        this.dragStartX = event.clientX
        this.dragStartValue = newValue
      }
      
    }
    
    // Update target value - spring physics will smooth it
    this.targetValue = newValue
    this.value = newValue
    
    // Elastic visual response (rubber-band past edges)
    const visualValue = rubberBand(rawValue)
    this.lastVisualValue = visualValue
    this.displayValue = visualValue
    this.updateDisplay(visualValue)
    
    // Trigger callback (always with clamped value)
    this.onChange(this.value)
  }
  
  handlePointerUp(event) {
    if (!this.isDragging) return
    if (this.pointerId !== null && event.pointerId !== this.pointerId) return
    
    this.isDragging = false
    this.pointerId = null
    this.element.classList.remove('active')
    this.element.classList.remove('fine-control')
    this.element.releasePointerCapture?.(event.pointerId)
    this.isThumbDrag = false
    
    // Remove move listener
    window.removeEventListener('pointermove', this.handlePointerMove)
    
    // Spring back from the last elastic position
    this.displayValue = this.lastVisualValue ?? this.displayValue
    this.targetValue = this.value
    
    // Start animation to settle any overshoot
    if (!this.animationFrame) {
      this.lastTime = performance.now()
      this.animationFrame = requestAnimationFrame(this.animate)
    }
    
    // Keep current display; animation will settle back
  }
  
  handleKeyDown(event) {
    // Shift = fine control (smaller steps), no Shift = normal steps
    const step = event.shiftKey ? 0.001 : 0.01
    let newValue = this.value
    
    switch (event.key) {
      case 'ArrowLeft':
      case 'ArrowDown':
        newValue = clamp(this.value - step)
        event.preventDefault()
        break
      case 'ArrowRight':
      case 'ArrowUp':
        newValue = clamp(this.value + step)
        event.preventDefault()
        break
      case 'Home':
        newValue = 0
        event.preventDefault()
        break
      case 'End':
        newValue = 1
        event.preventDefault()
        break
      default:
        return
    }
    
    this.setValue(newValue)
    this.onChange(this.value)
  }
  
  // Spring physics animation
  animate(currentTime) {
    const deltaTime = Math.min((currentTime - this.lastTime) / 1000, 0.1)  // Cap delta to prevent instability
    this.lastTime = currentTime
    
    if (!this.isDragging || this.allowSpringDuringDrag) {
      // Spring physics: F = -kx - cv
      const displacement = this.displayValue - this.targetValue
      const springForce = -this.springStiffness * displacement
      const dampingForce = -this.springDamping * this.velocity
      const acceleration = (springForce + dampingForce) / this.mass
      
      this.velocity += acceleration * deltaTime
      this.displayValue += this.velocity * deltaTime
      
      // Check if we've settled
      const isSettled = Math.abs(displacement) < 0.0001 && Math.abs(this.velocity) < 0.001
      
      if (isSettled) {
        this.displayValue = this.targetValue
        this.velocity = 0
        this.animationFrame = null
        this.updateDisplay(this.displayValue)
        return
      }
    } else {
      // During drag, display follows elastic value if present
      this.displayValue = this.lastVisualValue ?? this.targetValue
    }
    
    this.updateDisplay(this.displayValue)
    this.animationFrame = requestAnimationFrame(this.animate)
  }
  
  updateDisplay(value) {
    const clampedValue = clamp(value)
    const renderValue = Math.max(-RENDER_OVERSHOOT, Math.min(1 + RENDER_OVERSHOOT, value))
    const thumbX = renderValue * VB_WIDTH
    
    // Keep a constant curve shape and let the SVG clip at edges.
    const startX = thumbX - CURVE_SPREAD
    const endX = thumbX + CURVE_SPREAD
    const peakY = CURVE_PEAK
    
    // Control points tuned for a softer, rounded crest (less pointy)
    const leftCp1x = startX + CURVE_SPREAD * 0.45
    const leftCp2x = thumbX - CURVE_SPREAD * 0.25
    const rightCp1x = thumbX + CURVE_SPREAD * 0.25
    const rightCp2x = endX - CURVE_SPREAD * 0.45
    
    const lineStartX = -CURVE_EXTEND
    const lineEndX = VB_WIDTH + CURVE_EXTEND
    
    let d = `M ${lineStartX},${LINE_Y} `
    d += `L ${startX},${LINE_Y} `
    d += `C ${leftCp1x},${LINE_Y} ${leftCp2x},${peakY} ${thumbX},${peakY} `
    d += `C ${rightCp1x},${peakY} ${rightCp2x},${LINE_Y} ${endX},${LINE_Y} `
    d += `L ${lineEndX},${LINE_Y}`
    
    // Update SVG path
    if (this.path) {
      this.path.setAttribute('d', d)
    }
    
    // Update thumb position - calculate overshoot for stretch effect
    if (this.thumbEl) {
      this.thumbEl.style.left = `${renderValue * 100}%`
      this.thumbEl.style.transform = 'translate(-50%, -50%)'
    }
    
    // Update ARIA (always use clamped value)
    this.element.setAttribute('aria-valuenow', clampedValue.toFixed(3))
    this.element.dataset.value = clampedValue.toFixed(3)
  }

  
  setValue(newValue, animate = true) {
    newValue = clamp(newValue)
    this.value = newValue
    this.targetValue = newValue
    
    if (animate && !this.isDragging) {
      // Start animation for smooth transition
      if (!this.animationFrame) {
        this.lastTime = performance.now()
        this.animationFrame = requestAnimationFrame(this.animate)
      }
    } else {
      // Immediate update
      this.displayValue = newValue
      this.updateDisplay(newValue)
    }
  }
  
  getValue() {
    return this.value
  }
  
  destroy() {
    this.detachEvents()
    if (this.animationFrame) {
      cancelAnimationFrame(this.animationFrame)
    }
  }
}

