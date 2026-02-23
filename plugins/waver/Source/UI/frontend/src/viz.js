export class WaverViz {
  constructor(canvas) {
    this.canvas = canvas
    this.ctx = canvas.getContext("2d")
    this.state = {}
    this.resize()
  }

  resize() {
    const dpr = window.devicePixelRatio || 1
    const width = this.canvas.clientWidth || 400
    const height = this.canvas.clientHeight || 400
    this.canvas.width = Math.round(width * dpr)
    this.canvas.height = Math.round(height * dpr)
    this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0)
  }

  update(state) {
    this.state = state || {}
  }

  draw() {
    const ctx = this.ctx
    const w = this.canvas.clientWidth || 400
    const h = this.canvas.clientHeight || 400
    ctx.clearRect(0, 0, w, h)
    ctx.fillStyle = "rgba(255, 255, 255, 0.12)"
    ctx.beginPath()
    ctx.arc(w * 0.5, h * 0.5, 48, 0, Math.PI * 2)
    ctx.fill()
  }

  dispose() {}
}
