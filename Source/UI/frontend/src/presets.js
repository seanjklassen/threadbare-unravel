// Helper to get native functions via our polyfill
const getNativeFn = (name) => {
  if (typeof window.__getNativeFunction === 'function') {
    return window.__getNativeFunction(name)
  }
  return null
}

export class Presets {
  constructor() {
    this.presetSelect = document.getElementById('preset-select')
    this.presetName = document.querySelector('.preset-name')
    this.currentPresetIndex = 0
    this.presetList = []
    this.initialized = false
    
    // Delay init to allow main.js to set up window.__getNativeFunction
    setTimeout(() => this.init(), 0)
  }
  
  async init() {
    // If native functions aren't ready yet, retry
    if (typeof window.__getNativeFunction !== 'function') {
      setTimeout(() => this.init(), 100)
      return
    }
    
    if (this.initialized) return
    this.initialized = true
    
    const getPresetListFn = getNativeFn('getPresetList')
    
    // Get preset list from backend
    if (typeof getPresetListFn === 'function') {
      try {
        const presets = await getPresetListFn()
        this.presetList = presets || []
        this.populatePresets()
        console.log('Presets loaded:', this.presetList)
      } catch (error) {
        console.error('Failed to load presets:', error)
      }
    } else {
      console.warn('Native getPresetList not available')
      // Fallback preset list for testing
      this.presetList = ['unravel']
      this.populatePresets()
    }
    
    // Attach change listener
    if (this.presetSelect) {
      this.presetSelect.addEventListener('change', (e) => {
        this.loadPreset(parseInt(e.target.value, 10))
      })
    }
  }
  
  populatePresets() {
    if (!this.presetSelect) return
    
    // Clear existing options
    this.presetSelect.innerHTML = ''
    
    // Add options for each preset
    this.presetList.forEach((name, index) => {
      const option = document.createElement('option')
      option.value = index
      option.textContent = name
      this.presetSelect.appendChild(option)
    })
    
    // Set initial selection
    this.presetSelect.value = this.currentPresetIndex
    this.updatePresetName()
  }
  
  async loadPreset(index) {
    const loadPresetFn = getNativeFn('loadPreset')
    
    if (typeof loadPresetFn === 'function') {
      try {
        const success = await loadPresetFn(index)
        if (success) {
          this.currentPresetIndex = index
          this.updatePresetName()
          console.log('Loaded preset:', this.presetList[index])
        } else {
          console.error('Failed to load preset:', index)
        }
      } catch (error) {
        console.error('Error loading preset:', error)
      }
    } else {
      console.warn('Native loadPreset not available')
    }
  }
  
  updatePresetName() {
    if (this.presetName && this.presetList[this.currentPresetIndex]) {
      this.presetName.textContent = this.presetList[this.currentPresetIndex]
    }
  }
  
  update(state = {}) {
    if (typeof state.currentPreset !== 'undefined' && 
        state.currentPreset !== this.currentPresetIndex) {
      this.currentPresetIndex = state.currentPreset
      
      // Update select element
      if (this.presetSelect) {
        this.presetSelect.value = this.currentPresetIndex
      }
      
      // Update displayed name
      this.updatePresetName()
    }
  }
}

