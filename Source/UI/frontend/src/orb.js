// =============================================================================
// ORB VISUAL CONFIGURATION
// Edit this object to change the entire vibe of the visualizer
// =============================================================================
const CONFIG = {
  // Curve geometry
  pointCount: 40,
  curveWraps: 3,  // How many TWO_PI rotations the curve spans

  // Colors
  lineColor: '#A6CEE1',
  trailColor: { r: 225, g: 166, b: 166 },

  // Stroke
  stroke: {
    baseWidth: 2,       // Minimum stroke width
    widthRange: 3,      // How much tailLevel adds (so max = base + range)
    baseAlpha: 0.7,     // Minimum opacity
    alphaRange: 0.3,    // How much tailLevel adds
    referenceSize: 400, // Normalize stroke to this canvas dimension
  },

  // Radius
  radius: {
    base: 0.22,              // Base radius as fraction of minDim
    puckYInfluence: 0.12,    // How much puckY adds
    tailLevelInfluence: 0.18, // How much tailLevel adds
    inLevelInfluence: 0.08,  // How much inLevel adds
    sizeMin: 0.85,           // Min size multiplier
    sizeRange: 0.3,          // Size multiplier range (so max = min + range)
  },

  // Lissajous frequencies (control curve complexity)
  frequency: {
    xMin: 2,      // Min X frequency
    xRange: 4,    // X frequency range based on puckX
    yMin: 2.5,    // Min Y frequency
    yRange: 3.9,  // Y frequency range based on puckY
    yPhaseRatio: 0.75, // Y phase offset ratio
    ySquash: 0.8,      // Vertical squash factor
  },

  // Ghost trail
  trail: {
    minHistory: 4,       // Min frames of history
    historyRange: 12,    // Range based on decay (so max = min + range)
    widthRatio: 0.6,     // Trail stroke width as ratio of main stroke
    alphaMultiplier: 0.25, // Trail alpha multiplier
  },

  // Animation / Tempo sync
  tempo: {
    division: 0.25,   // Tempo division (0.25 = 1/4 speed, 0.125 = 1/8)
    driftMin: 0.5,    // Min phase multiplier
    driftRange: 1.5,  // Phase multiplier range based on drift param
  },

  // Jitter/smoothing (ghost wobble)
  jitter: {
    coefficient: 0.012,    // Jitter amplitude as fraction of minDim
    smoothingBase: 0.68,   // Base smoothing factor
    smoothingRange: 0.2,   // Smoothing range (less ghost = more smoothing)
    noiseSeedRate: 0.22,   // How fast noise seed evolves with phase
    noisePointRate: 0.5,   // How noise varies per point
  },
}

// =============================================================================
// ORB IMPLEMENTATION
// =============================================================================
const TWO_PI = Math.PI * 2

const clamp = (value, min, max) => Math.min(Math.max(value, min), max)

export class Orb {
  constructor(canvas) {
    this.canvas = canvas
    this.ctx = canvas.getContext('2d')
    this.points = new Array(CONFIG.pointCount)
    for (let i = 0; i < CONFIG.pointCount; i += 1) {
      this.points[i] = { x: 0, y: 0 }
    }

    this.phase = 0
    this.width = 0
    this.height = 0
    this.centerX = 0
    this.centerY = 0
    this.history = []
    this.maxHistory = CONFIG.trail.minHistory
    this.lastTime = performance.now()

    this.state = {
      inLevel: 0.35,
      tailLevel: 0.4,
      puckX: 0.5,
      puckY: 0.5,
      drift: 0.2,
      ghost: 0.2,
      decay: 0.4,
      size: 0.6,
      tempo: 120,  // BPM from DAW (default 120)
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

    // Radius calculation using CONFIG
    const { radius: R } = CONFIG
    const sizeMultiplier = R.sizeMin + size * R.sizeRange
    const radiusBase = minDim * (
      R.base + 
      puckY * R.puckYInfluence + 
      tailLevel * R.tailLevelInfluence + 
      inLevel * R.inLevelInfluence
    ) * sizeMultiplier

    // Stroke calculation using CONFIG
    const { stroke: S } = CONFIG
    const strokeScale = minDim / S.referenceSize
    const strokeWidth = (S.baseWidth + tailLevel * S.widthRange) * strokeScale
    const strokeAlpha = S.baseAlpha + tailLevel * S.alphaRange

    // Ghost trail history using CONFIG
    const { trail: T } = CONFIG
    this.maxHistory = Math.floor(T.minHistory + decay * T.historyRange)

    // Lissajous frequencies using CONFIG
    const { frequency: F } = CONFIG
    const freqX = F.xMin + puckX * F.xRange
    const freqY = F.yMin + puckY * F.yRange

    // Jitter and smoothing using CONFIG
    const { jitter: J } = CONFIG
    const jitterAmp = ghost * minDim * J.coefficient
    const smoothing = J.smoothingBase + J.smoothingRange * (1 - ghost)

    // Tempo-synced phase advancement using CONFIG
    const { tempo: TP } = CONFIG
    const tempo = clamp(this.state.tempo || 120, 40, 240)
    const now = performance.now()
    const deltaMs = now - this.lastTime
    this.lastTime = now

    const rotationsPerMs = (tempo / 60) * TP.division / 1000
    const basePhaseIncrement = rotationsPerMs * deltaMs * TWO_PI
    this.phase += basePhaseIncrement * (TP.driftMin + drift * TP.driftRange)

    // Generate curve points
    let prevRadius = radiusBase
    const totalRadians = TWO_PI * CONFIG.curveWraps
    for (let i = 0; i < CONFIG.pointCount; i += 1) {
      const t = (i / (CONFIG.pointCount - 1)) * totalRadians
      const noiseSeed = Math.sin(this.phase * J.noiseSeedRate + i * J.noisePointRate)
      const noise = (noiseSeed - Math.floor(noiseSeed)) * 2 - 1
      const targetRadius = radiusBase + noise * jitterAmp
      const radius = prevRadius + (targetRadius - prevRadius) * smoothing
      prevRadius = radius

      const px = this.centerX + Math.cos(t * freqX + this.phase) * radius
      const py = this.centerY + Math.sin(t * freqY + this.phase * F.yPhaseRatio) * radius * F.ySquash

      const point = this.points[i]
      point.x = px
      point.y = py
    }

    // Store history snapshot
    const snapshot = this.points.map((p) => ({ x: p.x, y: p.y }))
    this.history.push(snapshot)
    if (this.history.length > this.maxHistory) {
      this.history.shift()
    }

    // Path drawing helper
    const drawPath = (points, strokeStyle, alpha = 1, lineWidth = strokeWidth) => {
      if (!points.length) return

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

    // Draw ghost trail history
    const { trailColor: TC } = CONFIG
    for (let i = 0; i < this.history.length; i += 1) {
      const historyAlpha = (i + 1) / (this.maxHistory + 1)
      const trailWidth = strokeWidth * T.widthRatio
      drawPath(
        this.history[i],
        `rgba(${TC.r}, ${TC.g}, ${TC.b}, ${historyAlpha * T.alphaMultiplier})`,
        1,
        trailWidth
      )
    }

    // Draw main orb line
    drawPath(this.points, CONFIG.lineColor, strokeAlpha, strokeWidth)
    ctx.globalAlpha = 1
  }
}
