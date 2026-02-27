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
let momentSeed = ((Date.now() * 2654435761) >>> 0) || 1
let sigmaOffsets = activeSurface
  ? generateSigmaOffsets(activeSurface.landmarks.length, momentSeed)
  : []
let rbfThrottle = 0
let lastLoadedPresetIndex = -1
let arpEnabled = false
let transportActive = false
let savedPuckState = null
let isRestoringFromArp = false

const PRESET_TO_SURFACE_INDEX = [
  0, 0, 0, // Amber
  1, 1, 1, // Drift
  2, 2, 2, // Hush
  3, 3, 3, // Signal
  4, 4, 4, // Weight
  0, 3, 0, 1, // Showcase / hybrid
  0, 3, 2, 4, 3, 1, // Celluloid
]

const SURFACE_LABELS = {
  amber:  { left: "veiled",  right: "shimmering", top: "worn",      bottom: "tender" },
  drift:  { left: "still",   right: "restless",   top: "fading",    bottom: "vivid" },
  hush:   { left: "near",    right: "distant",    top: "ghosted",   bottom: "gentle" },
  signal: { left: "soft",    right: "sharp",      top: "frayed",    bottom: "focused" },
  weight: { left: "deep",    right: "hollow",     top: "weathered", bottom: "solid" },
}

const ARP_LABELS = { left: "patient", right: "scattered", top: "flowing", bottom: "clipped" }

function applyAxisLabels() {
  if (!shell?.controls?.setAxisLabels) return
  if (arpEnabled) {
    shell.controls.setAxisLabels(ARP_LABELS)
  } else if (activeSurface?.category && SURFACE_LABELS[activeSurface.category]) {
    shell.controls.setAxisLabels(SURFACE_LABELS[activeSurface.category])
  }
}

function applyRbfInterpolation(puckX, puckY) {
  if (!activeSurface) return

  const now = performance.now()
  if (now - rbfThrottle < 30) return
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
    if (id === "puckX" || id === "puckY") {
      sendHostParam(id, value)
    }
    return
  }

  if (id === "momentTrigger") {
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

function onPresetLoaded(index, puckX, puckY) {
  if (index === lastLoadedPresetIndex) return
  lastLoadedPresetIndex = index

  const mappedSurfaceIndex = PRESET_TO_SURFACE_INDEX[index] ?? 0
  const surface = getSurfaceByIndex(mappedSurfaceIndex)
  if (surface) {
    activeSurface = surface
    momentSeed = (momentSeed * 1664525 + 1013904223) >>> 0
    sigmaOffsets = generateSigmaOffsets(
      activeSurface.landmarks.length,
      momentSeed,
    )
    shell?.viz?.triggerMoment?.()

    if (typeof puckX === 'number' && typeof puckY === 'number') {
      const clampedX = Math.max(-1, Math.min(1, puckX))
      const clampedY = Math.max(-1, Math.min(1, puckY))
      morphState = { ...morphState, puckX: clampedX, puckY: clampedY }
      if (typeof sendMorphSnapshotNative === "function") {
        sendMorphSnapshotNative(clampedX, clampedY, morphState.blend)
      }
    }

    applyAxisLabels()
  }
}

function initApp() {
  applyWaverPaletteCssVars()
  syncPlaybackState(false)

  shell = initShell({
    VizClass: WaverViz,
    params: PARAMS,
    paramOrder: PARAM_IDS,
    themeTokens: THEME,
    axisLabels: {
      normal: SURFACE_LABELS[activeSurface?.category] ?? SURFACE_LABELS.amber,
    },
    puckBoundsInsetY: 84,
    getNativeFn: getNativeFunction,
    sendParam,
  })

  const drawerContent = document.getElementById("drawer-content")
  if (drawerContent) {
    drawer = buildDrawer(drawerContent, sendParam)
  }

  const arpBtn = document.querySelector(".btn-arp")
  if (arpBtn) {
    arpBtn.addEventListener("click", () => {
      if (transportActive) return
      const next = !arpEnabled
      if (!next && savedPuckState && typeof sendMorphSnapshotNative === "function") {
        sendMorphSnapshotNative(savedPuckState.puckX, savedPuckState.puckY, morphState.blend)
      }
      sendHostParam("arpEnabled", next ? 1 : 0)
    })
  }
}

function syncArpButton(enabled) {
  const btn = document.querySelector(".btn-arp")
  if (!btn) return
  btn.classList.toggle("active", enabled)
  btn.classList.toggle("playing", enabled && transportActive)
  btn.classList.toggle("transport-hidden", transportActive && !enabled)
  btn.setAttribute("aria-pressed", String(enabled))
}

function syncPlaybackState(value) {
  transportActive = Boolean(value)
  const app = document.getElementById("app")
  app?.classList.toggle("playing", transportActive)
  syncArpButton(arpEnabled)
}

function parsePlayingState(value) {
  if (typeof value === "boolean") return value
  if (typeof value === "number") return value > 0
  if (typeof value === "string") {
    const normalized = value.trim().toLowerCase()
    if (normalized === "1" || normalized === "true" || normalized === "playing" || normalized === "play") return true
    if (normalized === "0" || normalized === "false" || normalized === "stopped" || normalized === "stop") return false
  }
  return null
}

function parseTransportState(parsed) {
  const directState = parsePlayingState(parsed?.transportActive)
  if (directState !== null) return directState

  const playingState = parsePlayingState(parsed?.isPlaying)
  const recordingState = parsePlayingState(parsed?.isRecording)
  if (playingState === null && recordingState === null) return null

  return Boolean(playingState) || Boolean(recordingState)
}

function onArpStateChanged(nowEnabled) {
  const app = document.getElementById("app")

  if (nowEnabled && !arpEnabled) {
    savedPuckState = { puckX: morphState.puckX, puckY: morphState.puckY }
    document.dispatchEvent(new CustomEvent("tb:close-presets"))
    app?.classList.add("arping")
    if (shell?.controls?.toggleSettingsView) {
      shell.controls.toggleSettingsView(false, { reason: "arp-lock" })
    }
    arpEnabled = nowEnabled
    syncArpButton(arpEnabled)
    applyAxisLabels()
    return
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
  syncArpButton(arpEnabled)
  applyAxisLabels()
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
  const parsedPlaying = parseTransportState(parsed)
  if (parsedPlaying !== null) {
    syncPlaybackState(parsedPlaying)
  }

  if (parsed?.currentPreset !== undefined) {
    onPresetLoaded(parsed.currentPreset, parsed?.puckX, parsed?.puckY)
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
