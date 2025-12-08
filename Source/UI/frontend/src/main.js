import './style.css'
import { Orb } from './orb.js'
import { Controls } from './controls.js'
import { Presets } from './presets.js'

const canvas = document.getElementById('orb')
const shell = document.querySelector('.tb-canvas-shell')
const orb = new Orb(canvas)

const controls = new Controls({
  onPuckChange: ({ puckX, puckY }) => {
    currentState = { ...currentState, puckX, puckY }
    orb.update(currentState)
  },
  onFreezeChange: (isFrozen) => {
    uiState.frozen = isFrozen
    currentState = { ...currentState, freeze: isFrozen }
  },
})

const presets = new Presets()

const uiState = {
  frozen: false,
  canvasRect: shell.getBoundingClientRect(),
}

let currentState = {
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

// Mock loop removed – real data flows through window.updateState.
// const mockLoop = () => {}
// setInterval(mockLoop, 2000)

window.updateState = (payload) => {
  if (payload == null) return

  let parsed = payload
  if (typeof payload === 'string') {
    try {
      parsed = JSON.parse(payload)
    } catch (error) {
      // eslint-disable-next-line no-console
      console.warn('Invalid updateState payload', error)
      return
    }
  }

  if (typeof parsed !== 'object') return

  // Debug: log state updates
  console.log('State update received:', parsed)

  currentState = { ...currentState, ...parsed }
  orb.update(currentState)
  controls.update(currentState)
  presets.update(currentState)
}

// Debug: Check if setParameter is available
console.log('window.setParameter available:', typeof window.setParameter === 'function')

// Wait for native integration to be ready
let nativeIntegrationReady = typeof window.setParameter === 'function'

if (!nativeIntegrationReady) {
  console.warn('Native integration not ready immediately, polling...')
  
  // Poll for native function availability
  const checkInterval = setInterval(() => {
    if (typeof window.setParameter === 'function') {
      console.log('Native integration NOW available!')
      nativeIntegrationReady = true
      clearInterval(checkInterval)
    } else {
      console.log('Still waiting for native integration...')
    }
  }, 100)
  
  // Give up after 5 seconds
  setTimeout(() => {
    clearInterval(checkInterval)
    if (!nativeIntegrationReady) {
      console.error('Native integration failed to initialize after 5 seconds')
      console.log('Available window properties:', Object.keys(window).filter(k => k.includes('set') || k.includes('update')))
    }
  }, 5000)
}

resizeCanvas()
controls.update(currentState)
animate()
