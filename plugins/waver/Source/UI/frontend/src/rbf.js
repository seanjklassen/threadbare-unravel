const LOG_PARAMS = new Set([
  "filterCutoff",
  "lfoRate",
])

const LOG_TIME_PARAMS = new Set([
  "envAttack",
  "envDecay",
  "envRelease",
])

const LOG_MS_PARAMS = new Set(["portaTime"])

const CHOICE_PARAMS = new Set([
  "filterMode",
  "lfoShape",
  "chorusMode",
  "portaMode",
  "humFreq",
  "dcoSubOctave",
])

const LAYER_SUM_CEILING = 1.0

function toPerceptual(id, raw) {
  if (LOG_PARAMS.has(id)) return Math.log(Math.max(raw, 1e-6))
  if (LOG_TIME_PARAMS.has(id)) return Math.log(Math.max(raw, 1e-6))
  if (LOG_MS_PARAMS.has(id)) return Math.log(Math.max(raw, 0.001))
  return raw
}

function fromPerceptual(id, val) {
  if (LOG_PARAMS.has(id)) return Math.exp(val)
  if (LOG_TIME_PARAMS.has(id)) return Math.exp(val)
  if (LOG_MS_PARAMS.has(id)) return Math.exp(val)
  return val
}

function rbfWeight(px, py, lx, ly, sigma) {
  const dx = px - lx
  const dy = py - ly
  const distSq = dx * dx + dy * dy
  return Math.exp(-distSq / (2 * sigma * sigma))
}

/**
 * Interpolate parameter values from landmarks at a given puck position.
 *
 * @param {number} puckX - Puck X position (-1 to 1)
 * @param {number} puckY - Puck Y position (-1 to 1)
 * @param {Array} landmarks - Array of { x, y, params: { paramId: rawValue } }
 * @param {number} sigma - RBF radius (0.25-0.40 typical)
 * @param {number[]|null} sigmaOffsets - Per-landmark sigma jitter from Moment Mode
 * @returns {Object} Interpolated raw parameter values { paramId: value }
 */
export function interpolate(puckX, puckY, landmarks, sigma, sigmaOffsets) {
  if (!landmarks || landmarks.length === 0) return {}

  const allParamIds = new Set()
  for (const lm of landmarks) {
    if (lm.params) {
      for (const id of Object.keys(lm.params)) allParamIds.add(id)
    }
  }

  const weights = []
  let totalWeight = 0

  for (let i = 0; i < landmarks.length; i++) {
    const lm = landmarks[i]
    const s = sigma + (sigmaOffsets?.[i] ?? 0)
    const w = rbfWeight(puckX, puckY, lm.x, lm.y, Math.max(s, 0.05))
    weights.push(w)
    totalWeight += w
  }

  if (totalWeight < 1e-12) {
    return landmarks[0]?.params ? { ...landmarks[0].params } : {}
  }

  const result = {}

  for (const paramId of allParamIds) {
    if (CHOICE_PARAMS.has(paramId)) {
      let bestIdx = 0
      let bestW = -1
      for (let i = 0; i < landmarks.length; i++) {
        if (landmarks[i].params?.[paramId] !== undefined && weights[i] > bestW) {
          bestW = weights[i]
          bestIdx = i
        }
      }
      result[paramId] = landmarks[bestIdx].params[paramId]
      continue
    }

    let sum = 0
    let wSum = 0

    for (let i = 0; i < landmarks.length; i++) {
      const raw = landmarks[i].params?.[paramId]
      if (raw === undefined) continue
      const perceptual = toPerceptual(paramId, raw)
      sum += (weights[i] / totalWeight) * perceptual
      wSum += weights[i] / totalWeight
    }

    if (wSum > 1e-12) {
      result[paramId] = fromPerceptual(paramId, sum / wSum)
    }
  }

  if (
    result.layerDco !== undefined &&
    result.layerToy !== undefined &&
    result.layerOrgan !== undefined
  ) {
    const layerSum = result.layerDco + result.layerToy + result.layerOrgan
    if (layerSum > LAYER_SUM_CEILING) {
      const scale = LAYER_SUM_CEILING / layerSum
      result.layerDco *= scale
      result.layerToy *= scale
      result.layerOrgan *= scale
    }
  }

  return result
}

/**
 * Generate per-landmark sigma offsets for Moment Mode.
 * Deterministic given a seed.
 */
export function generateSigmaOffsets(count, seed) {
  let s = seed >>> 0 || 1
  const offsets = []
  for (let i = 0; i < count; i++) {
    s ^= s << 13
    s ^= s >> 17
    s ^= s << 5
    const norm = ((s >>> 0) / 0xffffffff) * 2 - 1
    offsets.push(norm * 0.06)
  }
  return offsets
}
