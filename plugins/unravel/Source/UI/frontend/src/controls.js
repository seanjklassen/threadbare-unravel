const clamp = (value, min = 0, max = 1) => Math.min(Math.max(value, min), max)

const DECAY_RANGE = { min: 0, max: 1 }
const SIZE_RANGE = { min: 0, max: 1 }
const PUCK_RADIUS = 40

// ===== ELASTIC SLIDER CLASS =====
class ElasticSlider {
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

const toDsp = (normValue) => clamp(normValue) * 2 - 1
const fromDsp = (dspValue) => clamp((dspValue + 1) / 2)

const normToRange = (normValue, range) =>
  range.min + clamp(normValue) * (range.max - range.min)

const rangeToNorm = (value, range) => {
  if (!range || range.max === range.min) return 0
  return clamp((value - range.min) / (range.max - range.min))
}

const to11Scale = (value = 0, range = { min: 0, max: 1 }) =>
  (rangeToNorm(value, range) * 11).toFixed(2)

// Cache the native function reference
let nativeSetParameter = null

const sendParam = (id, val) => {
  // Lazily get the native function using our polyfill
  if (!nativeSetParameter && window.__getNativeFunction) {
    nativeSetParameter = window.__getNativeFunction('setParameter')
  }
  
  if (typeof nativeSetParameter === 'function') {
    nativeSetParameter(id, val)
  }
  // Silently skip if not available - reduces console spam
}

const getBounds = (el) => el?.getBoundingClientRect()

const normalizeCoord = (value, invert = false) => {
  if (typeof value !== 'number' || Number.isNaN(value)) return null
  let norm =
    value >= -1 && value <= 1
      ? fromDsp(value)
      : clamp(value)
  norm = clamp(norm)
  return invert ? 1 - norm : norm
}

export class Controls {
  constructor(options = {}) {
    this.onPuckChange = options.onPuckChange || (() => {})
    this.onFreezeChange = options.onFreezeChange || (() => {})
    this.onEntropyChange = options.onEntropyChange || (() => {})  // Callback for orb entropy visual

    this.puck = document.getElementById('puck')
    this.surface = document.querySelector('.tb-canvas-shell')
    this.readoutDecay = document.querySelector('[data-readout="x"]')
    this.readoutSize = document.querySelector('[data-readout="y"]')
    this.freezeBtn = document.querySelector('.btn-freeze')
    this.settingsBtn = document.querySelector('.btn-settings')
    this.settingsView = document.querySelector('.settings-view')
    this.settingsBody = document.querySelector('.settings-body')
    this.app = document.getElementById('app')

    this.bounds = getBounds(this.surface)
    this.dimensions = null
    this.state = { puckX: 0.5, puckY: 0.5, freeze: false }
    this.isDragging = false
    this.pointerId = null
    this.dragOffset = { x: 0, y: 0 }
    this.velocity = { x: 0, y: 0 }
    this.lastPointerNorm = { x: 0, y: 0 }
    this.puckRadius = 24
    this.inertiaFrame = null

    // Smooth puck animation state
    this.targetPuckX = 0.5
    this.targetPuckY = 0.5
    this.puckAnimationFrame = null
    this.lerpSpeed = 0.12  // Smoothing factor (0-1, lower = smoother)
    
    // === DISINTEGRATION LOOPER STATE ===
    // looperState: 'idle' | 'recording' | 'looping'
    this.looperState = 'idle'
    this.loopProgress = 0
    this.entropy = 0

    // Settings drawer state
    this.elasticSliders = {}  // New elastic slider instances
    this.sliders = {}  // Legacy reference (deprecated)
    this.paramMetadata = {
      decay: { min: 0.4, max: 50.0, format: this.formatDecay.bind(this) },
      erPreDelay: { min: 0, max: 100, format: this.formatPredelay.bind(this) },
      size: { min: 0.5, max: 2.0, format: this.formatSize.bind(this) },
      tone: { min: -1.0, max: 1.0, format: this.formatTone.bind(this) },
      drift: { min: 0, max: 1, format: this.formatPercent.bind(this) },
      ghost: { min: 0, max: 1, format: this.formatPercent.bind(this) },
      duck: { min: 0, max: 1, format: this.formatPercent.bind(this) },
      mix: { min: 0, max: 1, format: this.formatPercent.bind(this) },
      output: { min: -24, max: 12, format: this.formatDb.bind(this) }
    }

    this.handlePointerDown = this.handlePointerDown.bind(this)
    this.handlePointerMove = this.handlePointerMove.bind(this)
    this.handlePointerUp = this.handlePointerUp.bind(this)
    this.applyInertiaStep = this.applyInertiaStep.bind(this)
    this.animatePuck = this.animatePuck.bind(this)

    // Settings overlay robustness (match presets dropdown behavior)
    this.settingsState = this.settingsView?.classList.contains('open') ? 'open' : 'closed' // 'open' | 'closed'
    this.settingsSuppressToggleUntil = 0
    this.settingsIgnoreClickUntil = 0

    this.onSettingsBtnPointerUp = this.onSettingsBtnPointerUp.bind(this)
    this.onDocPointerDownCaptureForSettings = this.onDocPointerDownCaptureForSettings.bind(this)
    this.onDocKeyDownCaptureForSettings = this.onDocKeyDownCaptureForSettings.bind(this)
    this.onCloseSettingsEvent = this.onCloseSettingsEvent.bind(this)

    this.initDrawerControls()
    this.attachEvents()
    
    // Initialize puck position with retry logic for slow DAW WebViews
    this.initPuckPosition()
  }

