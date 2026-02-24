import { initShell } from "@threadbare/shell/index.js"
import "@threadbare/shell/shell.css"
import "./waver.css"
import { createNativeFunctionBridge, createParamSender } from "@threadbare/bridge/juce-bridge.js"

import { WaverViz } from "./viz.js"
import { PARAMS, PARAM_IDS } from "./generated/params.js"
import { interpolate, generateSigmaOffsets } from "./rbf.js"
import { FACTORY_SURFACES, getSurfaceByIndex } from "./preset-surfaces.js"
import { buildDrawer } from "./drawer.js"
import { WAVER_PALETTE, applyWaverPaletteCssVars } from "./palette.js"

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
let arpEnabled = false
let savedPuckState = null
let isRestoringFromArp = false

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
    if ((id === "puckX" || id === "puckY") && !arpEnabled) {
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

  sendHostParam(id, value)
}

const THEME = {
  bg: WAVER_PALETTE.panelInk,
  text: WAVER_PALETTE.textUpper,
  accent: WAVER_PALETTE.surfaceBaseHover,
  "accent-hover": WAVER_PALETTE.panelInkSoft,
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
  applyWaverPaletteCssVars()

  shell = initShell({
    VizClass: WaverViz,
    params: PARAMS,
    paramOrder: PARAM_IDS,
    themeTokens: THEME,
    axisLabels: {
      normal: { left: "dark", right: "bright", top: "worn", bottom: "clean" },
    },
    puckBoundsInsetY: 84,
    getNativeFn: getNativeFunction,
    sendParam,
  })

  const drawerContent = document.getElementById("drawer-content")
  if (drawerContent) {
    drawer = buildDrawer(drawerContent, sendParam)
  }

  applyRbfInterpolation(morphState.puckX, morphState.puckY)
}

function onArpStateChanged(nowEnabled) {
  const app = document.getElementById("app")

  if (nowEnabled && !arpEnabled) {
    savedPuckState = { puckX: morphState.puckX, puckY: morphState.puckY }
    app?.classList.add("arping")
    if (shell?.controls?.toggleSettingsView) {
      shell.controls.toggleSettingsView(false, { reason: "arp-lock" })
    }
  } else if (!nowEnabled && arpEnabled) {
    app?.classList.remove("arping")
    if (savedPuckState) {
      isRestoringFromArp = true
      morphState = { ...morphState, puckX: savedPuckState.puckX, puckY: savedPuckState.puckY }
      if (typeof sendMorphSnapshotNative === "function") {
        sendMorphSnapshotNative(savedPuckState.puckX, savedPuckState.puckY, morphState.blend)
      }
      const normX = (savedPuckState.puckX + 1) * 0.5
      const normY = (1 - savedPuckState.puckY) * 0.5
      shell?.controls?.setPuckPositionImmediate(normX, normY)
      shell?.controls?.renderReadoutsFromNorm(normX, normY)
      rbfThrottle = 0
      applyRbfInterpolation(savedPuckState.puckX, savedPuckState.puckY)
      savedPuckState = null
      isRestoringFromArp = false
    }
  }
  arpEnabled = nowEnabled
}

function handleBackendState(payload) {
  let parsed = payload
  if (typeof payload === "string") {
    try { parsed = JSON.parse(payload) } catch { return }
  }

  if (typeof parsed?.arpEnabled === "boolean" || typeof parsed?.arpEnabled === "number") {
    const nowEnabled = Boolean(parsed.arpEnabled)
    if (nowEnabled !== arpEnabled) {
      onArpStateChanged(nowEnabled)
    }
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
