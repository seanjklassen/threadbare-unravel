import { initShell } from "@threadbare/shell/index.js"
import "@threadbare/shell/shell.css"
import "./waver.css"
import { createNativeFunctionBridge, createParamSender } from "@threadbare/bridge/juce-bridge.js"

import { WaverViz } from "./viz.js"
import { PARAMS, PARAM_IDS } from "./generated/params.js"
import { interpolate, generateSigmaOffsets } from "./rbf.js"
import { FACTORY_SURFACES, getSurfaceByIndex } from "./preset-surfaces.js"
import { buildDrawer } from "./drawer.js"

const getNativeFunction = createNativeFunctionBridge()
const sendHostParam = createParamSender(getNativeFunction)
const sendMorphSnapshotNative = getNativeFunction("setMorphSnapshot")
const enqueueUiEventNative = getNativeFunction("enqueueUiEvent")

let morphState = {
  puckX: 0.0,
  puckY: 0.0,
  blend: 0.35,
}

let activeSurface = FACTORY_SURFACES.length > 0 ? FACTORY_SURFACES[0] : null
let momentSeed = 42
let sigmaOffsets = activeSurface
  ? generateSigmaOffsets(activeSurface.landmarks.length, momentSeed)
  : []
let rbfThrottle = 0
let lastLoadedPresetIndex = -1

const PRESET_TO_SURFACE_INDEX = [
  0, 0, 0, // Amber
  1, 1, 1, // Drift
  2, 2, 2, // Hush
  3, 3, 3, // Signal
  4, 4, 4, // Weight
  0, 3, 0, 1, // Showcase / hybrid
]

function applyRbfInterpolation(puckX, puckY) {
  if (!activeSurface) return

  const now = performance.now()
  if (now - rbfThrottle < 100) return
  rbfThrottle = now

  const sigma = activeSurface.sigma ?? 0.32
  const result = interpolate(
    puckX,
    puckY,
    activeSurface.landmarks,
    sigma,
    sigmaOffsets,
  )

  for (const [id, value] of Object.entries(result)) {
    if (PARAMS[id]) {
      sendHostParam(id, value)
    }
  }
}

const sendParam = (id, value) => {
  if (id === "puckX" || id === "puckY" || id === "blend") {
    morphState = { ...morphState, [id]: value }
    if (typeof sendMorphSnapshotNative === "function") {
      sendMorphSnapshotNative(morphState.puckX, morphState.puckY, morphState.blend)
    }
    if (id === "puckX" || id === "puckY") {
      applyRbfInterpolation(morphState.puckX, morphState.puckY)
    }
    return
  }

  if (id === "momentTrigger") {
    momentSeed = (momentSeed * 1664525 + 1013904223) >>> 0
    if (activeSurface) {
      sigmaOffsets = generateSigmaOffsets(activeSurface.landmarks.length, momentSeed)
    }
    if (typeof enqueueUiEventNative === "function") {
      enqueueUiEventNative("moment")
    }
    if (shell?.viz) {
      shell.viz.triggerMoment?.()
    }
    applyRbfInterpolation(morphState.puckX, morphState.puckY)
    return
  }

  if (id === "arpEnabled" && typeof enqueueUiEventNative === "function") {
    enqueueUiEventNative("arp", Boolean(value))
  }

  sendHostParam(id, value)
}

const THEME = {
  bg: "#241D19",
  text: "#D3C7BB",
  accent: "#C4A46C",
  "accent-hover": "#D4B87C",
}

let shell = null
let drawer = null

function onPresetLoaded(index, presetPuckX = 0.0, presetPuckY = 0.0) {
  if (index === lastLoadedPresetIndex) return
  lastLoadedPresetIndex = index

  const mappedSurfaceIndex = PRESET_TO_SURFACE_INDEX[index] ?? 0
  const surface = getSurfaceByIndex(mappedSurfaceIndex)
  if (surface) {
    activeSurface = surface
    sigmaOffsets = generateSigmaOffsets(
      activeSurface.landmarks.length,
      momentSeed,
    )

    const clampedX = Math.max(-1, Math.min(1, Number(presetPuckX) || 0))
    const clampedY = Math.max(-1, Math.min(1, Number(presetPuckY) || 0))
    const normX = (clampedX + 1) * 0.5
    const normY = (1 - clampedY) * 0.5

    morphState = { ...morphState, puckX: clampedX, puckY: clampedY }
    if (typeof sendMorphSnapshotNative === "function") {
      sendMorphSnapshotNative(clampedX, clampedY, morphState.blend)
    }
    shell?.controls?.animatePuckTo(normX, normY)

    rbfThrottle = 0
    applyRbfInterpolation(clampedX, clampedY)
  }
}

function initApp() {
  shell = initShell({
    VizClass: WaverViz,
    params: PARAMS,
    paramOrder: PARAM_IDS,
    themeTokens: THEME,
    axisLabels: {
      normal: { left: "dark", right: "bright", top: "worn", bottom: "clean" },
    },
    getNativeFn: getNativeFunction,
    sendParam,
  })

  const drawerContent = document.getElementById("drawer-content")
  if (drawerContent) {
    drawer = buildDrawer(drawerContent, sendParam)
  }

  applyRbfInterpolation(morphState.puckX, morphState.puckY)
}

function handleBackendState(payload) {
  let parsed = payload
  if (typeof payload === "string") {
    try { parsed = JSON.parse(payload) } catch { return }
  }
  if (parsed?.currentPreset !== undefined) {
    onPresetLoaded(parsed.currentPreset, parsed?.presetPuckX, parsed?.presetPuckY)
  }
  shell?.updateState(payload)
  if (typeof parsed === "object" && parsed !== null) {
    drawer?.update(parsed)
  }
}

if (window.__JUCE__?.backend?.addEventListener) {
  window.__JUCE__.backend.addEventListener("updateState", handleBackendState)
}

window.updateState = handleBackendState

if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", initApp)
} else {
  initApp()
}
