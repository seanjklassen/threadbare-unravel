// Helper to get native functions via our polyfill
const getNativeFn = (name) => {
  if (typeof window.__getNativeFunction === 'function') {
    return window.__getNativeFunction(name)
  }
  return null
}

export class Presets {
  constructor() {
    this.presetPill = document.querySelector('.preset-pill')
    this.presetDropdown = document.querySelector('.preset-dropdown')
    this.presetName = document.querySelector('.preset-name')
    this.currentPresetIndex = 0
    this.presetList = []
    this.initialized = false
    this.isOpen = false
    
    // Bind methods
    this.handlePillClick = this.handlePillClick.bind(this)
    this.handleOptionClick = this.handleOptionClick.bind(this)
    this.handleKeyDown = this.handleKeyDown.bind(this)
    this.handleClickOutside = this.handleClickOutside.bind(this)
    
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
    
    // Attach event listeners
    this.attachEvents()
  }
  
  attachEvents() {
    if (this.presetPill) {
      this.presetPill.addEventListener('click', this.handlePillClick)
      this.presetPill.addEventListener('keydown', this.handleKeyDown)
    }
    
    // Click outside to close
    document.addEventListener('click', this.handleClickOutside)
  }
  
  handlePillClick(e) {
    // Don't toggle if clicking an option (let option handler deal with it)
    if (e.target.closest('.preset-option')) return
    
    e.stopPropagation()
    this.toggleDropdown()
  }
  
  handleOptionClick(e) {
    const option = e.target.closest('.preset-option')
    if (!option) return
    
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
        this.toggleDropdown()
        break
      case 'Escape':
        if (this.isOpen) {
          e.preventDefault()
          this.closeDropdown()
        }
        break
      case 'ArrowDown':
        if (this.isOpen) {
          e.preventDefault()
          this.focusNextOption(1)
        } else {
          e.preventDefault()
          this.openDropdown()
        }
        break
      case 'ArrowUp':
        if (this.isOpen) {
          e.preventDefault()
          this.focusNextOption(-1)
        }
        break
    }
  }
  
  handleClickOutside(e) {
    if (this.isOpen && !this.presetPill?.contains(e.target)) {
      this.closeDropdown()
    }
  }
  
  toggleDropdown() {
    if (this.isOpen) {
      this.closeDropdown()
    } else {
      this.openDropdown()
    }
  }
  
  openDropdown() {
    if (!this.presetPill || !this.presetDropdown) return
    this.isOpen = true
    this.presetPill.classList.add('open')
    this.presetPill.setAttribute('aria-expanded', 'true')
    
    // Focus current option
    const currentOption = this.presetDropdown.querySelector('.preset-option.selected')
    currentOption?.focus()
  }
  
  closeDropdown() {
    if (!this.presetPill) return
    this.isOpen = false
    this.presetPill.classList.remove('open')
    this.presetPill.setAttribute('aria-expanded', 'false')
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
