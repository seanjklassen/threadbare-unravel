// =============================================================================
// CONTROLS - Shared UI component
// Puck, settings drawer, and slider controls for audio plugins
// =============================================================================

import { ElasticSlider } from './elastic-slider.js'

const clamp = (value, min = 0, max = 1) => Math.min(Math.max(value, min), max)

const DECAY_RANGE = { min: 0, max: 1 }
const SIZE_RANGE = { min: 0, max: 1 }
const PUCK_RADIUS = 40

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

/**
 * Controls manager for the Threadbare UI shell.
 * 
 * @param {Object} options
 * @param {Object} options.params - Parameter metadata object { paramId: { min, max, default, ... } }
 * @param {string[]} [options.paramOrder] - Order of parameters for drawer display
 * @param {Function} [options.onPuckChange] - Callback when puck position changes
 * @param {Function} [options.onFreezeChange] - Callback when freeze state changes
 * @param {Function} [options.sendParam] - Function to send parameter to backend: sendParam(id, value)
 */
export class Controls {
  constructor(options = {}) {
    // Required: params metadata (injected, NOT imported from generated file)
    this.params = options.params || {}
    this.paramOrder = options.paramOrder || Object.keys(this.params)
    
    // Callbacks
    this.onPuckChange = options.onPuckChange || (() => {})
    this.onFreezeChange = options.onFreezeChange || (() => {})
    this.sendParam = options.sendParam || (() => {})

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
    this.looperState = 'idle'  // 'idle' | 'recording' | 'looping'
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

    // Settings drawer state
    this.elasticSliders = {}
    this.sliders = {}  // Legacy reference (deprecated)
    
    // Build paramMetadata from injected params with formatting functions
    this.paramMetadata = this._buildParamMetadata()

    this.handlePointerDown = this.handlePointerDown.bind(this)
    this.handlePointerMove = this.handlePointerMove.bind(this)
    this.handlePointerUp = this.handlePointerUp.bind(this)
    this.applyInertiaStep = this.applyInertiaStep.bind(this)
    this.animatePuck = this.animatePuck.bind(this)

    // Settings overlay robustness (match presets dropdown behavior)
    this.settingsState = this.settingsView?.classList.contains('open') ? 'open' : 'closed'
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

  _buildParamMetadata() {
    const metadata = {}
    
    // Define formatting functions based on param type/unit
    const formatters = {
      decay: this.formatDecay.bind(this),
      erPreDelay: this.formatPredelay.bind(this),
      size: this.formatSize.bind(this),
      tone: this.formatTone.bind(this),
      drift: this.formatPercent.bind(this),
      ghost: this.formatPercent.bind(this),
      duck: this.formatPercent.bind(this),
      mix: this.formatPercent.bind(this),
      output: this.formatDb.bind(this)
    }
    
    // Build metadata from injected params
    for (const [id, param] of Object.entries(this.params)) {
      metadata[id] = {
        ...param,
        format: formatters[id] || this.formatPercent.bind(this)
      }
    }
    
    return metadata
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
    // Use paramOrder if provided, otherwise fall back to all params
    const paramIds = this.paramOrder.filter(id => 
      // Only include params that are in the drawer (not puckX/puckY/freeze)
      !['puckX', 'puckY', 'freeze'].includes(id)
    )
    
    paramIds.forEach(id => {
      const row = document.querySelector(`.control-row[data-param="${id}"]`)
      if (!row) return

      const sliderElement = row.querySelector('.elastic-slider')
      if (sliderElement) {
        const metadata = this.paramMetadata[id]
        if (!metadata) return
        
        // Create elastic slider instance
        const elasticSlider = new ElasticSlider(sliderElement, {
          onChange: (normValue) => {
            // Convert normalized (0-1) to actual parameter value
            const actualValue = metadata.min + normValue * (metadata.max - metadata.min)
            
            // Send to backend
            this.sendParam(id, actualValue)
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
        this.sendParam('puckX', toDsp(0.5))
        this.sendParam('puckY', toDsp(1 - 0.5))
        this.onPuckChange({ puckX: 0.5, puckY: 0.5 })
        this.renderReadoutsFromNorm(0.5, 0.5)
      })
    }

    if (this.freezeBtn) {
      // Track the freeze value we last sent to DSP (for edge detection)
      this._lastSentFreeze = false
      
      this.freezeBtn.addEventListener('click', () => {
        // Toggle freeze param to create an edge for the DSP state machine
        // DSP responds to edges: rising edge (0→1) advances state
        const nextFreeze = !this._lastSentFreeze
        this._lastSentFreeze = nextFreeze
        
        this.sendParam('freeze', nextFreeze ? 1 : 0)
        this.state.freeze = nextFreeze
        this.onFreezeChange(nextFreeze)
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
      const puckCenterX = bounds.left + dims.minPxX + this.state.puckX * dims.spanX
      const puckCenterY = bounds.top + dims.minPxY + this.state.puckY * dims.spanY
      
      // Store the offset from click point to puck center (in pixels)
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
    this.sendParam('puckX', toDsp(nextX))
    this.sendParam('puckY', toDsp(1 - nextY))
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
      // Send final DSP params when animation completes
      this.sendParam('puckX', toDsp(this.targetPuckX))
      this.sendParam('puckY', toDsp(1 - this.targetPuckY))
      this.onPuckChange({ puckX: this.targetPuckX, puckY: this.targetPuckY })
      return
    }
    
    // Lerp toward target
    const nextX = this.state.puckX + dx * this.lerpSpeed
    const nextY = this.state.puckY + dy * this.lerpSpeed
    this.setPuckPositionImmediate(nextX, nextY)
    this.renderReadoutsFromNorm(nextX, nextY)
    
    // Send DSP params during animation for smooth audio changes
    this.sendParam('puckX', toDsp(nextX))
    this.sendParam('puckY', toDsp(1 - nextY))
    
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

  /**
   * Update freeze button visual for disintegration looper states
   * States: 'idle' | 'recording' | 'looping'
   */
  updateLooperVisual() {
    if (!this.freezeBtn) return
    
    // Remove all looper state classes
    this.freezeBtn.classList.remove('idle', 'recording', 'looping')
    
    // Add current state class
    this.freezeBtn.classList.add(this.looperState)
    
    // Update aria for accessibility
    this.freezeBtn.setAttribute('aria-pressed', String(this.looperState !== 'idle'))
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
    // Process looperState FIRST - it's authoritative for button visual
    if (typeof state.looperState !== 'undefined') {
      const stateMap = ['idle', 'recording', 'looping']
      const newLooperState = stateMap[state.looperState] || 'idle'
      
      if (newLooperState !== this.looperState) {
        const wasLooping = this.looperState === 'looping'
        const nowLooping = newLooperState === 'looping'
        
        // Save puck position when ENTERING looping mode
        if (!wasLooping && nowLooping) {
          this._savedPuckX = this.state.puckX
          this._savedPuckY = this.state.puckY
          // Smoothly animate puck to center for looping mode
          this._loopingPuckTarget = { x: 0.5, y: 0.5 }
          this.animatePuckTo(0.5, 0.5)
          // Sync freeze state: we entered via falling edge, so freeze=0
          this._lastSentFreeze = false
        }
        
        // Restore puck position when EXITING looping mode
        if (wasLooping && !nowLooping && this._savedPuckX !== undefined) {
          // Smoothly animate puck back to saved position
          this.animatePuckTo(this._savedPuckX, this._savedPuckY)
          // Sync freeze state: we exited, DSP is now in Idle with freeze=0
          this._lastSentFreeze = false
          this._savedPuckX = undefined
          this._savedPuckY = undefined
        }
        
        this.looperState = newLooperState
        this.updateLooperVisual()
      }
    }
    
    // Only update freeze visual when in Idle state
    // When Recording or Looping, the looperState controls the visual
    if (typeof state.freeze !== 'undefined') {
      if (this.looperState === 'idle') {
      this.setFreezeVisual(Boolean(state.freeze))
      }
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
        if (!metadata) return
        
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
    this.sendParam('puckX', toDsp(nextX))
    this.sendParam('puckY', toDsp(1 - nextY))
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

