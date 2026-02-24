import { WAVER_PALETTE, hexToRgb } from "./palette.js"

const { r: AMBER_R, g: AMBER_G, b: AMBER_B } = hexToRgb(WAVER_PALETTE.surfaceBase)
const { r: COPPER_R, g: COPPER_G, b: COPPER_B } = hexToRgb(WAVER_PALETTE.waveformShadowDrift)
const { r: BG_R, g: BG_G, b: BG_B } = hexToRgb(WAVER_PALETTE.panelInkSoft)

const ARP_BLUE = { r: 106, g: 158, b: 181 }

const TRAIL_COUNT = 3
const POINT_COUNT = 80
const OVERSHOOT = 30

export class WaverViz {
  constructor(canvas) {
    this.canvas = canvas
    this.ctx = canvas.getContext("2d")
    this.state = {}

    this.smoothRms = 0
    this.smoothPeak = 0
    this.phase = Math.random() * Math.PI * 2
    this.driftPhase = Math.random() * Math.PI * 2
    this.breathPhase = Math.random() * Math.PI * 2
    this.momentFlash = 0

    this.trails = []
    for (let i = 0; i < TRAIL_COUNT; i++) {
      this.trails.push({ phase: this.phase, driftPhase: this.driftPhase, waveH: 0, energy: 0 })
    }

    this.arpMix = 0

    this.reducedMotion =
      window.matchMedia?.("(prefers-reduced-motion: reduce)")?.matches ?? false
    this.lastSnapshotTime = 0

    this._colorCache = ""
    this._glowCache = ""
    this._skyFillCache = ""
    this._groundFillCache = ""
    this._lastColorKey = -1

    this.resize()
  }

