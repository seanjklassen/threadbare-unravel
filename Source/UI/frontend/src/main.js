import './style.css'
import { Orb } from './orb.js'

const canvas = document.getElementById('orb')
const orb = new Orb(canvas)
const puck = document.getElementById('puck')
const readoutX = document.querySelector('[data-readout="x"]')
const readoutY = document.querySelector('[data-readout="y"]')
const freezeBtn = document.querySelector('.btn-freeze')
const shell = document.querySelector('.tb-canvas-shell')

const uiState = {
  frozen: false,
  canvasRect: shell.getBoundingClientRect(),
}

const orbInput = {
  inLevel: 0.35,
  tailLevel: 0.4,
  puckX: 0.5,
  puckY: 0.5,
  drift: 0.2,
  ghost: 0.2,
}

const clamp = (value, min, max) => Math.min(Math.max(value, min), max)
const formatValue = (value) => value.toFixed(3)

const updatePuck = ({ x, y }) => {
  const left = uiState.canvasRect.width * x
  const top = uiState.canvasRect.height * y
  puck.style.left = `${left}px`
  puck.style.top = `${top}px`
  readoutX.textContent = `${formatValue(x * 10)}`
  readoutY.textContent = `${formatValue(y * 10)}`
}

const syncHud = () => {
  const x = clamp(orbInput.puckX, 0.02, 0.98)
  const y = clamp(orbInput.puckY, 0.02, 0.98)
  orbInput.puckX = x
  orbInput.puckY = y
  updatePuck({ x, y })
}

let simulateInput = true
let autoTheta = 0

const animate = () => {
  if (!uiState.frozen) {
    if (simulateInput) {
      autoTheta += 0.01
      const wobble = 0.25 + 0.15 * Math.sin(autoTheta * 0.7)
      orbInput.inLevel = clamp(0.45 + 0.35 * Math.sin(autoTheta * 1.1), 0, 1)
      orbInput.tailLevel = clamp(0.35 + 0.4 * Math.cos(autoTheta * 0.8), 0, 1)
      orbInput.puckX = 0.5 + wobble * Math.sin(autoTheta * 0.9)
      orbInput.puckY = 0.5 + wobble * Math.cos(autoTheta * 0.8)
      orbInput.drift = clamp(0.12 + 0.25 * Math.sin(autoTheta * 0.35), 0, 1)
      orbInput.ghost =
        0.15 + 0.25 * (0.5 + 0.5 * Math.cos(autoTheta * 0.6))
      syncHud()
    }

    orb.update(orbInput)
    orb.draw()
  }

  requestAnimationFrame(animate)
}

const resizeCanvas = () => {
  uiState.canvasRect = shell.getBoundingClientRect()
  orb.resize()
  syncHud()
}

freezeBtn.addEventListener('click', () => {
  uiState.frozen = !uiState.frozen
  freezeBtn.setAttribute('aria-pressed', String(uiState.frozen))
})

window.addEventListener('resize', resizeCanvas)

window.updateState = (payload = {}) => {
  Object.assign(orbInput, payload)
  simulateInput = false
  syncHud()
  orb.update(orbInput)
  // eslint-disable-next-line no-console
  console.log('Threadbare state update:', { ...orbInput })
}

resizeCanvas()
syncHud()
animate()