  initPuckPosition() {
    const tryPosition = () => {
      this.bounds = getBounds(this.surface)
      if (this.bounds && this.bounds.width > 0 && this.bounds.height > 0) {
        this.cacheDimensions()
        this.setPuckPositionImmediate(this.state.puckX, this.state.puckY)
        return true
      }
      return false
    }

    // Try immediately
    if (tryPosition()) return

    // Retry with requestAnimationFrame (next frame after layout)
    requestAnimationFrame(() => {
      if (tryPosition()) return

      // Second retry after a short delay (for slow DAW WebViews)
      setTimeout(() => {
        if (tryPosition()) return
        
        // Final retry after longer delay
        setTimeout(() => tryPosition(), 200)
      }, 50)
    })
  }

  initDrawerControls() {
    // Initialize all elastic slider references
    const paramIds = ['decay', 'erPreDelay', 'size', 'tone', 'drift', 'ghost', 'duck', 'mix', 'output']
    
    paramIds.forEach(id => {
      const row = document.querySelector(`.control-row[data-param="${id}"]`)
      if (!row) return

      const sliderElement = row.querySelector('.elastic-slider')
      if (sliderElement) {
        const metadata = this.paramMetadata[id]
        
        // Create elastic slider instance
        const elasticSlider = new ElasticSlider(sliderElement, {
          onChange: (normValue) => {
            // Convert normalized (0-1) to actual parameter value
            const actualValue = metadata.min + normValue * (metadata.max - metadata.min)
            
            // Send to backend
            sendParam(id, actualValue)
          }
        })
        
        this.elasticSliders[id] = elasticSlider
      }
    })
  }

