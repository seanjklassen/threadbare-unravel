// =============================================================================
// ORB VISUAL CONFIGURATION
// Edit this object to change the entire vibe of the visualizer
// =============================================================================
const CONFIG = {
  // Curve geometry
  pointCount: 60,
  curveWraps: 1.0618,  // How many TWO_PI rotations the curve spans

  // Colors (dustier coral for wistful/spectral aesthetic)
  lineColor: '#E8B8A8',
  trailColor: { r: 232, g: 184, b: 168 },

  // Stroke (thinner for more spectral/ethereal feel)
  stroke: {
    baseWidth: 2,       // Minimum stroke width
    widthRange: 2,      // How much tailLevel adds (so max = base + range)
    baseAlpha: 0.7,     // Minimum opacity
    alphaRange: 0.3,    // How much tailLevel adds
    referenceSize: 400, // Normalize stroke to this canvas dimension
  },

  // Radius
  radius: {
    base: 0.27,              // Base radius as fraction of minDim
    puckYInfluence: 0.12,    // How much puckY adds
    tailLevelInfluence: 0.18, // How much tailLevel adds
    inLevelInfluence: 0.08,  // How much inLevel adds
    sizeMin: 0.85,           // Min size multiplier
    sizeRange: 0.3,          // Size multiplier range (so max = min + range)
  },

  // Lissajous frequencies (control curve complexity)
  frequency: {
    xMin: Math.PI,                    // 3.14159... (transcendental)
    xRange: Math.E * 2,               // 5.43656... (2e)
    yMin: Math.sqrt(5) * 2.5,         // 5.59017... (2.5√5)
    yRange: Math.PI * 1.5,            // 4.71239... (1.5π)
    yPhaseRatio: 1 / ((1 + Math.sqrt(5)) / 2), // 0.618... (1/φ)
    ySquash: Math.sqrt(2) / 2 + 0.1,  // 0.807... (√2/2 + 0.1)
  },

  // Ghost trail
  trail: {
    minHistory: 3,       // Min frames of history
    historyRange: 8,    // Range based on decay (so max = min + range)
    widthRatio: 0.5,     // Trail stroke width as ratio of main stroke
    alphaMultiplier: 0.2, // Trail alpha multiplier
  },

  // Animation / Tempo sync
  tempo: {
    division: 0.125,   // Tempo division (0.25 = 1/4 speed, 0.125 = 1/8)
    driftMin: 0.5,    // Min phase multiplier
    driftRange: 1.5,  // Phase multiplier range based on drift param
  },
  
  // === DISINTEGRATION LOOPER: Entropy Visual Effects ===
  // As entropy increases, the orb evaporates upward and fades
  entropy: {
    maxAlphaReduction: 0.5,     // How much alpha fades at full entropy (50%)
    maxRadiusReduction: 0.25,   // How much radius shrinks at full entropy (25%)
    maxSpeedReduction: 0.6,     // How much animation slows at full entropy (60%)
    maxAscension: 0.18,         // Max vertical lift as fraction of canvas height (18%)
    maxTrailReduction: 0.7,     // How much trail shortens at full entropy (70%)
    colorShift: {
      r: 255,  // Target R at full entropy (warmer)
      g: 240,  // Target G at full entropy
      b: 220,  // Target B at full entropy (warmer tint)
    },
  },

  // Jitter/smoothing (ghost wobble)
  jitter: {
    coefficient: 0.048,    // Jitter amplitude as fraction of minDim
    smoothingBase: 0.68,   // Base smoothing factor
    smoothingRange: 0.2,   // Smoothing range (less ghost = more smoothing)
    noiseSeedRate: 0.22,   // How fast noise seed evolves with phase
    noisePointRate: 0.5,   // How noise varies per point
  },

  // Z-axis rotation (whole orb spins around center)
  rotation: {
    speed: 0.0005,          // Rotation speed relative to phase (slower = more subtle)
  },

  // State smoothing (lerp toward target values for seamless transitions)
  smoothing: {
    lerpSpeed: 0.12,      // How fast to interpolate toward target (0-1, lower = smoother)
  },
}

// =============================================================================
// ORB IMPLEMENTATION
// =============================================================================
const TWO_PI = Math.PI * 2

const clamp = (value, min, max) => Math.min(Math.max(value, min), max)

// Linear interpolation helper
const lerp = (current, target, t) => current + (target - current) * t

export class Orb {
  constructor(canvas) {
    this.canvas = canvas
    this.ctx = canvas.getContext('2d')
    this.points = new Array(CONFIG.pointCount)
    for (let i = 0; i < CONFIG.pointCount; i += 1) {
      this.points[i] = { x: 0, y: 0 }
    }

    this.phase = 0
    this.rotation = 0
    this.width = 0
    this.height = 0
    this.centerX = 0
    this.centerY = 0
    this.history = []
    this.maxHistory = CONFIG.trail.minHistory
    this.lastTime = performance.now()

    // Target state (what we're lerping toward)
    this.targetState = {
      inLevel: 0.35,
      tailLevel: 0.4,
      puckX: 0.5,
      puckY: 0.5,
      drift: 0.2,
      ghost: 0.2,
      decay: 0.4,
      size: 0.6,
      tempo: 120,
      // === DISINTEGRATION LOOPER STATE ===
      entropy: 0,          // 0-1, how much the loop has disintegrated
      looperActive: false, // true when looping (not idle/recording)
    }

    // Current smoothed state (used for rendering)
    this.state = { ...this.targetState }

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
    // Update target state (not current state directly)
    const keys = Object.keys(patch)
    for (let i = 0; i < keys.length; i += 1) {
      const key = keys[i]
      if (Object.prototype.hasOwnProperty.call(this.targetState, key)) {
        this.targetState[key] = patch[key]
      }
    }
    
    // === DISINTEGRATION LOOPER: Map looperState to looperActive ===
    // looperState: 0 = Idle, 1 = Recording, 2 = Looping
    // looperActive: true only when in Looping state (not recording)
    if (typeof patch.looperState !== 'undefined') {
      this.targetState.looperActive = (patch.looperState === 2)
    }
  }

