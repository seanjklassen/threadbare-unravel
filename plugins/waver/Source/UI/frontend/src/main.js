import { initShell } from "@threadbare/shell/index.js"
import "@threadbare/shell/shell.css"
import { createNativeFunctionBridge, createParamSender } from "@threadbare/bridge/juce-bridge.js"

import { WaverViz } from "./viz.js"
import { PARAMS, PARAM_IDS } from "./generated/params.js"

const getNativeFunction = createNativeFunctionBridge()
const sendHostParam = createParamSender(getNativeFunction)
const sendMorphSnapshotNative = getNativeFunction("setMorphSnapshot")
const enqueueUiEventNative = getNativeFunction("enqueueUiEvent")

let morphState = {
  puckX: 0.0,
  puckY: 0.0,
  blend: 0.35,
}

const sendParam = (id, value) => {
  if (id === "puckX" || id === "puckY" || id === "blend") {
    morphState = { ...morphState, [id]: value }
    if (typeof sendMorphSnapshotNative === "function") {
      sendMorphSnapshotNative(morphState.puckX, morphState.puckY, morphState.blend)
      return
    }
  }

  if (id === "momentTrigger" && typeof enqueueUiEventNative === "function") {
    enqueueUiEventNative("moment")
    return
  }

  if (id === "arpEnabled" && typeof enqueueUiEventNative === "function") {
    enqueueUiEventNative("arp", Boolean(value))
  }

  sendHostParam(id, value)
}

const THEME = {
  bg: "#2b2b2b", // TODO: replace with plugin theme token
  text: "#d8d8d8", // TODO: replace with plugin theme token
  accent: "#9bc8ff", // TODO: replace with plugin theme token
  "accent-hover": "#c3e0ff", // TODO: replace with plugin theme token
}

let shell = null

function initApp() {
  shell = initShell({
    VizClass: WaverViz,
    params: PARAMS,
    paramOrder: PARAM_IDS,
    themeTokens: THEME,
    getNativeFn: getNativeFunction,
    sendParam,
  })
}

if (window.__JUCE__?.backend?.addEventListener) {
  window.__JUCE__.backend.addEventListener("updateState", (payload) => {
    shell?.updateState(payload)
  })
}

window.updateState = (payload) => {
  shell?.updateState(payload)
}

if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", initApp)
} else {
  initApp()
}