  attachEvents() {
    const supportsPointer = typeof window !== 'undefined' && 'PointerEvent' in window

    if (this.puck) {
      this.puck.addEventListener('pointerdown', this.handlePointerDown)
      this.puck.addEventListener('dblclick', (event) => {
        event.preventDefault()
        this.stopInertia()
        this.stopPuckAnimation()
        this.setPuckPositionImmediate(0.5, 0.5)
        sendParam('puckX', toDsp(0.5))
        sendParam('puckY', toDsp(1 - 0.5))
        this.onPuckChange({ puckX: 0.5, puckY: 0.5 })
        this.renderReadoutsFromNorm(0.5, 0.5)
      })
    }

    if (this.freezeBtn) {
      this.freezeBtn.addEventListener('click', () => {
        const next = !this.freezeBtn.classList.contains('active')
        this.setFreezeVisual(next)
        sendParam('freeze', next ? 1 : 0)
        this.onFreezeChange(next)
      })
    }

    // Settings view toggle (same button opens and closes)
    if (this.settingsBtn && this.settingsView) {
      // Prefer pointerup toggle (prevents click-order quirks in DAW WebViews)
      if (supportsPointer) {
        this.settingsBtn.addEventListener('pointerup', this.onSettingsBtnPointerUp)
      } else {
        this.settingsBtn.addEventListener('mouseup', this.onSettingsBtnPointerUp)
      }

      // Keep click for accessibility, but ignore right after pointer toggles.
      this.settingsBtn.addEventListener('click', (e) => {
        e.preventDefault()
        e.stopPropagation()
        if (performance.now() < this.settingsIgnoreClickUntil) return
        this.toggleSettingsView()
      })
    }

    // Elastic sliders are self-contained - they handle their own events
    // via the ElasticSlider class initialized in initDrawerControls()

    window.addEventListener('pointerup', this.handlePointerUp)
    window.addEventListener('pointercancel', this.handlePointerUp)
    window.addEventListener('resize', () => this.refreshBounds())

    // Deterministic close for settings (capture, before click is dispatched)
    if (supportsPointer) {
      document.addEventListener('pointerdown', this.onDocPointerDownCaptureForSettings, { capture: true })
    } else {
      document.addEventListener('mousedown', this.onDocPointerDownCaptureForSettings, { capture: true })
    }
    document.addEventListener('keydown', this.onDocKeyDownCaptureForSettings, { capture: true })

    // Allow other modules (e.g. presets) to close settings deterministically
    document.addEventListener('tb:close-settings', this.onCloseSettingsEvent)
  }

  onCloseSettingsEvent() {
    this.toggleSettingsView(false, { reason: 'external', deferFocusToBtn: true, suppressToggleMs: 250 })
  }

  onSettingsBtnPointerUp(e) {
    // Avoid double-toggles from trailing click
    e.preventDefault()
    e.stopPropagation()
    if (performance.now() < this.settingsSuppressToggleUntil) return
    this.settingsIgnoreClickUntil = performance.now() + 400
    this.toggleSettingsView(undefined, { reason: 'button', deferFocusToBtn: false })
  }

  onDocPointerDownCaptureForSettings(e) {
    if (this.settingsState !== 'open') return

    const t = e.target && e.target.nodeType === 1 ? e.target : e.target?.parentElement
    if (!t) return

    // Ignore clicks on the settings button (let button toggle own the gesture)
    if (this.settingsBtn && this.settingsBtn.contains(t)) return

    // Ignore clicks inside the drawer content
    if (this.settingsBody && this.settingsBody.contains(t)) return

    // If the click landed on the settings overlay (backdrop), close immediately.
    if (this.settingsView && this.settingsView.contains(t)) {
      e.preventDefault()
      this.settingsIgnoreClickUntil = performance.now() + 400
      this.toggleSettingsView(false, { reason: 'backdrop', deferFocusToBtn: true, suppressToggleMs: 250 })
    }
  }

  onDocKeyDownCaptureForSettings(e) {
    if (this.settingsState !== 'open') return
    if (e.key !== 'Escape') return
    e.preventDefault()
    e.stopPropagation()
    this.settingsIgnoreClickUntil = performance.now() + 400
    this.toggleSettingsView(false, { reason: 'escape', deferFocusToBtn: true, suppressToggleMs: 250 })
  }

  refreshBounds() {
    this.bounds = getBounds(this.surface)
    this.cacheDimensions()
    this.setPuckPositionImmediate(this.state.puckX, this.state.puckY)
  }

  handlePointerDown(event) {
    if (!this.puck) return
    this.cacheBounds()
    this.stopInertia()
    this.stopPuckAnimation()
    this.isDragging = true
    this.pointerId = event.pointerId
    this.puck.classList.add('active')
    this.puck.setPointerCapture?.(event.pointerId)
    
    const bounds = this.bounds
    const dims = this.dimensions
    if (bounds && dims) {
      // Calculate the puck's actual center position in screen coordinates
      // (must match the formula used in setPuckPositionImmediate)
      const puckCenterX = bounds.left + dims.minPxX + this.state.puckX * dims.spanX
      const puckCenterY = bounds.top + dims.minPxY + this.state.puckY * dims.spanY
      
      // Store the offset from click point to puck center (in pixels)
      // This ensures the puck stays under the same part of the finger/cursor
      this.dragOffset = {
        x: event.clientX - puckCenterX,
        y: event.clientY - puckCenterY,
      }
    } else {
      this.dragOffset = { x: 0, y: 0 }
    }
    
    this.lastPointerNorm = { x: this.state.puckX, y: this.state.puckY }
    this.velocity = { x: 0, y: 0 }
    window.addEventListener('pointermove', this.handlePointerMove)
  }

