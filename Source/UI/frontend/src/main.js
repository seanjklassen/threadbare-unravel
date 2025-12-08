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


// =============================================================================
// Application Code - Wait for DOM to be ready
// =============================================================================

import './style.css'
import { Orb } from './orb.js'
import { Controls } from './controls.js'
import { Presets } from './presets.js'

// App state (declared at module scope so updateState can access them)
let orb = null
let controls = null
let presets = null
let uiState = null
let currentState = null

const initApp = () => {
  const canvas = document.getElementById('orb')
  const shell = document.querySelector('.tb-canvas-shell')
  
  if (!canvas || !shell) {
    console.error('Required DOM elements not found!')
    return
  }
  
  orb = new Orb(canvas)

  controls = new Controls({
    onPuckChange: ({ puckX, puckY }) => {
      currentState = { ...currentState, puckX, puckY }
      orb.update(currentState)
    },
    onFreezeChange: (isFrozen) => {
      uiState.frozen = isFrozen
      currentState = { ...currentState, freeze: isFrozen }
    },
  })

  presets = new Presets()

  uiState = {
    frozen: false,
    canvasRect: shell.getBoundingClientRect(),
  }

  currentState = {
    inLevel: 0.35,
    tailLevel: 0.4,
    puckX: 0.5,
    puckY: 0.5,
    drift: 0.2,
    ghost: 0.2,
    decay: 0.4,
    size: 0.6,
    freeze: false,
  }

  const resizeCanvas = () => {
    uiState.canvasRect = shell.getBoundingClientRect()
    orb.resize()
    controls.refresh()
  }

  window.addEventListener('resize', resizeCanvas)

  const animate = () => {
    if (!uiState.frozen) {
      orb.update(currentState)
      orb.draw()
    }
    requestAnimationFrame(animate)
  }

  resizeCanvas()
  controls.update(currentState)
  animate()
}

// Handler for state updates from C++ backend
window.updateState = (payload) => {
  if (payload == null) return
  if (!orb || !controls || !presets) return // App not initialized yet

  let parsed = payload
  if (typeof payload === 'string') {
    try {
      parsed = JSON.parse(payload)
    } catch (error) {
      console.warn('Invalid updateState payload', error)
      return
    }
  }

  if (typeof parsed !== 'object') return

  currentState = { ...currentState, ...parsed }
  orb.update(currentState)
  controls.update(currentState)
  presets.update(currentState)
}

// Wait for DOM to be ready before initializing
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', initApp)
} else {
  initApp()
}
