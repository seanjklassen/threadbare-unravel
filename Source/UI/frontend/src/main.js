import './style.css'

const canvas = document.getElementById('orb')
const ctx = canvas.getContext('2d')
const puck = document.getElementById('puck')
const readoutX = document.querySelector('[data-readout="x"]')
const readoutY = document.querySelector('[data-readout="y"]')
const freezeBtn = document.querySelector('.btn-freeze')
const shell = document.querySelector('.tb-canvas-shell')
const rootStyles = getComputedStyle(document.documentElement)
const orbitStroke =
  rootStyles.getPropertyValue('--orb-line').trim() || '#E1A6A6'

const state = {
  t: 0,
  frozen: false,
  speed: 0.005,
  canvasRect: shell.getBoundingClientRect(),
}

window.updateState = (payload = {}) => {
  // eslint-disable-next-line no-console
  console.log('Threadbare state update:', payload)
}

let lastXY = { x: 0.5, y: 0.5 }

const resizeCanvas = () => {
  const dpr = window.devicePixelRatio || 1
  const rect = shell.getBoundingClientRect()
  state.canvasRect = rect
  canvas.width = rect.width * dpr
  canvas.height = rect.height * dpr
  ctx.setTransform(1, 0, 0, 1, 0, 0)
  ctx.scale(dpr, dpr)
}

const formatValue = (value) => value.toFixed(3)

const updatePuck = ({ x, y }) => {
  const left = state.canvasRect.width * x
  const top = state.canvasRect.height * y
  puck.style.left = `${left}px`
  puck.style.top = `${top}px`
  readoutX.textContent = `${formatValue(x * 10)}`
  readoutY.textContent = `${formatValue(y * 10)}`
}

const drawOrbit = (time) => {
  const { width, height } = state.canvasRect
  ctx.clearRect(0, 0, width, height)
  ctx.lineWidth = 2
  ctx.strokeStyle = orbitStroke
  ctx.globalAlpha = 0.85
  ctx.beginPath()
  const steps = 720
  for (let i = 0; i <= steps; i += 1) {
    const phase = (i / steps) * Math.PI * 2
    const offset = phase + time * 0.6
    const x =
      width / 2 +
      Math.sin(offset * 1.7) * width * 0.35 +
      Math.cos(offset * 0.85) * width * 0.08
    const y =
      height / 2 +
      Math.cos(offset * 1.3) * height * 0.32 +
      Math.sin(offset * 0.65) * height * 0.06
    if (i === 0) {
      ctx.moveTo(x, y)
    } else {
      ctx.lineTo(x, y)
    }
  }
  ctx.stroke()
  ctx.globalAlpha = 1
}

const animate = () => {
  if (!state.frozen) {
    state.t += state.speed
    drawOrbit(state.t)
    const coords = {
      x: 0.5 + 0.35 * Math.sin(state.t * 0.9) + 0.1 * Math.sin(state.t * 2.2),
      y:
        0.5 +
        0.28 * Math.cos(state.t * 0.7 + Math.PI / 3) +
        0.08 * Math.sin(state.t * 1.4),
    }
    const clamped = {
      x: Math.min(Math.max(coords.x, 0.05), 0.95),
      y: Math.min(Math.max(coords.y, 0.05), 0.95),
    }
    lastXY = clamped
    updatePuck(lastXY)
  }
  requestAnimationFrame(animate)
}

freezeBtn.addEventListener('click', () => {
  state.frozen = !state.frozen
  freezeBtn.setAttribute('aria-pressed', String(state.frozen))
})

window.addEventListener('resize', () => {
  resizeCanvas()
  updatePuck(lastXY)
  drawOrbit(state.t)
})

resizeCanvas()
updatePuck(lastXY)
drawOrbit(state.t)
animate()

const debugLoop = () => {
  window.updateState({
    frozen: state.frozen,
    coords: lastXY,
    timestamp: Date.now(),
  })
}

setInterval(debugLoop, 2000)