  handlePointerMove(event) {
    if (!this.isDragging || (this.pointerId !== null && event.pointerId !== this.pointerId)) {
      return
    }
    this.updateFromPointer(event)
  }

  handlePointerUp(event) {
    if (!this.isDragging) return
    if (this.pointerId !== null && event.pointerId !== this.pointerId && event.type !== 'pointercancel') {
      return
    }
    this.isDragging = false
    this.pointerId = null
    this.puck?.classList.remove('active')
    this.puck?.releasePointerCapture?.(event.pointerId)
    window.removeEventListener('pointermove', this.handlePointerMove)
    this.startInertia()
  }

  cacheBounds() {
    this.bounds = getBounds(this.surface) || this.bounds
    this.cacheDimensions()
  }

  cacheDimensions() {
    const bounds = this.bounds
    if (!bounds) return
    const width = Math.max(bounds.width, 1)
    const height = Math.max(bounds.height, 1)
    const minPxX = Math.min(PUCK_RADIUS, width / 2)
    const maxPxX = Math.max(width - PUCK_RADIUS, minPxX)
    const minPxY = Math.min(PUCK_RADIUS, height / 2)
    const maxPxY = Math.max(height - PUCK_RADIUS, minPxY)
    this.dimensions = {
      width,
      height,
      minPxX,
      maxPxX,
      spanX: Math.max(maxPxX - minPxX, 1),
      minPxY,
      maxPxY,
      spanY: Math.max(maxPxY - minPxY, 1),
    }
  }

  pointerToNorm(event) {
    this.cacheBounds()
    const bounds = this.bounds
    const dims = this.dimensions
    if (!bounds || !dims) return { x: this.state.puckX, y: this.state.puckY }
    const centerAbsX = event.clientX - (this.dragOffset?.x || 0)
    const centerAbsY = event.clientY - (this.dragOffset?.y || 0)
    const localX = centerAbsX - bounds.left
    const localY = centerAbsY - bounds.top
    const clampedX = clamp(localX, dims.minPxX, dims.maxPxX)
    const clampedY = clamp(localY, dims.minPxY, dims.maxPxY)
    const normX = (clampedX - dims.minPxX) / dims.spanX
    const normY = (clampedY - dims.minPxY) / dims.spanY
    return { x: clamp(normX), y: clamp(normY) }
  }

  updateFromPointer(event) {
    event.preventDefault()
    const { x, y } = this.pointerToNorm(event)
    const nextX = clamp(x)
    const nextY = clamp(y)
    this.velocity = {
      x: nextX - this.lastPointerNorm.x,
      y: nextY - this.lastPointerNorm.y,
    }
    this.lastPointerNorm = { x: nextX, y: nextY }
    this.setPuckPositionImmediate(nextX, nextY)
    sendParam('puckX', toDsp(nextX))
    sendParam('puckY', toDsp(1 - nextY))
    this.onPuckChange({ puckX: nextX, puckY: nextY })
    this.renderReadoutsFromNorm(nextX, nextY)
  }

  // Immediately set puck position without animation
  setPuckPositionImmediate(x = this.state.puckX, y = this.state.puckY) {
    if (!this.puck) return
    this.cacheBounds()
    const dims = this.dimensions
    if (!dims) return
    const clampedX = clamp(x)
    const clampedY = clamp(y)
    this.state.puckX = clampedX
    this.state.puckY = clampedY
    const visualX = dims.minPxX + clampedX * dims.spanX
    const visualY = dims.minPxY + clampedY * dims.spanY
    this.puck.style.left = `${visualX}px`
    this.puck.style.top = `${visualY}px`
  }

  // Set puck position with smooth animation (used for preset changes)
  setPuckPosition(x = this.state.puckX, y = this.state.puckY) {
    this.setPuckPositionImmediate(x, y)
  }

