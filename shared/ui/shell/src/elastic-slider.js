// =============================================================================
// ELASTIC SLIDER - Shared UI component
// A spring-physics-based slider for audio parameter control
// =============================================================================

const clamp = (value, min = 0, max = 1) => Math.min(Math.max(value, min), max)

export class ElasticSlider {
  constructor(element, options = {}) {
    this.element = element
    this.onChange = options.onChange || (() => {})
    
    // DOM elements
    this.track = element.querySelector('.elastic-slider__track')
    this.fill = element.querySelector('.elastic-slider__fill')
    
    // State
    this.value = parseFloat(element.dataset.value) || 0
    this.targetValue = this.value
    this.displayValue = this.value  // Animated display value
    this.velocity = 0
    this.isDragging = false
    this.pointerId = null
    
    // Fine control (Shift key modifier)
    this.fineControlSensitivity = 0.1  // 10% of normal sensitivity when Shift held
    this.dragStartX = 0
    this.dragStartValue = 0
    
    // Spring physics parameters (refined for smoother feel)
    this.springStiffness = 120   // Slightly softer
    this.springDamping = 14      // More damping for elegance
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
    
    // Trigger bounce animation
    this.fill.classList.add('bounce')
    setTimeout(() => {
      this.fill.classList.remove('bounce')
    }, 500)
    
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
    
    // Remove any ongoing bounce animations
    this.fill.classList.remove('bounce')
    
    // Set pointer capture
    this.element.setPointerCapture?.(event.pointerId)
    
    // Store drag start position for fine control mode
    this.dragStartX = event.clientX
    this.dragStartValue = this.value
    
    // Get initial position (normal click positioning)
    const rect = this.track.getBoundingClientRect()
    const x = event.clientX - rect.left
    const newValue = clamp(x / rect.width)
    
    // Set target immediately (spring will animate)
    this.targetValue = newValue
    this.value = newValue
    
    // Start animation if not already running
    if (!this.animationFrame) {
      this.lastTime = performance.now()
      this.animationFrame = requestAnimationFrame(this.animate)
    }
    
    // Attach move listener
    window.addEventListener('pointermove', this.handlePointerMove)
    
    // Trigger callback
    this.onChange(this.value)
  }
  
  handlePointerMove(event) {
    if (!this.isDragging) return
    if (this.pointerId !== null && event.pointerId !== this.pointerId) return
    
    const rect = this.track.getBoundingClientRect()
    let newValue
    
    // Fine control mode: Shift key reduces sensitivity for precision
    if (event.shiftKey) {
      this.element.classList.add('fine-control')
      
      // Calculate delta from drag start, apply reduced sensitivity
      const deltaX = event.clientX - this.dragStartX
      const normalizedDelta = deltaX / rect.width
      const fineDelta = normalizedDelta * this.fineControlSensitivity
      newValue = clamp(this.dragStartValue + fineDelta)
    } else {
      this.element.classList.remove('fine-control')
      
      // Normal mode: direct positioning
      const x = event.clientX - rect.left
      newValue = clamp(x / rect.width)
      
      // Update drag start for smooth transition if Shift is pressed mid-drag
      this.dragStartX = event.clientX
      this.dragStartValue = newValue
    }
    
    // Update target value - spring physics will smooth it
    this.targetValue = newValue
    this.value = newValue
    
    // For immediate feedback, directly update display during drag
    this.updateDisplay(newValue)
    
    // Trigger callback
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
    
    // Remove move listener
    window.removeEventListener('pointermove', this.handlePointerMove)
    
    // Trigger elastic settle animation on fill
    this.fill.classList.add('bounce')
    
    // Remove bounce class after animation completes
    setTimeout(() => {
      this.fill.classList.remove('bounce')
    }, 500)
    
    // Give the spring some initial velocity for subtle bounce effect
    const delta = this.targetValue - this.displayValue
    this.velocity = delta * 3  // Gentler impulse
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
    
    if (!this.isDragging) {
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
      // During drag, display follows target closely
      this.displayValue = this.targetValue
    }
    
    this.updateDisplay(this.displayValue)
    this.animationFrame = requestAnimationFrame(this.animate)
  }
  
  updateDisplay(value) {
    const percent = clamp(value) * 100
    this.fill.style.width = `${percent}%`
    
    // Update ARIA
    this.element.setAttribute('aria-valuenow', value.toFixed(3))
    this.element.dataset.value = value.toFixed(3)
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

