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

    this.puck = document.getElementById('puck')
    this.surface = document.querySelector('.tb-canvas-shell')
    this.readoutDecay = document.querySelector('[data-readout="x"]')
    this.readoutSize = document.querySelector('[data-readout="y"]')
    this.freezeBtn = document.querySelector('.btn-freeze')
    this.settingsBtn = document.querySelector('.btn-settings')
    this.drawer = document.querySelector('.settings-drawer')
    this.closeDrawerBtn = document.querySelector('.btn-close-drawer')

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

    // Settings drawer state
    this.sliders = {}
    this.pupils = {}
    this.pupilEnabled = {}
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

    this.initDrawerControls()
    this.attachEvents()
  }

  initDrawerControls() {
    // Initialize all slider and pupil references
    const paramIds = ['decay', 'erPreDelay', 'size', 'tone', 'drift', 'ghost', 'duck', 'mix', 'output']
    
    paramIds.forEach(id => {
      const row = document.querySelector(`.control-row[data-param="${id}"]`)
      if (!row) return

      const slider = row.querySelector('.param-slider')
      const valueDisplay = row.querySelector('.param-value')
      const pupil = row.querySelector('.pupil-toggle')
      const baseTrack = row.querySelector('.slider-track-base')
      const macroTrack = row.querySelector('.slider-track-macro')

      if (slider && valueDisplay && pupil && baseTrack && macroTrack) {
        this.sliders[id] = { 
          slider, 
          valueDisplay, 
          baseTrack, 
          macroTrack,
          currentValue: parseFloat(slider.value)
        }
        this.pupils[id] = pupil
        this.pupilEnabled[id] = pupil.getAttribute('aria-pressed') === 'true'
      }
    })
  }

  attachEvents() {
    if (this.puck) {
      this.puck.addEventListener('pointerdown', this.handlePointerDown)
      this.puck.addEventListener('dblclick', (event) => {
        event.preventDefault()
        this.stopInertia()
        this.setPuckPosition(0.5, 0.5)
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

    // Drawer toggle
    if (this.settingsBtn && this.drawer) {
      this.settingsBtn.addEventListener('click', () => {
        this.drawer.classList.toggle('open')
      })
    }

    if (this.closeDrawerBtn && this.drawer) {
      this.closeDrawerBtn.addEventListener('click', () => {
        this.drawer.classList.remove('open')
      })
    }

    // Attach slider listeners
    Object.keys(this.sliders).forEach(id => {
      const { slider, valueDisplay, baseTrack } = this.sliders[id]
      
      slider.addEventListener('input', (e) => {
        const normValue = parseFloat(e.target.value)
        this.sliders[id].currentValue = normValue
        
        // Update base track width
        baseTrack.style.width = `${normValue * 100}%`
        
        // Update value display
        const metadata = this.paramMetadata[id]
        const actualValue = metadata.min + normValue * (metadata.max - metadata.min)
        valueDisplay.textContent = metadata.format(actualValue)
        
        // Send to backend (backend expects actual parameter value, not normalized)
        sendParam(id, actualValue)
      })
    })

    // Attach pupil toggle listeners
    Object.keys(this.pupils).forEach(id => {
      const pupil = this.pupils[id]
      
      pupil.addEventListener('click', () => {
        this.togglePupil(id)
      })
    })

    window.addEventListener('pointerup', this.handlePointerUp)
    window.addEventListener('pointercancel', this.handlePointerUp)
    window.addEventListener('resize', () => this.refreshBounds())
  }

  refreshBounds() {
    this.bounds = getBounds(this.surface)
    this.cacheDimensions()
    this.setPuckPosition(this.state.puckX, this.state.puckY)
  }

  handlePointerDown(event) {
    if (!this.puck) return
    this.cacheBounds()
    this.stopInertia()
    this.isDragging = true
    this.pointerId = event.pointerId
    this.puck.classList.add('active')
    this.puck.setPointerCapture?.(event.pointerId)
    const bounds = this.bounds
    if (bounds) {
      const centerX = bounds.left + bounds.width * this.state.puckX
      const centerY = bounds.top + bounds.height * this.state.puckY
      this.dragOffset = {
        x: event.clientX - centerX,
        y: event.clientY - centerY,
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
    this.setPuckPosition(nextX, nextY)
    sendParam('puckX', toDsp(nextX))
    sendParam('puckY', toDsp(1 - nextY))
    this.onPuckChange({ puckX: nextX, puckY: nextY })
    this.renderReadoutsFromNorm(nextX, nextY)
  }

  setPuckPosition(x = this.state.puckX, y = this.state.puckY) {
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

  setFreezeVisual(isActive) {
    if (!this.freezeBtn) return
    this.freezeBtn.classList.toggle('active', isActive)
    this.freezeBtn.setAttribute('aria-pressed', String(!!isActive))
    this.state.freeze = !!isActive
  }

  update(state = {}) {
    let nextX = this.state.puckX
    let nextY = this.state.puckY

    const incomingX = normalizeCoord(state.puckX)
    const incomingY = normalizeCoord(state.puckY, true)
    if (incomingX !== null) nextX = incomingX
    if (incomingY !== null) nextY = incomingY

    if (!this.isDragging) {
      this.setPuckPosition(nextX, nextY)
    }

    if (!this.isDragging) {
      this.applyInertiaIfNeeded()
    }

    if (typeof state.freeze !== 'undefined') {
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

    // Update drawer parameters from state
    Object.keys(this.sliders).forEach(id => {
      const stateKey = stateMapping[id] || id
      
      if (typeof state[stateKey] !== 'undefined') {
        const metadata = this.paramMetadata[id]
        const actualValue = state[stateKey]
        
        // Convert actual value to normalized (0-1)
        const normValue = clamp((actualValue - metadata.min) / (metadata.max - metadata.min))
        
        const slider = this.sliders[id]
        slider.currentValue = normValue
        slider.slider.value = normValue
        slider.baseTrack.style.width = `${normValue * 100}%`
        slider.valueDisplay.textContent = metadata.format(actualValue)
      }

      // Update macro layer if there's a macro offset in state
      if (typeof state[`${id}Macro`] !== 'undefined') {
        this.updateMacroLayer(id, state[`${id}Macro`])
      }
    })

    // Update pupil states from state
    Object.keys(this.pupils).forEach(id => {
      if (typeof state[`${id}Enabled`] !== 'undefined') {
        const shouldBeEnabled = Boolean(state[`${id}Enabled`])
        if (this.pupilEnabled[id] !== shouldBeEnabled) {
          this.pupilEnabled[id] = shouldBeEnabled
          this.pupils[id].setAttribute('aria-pressed', String(shouldBeEnabled))
          
          const macroTrack = this.sliders[id]?.macroTrack
          if (macroTrack) {
            if (shouldBeEnabled) {
              macroTrack.classList.remove('hidden')
            } else {
              macroTrack.classList.add('hidden')
            }
          }
        }
      }
    })

    this.renderReadoutsFromNorm(nextX, nextY)
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
      this.readoutSize.textContent = to11Scale(clampedY, DECAY_RANGE)
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

    this.setPuckPosition(nextX, nextY)
    sendParam('puckX', toDsp(nextX))
    sendParam('puckY', toDsp(1 - nextY))
    this.onPuckChange({ puckX: nextX, puckY: nextY })
    this.renderReadoutsFromNorm(nextX, nextY)

    this.inertiaFrame = requestAnimationFrame(this.applyInertiaStep)
  }

  // === PUPIL TOGGLE LOGIC ===
  togglePupil(id) {
    const pupil = this.pupils[id]
    if (!pupil) return

    // Toggle enabled state
    this.pupilEnabled[id] = !this.pupilEnabled[id]
    const isEnabled = this.pupilEnabled[id]

    // Update ARIA state
    pupil.setAttribute('aria-pressed', String(isEnabled))

    // Add pulse animation
    pupil.classList.add('pulse')
    setTimeout(() => pupil.classList.remove('pulse'), 300)

    // Update macro track visibility
    const macroTrack = this.sliders[id]?.macroTrack
    if (macroTrack) {
      if (isEnabled) {
        macroTrack.classList.remove('hidden')
      } else {
        macroTrack.classList.add('hidden')
      }
    }

    // Send to backend (for future preset storage)
    if (typeof window.setParameterEnabled === 'function') {
      window.setParameterEnabled(id, isEnabled)
    }
  }

  updateMacroLayer(id, macroOffset) {
    if (!this.sliders[id]) return
    
    const { macroTrack, currentValue } = this.sliders[id]
    const isEnabled = this.pupilEnabled[id]

    if (isEnabled && macroOffset !== 0) {
      // Calculate effective value with macro offset
      const effectiveValue = clamp(currentValue + macroOffset)
      macroTrack.style.width = `${effectiveValue * 100}%`
      macroTrack.classList.remove('hidden')
    } else {
      // Just show base value
      macroTrack.style.width = `${currentValue * 100}%`
      if (!isEnabled) {
        macroTrack.classList.add('hidden')
      }
    }
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

