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

    const radiusBase = minDim * (0.24 + tailLevel * 0.28 + inLevel * 0.08)
    const freqX = 0.85 + puckX * 2.4
    const freqY = 0.9 + puckY * 2
    const jitterAmp = ghost * minDim * 0.006
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

    const drawPath = (points, strokeStyle, alpha = 1) => {
      if (!points.length) {
        return
      }

      ctx.beginPath()
      ctx.lineWidth = 2
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

    for (let i = 0; i < this.history.length; i += 1) {
      const alpha = (i + 1) / (this.maxHistory + 1)
      drawPath(this.history[i], `rgba(225, 166, 166, ${alpha * 0.2})`, 1)
    }

    drawPath(this.points, LINE_COLOR, 1)
    ctx.globalAlpha = 1
  }
}