  resize() {
    const dpr = window.devicePixelRatio || 1
    const w = this.canvas.clientWidth || 400
    const h = this.canvas.clientHeight || 400
    this.canvas.width = Math.round(w * dpr)
    this.canvas.height = Math.round(h * dpr)
    this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0)
    this.w = w
    this.h = h
  }

  update(state) {
    this.state = state || {}
    const rms = state?.rms ?? 0
    const peak = state?.peak ?? 0
    this.smoothRms += (rms - this.smoothRms) * 0.12
    this.smoothPeak += (peak - this.smoothPeak) * 0.15
    const arpTarget = state?.arpEnabled ? 1 : 0
    this.arpMix += (arpTarget - this.arpMix) * 0.08
  }

  draw() {
    const ctx = this.ctx
    const w = this.w
    const h = this.h
    const now = performance.now()

    if (this.reducedMotion && now - this.lastSnapshotTime < 1000) return
    this.lastSnapshotTime = now

    ctx.clearRect(0, 0, w, h)

    const age = this._age()
    const rms = this.smoothRms
    const peak = this.smoothPeak

    const dbRms = 20 * Math.log10(Math.max(rms, 0.0001))
    const energy = Math.max(0, Math.min(1, (dbRms + 48) / 48))

    const crest = rms > 0.001 ? peak / rms : 1.0
    const transientKick = Math.pow(Math.max(0, (crest - 2.0) / 8.0), 0.6)

    this.phase += 0.003 + energy * 0.025
    this.driftPhase += 0.0011 + energy * 0.0008
    this.breathPhase += 0.003

    if (this.momentFlash > 0.01) {
      this.momentFlash *= 0.92
    } else {
      this.momentFlash = 0
    }

    this._updateColors(age, energy)
    ctx.fillStyle = this._skyFillCache
    ctx.fillRect(0, 0, w, h)

    const centerY = h * 0.5 + Math.sin(this.breathPhase * 0.4) * h * 0.02
    const baseAmp = h * 0.025
    const signalAmp = h * 0.30 * energy
    const peakAmp = h * 0.12 * transientKick
    const waveH = baseAmp + signalAmp + peakAmp
    const breathMod = 1.0 + Math.sin(this.breathPhase) * 0.15
    const lineWidth = 1.5 + age * 1.0 + transientKick * 0.8

    for (let t = TRAIL_COUNT - 1; t >= 1; t--) {
      this.trails[t].phase = this.trails[t - 1].phase
      this.trails[t].driftPhase = this.trails[t - 1].driftPhase
      this.trails[t].waveH = this.trails[t - 1].waveH
      this.trails[t].energy = this.trails[t - 1].energy
    }
    this.trails[0].phase = this.phase
    this.trails[0].driftPhase = this.driftPhase
    this.trails[0].waveH = waveH * breathMod
    this.trails[0].energy = energy

    ctx.beginPath()
    this._traceFillPath(ctx, w, h, centerY, this.trails[0])
    ctx.fillStyle = this._groundFillCache
    ctx.globalAlpha = 0.96
    ctx.fill()

    ctx.lineJoin = "round"
    ctx.lineCap = "round"

    for (let t = TRAIL_COUNT - 1; t >= 1; t--) {
      const trail = this.trails[t]
      if (trail.waveH < 0.5) continue
      const alpha = 0.02 + (TRAIL_COUNT - 1 - t) * 0.02
      ctx.beginPath()
      this._tracePath(ctx, w, centerY, trail)
      ctx.strokeStyle = this._glowCache
      ctx.lineWidth = lineWidth + 2
      ctx.globalAlpha = alpha
      ctx.stroke()
    }

    ctx.beginPath()
    this._tracePath(ctx, w, centerY, this.trails[0])
    ctx.strokeStyle = this._glowCache
    ctx.lineWidth = lineWidth + 4
    ctx.globalAlpha = 0.10 + energy * 0.20 + this.momentFlash * 0.25
    ctx.stroke()

    ctx.beginPath()
    this._tracePath(ctx, w, centerY, this.trails[0])
    ctx.strokeStyle = this._colorCache
    ctx.lineWidth = lineWidth
    ctx.globalAlpha = 0.80
    ctx.stroke()

    ctx.globalAlpha = 1
  }

  triggerMoment() {
    this.momentFlash = 1.0
  }

  dispose() {}

  _tracePath(ctx, w, centerY, trail) {
    for (let i = -1; i <= POINT_COUNT + 1; i++) {
      const t = i / POINT_COUNT
      const x = -OVERSHOOT + t * (w + OVERSHOOT * 2)
      const y = centerY + this._wave(t, trail.waveH, trail.energy, trail.phase, trail.driftPhase)
      if (i === -1) ctx.moveTo(x, y)
      else ctx.lineTo(x, y)
    }
  }

  _traceFillPath(ctx, w, h, centerY, trail) {
    this._tracePath(ctx, w, centerY, trail)
    ctx.lineTo(w + OVERSHOOT, h + OVERSHOOT)
    ctx.lineTo(-OVERSHOOT, h + OVERSHOOT)
    ctx.closePath()
  }

  _wave(t, amplitude, energy, phase, drift) {
    const p = phase
    const d = drift
    const b = this.breathPhase

    const fundamental = Math.sin(t * Math.PI * 2.0 + p) * 0.5
    const second = Math.sin(t * Math.PI * 3.7 + p * 0.7 + d) * (0.12 + energy * 0.18)
    const third = Math.sin(t * Math.PI * 5.3 + p * 0.4 + d * 0.8) * (0.05 + energy * 0.15)
    const shimmer = Math.sin(t * Math.PI * 9.1 + p * 0.25 + d * 1.6) * energy * 0.12
    const lowDrift = Math.sin(t * Math.PI * 1.1 + b * 0.5) * 0.08

    return (fundamental + second + third + shimmer + lowDrift) * amplitude
  }

  _age() {
    const puckY = this.state.puckY ?? 0
    return Math.max(0, Math.min(1, (puckY + 1) / 2))
  }

  _updateColors(age, energy) {
    const quantAge = Math.round(age * 20) / 20
    const quantEnergy = Math.round(energy * 10) / 10
    const quantArp = Math.round(this.arpMix * 20) / 20
    const shift = Math.min(1, quantAge * 0.4 + this.momentFlash * 0.5)
    const key = shift * 10000 + quantEnergy * 100 + quantArp
    if (key === this._lastColorKey) return
    this._lastColorKey = key

    const lerp = (a, b, t) => Math.round(a + (b - a) * t)

    let r = Math.round(AMBER_R + (COPPER_R - AMBER_R) * shift)
    let g = Math.round(AMBER_G + (COPPER_G - AMBER_G) * shift)
    let b = Math.round(AMBER_B + (COPPER_B - AMBER_B) * shift)
    r = lerp(r, ARP_BLUE.r, quantArp)
    g = lerp(g, ARP_BLUE.g, quantArp)
    b = lerp(b, ARP_BLUE.b, quantArp)

    this._colorCache = `rgb(${r},${g},${b})`
    this._glowCache = `rgba(${r},${g},${b},0.3)`
    const skyLift = Math.round(quantEnergy * 6 + this.momentFlash * 10)
    let skyR = Math.min(255, AMBER_R + skyLift)
    let skyG = Math.min(255, AMBER_G + skyLift)
    let skyB = Math.min(255, AMBER_B + Math.round(skyLift * 0.65))
    skyR = lerp(skyR, lerp(AMBER_R, ARP_BLUE.r, 0.15), quantArp)
    skyG = lerp(skyG, lerp(AMBER_G, ARP_BLUE.g, 0.15), quantArp)
    skyB = lerp(skyB, lerp(AMBER_B, ARP_BLUE.b, 0.2), quantArp)
    this._skyFillCache = `rgb(${skyR},${skyG},${skyB})`
    const groundMix = 0.05 + quantEnergy * 0.11 + shift * 0.32
    let groundR = Math.round(BG_R + (r - BG_R) * groundMix)
    let groundG = Math.round(BG_G + (g - BG_G) * groundMix)
    let groundB = Math.round(BG_B + (b - BG_B) * groundMix)
    groundR = lerp(groundR, lerp(BG_R, ARP_BLUE.r, 0.2), quantArp)
    groundG = lerp(groundG, lerp(BG_G, ARP_BLUE.g, 0.2), quantArp)
    groundB = lerp(groundB, lerp(BG_B, ARP_BLUE.b, 0.25), quantArp)
    this._groundFillCache = `rgb(${groundR},${groundG},${groundB})`

    const compR = Math.round(groundR * 0.96 + skyR * 0.04)
    const compG = Math.round(groundG * 0.96 + skyG * 0.04)
    const compB = Math.round(groundB * 0.96 + skyB * 0.04)
    document.documentElement.style.setProperty(
      "--waver-ground-fill",
      `rgb(${compR},${compG},${compB})`,
    )
  }
}
