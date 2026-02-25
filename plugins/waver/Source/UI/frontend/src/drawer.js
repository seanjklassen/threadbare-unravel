import { PARAMS } from "./generated/params.js"

const GROUPS = [
  {
    label: "tone",
    ids: ["filterCutoff", "filterRes", "filterMode", "filterKeyTrack", "envToFilter"],
  },
  {
    label: "shape",
    ids: [
      "macroShape", "dcoSubLevel", "dcoSubOctave", "noiseLevel", "noiseColor",
      "toyIndex", "toyRatio", "layerDco", "layerToy", "layerOrgan",
      "organ16", "organ8", "organ4", "organMix",
    ],
  },
  {
    label: "motion",
    ids: [
      "lfoRate", "lfoShape", "lfoToVibrato", "lfoToPwm", "chorusMode",
      "driftAmount", "stereoWidth", "portaTime", "portaMode",
      "envAttack", "envDecay", "envSustain", "envRelease",
    ],
  },
  {
    label: "print",
    ids: [
      "driveGain", "tapeSat", "wowDepth", "flutterDepth",
      "hissLevel", "humFreq", "printMix", "outputGain", "qualityMode",
    ],
  },
]

function normalise(id, value) {
  const p = PARAMS[id]
  if (!p || p.type === "choice" || p.type === "bool") return 0
  return Math.max(0, Math.min(1, (value - p.min) / (p.max - p.min)))
}

function denormalise(id, norm) {
  const p = PARAMS[id]
  if (!p) return norm
  return p.min + norm * (p.max - p.min)
}

function formatValue(id, raw) {
  const p = PARAMS[id]
  if (!p) return String(raw)
  if (p.unit === "Hz") return raw >= 1000 ? `${(raw / 1000).toFixed(1)}k` : `${Math.round(raw)}`
  if (p.unit === "dB") return raw >= 0 ? `+${raw.toFixed(1)}` : raw.toFixed(1)
  if (p.unit === "s") return raw < 1 ? `${(raw * 1000).toFixed(0)}ms` : `${raw.toFixed(2)}s`
  if (p.unit === "ms") return `${Math.round(raw)}ms`
  if (p.unit === "cents") return `${Math.round(raw)}c`
  if (p.max <= 1 && p.min >= -1) return `${(raw * 100).toFixed(0)}%`
  if (p.max <= 8 && p.min >= 0 && Number.isInteger(p.max)) return String(Math.round(raw))
  return raw.toFixed(2)
}

export function buildDrawer(container, sendParam) {
  const sliderInstances = {}
  const choiceInstances = {}

  for (const group of GROUPS) {
    const heading = document.createElement("h3")
    heading.className = "drawer-group-heading"
    heading.textContent = group.label
    container.appendChild(heading)

    for (const id of group.ids) {
      const p = PARAMS[id]
      if (!p) continue

      if (p.type === "choice") {
        const row = buildChoiceRow(id, p, sendParam)
        container.appendChild(row.el)
        choiceInstances[id] = row
      } else {
        const row = buildSliderRow(id, p, sendParam)
        container.appendChild(row.el)
        sliderInstances[id] = row
      }
    }
  }

  return {
    update(stateMap) {
      for (const [id, inst] of Object.entries(sliderInstances)) {
        const val = stateMap[id]
        if (val !== undefined && !inst.dragging) {
          inst.setNorm(normalise(id, val))
        }
      }
      for (const [id, inst] of Object.entries(choiceInstances)) {
        const val = stateMap[id]
        if (val !== undefined) {
          inst.setIndex(Math.round(val))
        }
      }
    },
  }
}