  // Animate puck to target position
  animatePuckTo(targetX, targetY) {
    this.targetPuckX = clamp(targetX)
    this.targetPuckY = clamp(targetY)
    
    // Start animation if not already running
    if (!this.puckAnimationFrame) {
      this.animatePuck()
    }
  }

  // Animation loop using lerp
  animatePuck() {
    const dx = this.targetPuckX - this.state.puckX
    const dy = this.targetPuckY - this.state.puckY
    
    // If close enough, snap to target and stop animation
    if (Math.abs(dx) < 0.001 && Math.abs(dy) < 0.001) {
      this.setPuckPositionImmediate(this.targetPuckX, this.targetPuckY)
      this.puckAnimationFrame = null
      this.renderReadoutsFromNorm(this.targetPuckX, this.targetPuckY)
      return
    }
    
    // Lerp toward target
    const nextX = this.state.puckX + dx * this.lerpSpeed
    const nextY = this.state.puckY + dy * this.lerpSpeed
    this.setPuckPositionImmediate(nextX, nextY)
    this.renderReadoutsFromNorm(nextX, nextY)
    
    // Notify orb of position change during animation
    this.onPuckChange({ puckX: nextX, puckY: nextY })
    
    this.puckAnimationFrame = requestAnimationFrame(() => this.animatePuck())
  }

  // Stop puck animation
  stopPuckAnimation() {
    if (this.puckAnimationFrame) {
      cancelAnimationFrame(this.puckAnimationFrame)
      this.puckAnimationFrame = null
    }
  }

  setFreezeVisual(isActive) {
    if (!this.freezeBtn) return
    this.freezeBtn.classList.toggle('active', isActive)
    this.freezeBtn.setAttribute('aria-pressed', String(!!isActive))
    this.state.freeze = !!isActive
  }
  
  // === DISINTEGRATION LOOPER VISUAL STATE ===
  updateLooperVisual() {
    if (!this.freezeBtn) return
    
    // Clear all looper states from button
    this.freezeBtn.classList.remove('recording', 'looping', 'active')
    
    // === PUCK VISUAL STATE ===
    // Update puck classes for pupil color + breathing animation
    if (this.puck) {
      this.puck.classList.remove('idle', 'recording', 'looping')
      this.puck.classList.add(this.looperState)
    }
    
    switch (this.looperState) {
      case 'recording':
        this.freezeBtn.classList.add('recording')
        this.freezeBtn.setAttribute('aria-label', `Recording... ${Math.round(this.loopProgress * 100)}%`)
        break
        
      case 'looping':
        this.freezeBtn.classList.add('looping')
        // Show entropy level in aria-label for accessibility
        this.freezeBtn.setAttribute('aria-label', `Looping - Entropy ${Math.round(this.entropy * 100)}%`)
        break
        
      case 'idle':
      default:
        this.freezeBtn.setAttribute('aria-label', 'Disintegrate')
        break
    }
  }

  /**
   * Toggle the full-screen settings view
   * @param {boolean} [shouldOpen] - Force open (true) or close (false), or toggle if undefined
   */
  toggleSettingsView(shouldOpen, { reason = 'unknown', deferFocusToBtn = false, suppressToggleMs = 0 } = {}) {
    if (!this.settingsView || !this.app) return
    
    const isCurrentlyOpen = this.settingsView.classList.contains('open')
    const nextState = shouldOpen !== undefined ? shouldOpen : !isCurrentlyOpen
    
    if (nextState) {
      if (performance.now() < this.settingsSuppressToggleUntil) return

      // Mutual exclusivity: opening settings closes presets
      document.dispatchEvent(new CustomEvent('tb:close-presets'))

      // Opening settings view
      this.settingsView.classList.add('open')
      this.settingsView.setAttribute('aria-hidden', 'false')
      this.app.classList.add('settings-open')
      this.settingsBtn?.setAttribute('aria-expanded', 'true')
      this.settingsBtn?.setAttribute('aria-label', 'Close settings')
      this.settingsState = 'open'

      // Focus first control (deferred to avoid retargeting in WebViews)
      const focusFirst = () => {
        const first = this.settingsView.querySelector('.elastic-slider, button, [tabindex]')
        first?.focus?.({ preventScroll: true })
      }
      if (typeof queueMicrotask === 'function') queueMicrotask(focusFirst)
      else setTimeout(focusFirst, 0)
    } else {
      // Closing settings view
      this.settingsView.classList.remove('open')
      this.settingsView.setAttribute('aria-hidden', 'true')
      this.app.classList.remove('settings-open')
      this.settingsBtn?.setAttribute('aria-expanded', 'false')
      this.settingsBtn?.setAttribute('aria-label', 'Open settings')
      this.settingsState = 'closed'

      if (suppressToggleMs > 0) {
        this.settingsSuppressToggleUntil = performance.now() + suppressToggleMs
      }

      // Return focus to settings button (optionally deferred)
      const focusBtn = () => this.settingsBtn?.focus?.({ preventScroll: true })
      if (deferFocusToBtn) {
        if (typeof queueMicrotask === 'function') queueMicrotask(focusBtn)
        else setTimeout(focusBtn, 0)
      } else {
        focusBtn()
      }
    }
  }

