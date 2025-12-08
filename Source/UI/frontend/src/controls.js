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

const sendParam = (id, val) => {
  if (typeof window.setParameter === 'function') {
    window.setParameter(id, val)
  }
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

    this.handlePointerDown = this.handlePointerDown.bind(this)
    this.handlePointerMove = this.handlePointerMove.bind(this)
    this.handlePointerUp = this.handlePointerUp.bind(this)
    this.applyInertiaStep = this.applyInertiaStep.bind(this)

    this.attachEvents()
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

    if (this.settingsBtn) {
      this.settingsBtn.addEventListener('click', () => {
        if (this.drawer) {
          this.drawer.classList.toggle('hidden')
        }
      })
    }

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
}

