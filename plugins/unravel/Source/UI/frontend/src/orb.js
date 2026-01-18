// =============================================================================
// ORB VISUAL CONFIGURATION
// Edit this object to change the entire vibe of the visualizer
// =============================================================================

// Check for reduced motion preference at module level
const prefersReducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches

const CONFIG = {
  // Curve geometry
  pointCount: 60,
  curveWraps: 1.0618,  // How many TWO_PI rotations the curve spans

  // === STATE-AWARE COLORS ===
  colors: {
    idle: { r: 232, g: 184, b: 168 },      // #E8B8A8 - dusty coral
    recording: { r: 204, g: 138, b: 126 }, // #CC8A7E - darker coral
    // Looping: amber → dusty blue gradient based on entropy
    loopingStart: { r: 255, g: 200, b: 140 },  // Warm amber (entropy 0%)
    loopingEnd: { r: 168, g: 192, b: 212 },    // #A8C0D4 - dusty blue (entropy 100%)
  },
  
  // Recording pulse (synced with button animation)
  recordingPulse: {
    duration: 1200,   // 1.2s to match button animation
    minAlpha: 0.6,    // Pulse range minimum
    maxAlpha: 1.0,    // Pulse range maximum
  },

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

  // === INPUT REACTIVITY: Bloom effect on transients ===
  inputReactivity: {
    bloomThreshold: 0.2,    // Low threshold to catch most input
    bloomScale: 1.25,       // More dramatic radius boost
    bloomDecay: 0.96,       // Even slower decay for visibility
    bloomAlphaBoost: 0.25,  // Stronger alpha boost
  },

  // === BREATHING IDLE ANIMATION ===
  breathing: {
    enabled: true,
    rate: 0.003,            // Visible breathing cycle (~2 seconds)
    scaleRange: 0.05,       // +/- 5% scale oscillation (clearly visible)
    idleThreshold: 0.2,     // Higher threshold - easier to trigger full breathing
    fadeInSpeed: 0.03,      // How fast breathing intensifies
    fadeOutSpeed: 0.02,     // Slower fade out
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

  // Transport-aware animation (smooth deceleration/acceleration with playhead)
  transport: {
    decelerationRate: 0.03,  // How fast it slows when stopped (lower = more gradual)
    accelerationRate: 0.06,  // How fast it speeds up when playing
    idleSpeed: 0.15,         // Minimum speed when stopped (keeps orb alive)
  },

  // === DISINTEGRATION LOOPER: Entropy Visual Effects ===
  entropy: {
    maxAlphaReduction: 0.7,     // More dramatic fade (70%)
    maxRadiusReduction: 0.3,    // Shrinks more noticeably (30%)
    maxSpeedReduction: 0.85,    // Nearly stops at full entropy (85%)
    maxTrailReduction: 0.8,     // Trail mostly gone (80%)
    // Flicker effect at high entropy (memory degradation artifact)
    flickerThreshold: 0.5,      // Start flicker at 50% entropy
    flickerAmount: 0.12,        // Max alpha wobble amount
  },

  // Jitter/smoothing (ghost wobble)
  jitter: {
    coefficient: 0.048,    // Jitter amplitude as fraction of minDim
    smoothingBase: 0.68,   // Base smoothing factor
    smoothingRange: 0.2,   // Smoothing range (less ghost = more smoothing)
    noiseSeedRate: 0.22,   // How fast noise seed evolves with phase
    noisePointRate: 0.5,   // How noise varies per point
  },

  // === FULL 3D ROTATION ===
  rotation3d: {
    xSpeed: 0.0003,         // Tilt forward/back oscillation speed
    ySpeed: 0.0004,         // Tilt left/right oscillation speed
    zSpeed: 0.0005,         // Spin speed (continuous rotation)
    xAmplitude: 0.15,       // Max x tilt in radians (~8 degrees)
    yAmplitude: 0.12,       // Max y tilt in radians (~7 degrees)
    perspective: 800,       // Perspective distance for 3D projection
  },

  // State smoothing (lerp toward target values for seamless transitions)
  smoothing: {
    lerpSpeed: 0.12,        // How fast to interpolate toward target (0-1, lower = smoother)
    colorLerpSpeed: 0.08,   // Slower color transitions for smoothness
  },

  // === ACCESSIBILITY: Reduced motion fallbacks ===
  accessibility: {
    reducedMotion: {
      disableRotation: true,
      disableBreathing: true,
      staticTrailLength: 3,
    },
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
      this.points[i] = { x: 0, y: 0, z: 0 }  // Added z for 3D
    }

    this.phase = 0
    this.rotationX = 0
    this.rotationY = 0
    this.rotationZ = 0
    this.width = 0
    this.height = 0
    this.centerX = 0
    this.centerY = 0
    this.history = []
    this.maxHistory = CONFIG.trail.minHistory
    this.lastTime = performance.now()

    // Bloom effect state
    this.bloomAmount = 0

    // Breathing animation state
    this.breathPhase = 0
    this.breathIntensity = 0  // Fades in/out smoothly

    // Current color (for smooth transitions)
    this.currentColor = { ...CONFIG.colors.idle }

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
      // === LOOPER STATE ===
      looperState: 0,       // 0 = Idle, 1 = Recording, 2 = Looping
      entropy: 0,           // 0-1, how much the loop has disintegrated
      // === TRANSPORT STATE ===
      isPlaying: true,      // DAW playhead state
    }

    // Current smoothed state (used for rendering)
    this.state = { ...this.targetState }
    
    // Transport-aware playback speed (separate from state for smoother control)
    this.playbackSpeed = 1.0

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
  }

  draw() {
    if (!this.ctx || this.width === 0 || this.height === 0) return

    const ctx = this.ctx
    ctx.clearRect(0, 0, this.width, this.height)

    const now = performance.now()
    const deltaMs = now - this.lastTime
    this.lastTime = now

    // Lerp current state toward target state for smooth transitions
    const lerpSpeed = CONFIG.smoothing.lerpSpeed
    const keys = Object.keys(this.state)
    for (let i = 0; i < keys.length; i += 1) {
      const key = keys[i]
      if (key === 'tempo' || key === 'looperState' || key === 'isPlaying') {
        // These don't need smoothing - snap to target
        this.state[key] = this.targetState[key]
      } else {
        this.state[key] = lerp(this.state[key], this.targetState[key], lerpSpeed)
      }
    }

    // Extract state values
    const minDim = Math.max(1, Math.min(this.width, this.height))
    const inLevel = clamp(this.state.inLevel, 0, 1)
    const tailLevel = clamp(this.state.tailLevel, 0, 1)
    const puckX = clamp(this.state.puckX, 0, 1)
    const puckY = clamp(this.state.puckY, 0, 1)
    const drift = clamp(this.state.drift, 0, 1)
    const ghost = clamp(this.state.ghost, 0, 1)
    const decay = clamp(this.state.decay, 0, 1)
    const size = clamp(this.state.size, 0, 1)
    const looperState = this.state.looperState || 0  // 0=Idle, 1=Recording, 2=Looping
    const entropy = clamp(this.state.entropy || 0, 0, 1)
    const isPlaying = this.state.isPlaying !== false

    // === TRANSPORT-AWARE PLAYBACK SPEED ===
    // When stopped, slow down to idle speed (not zero) so orb stays alive
    const { transport: TR } = CONFIG
    const targetSpeed = isPlaying ? 1.0 : TR.idleSpeed
    const speedRate = targetSpeed > this.playbackSpeed ? TR.accelerationRate : TR.decelerationRate
    this.playbackSpeed = lerp(this.playbackSpeed, targetSpeed, speedRate)

    // === STATE-AWARE COLOR CALCULATION ===
    const { colors: C, recordingPulse: RP } = CONFIG
    let targetColor
    let pulseAlpha = 1.0

    if (looperState === 1) {
      // Recording: darker coral with pulse
      targetColor = C.recording
      // Sinusoidal pulse synced with button animation (1.2s)
      const pulsePhase = (now % RP.duration) / RP.duration
      pulseAlpha = RP.minAlpha + (RP.maxAlpha - RP.minAlpha) * (0.5 + 0.5 * Math.sin(pulsePhase * TWO_PI))
    } else if (looperState === 2) {
      // Looping: amber → dusty blue based on entropy
      // At entropy 0: warm amber (loopingStart)
      // At entropy 1: dusty blue (loopingEnd)
      targetColor = {
        r: lerp(C.loopingStart.r, C.loopingEnd.r, entropy),
        g: lerp(C.loopingStart.g, C.loopingEnd.g, entropy),
        b: lerp(C.loopingStart.b, C.loopingEnd.b, entropy),
      }
    } else {
      // Idle: dusty coral
      targetColor = C.idle
    }

    // Smooth color transition
    const colorLerp = CONFIG.smoothing.colorLerpSpeed
    this.currentColor.r = lerp(this.currentColor.r, targetColor.r, colorLerp)
    this.currentColor.g = lerp(this.currentColor.g, targetColor.g, colorLerp)
    this.currentColor.b = lerp(this.currentColor.b, targetColor.b, colorLerp)

    // === INPUT REACTIVITY: Bloom effect ===
    // Use TARGET (raw) inLevel for transient detection, not smoothed state
    const { inputReactivity: IR } = CONFIG
    const rawInLevel = clamp(this.targetState.inLevel, 0, 1)
    if (rawInLevel > IR.bloomThreshold) {
      // Spike detected - boost bloom toward 1
      const bloomTarget = (rawInLevel - IR.bloomThreshold) / (1 - IR.bloomThreshold)
      this.bloomAmount = Math.max(this.bloomAmount, bloomTarget)
    }
    // Decay bloom each frame
    this.bloomAmount *= IR.bloomDecay

    // === BREATHING IDLE ANIMATION ===
    const { breathing: BR } = CONFIG
    let breathScale = 1.0
    const useBreathing = BR.enabled && !prefersReducedMotion && !CONFIG.accessibility.reducedMotion.disableBreathing

    if (useBreathing) {
      // Use raw target values for more responsive detection
      const rawInLevel = clamp(this.targetState.inLevel, 0, 1)
      const rawIsPlaying = this.targetState.isPlaying
      
      // Breathing is always subtly active, but stronger when idle
      // "Idle" = DAW stopped (isPlaying explicitly false) OR very low input
      const isIdle = rawIsPlaying === false || rawInLevel < BR.idleThreshold
      const targetIntensity = isIdle ? 1.0 : 0.3  // Always at least 30% breathing
      
      if (targetIntensity > this.breathIntensity) {
        this.breathIntensity = Math.min(targetIntensity, this.breathIntensity + BR.fadeInSpeed)
      } else {
        this.breathIntensity = Math.max(targetIntensity, this.breathIntensity - BR.fadeOutSpeed)
      }
      
      // Always advance breath phase
      this.breathPhase += deltaMs * BR.rate
      breathScale = 1 + Math.sin(this.breathPhase) * BR.scaleRange * this.breathIntensity
    }

    // === RADIUS CALCULATION ===
    const { radius: R, entropy: E } = CONFIG
    const sizeMultiplier = R.sizeMin + size * R.sizeRange
    let radiusBase = minDim * (
      R.base + 
      puckY * R.puckYInfluence + 
      tailLevel * R.tailLevelInfluence + 
      inLevel * R.inLevelInfluence
    ) * sizeMultiplier

    // Apply bloom effect
    radiusBase *= (1 + this.bloomAmount * (IR.bloomScale - 1))

    // Apply breathing
    radiusBase *= breathScale

    // Apply entropy-based radius reduction (looping only)
    if (looperState === 2 && entropy > 0) {
      radiusBase *= (1 - entropy * E.maxRadiusReduction)
    }

    // === STROKE CALCULATION ===
    const { stroke: S } = CONFIG
    const strokeScale = minDim / S.referenceSize
    const strokeWidth = Math.max((S.baseWidth + tailLevel * S.widthRange) * strokeScale, 3)
    let strokeAlpha = S.baseAlpha + tailLevel * S.alphaRange

    // Apply bloom alpha boost
    strokeAlpha = Math.min(1, strokeAlpha + this.bloomAmount * IR.bloomAlphaBoost)

    // Apply recording pulse
    strokeAlpha *= pulseAlpha

    // Apply entropy-based alpha reduction (looping only)
    if (looperState === 2 && entropy > 0) {
      strokeAlpha *= (1 - entropy * E.maxAlphaReduction)
      
      // Add flicker at high entropy
      if (entropy > E.flickerThreshold) {
        const flickerNorm = (entropy - E.flickerThreshold) / (1 - E.flickerThreshold)
        const flicker = (Math.random() - 0.5) * 2 * E.flickerAmount * flickerNorm
        strokeAlpha = Math.max(0.1, strokeAlpha + flicker)
      }
    }

    // === GHOST TRAIL HISTORY ===
    const { trail: T } = CONFIG
    let trailHistoryMax = T.minHistory + decay * T.historyRange
    
    // Reduced motion: use static trail length
    if (prefersReducedMotion) {
      trailHistoryMax = CONFIG.accessibility.reducedMotion.staticTrailLength
    } else if (looperState === 2 && entropy > 0) {
      trailHistoryMax *= (1 - entropy * E.maxTrailReduction)
    }
    this.maxHistory = Math.max(1, Math.floor(trailHistoryMax))

    // === LISSAJOUS FREQUENCIES ===
    const { frequency: F } = CONFIG
    const freqX = F.xMin + puckX * F.xRange
    const freqY = F.yMin + puckY * F.yRange

    // === JITTER/WOBBLE ===
    const { jitter: J } = CONFIG
    const jitterAmp = ghost * minDim * J.coefficient
    const smoothing = J.smoothingBase + J.smoothingRange * (1 - ghost)

    // === TEMPO-SYNCED PHASE ===
    const { tempo: TP } = CONFIG
    const tempo = clamp(this.state.tempo || 120, 40, 240)
    const rotationsPerMs = (tempo / 60) * TP.division / 1000
    let basePhaseIncrement = rotationsPerMs * deltaMs * TWO_PI

    // Apply entropy-based speed reduction (looping only)
    if (looperState === 2 && entropy > 0) {
      basePhaseIncrement *= (1 - entropy * E.maxSpeedReduction)
    }

    // Apply transport-aware playback speed
    basePhaseIncrement *= this.playbackSpeed

    this.phase += basePhaseIncrement * (TP.driftMin + drift * TP.driftRange)

    // === 3D ROTATION ===
    const { rotation3d: R3 } = CONFIG
    const useRotation = !prefersReducedMotion && !CONFIG.accessibility.reducedMotion.disableRotation

    if (useRotation) {
      // X and Y oscillate (tilt back and forth), Z rotates continuously
      this.rotationX = Math.sin(this.phase * R3.xSpeed * 1000) * R3.xAmplitude
      this.rotationY = Math.sin(this.phase * R3.ySpeed * 1000 + Math.PI * 0.5) * R3.yAmplitude
      this.rotationZ += basePhaseIncrement * R3.zSpeed
    }

    // Precompute trig for 3D rotation
    const cosX = Math.cos(this.rotationX)
    const sinX = Math.sin(this.rotationX)
    const cosY = Math.cos(this.rotationY)
    const sinY = Math.sin(this.rotationY)
    const cosZ = Math.cos(this.rotationZ)
    const sinZ = Math.sin(this.rotationZ)

    // === GENERATE CURVE POINTS ===
    let prevRadius = radiusBase
    const totalRadians = TWO_PI * CONFIG.curveWraps
    
    for (let i = 0; i < CONFIG.pointCount; i += 1) {
      const t = (i / (CONFIG.pointCount - 1)) * totalRadians
      const noiseSeed = Math.sin(this.phase * J.noiseSeedRate + i * J.noisePointRate)
      const noise = (noiseSeed - Math.floor(noiseSeed)) * 2 - 1
      const targetRadius = radiusBase + noise * jitterAmp
      const radius = prevRadius + (targetRadius - prevRadius) * smoothing
      prevRadius = radius

      // Base 2D lissajous position (centered at origin)
      let px = Math.cos(t * freqX + this.phase) * radius
      let py = Math.sin(t * freqY + this.phase * F.yPhaseRatio) * radius * F.ySquash
      let pz = 0

      if (useRotation) {
        // Apply 3D rotation (Y-X-Z order for natural tilt feel)
        // Rotate around Y axis (left-right tilt)
        let x1 = px * cosY + pz * sinY
        let z1 = -px * sinY + pz * cosY

        // Rotate around X axis (forward-back tilt)
        let y1 = py * cosX - z1 * sinX
        let z2 = py * sinX + z1 * cosX

        // Rotate around Z axis (spin)
        let x2 = x1 * cosZ - y1 * sinZ
        let y2 = x1 * sinZ + y1 * cosZ

        // Simple perspective projection
        const perspective = R3.perspective
        const scale = perspective / (perspective + z2)
        
        px = x2 * scale
        py = y2 * scale
        pz = z2
      }

      const point = this.points[i]
      point.x = this.centerX + px
      point.y = this.centerY + py
      point.z = pz
    }

    // Store history snapshot
    const snapshot = this.points.map((p) => ({ x: p.x, y: p.y }))
    this.history.push(snapshot)
    if (this.history.length > this.maxHistory) {
      this.history.shift()
    }

    // === PATH DRAWING HELPER ===
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

    // Get current draw color (rounded for rgba)
    const drawColor = {
      r: Math.round(this.currentColor.r),
      g: Math.round(this.currentColor.g),
      b: Math.round(this.currentColor.b),
    }

    // Draw ghost trail history
    for (let i = 0; i < this.history.length; i += 1) {
      const historyAlpha = (i + 1) / (this.maxHistory + 1)
      const trailWidth = strokeWidth * T.widthRatio
      drawPath(
        this.history[i],
        `rgba(${drawColor.r}, ${drawColor.g}, ${drawColor.b}, ${historyAlpha * T.alphaMultiplier})`,
        1,
        trailWidth
      )
    }

    // Draw main orb line
    const mainColor = `rgb(${drawColor.r}, ${drawColor.g}, ${drawColor.b})`
    drawPath(this.points, mainColor, strokeAlpha, strokeWidth)

    ctx.globalAlpha = 1
  }
}