  draw() {
    if (!this.ctx || this.width === 0 || this.height === 0) return

    const ctx = this.ctx
    ctx.clearRect(0, 0, this.width, this.height)

    // Lerp current state toward target state for smooth transitions
    const lerpSpeed = CONFIG.smoothing.lerpSpeed
    const keys = Object.keys(this.state)
    for (let i = 0; i < keys.length; i += 1) {
      const key = keys[i]
      if (key === 'tempo' || key === 'looperActive') {
        // Tempo and looperActive don't need smoothing - snap to target
        this.state[key] = this.targetState[key]
      } else {
        this.state[key] = lerp(this.state[key], this.targetState[key], lerpSpeed)
      }
    }

    const minDim = Math.max(1, Math.min(this.width, this.height))
    const inLevel = clamp(this.state.inLevel, 0, 1)
    const tailLevel = clamp(this.state.tailLevel, 0, 1)
    const puckX = clamp(this.state.puckX, 0, 1)
    const puckY = clamp(this.state.puckY, 0, 1)
    const drift = clamp(this.state.drift, 0, 1)
    const ghost = clamp(this.state.ghost, 0, 1)
    const decay = clamp(this.state.decay, 0, 1)
    const size = clamp(this.state.size, 0, 1)
    
    // === DISINTEGRATION LOOPER: Entropy state ===
    const entropy = clamp(this.state.entropy || 0, 0, 1)
    const looperActive = this.state.looperActive || false
    const { entropy: E } = CONFIG

    // Radius calculation using CONFIG
    // With entropy: orb shrinks as it "evaporates"
    const { radius: R } = CONFIG
    const sizeMultiplier = R.sizeMin + size * R.sizeRange
    let radiusBase = minDim * (
      R.base + 
      puckY * R.puckYInfluence + 
      tailLevel * R.tailLevelInfluence + 
      inLevel * R.inLevelInfluence
    ) * sizeMultiplier
    
    // Apply entropy-based radius reduction (orb shrinks as it evaporates)
    if (looperActive && entropy > 0) {
      radiusBase *= (1 - entropy * E.maxRadiusReduction)
    }

    // Stroke calculation using CONFIG
    // Floor at 3px to prevent thin lines on low-DPI WebViews (DAW environments)
    const { stroke: S } = CONFIG
    const strokeScale = minDim / S.referenceSize
    const strokeWidth = Math.max((S.baseWidth + tailLevel * S.widthRange) * strokeScale, 3)
    let strokeAlpha = S.baseAlpha + tailLevel * S.alphaRange
    
    // Apply entropy-based alpha reduction (orb fades as it evaporates)
    if (looperActive && entropy > 0) {
      strokeAlpha *= (1 - entropy * E.maxAlphaReduction)
    }

    // Ghost trail history using CONFIG
    // With entropy: trail shortens (memory evaporates)
    const { trail: T } = CONFIG
    let trailHistoryMax = T.minHistory + decay * T.historyRange
    if (looperActive && entropy > 0) {
      trailHistoryMax *= (1 - entropy * E.maxTrailReduction)
    }
    this.maxHistory = Math.max(1, Math.floor(trailHistoryMax))
    
    // === DISINTEGRATION LOOPER: Vertical Ascension ===
    // Orb rises toward the top as entropy increases (evaporating upward)
    let drawCenterY = this.centerY
    if (looperActive && entropy > 0) {
      const ascensionOffset = entropy * E.maxAscension * this.height
      drawCenterY = this.centerY - ascensionOffset
    }

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
    let basePhaseIncrement = rotationsPerMs * deltaMs * TWO_PI
    
    // Apply entropy-based speed reduction (orb slows as it evaporates)
    if (looperActive && entropy > 0) {
      basePhaseIncrement *= (1 - entropy * E.maxSpeedReduction)
    }
    
    this.phase += basePhaseIncrement * (TP.driftMin + drift * TP.driftRange)

    // Z-axis rotation (accumulates slower than phase for subtle spin)
    this.rotation += basePhaseIncrement * CONFIG.rotation.speed

    // Generate curve points (using drawCenterY for vertical ascension)
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
      const py = drawCenterY + Math.sin(t * freqY + this.phase * F.yPhaseRatio) * radius * F.ySquash

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

    // Apply z-axis rotation around center (uses drawCenterY for ascension)
    ctx.save()
    ctx.translate(this.centerX, drawCenterY)
    ctx.rotate(this.rotation)
    ctx.translate(-this.centerX, -drawCenterY)

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

    ctx.restore()
    ctx.globalAlpha = 1
  }
}
