// =============================================================================
// PRESETS - Shared UI component
// Dropdown preset manager with robust open/close handling for DAW WebViews
// =============================================================================

/**
 * Presets dropdown manager with robust open/close handling.
 * Uses a state-based approach to prevent race conditions.
 * 
 * @param {Object} options
 * @param {Function} options.getNativeFn - Function to get native backend functions
 */
export class Presets {
  constructor(options = {}) {
    // Dependency injection for native function access
    this.getNativeFn = options.getNativeFn || (() => null)

    // Manual regression checklist:
    // - Mouse: click pill toggles open/close; click option selects and closes; click outside closes; no flash on release.
    // - Mouse: press option, drag off, release => should NOT select.
    // - Keyboard: Enter toggles; Arrow keys navigate while open; focus stays sensible.
    // - Rapid: repeated quick clicks on pill/option/outside => no reopen/flash.
    // - Other UI: puck/orb dragging and other controls behave unchanged.

    this.presetPill = document.querySelector('.preset-pill')
    this.presetDropdown = document.querySelector('.preset-dropdown')
    this.presetName = document.querySelector('.preset-name')
    this.app = document.getElementById('app')
    this.currentPresetIndex = 0
    this.presetList = []
    this.initialized = false
    
    // State management
    this.state = 'closed' // 'closed' | 'open'

    // Pointer-gesture state (prevents click-through + supports "press-drag-release off option cancels")
    this.activePointerId = null
    this.downOption = null
    this.downWasOnOption = false
    this.downStartedInPillToggleRegion = false
    this.suppressToggleUntil = 0
    this.ignorePillClickUntil = 0
    
    // Bind methods
    this.handleOptionClick = this.handleOptionClick.bind(this)
    this.handleKeyDown = this.handleKeyDown.bind(this)
    this.handleClickOutside = this.handleClickOutside.bind(this)
    this.handlePillClick = this.handlePillClick.bind(this)
    this.toggleDropdown = this.toggleDropdown.bind(this)

    // New pointer/capture model
    this.onDocPointerDownCapture = this.onDocPointerDownCapture.bind(this)
    this.onDocPointerUpCapture = this.onDocPointerUpCapture.bind(this)
    this.onPillPointerUp = this.onPillPointerUp.bind(this)
    this.onPillPointerDown = this.onPillPointerDown.bind(this)
    
    // Delay init to allow main.js to set up native functions
    setTimeout(() => this.init(), 0)
  }
  
