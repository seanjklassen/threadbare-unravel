// =============================================================================
// THREADBARE UI SHELL - Entry Point
// 
// A shared UI chassis for Threadbare audio plugins, providing:
// - Puck (XY pad) control
// - Preset dropdown
// - Settings drawer with elastic sliders
// - Theme token support
// =============================================================================

import { Controls } from './controls.js'
import { Presets } from './presets.js'
import { ElasticSlider } from './elastic-slider.js'

// Re-export components for direct use if needed
export { Controls, Presets, ElasticSlider }

/**
 * Apply theme tokens as CSS variables on :root
 * @param {Object} themeTokens - Object of CSS variable names to values
 */
function applyThemeTokens(themeTokens) {
  if (!themeTokens || typeof themeTokens !== 'object') return
  
  const root = document.documentElement
  for (const [key, value] of Object.entries(themeTokens)) {
    // Ensure variable name starts with --
    const varName = key.startsWith('--') ? key : `--${key}`
    root.style.setProperty(varName, value)
  }
}

/**
 * Initialize the Threadbare UI shell.
 * 
 * @param {Object} options
 * @param {Function} options.VizClass - Visualization class constructor (e.g., Orb)
 * @param {Object} options.params - Parameter metadata object { paramId: { min, max, default, ... } }
 * @param {string[]} [options.paramOrder] - Order of parameter IDs for drawer display
 * @param {Object} [options.themeTokens] - CSS variables to apply { bg: '#31312b', accent: '#E0E993', ... }
 * @param {Function} [options.getNativeFn] - Function to get native backend functions
 * @param {Function} [options.sendParam] - Function to send parameter to backend
 * @param {Function} [options.onStateUpdate] - Callback when state updates from backend
 * @returns {Object} Shell instance with viz, controls, presets, and lifecycle methods
 */
export function initShell(options = {}) {
  const {
    VizClass,
    params,
    paramOrder,
    themeTokens,
    getNativeFn = () => null,
    sendParam = () => {},
    onStateUpdate,
  } = options

  // Apply theme tokens first (before UI renders)
  if (themeTokens) {
    applyThemeTokens(themeTokens)
  }

  // Get visualization container
  const vizSlot = document.getElementById('viz-slot') || document.getElementById('orb')?.parentElement
  const canvas = document.getElementById('orb')
  const shell = document.querySelector('.tb-canvas-shell')
  
  if (!canvas || !shell) {
    console.error('Threadbare Shell: Required DOM elements not found!')
    return null
  }

  // App state
  let viz = null
  let controls = null
  let presets = null
  let uiState = null
  let currentState = {}

  // Initialize visualization
  if (VizClass && canvas) {
    viz = new VizClass(canvas)
  }

  // Initialize controls with injected params (NOT importing from generated file)
  controls = new Controls({
    params,
    paramOrder,
    sendParam,
    onPuckChange: ({ puckX, puckY }) => {
      currentState = { ...currentState, puckX, puckY }
      viz?.update(currentState)
    },
    onFreezeChange: (isFrozen) => {
      // Note: We don't stop the orb animation for disintegration looper
      // The orb uses entropy effects instead of freezing
      currentState = { ...currentState, freeze: isFrozen }
    },
  })

  // Initialize presets with injected native function getter
  presets = new Presets({
    getNativeFn,
  })

  // UI state for freeze/resize handling
  uiState = {
    frozen: false,
    canvasRect: shell.getBoundingClientRect(),
  }

  // Initial state (can be updated via updateState)
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

  // Resize handler
  const resizeCanvas = () => {
    uiState.canvasRect = shell.getBoundingClientRect()
    viz?.resize()
    controls?.refresh()
  }

  window.addEventListener('resize', resizeCanvas)

  // Animation loop - always runs (disintegration looper uses entropy effects, not freeze)
  const animate = () => {
      viz?.update(currentState)
      viz?.draw()
    requestAnimationFrame(animate)
  }

  // Initialize
  resizeCanvas()
  controls?.update(currentState)
  animate()

  // Handler for state updates from C++ backend
  const updateState = (payload) => {
    if (payload == null) return
    if (!viz || !controls || !presets) return

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

    // Skip puckX/puckY updates while user is dragging to prevent flicker
    if (controls?.isDragging) {
      const { puckX, puckY, ...rest } = parsed
      currentState = { ...currentState, ...rest }
    } else {
      currentState = { ...currentState, ...parsed }
    }
    
    viz.update(currentState)
    controls.update(currentState)
    presets.update(currentState)
    
    // Notify external listener if provided
    onStateUpdate?.(currentState)
  }

  // Return shell instance with public API
  return {
    viz,
    controls,
    presets,
    updateState,
    getCurrentState: () => ({ ...currentState }),
    setUiState: (updates) => {
      uiState = { ...uiState, ...updates }
    },
  }
}

