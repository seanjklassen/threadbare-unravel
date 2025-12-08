const POINT_COUNT = 40
const TWO_PI = Math.PI * 2
const LINE_COLOR = '#E1A6A6'

const clamp = (value, min, max) => Math.min(Math.max(value, min), max)

export class Orb {
  constructor(canvas) {
    this.canvas = canvas
    this.ctx = canvas.getContext('2d')
    this.points = new Array(POINT_COUNT)
    for (let i = 0; i < POINT_COUNT; i += 1) {
      this.points[i] = { x: 0, y: 0 }
    }

    this.phase = 0
    this.width = 0
    this.height = 0
    this.centerX = 0
    this.centerY = 0
    this.history = []
    this.maxHistory = 8

    this.state = {
      inLevel: 0.35,
      tailLevel: 0.4,
      puckX: 0.5,
      puckY: 0.5,
      drift: 0.2,
      ghost: 0.2,
      decay: 0.4,
      size: 0.6,
    }

    this.resize()
  }

  resize() {
    if (!this.canvas || !this.ctx) return

    const dpr = window.devicePixelRatio || 1
    const rect = this.canvas.getBoundingClientRect()
    this.width = rect.width
    this.height = rect.height
    this.centerX = this.width * 0.5
    this.centerY = this.height * 0.5

    this.canvas.width = rect.width * dpr
    this.canvas.height = rect.height * dpr
    this.ctx.setTransform(1, 0, 0, 1, 0, 0)
    this.ctx.scale(dpr, dpr)
  }

  update(patch = {}) {
    const keys = Object.keys(patch)
    for (let i = 0; i < keys.length; i += 1) {
      const key = keys[i]
      if (Object.prototype.hasOwnProperty.call(this.state, key)) {
        this.state[key] = patch[key]
      }
    }
  }

  draw() {
    if (!this.ctx || this.width === 0 || this.height === 0) return

    const ctx = this.ctx
    ctx.clearRect(0, 0, this.width, this.height)

    const minDim = Math.max(1, Math.min(this.width, this.height))
    const inLevel = clamp(this.state.inLevel, 0, 1)
    const tailLevel = clamp(this.state.tailLevel, 0, 1)
    const puckX = clamp(this.state.puckX, 0, 1)
    const puckY = clamp(this.state.puckY, 0, 1)
    const drift = clamp(this.state.drift, 0, 1)
    const ghost = clamp(this.state.ghost, 0, 1)
    const decay = clamp(this.state.decay, 0, 1)
    const size = clamp(this.state.size, 0, 1)

    // Spec: "Orb radius grows with puckY and tailLevel"
    // Size acts as an overall scale multiplier (0.85 to 1.15)
    const sizeMultiplier = 0.85 + size * 0.3
    // Base radius ensures orb is always visible, then grows with parameters
    const radiusBase = minDim * (0.22 + puckY * 0.12 + tailLevel * 0.18 + inLevel * 0.08) * sizeMultiplier
    
    // Spec: "Stroke thickness & opacity follow tailLevel"
    // Scale stroke with canvas size for consistent appearance
    const strokeScale = minDim / 400  // Normalize to ~400px reference
    const strokeWidth = (2 + tailLevel * 3) * strokeScale   // Range: 2 to 5, scaled
    const strokeAlpha = 0.7 + tailLevel * 0.3   // Range: 0.7 to 1.0
    
    // More decay = longer ghost trail (4 to 16 frames)
    this.maxHistory = Math.floor(4 + decay * 12)
    
    const freqX = 0.85 + puckX * 2.4
    const freqY = 0.9 + puckY * 2
    const jitterAmp = ghost * minDim * 0.012  // Increased from 0.006 for more visible wobble
    const smoothing = 0.68 + 0.2 * (1 - ghost)

    this.phase += 0.0018 + drift * 0.018

    let prevRadius = radiusBase
    for (let i = 0; i < POINT_COUNT; i += 1) {
      const t = (i / (POINT_COUNT - 1)) * TWO_PI
      const noiseSeed = Math.sin(this.phase * 0.22 + i * 1.381)
      const noise = (noiseSeed - Math.floor(noiseSeed)) * 2 - 1
      const targetRadius = radiusBase + noise * jitterAmp
      const radius = prevRadius + (targetRadius - prevRadius) * smoothing
      prevRadius = radius

      const px = this.centerX + Math.cos(t * freqX + this.phase) * radius
      const py =
        this.centerY + Math.sin(t * freqY + this.phase * 0.35) * radius * 0.9

      const point = this.points[i]
      point.x = px
      point.y = py
    }

    const snapshot = this.points.map((p) => ({ x: p.x, y: p.y }))
    this.history.push(snapshot)
    if (this.history.length > this.maxHistory) {
      this.history.shift()
    }

    const drawPath = (points, strokeStyle, alpha = 1, lineWidth = strokeWidth) => {
      if (!points.length) {
        return
      }

      ctx.beginPath()
      ctx.lineWidth = lineWidth
      ctx.lineJoin = 'round'
      ctx.lineCap = 'round'
      ctx.strokeStyle = strokeStyle
      ctx.globalAlpha = alpha
      ctx.moveTo(points[0].x, points[0].y)

      for (let i = 0; i < points.length - 1; i += 1) {
        const curr = points[i]
        const next = points[i + 1]
        const mx = (curr.x + next.x) * 0.5
        const my = (curr.y + next.y) * 0.5
        ctx.quadraticCurveTo(curr.x, curr.y, mx, my)
      }

      const tail = points[points.length - 1]
      ctx.lineTo(tail.x, tail.y)
      ctx.stroke()
    }

    // Draw ghost trail history (thinner lines)
    for (let i = 0; i < this.history.length; i += 1) {
      const historyAlpha = (i + 1) / (this.maxHistory + 1)
      const trailWidth = strokeWidth * 0.6  // Thinner trail
      drawPath(this.history[i], `rgba(225, 166, 166, ${historyAlpha * 0.25})`, 1, trailWidth)
    }

    // Draw main orb line with dynamic stroke width and alpha
    drawPath(this.points, LINE_COLOR, strokeAlpha, strokeWidth)
    ctx.globalAlpha = 1
  }
}
