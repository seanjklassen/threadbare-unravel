// =============================================================================
// UNRAVEL - Main Entry Point
// 
// This is the plugin-specific entry point that uses the shared Threadbare shell.
// =============================================================================

// =============================================================================
// JUCE 8 Native Function Integration - MUST BE FIRST
// =============================================================================

// Promise handler to track pending native function calls
class PromiseHandler {
  constructor() {
    this.lastPromiseId = 0
    this.promises = new Map()
    
    // Listen for completion events from the backend (with safety check)
    if (window.__JUCE__?.backend?.addEventListener) {
      window.__JUCE__.backend.addEventListener('__juce__complete', ({ promiseId, result }) => {
        if (this.promises.has(promiseId)) {
          this.promises.get(promiseId).resolve(result)
          this.promises.delete(promiseId)
        }
      })
    }
  }
  
  createPromise() {
    const promiseId = this.lastPromiseId++
    const result = new Promise((resolve, reject) => {
      this.promises.set(promiseId, { resolve, reject })
    })
    return [promiseId, result]
  }
}

// Initialize promise handler (with safety)
let promiseHandler = null
if (window.__JUCE__?.backend) {
  promiseHandler = new PromiseHandler()
}

// Replicate JUCE's getNativeFunction
const getNativeFunction = (name) => {
  const registeredFunctions = window.__JUCE__?.initialisationData?.__juce__functions || []
  
  if (!registeredFunctions.includes(name)) {
    return null
  }
  
  if (!promiseHandler || !window.__JUCE__?.backend?.emitEvent) {
    return null
  }
  
  // Return a function that invokes the backend via emitEvent
  return function() {
    const [promiseId, result] = promiseHandler.createPromise()
    
    window.__JUCE__.backend.emitEvent('__juce__invoke', {
      name: name,
      params: Array.prototype.slice.call(arguments),
      resultId: promiseId
    })
    
    return result
  }
}

// Make it globally available IMMEDIATELY
window.__getNativeFunction = getNativeFunction

// Cache native setParameter function
let nativeSetParameter = null

const sendParam = (id, val) => {
  // Lazily get the native function using our polyfill
  if (!nativeSetParameter && window.__getNativeFunction) {
    nativeSetParameter = window.__getNativeFunction('setParameter')
  }
  
  if (typeof nativeSetParameter === 'function') {
    nativeSetParameter(id, val)
  }
  // Silently skip if not available - reduces console spam
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
  })

  if (!shell) {
    console.error('Failed to initialize Threadbare shell!')
    return
  }

  console.log('Unravel initialized with Threadbare shell')
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