  update(state = {}) {
    // DEBUG: Always show debug element
    let debugEl = document.getElementById('looper-debug')
    if (!debugEl) {
      debugEl = document.createElement('div')
      debugEl.id = 'looper-debug'
      debugEl.style.cssText = 'position:fixed;top:10px;left:10px;background:rgba(0,0,0,0.9);color:#0f0;font-family:monospace;font-size:11px;padding:8px;z-index:9999;border-radius:4px;max-width:200px;'
      document.body.appendChild(debugEl)
    }
    const hasLooper = typeof state.looperState !== 'undefined'
    debugEl.innerHTML = `looperState: ${hasLooper ? state.looperState : 'MISSING'}<br>freeze: ${state.freeze}<br>keys: ${Object.keys(state).slice(0,5).join(',')}`
    
    let nextX = this.state.puckX
    let nextY = this.state.puckY

    const incomingX = normalizeCoord(state.puckX)
    const incomingY = normalizeCoord(state.puckY, true)
    if (incomingX !== null) nextX = incomingX
    if (incomingY !== null) nextY = incomingY

    if (!this.isDragging) {
      // Check if position changed significantly (likely a preset change)
      const dx = Math.abs(nextX - this.state.puckX)
      const dy = Math.abs(nextY - this.state.puckY)
      const significantChange = dx > 0.05 || dy > 0.05
      
      if (significantChange && !this.inertiaFrame) {
        // Animate to new position smoothly (preset change)
        this.animatePuckTo(nextX, nextY)
      } else if (!this.puckAnimationFrame) {
        // Small change or no animation running - set immediately
        this.setPuckPositionImmediate(nextX, nextY)
      }
    }

    if (!this.isDragging) {
      this.applyInertiaIfNeeded()
    }

    // === DISINTEGRATION LOOPER STATE UPDATE ===
    // looperState from backend: 0 = Idle, 1 = Recording, 2 = Looping
    if (typeof state.looperState !== 'undefined') {
      const stateMap = ['idle', 'recording', 'looping']
      const newState = stateMap[state.looperState] || 'idle'
      
      if (newState !== this.looperState) {
        this.looperState = newState
        this.updateLooperVisual()
      }
    }
    
    if (typeof state.loopProgress !== 'undefined') {
      this.loopProgress = state.loopProgress
      // Update visual during recording (progress indicator)
      if (this.looperState === 'recording') {
        this.updateLooperVisual()
      }
    }
    
    if (typeof state.entropy !== 'undefined') {
      this.entropy = state.entropy
      // Notify orb of entropy changes for visual feedback
      this.onEntropyChange?.(state.entropy)
      // Update aria-label during looping
      if (this.looperState === 'looping') {
        this.updateLooperVisual()
      }
    }
    
    // Legacy freeze visual (for backwards compatibility, though looper state takes precedence)
    if (typeof state.freeze !== 'undefined' && this.looperState === 'idle') {
      this.setFreezeVisual(Boolean(state.freeze))
    }

    // Map backend state property names to frontend parameter IDs
    const stateMapping = {
      'decay': 'decaySeconds',  // Backend sends decaySeconds
      'erPreDelay': 'erPreDelay',
      'size': 'size',
      'tone': 'tone',
      'drift': 'drift',
      'ghost': 'ghost',
      'duck': 'duck',
      'mix': 'mix',
      'output': 'output'
    }

    // Update elastic slider parameters from state
    Object.keys(this.elasticSliders).forEach(id => {
      const stateKey = stateMapping[id] || id
      
      if (typeof state[stateKey] !== 'undefined') {
        const metadata = this.paramMetadata[id]
        const actualValue = state[stateKey]
        
        // Convert actual value to normalized (0-1)
        const normValue = clamp((actualValue - metadata.min) / (metadata.max - metadata.min))
        
        // Update elastic slider with animation
        const elasticSlider = this.elasticSliders[id]
        if (elasticSlider && !elasticSlider.isDragging) {
          elasticSlider.setValue(normValue, true)
        }
      }
    })

    // Only update readouts if not currently dragging
    // (During drag, readouts are updated directly by drag handlers)
    if (!this.isDragging) {
      this.renderReadoutsFromNorm(nextX, nextY)
    }
  }