  async init() {
    if (this.initialized) return
    this.initialized = true
    
    const getPresetListFn = this.getNativeFn('getPresetList')
    
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
        'close',
        'tether',
        'pulse',
        'bloom',
        'mist',
        'rewind',
        'halation',
        'stasis',
        'shiver'
      ]
      this.populatePresets()
    }
    
    // Attach event listeners
    this.attachEvents()
  }

  attachEvents() {
    const supportsPointer = typeof window !== 'undefined' && 'PointerEvent' in window

    if (this.presetPill) {
      // Pointer-based toggle (avoid relying on click ordering)
      if (supportsPointer) {
        this.presetPill.addEventListener('pointerdown', this.onPillPointerDown)
        this.presetPill.addEventListener('pointerup', this.onPillPointerUp)
      } else {
        this.presetPill.addEventListener('mousedown', this.onPillPointerDown)
        this.presetPill.addEventListener('mouseup', this.onPillPointerUp)
      }

      // Legacy click handler left in place for now but becomes a no-op under suppression.
      // (We keep it to preserve any accessibility quirks from older browsers.)
      this.presetPill.addEventListener('click', this.handlePillClick)
      this.presetPill.addEventListener('keydown', this.handleKeyDown)
    }
    
    // Deterministic outside-close (capture, before click is dispatched)
    if (supportsPointer) {
      document.addEventListener('pointerdown', this.onDocPointerDownCapture, { capture: true })
      document.addEventListener('pointerup', this.onDocPointerUpCapture, { capture: true })
    } else {
      document.addEventListener('mousedown', this.onDocPointerDownCapture, { capture: true })
      document.addEventListener('mouseup', this.onDocPointerUpCapture, { capture: true })
    }

    // Legacy outside click handler (bubble) kept for safety, but should be redundant now.
    document.addEventListener('click', this.handleClickOutside)

    // Allow other modules (e.g. settings) to close presets deterministically
    document.addEventListener('tb:close-presets', () => {
      if (this.state !== 'open') return
      this.closeDropdown({ reason: 'external', deferFocusToPill: true, suppressToggleMs: 250 })
    })
  }
  
  handlePillClick(e) {
    e.preventDefault()
    e.stopPropagation()

    if (performance.now() < this.ignorePillClickUntil) {
      return
    }
    // If we just closed due to pointer interaction, ignore stray click toggles.
    if (performance.now() < this.suppressToggleUntil) return
    
    // Don't toggle if clicking an option (let option handler deal with it)
    if (e.target.closest('.preset-option')) return
    
    this.toggleDropdown()
  }
  
  handleOptionClick(e) {
    const option = e.target.closest('.preset-option')
    if (!option) return
    
    // Stop propagation to prevent toggle back
    e.preventDefault()
    e.stopPropagation()
    
    const index = parseInt(option.dataset.index, 10)
    // Use effect-based selection (handles its own close)
    this.selectWithEffect(index)
  }
  
  handleKeyDown(e) {
    switch (e.key) {
      case 'Enter':
        e.preventDefault()
        if (this.state === 'closed') {
          this.openDropdown()
        } else {
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
  
  handleClickOutside(e) {
    if (this.state !== 'open') return
    const t = e.target && e.target.nodeType === 1 ? e.target : e.target?.parentElement
    if (!t) return
    if (this.presetPill?.contains(t)) return
    if (this.presetDropdown?.contains(t)) return
    this.closeDropdown({ reason: 'outside-click', deferFocusToPill: true, suppressToggleMs: 250 })
  }

  toggleDropdown() {
    if (this.state === 'open') {
      this.closeDropdown({ reason: 'pill-toggle', deferFocusToPill: false, suppressToggleMs: 0 })
    } else {
      this.openDropdown()
    }
  }
  
  openDropdown() {
    if (!this.presetPill || !this.presetDropdown || !this.app) return
    if (this.state === 'open') return

    // Mutual exclusivity: opening presets closes settings
    document.dispatchEvent(new CustomEvent('tb:close-settings'))
    
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
  
  closeDropdown({ reason = 'unknown', deferFocusToPill = false, suppressToggleMs = 0 } = {}) {
    if (!this.presetPill || !this.app) return
    if (this.state === 'closed') return

    this.state = 'closed'
    this.app.classList.remove('presets-open')
    this.presetPill.classList.remove('open')
    this.presetPill.setAttribute('aria-expanded', 'false')

    if (suppressToggleMs > 0) {
      this.suppressToggleUntil = performance.now() + suppressToggleMs
    }

    // Don't focus synchronously during pointer events; it can contribute to retargeting.
    const focusPill = () => this.presetPill?.focus({ preventScroll: true })
    if (deferFocusToPill) {
      if (typeof queueMicrotask === 'function') queueMicrotask(focusPill)
      else setTimeout(focusPill, 0)
    } else {
      focusPill()
    }

    // Clear gesture state
    this.activePointerId = null
    this.downOption = null
    this.downWasOnOption = false
    this.downStartedInPillToggleRegion = false
  }

  // =====================
  // Pointer / Capture model
  // =====================
  _isOptionEl(target) {
    const t = target && target.nodeType === 1 ? target : target?.parentElement
    if (!t || typeof t.closest !== 'function') return null
    return t.closest('.preset-option')
  }

  onDocPointerDownCapture(e) {
    if (this.state !== 'open') return

    const option = this._isOptionEl(e.target)
    if (option) {
      this.activePointerId = e.pointerId ?? 'mouse'
      this.downOption = option
      this.downWasOnOption = true
      return
    }

    const t = e.target && e.target.nodeType === 1 ? e.target : e.target?.parentElement
    const isOnPill = !!(t && this.presetPill && this.presetPill.contains(t))
    if (isOnPill) {
      // Let the pill's own pointerup toggle close/open. Closing here causes close→reopen races.
      return
    }

    const isOnDropdown = !!(t && this.presetDropdown && this.presetDropdown.contains(t))
    if (isOnDropdown) {
      // Clicked the overlay background (not an option) => close immediately.
      this.closeDropdown({ reason: 'overlay', deferFocusToPill: true, suppressToggleMs: 250 })
      return
    }

    // True outside click
    this.closeDropdown({ reason: 'outside', deferFocusToPill: true, suppressToggleMs: 250 })
  }

  onDocPointerUpCapture(e) {
    if (this.state !== 'open') {
      // If we already closed on pointerdown, swallow stale gesture state.
      this.activePointerId = null
      this.downOption = null
      this.downWasOnOption = false
      return
    }

    if (!this.downWasOnOption || !this.downOption) return
    const pointerId = e.pointerId ?? 'mouse'
    if (this.activePointerId != null && pointerId !== this.activePointerId) return

    const upOption = this._isOptionEl(e.target)
    if (upOption && upOption === this.downOption) {
      const index = parseInt(upOption.dataset.index, 10)
      if (!Number.isNaN(index)) {
        // Use effect-based selection
        this.selectWithEffect(index)
      }
      // Don't close here - selectWithEffect handles the delayed close
    } else {
      // Press-drag-release off the option => cancel.
      this.downOption = null
      this.downWasOnOption = false
      this.activePointerId = null
    }
  }

  onPillPointerDown(e) {
    // Pill contains dropdown; only treat as "toggle gesture" if we're not on dropdown/option.
    const onOption = !!this._isOptionEl(e.target)
    this.downStartedInPillToggleRegion = !onOption
  }

  onPillPointerUp(e) {
    if (!this.downStartedInPillToggleRegion) return
    if (performance.now() < this.suppressToggleUntil) return

    // When open, pointerdown capture already closes on overlay/outside; this is just the pill toggle.
    e.preventDefault()
    e.stopPropagation()
    this.ignorePillClickUntil = performance.now() + 400
    this.toggleDropdown()
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
      option.style.setProperty('--i', String(index))
      option.textContent = name
      
      if (index === this.currentPresetIndex) {
        option.classList.add('selected')
        option.setAttribute('aria-selected', 'true')
      }
      
      option.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
          e.preventDefault()
          // Use effect-based selection (handles its own close)
          this.selectWithEffect(index)
          return
        }

        if (e.key === 'ArrowDown') {
          e.preventDefault()
          this.focusNextOption(1)
          return
        }

        if (e.key === 'ArrowUp') {
          e.preventDefault()
          this.focusNextOption(-1)
        }
      })
      
      // Direct click handler as fallback for when pointer capture doesn't work
      option.addEventListener('click', (e) => {
        e.preventDefault()
        e.stopPropagation()
        // Only trigger if not already handled by pointer model
        if (!this.downWasOnOption) {
          this.selectWithEffect(index)
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

  /**
   * Select preset with visual "memory lock-in" effect
   * Adds selecting classes, waits for effect, then closes
   */
  selectWithEffect(index) {
    if (!this.app || !this.presetDropdown) {
      // Fallback to simple select + close
      this.selectPreset(index)
      this.closeDropdown({ reason: 'option-fallback', deferFocusToPill: true, suppressToggleMs: 250 })
      return
    }

    // Find the target option
    const targetOption = this.presetDropdown.querySelector(`[data-index="${index}"]`)
    if (!targetOption) {
      this.selectPreset(index)
      this.closeDropdown({ reason: 'option-no-target', deferFocusToPill: true, suppressToggleMs: 250 })
      return
    }

    // Add selecting classes for CSS effect
    this.app.classList.add('selecting')
    targetOption.classList.add('selecting-target')

    // Actually select the preset
    this.selectPreset(index)

    // Brief moment for the selection to register, then flow into close
    setTimeout(() => {
      // Clean up selecting classes
      this.app.classList.remove('selecting')
      targetOption.classList.remove('selecting-target')
      
      // Now close with normal spectral blur animation
      this.closeDropdown({ reason: 'option-effect', deferFocusToPill: true, suppressToggleMs: 250 })
    }, 120) // Quick acknowledgment, then seamless close
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
    const loadPresetFn = this.getNativeFn('loadPreset')
    
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

