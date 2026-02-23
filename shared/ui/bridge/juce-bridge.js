// =============================================================================
// THREADBARE JUCE BRIDGE
//
// Shared JUCE 8 native-function integration for plugin frontends.
// =============================================================================

class PromiseHandler {
  constructor() {
    this.lastPromiseId = 0
    this.promises = new Map()

    if (window.__JUCE__?.backend?.addEventListener) {
      window.__JUCE__.backend.addEventListener('__juce__complete', ({ promiseId, result }) => {
        if (this.promises.has(promiseId)) {
          this.promises.get(promiseId).resolve(result)
          this.promises.delete(promiseId)
        }
      })
    }
  }

  createPromise() {
    const promiseId = this.lastPromiseId++
    const result = new Promise((resolve, reject) => {
      this.promises.set(promiseId, { resolve, reject })
    })
    return [promiseId, result]
  }
}

export const createNativeFunctionBridge = () => {
  let promiseHandler = null
  if (window.__JUCE__?.backend) {
    promiseHandler = new PromiseHandler()
  }

  const getNativeFunction = (name) => {
    const registeredFunctions = window.__JUCE__?.initialisationData?.__juce__functions || []

    if (!registeredFunctions.includes(name)) {
      return null
    }

    if (!promiseHandler || !window.__JUCE__?.backend?.emitEvent) {
      return null
    }

    return function() {
      const [promiseId, result] = promiseHandler.createPromise()

      window.__JUCE__.backend.emitEvent('__juce__invoke', {
        name: name,
        params: Array.prototype.slice.call(arguments),
        resultId: promiseId,
      })

      return result
    }
  }

  // Preserve compatibility with existing shell/frontend call sites.
  window.__getNativeFunction = getNativeFunction
  return getNativeFunction
}

export const createParamSender = (getNativeFn) => {
  let nativeSetParameter = null

  return (id, val) => {
    if (!nativeSetParameter && typeof getNativeFn === 'function') {
      nativeSetParameter = getNativeFn('setParameter')
    }

    if (typeof nativeSetParameter === 'function') {
      nativeSetParameter(id, val)
    }
  }
}

