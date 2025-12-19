// Helper to get native functions via our polyfill
const getNativeFn = (name) => {
  if (typeof window.__getNativeFunction === 'function') {
    return window.__getNativeFunction(name)
  }
  return null
}

/**
 * Presets dropdown manager with robust open/close handling.
 * Uses a state-based approach to prevent race conditions.
 */
export class Presets {
  constructor() {
    this.presetPill = document.querySelector('.preset-pill')
    this.presetDropdown = document.querySelector('.preset-dropdown')
    this.presetName = document.querySelector('.preset-name')
    this.app = document.getElementById('app')
    this.currentPresetIndex = 0
    this.presetList = []
    this.initialized = false
    
    // State management
    this.state = 'closed' // 'closed' | 'open'
    this._lastToggleTime = 0
    
    // Bind methods
    this.handlePillPointerDown = this.handlePillPointerDown.bind(this)
    this.handleOptionClick = this.handleOptionClick.bind(this)
    this.handleKeyDown = this.handleKeyDown.bind(this)
    this.handleDocumentPointerDown = this.handleDocumentPointerDown.bind(this)
    
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
      // Fallback preset list for testing/development
      this.presetList = [
        'unravel',
        'shimmer',
        'cathedral',
        'whisper',
        'drift',
        'ghost'
      ]
      this.populatePresets()
    }
    
    // Attach event listeners
    this.attachEvents()
  }
  
  attachEvents() {
    if (this.presetPill) {
      // Use pointerdown for maximum responsiveness in JUCE
      this.presetPill.addEventListener('pointerdown', this.handlePillPointerDown)
      this.presetPill.addEventListener('keydown', this.handleKeyDown)
      
      // Prevent click from doing anything else
      this.presetPill.addEventListener('click', (e) => {
        e.preventDefault()
        e.stopPropagation()
      })
    }
    
    // Document listener for outside clicks (bubble phase)
    document.addEventListener('pointerdown', this.handleDocumentPointerDown)
  }
  
  handlePillPointerDown(e) {
    // Stop propagation so document listener doesn't see this as an "outside" click
    e.preventDefault()
    e.stopPropagation()
    
    const now = Date.now()
    if (now - this._lastToggleTime < 200) return
    this._lastToggleTime = now
    
    // Toggle based on current state
    if (this.state === 'closed') {
      this.openDropdown()
    } else {
      this.closeDropdown()
    }
  }
  
  handleOptionClick(e) {
    const option = e.target.closest('.preset-option')
    if (!option) return
    
    // Stop propagation to prevent toggle back
    e.preventDefault()
    e.stopPropagation()
    
    const index = parseInt(option.dataset.index, 10)
    this.selectPreset(index)
    this.closeDropdown()
  }
  
  handleKeyDown(e) {
    switch (e.key) {
      case 'Enter':
      case ' ':
        e.preventDefault()
        if (this.state === 'closed') {
          this.openDropdown()
        } else {
          this.closeDropdown()
        }
        break
      case 'Escape':
        if (this.state === 'open') {
          e.preventDefault()
          this.closeDropdown()
        }
        break
      case 'ArrowDown':
        e.preventDefault()
        if (this.state === 'open') {
          this.focusNextOption(1)
        } else {
          this.openDropdown()
        }
        break
      case 'ArrowUp':
        if (this.state === 'open') {
          e.preventDefault()
          this.focusNextOption(-1)
        }
        break
    }
  }
  
  handleDocumentPointerDown(e) {
    // Only close if open
    if (this.state !== 'open') return
    
    // If clicking the pill, handlePillPointerDown will stop propagation
    // If it reached here, it's definitely outside the pill
    
    // We also check if it's inside the dropdown itself (which is now a sibling)
    if (this.presetDropdown?.contains(e.target)) return
    
    // Close the dropdown
    this.closeDropdown()
  }
  
  openDropdown() {
    if (!this.presetPill || !this.presetDropdown || !this.app) return
    if (this.state === 'open') return
    
    this.state = 'open'
    this.app.classList.add('presets-open')
    this.presetPill.classList.add('open')
    this.presetPill.classList.add('has-opened')
    this.presetPill.setAttribute('aria-expanded', 'true')
    
    // Focus the selected option
    const currentOption = this.presetDropdown.querySelector('.preset-option.selected')
    if (currentOption) {
      setTimeout(() => currentOption.focus(), 10)
    }
  }
  
  closeDropdown() {
    if (!this.presetPill || !this.app) return
    if (this.state === 'closed') return
    
    this.state = 'closed'
    this.app.classList.remove('presets-open')
    this.presetPill.classList.remove('open')
    this.presetPill.setAttribute('aria-expanded', 'false')
    
    // Return focus to pill
    this.presetPill.focus()
  }
  
  focusNextOption(direction) {
    const options = Array.from(this.presetDropdown?.querySelectorAll('.preset-option') || [])
    if (options.length === 0) return
    
    const currentFocused = document.activeElement
    const currentIndex = options.indexOf(currentFocused)
    
    let nextIndex
    if (currentIndex === -1) {
      nextIndex = direction > 0 ? 0 : options.length - 1
    } else {
      nextIndex = currentIndex + direction
      if (nextIndex < 0) nextIndex = options.length - 1
      if (nextIndex >= options.length) nextIndex = 0
    }
    
    options[nextIndex]?.focus()
  }
  
  populatePresets() {
    if (!this.presetDropdown) return
    
    // Clear existing options
    this.presetDropdown.innerHTML = ''
    
    // Add options for each preset
    this.presetList.forEach((name, index) => {
      const option = document.createElement('li')
      option.className = 'preset-option'
      option.setAttribute('role', 'option')
      option.setAttribute('tabindex', '-1')
      option.dataset.index = index
      option.textContent = name
      
      if (index === this.currentPresetIndex) {
        option.classList.add('selected')
        option.setAttribute('aria-selected', 'true')
      }
      
      // Use both pointerdown and click for maximum compatibility
      option.addEventListener('pointerdown', this.handleOptionClick)
      option.addEventListener('click', this.handleOptionClick)
      option.addEventListener('keydown', (e) => {
        if (e.key === 'Enter' || e.key === ' ') {
          e.preventDefault()
          this.selectPreset(index)
          this.closeDropdown()
        }
      })
      
      this.presetDropdown.appendChild(option)
    })
    
    this.updatePresetName()
  }
  
  selectPreset(index) {
    this.loadPreset(index)
    this.updateSelectedOption(index)
  }
  
  updateSelectedOption(index) {
    if (!this.presetDropdown) return
    
    // Remove selected from all
    this.presetDropdown.querySelectorAll('.preset-option').forEach(opt => {
      opt.classList.remove('selected')
      opt.setAttribute('aria-selected', 'false')
    })
    
    // Add selected to current
    const option = this.presetDropdown.querySelector(`[data-index="${index}"]`)
    if (option) {
      option.classList.add('selected')
      option.setAttribute('aria-selected', 'true')
    }
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
      // Dev mode - just update locally
      this.currentPresetIndex = index
      this.updatePresetName()
      console.warn('Native loadPreset not available, updated locally')
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
      
      // Update selected option in dropdown
      this.updateSelectedOption(this.currentPresetIndex)
      
      // Update displayed name
      this.updatePresetName()
    }
  }
}