  refresh() {
    this.refreshBounds()
  }

  renderReadoutsFromNorm(normX = this.state.puckX, normY = this.state.puckY) {
    const clampedX = clamp(normX)
    const clampedY = clamp(normY)
    if (this.readoutDecay) {
      this.readoutDecay.textContent = to11Scale(clampedX, SIZE_RANGE)
    }
    if (this.readoutSize) {
      // Invert Y so top = 11.00, bottom = 0.00
      this.readoutSize.textContent = to11Scale(1 - clampedY, DECAY_RANGE)
    }
  }

  applyInertiaIfNeeded() {
    if (this.velocity && (Math.abs(this.velocity.x) > 0.0001 || Math.abs(this.velocity.y) > 0.0001)) {
      this.startInertia()
    }
  }

  startInertia() {
    if (this.inertiaFrame) return
    const threshold = 0.0002
    if (Math.abs(this.velocity.x) < threshold && Math.abs(this.velocity.y) < threshold) {
      this.velocity = { x: 0, y: 0 }
      return
    }
    this.inertiaFrame = requestAnimationFrame(this.applyInertiaStep)
  }

  stopInertia() {
    if (this.inertiaFrame) {
      cancelAnimationFrame(this.inertiaFrame)
      this.inertiaFrame = null
    }
    this.velocity = { x: 0, y: 0 }
  }

  applyInertiaStep() {
    if (this.isDragging) {
      this.stopInertia()
      return
    }

    this.velocity.x *= 0.92
    this.velocity.y *= 0.92

    const threshold = 0.001
    if (Math.abs(this.velocity.x) < threshold && Math.abs(this.velocity.y) < threshold) {
      this.stopInertia()
      return
    }

    let nextX = this.state.puckX + this.velocity.x
    let nextY = this.state.puckY + this.velocity.y

    if (nextX <= 0 || nextX >= 1) {
      nextX = clamp(nextX)
      this.velocity.x *= -0.4
    }
    if (nextY <= 0 || nextY >= 1) {
      nextY = clamp(nextY)
      this.velocity.y *= -0.4
    }

    this.setPuckPositionImmediate(nextX, nextY)
    sendParam('puckX', toDsp(nextX))
    sendParam('puckY', toDsp(1 - nextY))
    this.onPuckChange({ puckX: nextX, puckY: nextY })
    this.renderReadoutsFromNorm(nextX, nextY)

    this.inertiaFrame = requestAnimationFrame(this.applyInertiaStep)
  }

  // === VALUE FORMATTING HELPERS ===
  formatDecay(seconds) {
    if (seconds < 1) {
      return `${(seconds * 1000).toFixed(0)}ms`
    }
    return `${seconds.toFixed(1)}s`
  }

  formatPredelay(ms) {
    return `${Math.round(ms)}ms`
  }

  formatSize(scalar) {
    return scalar.toFixed(1)
  }

  formatTone(bipolar) {
    // Map -1..1 to 0..11 scale for display
    const scaled = (bipolar + 1) / 2 * 11
    return scaled.toFixed(2)
  }

  formatPercent(normalized) {
    // Map 0..1 to 0..11 scale for display
    const scaled = normalized * 11
    return scaled.toFixed(2)
  }

  formatDb(db) {
    const rounded = Math.round(db)
    return rounded >= 0 ? `+${rounded}dB` : `${rounded}dB`
  }
}

