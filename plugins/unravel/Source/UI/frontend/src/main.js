// =============================================================================
// UNRAVEL - Main Entry Point
// 
// This is the plugin-specific entry point that uses the shared Threadbare shell.
// =============================================================================

// =============================================================================
// JUCE 8 Native Function Integration - MUST BE FIRST
// =============================================================================

import { createNativeFunctionBridge, createParamSender } from '@threadbare/bridge/juce-bridge.js'

const getNativeFunction = createNativeFunctionBridge()
const sendParam = createParamSender(getNativeFunction)

let nativeLooperTrigger = null

const getLooperTrigger = () => {
  if (!nativeLooperTrigger) {
    nativeLooperTrigger = getNativeFunction('triggerDisintegration')
  }
  return typeof nativeLooperTrigger === 'function' ? nativeLooperTrigger : null
}


// =============================================================================
// UNRAVEL PLUGIN SETUP
// =============================================================================

// Import shared shell (using Vite alias)
import { initShell } from '@threadbare/shell/index.js'

// Import shell styles (shared)
import '@threadbare/shell/shell.css'

// Import plugin-specific visualization
import { Orb } from './orb.js'

// Import generated parameters (plugin-specific, single source of truth)
import { PARAMS, PARAM_IDS } from './generated/params.js'

// Unravel theme tokens (override shell defaults if desired)
// Using dustier coral for wistful/spectral aesthetic per 1.2 emotional targets
const UNRAVEL_THEME = {
  bg: '#31312b',
  text: '#C8C7B8',
  accent: '#E0E993',
  'accent-hover': '#E8B8A8',
}

// Shell instance reference
let shell = null

// Initialize the app
const initApp = () => {
  // Initialize the shared shell with Unravel-specific config
  shell = initShell({
    VizClass: Orb,
    params: PARAMS,
    paramOrder: PARAM_IDS,
    themeTokens: UNRAVEL_THEME,
    getNativeFn: getNativeFunction,
    sendParam: sendParam,
    sendLooperTrigger: getLooperTrigger(),
  })

  if (!shell) {
    console.error('Failed to initialize Threadbare shell!')
    return
  }
}

// Handler for state updates from C++ backend
// CRITICAL: JUCE 8 uses event-based communication, NOT global function calls
if (window.__JUCE__?.backend?.addEventListener) {
  window.__JUCE__.backend.addEventListener('updateState', (payload) => {
    if (shell) {
      shell.updateState(payload)
    }
  })
}

// Also expose globally for compatibility
window.updateState = (payload) => {
  if (shell) {
    shell.updateState(payload)
  }
}

// Wait for DOM to be ready before initializing
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', initApp)
} else {
  initApp()
}