function buildSliderRow(id, param, sendParam) {
  const el = document.createElement("div")
  el.className = "control-row"
  el.dataset.param = id

  const label = document.createElement("label")
  label.className = "param-label"
  label.textContent = param.name

  const valueDisplay = document.createElement("span")
  valueDisplay.className = "param-value"
  valueDisplay.textContent = formatValue(id, param.default ?? 0)

  const track = document.createElement("div")
  track.className = "waver-slider"
  track.setAttribute("role", "slider")
  track.setAttribute("tabindex", "0")
  track.setAttribute("aria-label", param.name)
  track.setAttribute("aria-valuemin", "0")
  track.setAttribute("aria-valuemax", "1")

  const fill = document.createElement("div")
  fill.className = "waver-slider__fill"
  const thumb = document.createElement("div")
  thumb.className = "waver-slider__thumb"
  track.appendChild(fill)
  track.appendChild(thumb)

  el.appendChild(label)
  el.appendChild(track)
  el.appendChild(valueDisplay)

  let dragging = false
  let norm = normalise(id, param.default ?? param.min ?? 0)

  function render(n) {
    const pct = `${(n * 100).toFixed(1)}%`
    fill.style.width = pct
    thumb.style.left = pct
    const raw = denormalise(id, n)
    valueDisplay.textContent = formatValue(id, raw)
    track.setAttribute("aria-valuenow", n.toFixed(3))
  }
  render(norm)

  function onMove(clientX) {
    const rect = track.getBoundingClientRect()
    const n = Math.max(0, Math.min(1, (clientX - rect.left) / rect.width))
    norm = n
    render(n)
    sendParam(id, denormalise(id, n))
  }

  track.addEventListener("pointerdown", (e) => {
    dragging = true
    track.setPointerCapture(e.pointerId)
    onMove(e.clientX)
  })
  track.addEventListener("pointermove", (e) => { if (dragging) onMove(e.clientX) })
  track.addEventListener("pointerup", () => { dragging = false })
  track.addEventListener("pointercancel", () => { dragging = false })

  track.addEventListener("keydown", (e) => {
    let step = 0.02
    if (e.shiftKey) step = 0.1
    if (e.key === "ArrowRight" || e.key === "ArrowUp") {
      norm = Math.min(1, norm + step)
      render(norm)
      sendParam(id, denormalise(id, norm))
      e.preventDefault()
    } else if (e.key === "ArrowLeft" || e.key === "ArrowDown") {
      norm = Math.max(0, norm - step)
      render(norm)
      sendParam(id, denormalise(id, norm))
      e.preventDefault()
    }
  })

  return {
    el,
    get dragging() { return dragging },
    setNorm(n) {
      norm = n
      render(n)
    },
  }
}

function buildChoiceRow(id, param, sendParam) {
  const el = document.createElement("div")
  el.className = "control-row control-row--choice"
  el.dataset.param = id

  const label = document.createElement("label")
  label.className = "param-label"
  label.textContent = param.name

  const group = document.createElement("div")
  group.className = "choice-group"
  group.setAttribute("role", "radiogroup")
  group.setAttribute("aria-label", param.name)

  const options = param.options || []
  let currentIndex = param.default ?? 0

  const buttons = options.map((opt, i) => {
    const btn = document.createElement("button")
    btn.className = "choice-btn"
    btn.type = "button"
    btn.textContent = opt
    btn.setAttribute("role", "radio")
    btn.setAttribute("aria-checked", i === currentIndex ? "true" : "false")
    if (i === currentIndex) btn.classList.add("active")

    btn.addEventListener("click", () => {
      currentIndex = i
      buttons.forEach((b, j) => {
        b.classList.toggle("active", j === i)
        b.setAttribute("aria-checked", j === i ? "true" : "false")
      })
      sendParam(id, i)
    })

    group.appendChild(btn)
    return btn
  })

  el.appendChild(label)
  el.appendChild(group)

  return {
    el,
    setIndex(idx) {
      currentIndex = idx
      buttons.forEach((b, j) => {
        b.classList.toggle("active", j === idx)
        b.setAttribute("aria-checked", j === idx ? "true" : "false")
      })
    },
  }
}
