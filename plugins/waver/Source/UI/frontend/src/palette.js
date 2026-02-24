const WAVER_PALETTE = Object.freeze({
  surfaceBase: "#31312B",
  surfaceBaseHover: "#DD8469",
  textPrimary: "#D3C7BB",
  panelInk: "#31312B",
  panelInkSoft: "#7F9BA5",
  waveformShadowDrift: "#7F9BA5",
  textUpper: "#D3C7BB",
  textLower: "#31312B",
})

function hexToRgb(hex) {
  const normalized = hex.replace("#", "")
  const value = Number.parseInt(normalized, 16)
  return {
    r: (value >> 16) & 0xff,
    g: (value >> 8) & 0xff,
    b: value & 0xff,
  }
}

function alpha(hex, opacity) {
  const { r, g, b } = hexToRgb(hex)
  const clamped = Math.max(0, Math.min(1, opacity))
  return `rgba(${r},${g},${b},${clamped})`
}

function applyWaverPaletteCssVars(target = document.documentElement) {
  if (!target?.style) return
  target.style.setProperty("--waver-surface-base", WAVER_PALETTE.surfaceBase)
  target.style.setProperty("--waver-surface-base-hover", WAVER_PALETTE.surfaceBaseHover)
  target.style.setProperty("--waver-text-primary", WAVER_PALETTE.textPrimary)
  target.style.setProperty("--waver-panel-ink", WAVER_PALETTE.panelInk)
  target.style.setProperty("--waver-panel-ink-soft", WAVER_PALETTE.panelInkSoft)
  target.style.setProperty("--waver-waveform-shadow-drift", WAVER_PALETTE.waveformShadowDrift)
  target.style.setProperty("--waver-text-upper", WAVER_PALETTE.textUpper)
  target.style.setProperty("--waver-text-lower", WAVER_PALETTE.textLower)
  target.style.setProperty("--waver-overlay-surface", alpha(WAVER_PALETTE.panelInkSoft, 0.3))
  target.style.setProperty("--waver-overlay-panel", alpha(WAVER_PALETTE.panelInk, 0.42))
  target.style.setProperty("--waver-border-soft", alpha(WAVER_PALETTE.panelInkSoft, 0.15))
}

export {
  WAVER_PALETTE,
  applyWaverPaletteCssVars,
  hexToRgb,
}
