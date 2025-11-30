# THREADBARE_CURSOR_RULES.md

version: 1.0.0\
framework: JUCE 8\
language: C++20

## 1. ARCHITECTURAL MANIFESTO

You are an expert Audio Software Engineer specializing in JUCE 8 and
C++20. Your goal is to generate "Threadbare" code: code that is stripped
of all non-essential runtime overhead, strictly adheres to hard
real-time constraints, and employs robust architectural patterns for
hybrid Web/Native plugins.

## 2. FILE ORGANIZATION & STRUCTURE

Enforce a strict separation of concerns to facilitate isolation testing
and modularity.

-   **Source/DSP:** Pure C++ logic. NO JUCE UI dependencies. MUST use
    std::span for buffer passing.\
-   **Source/UI:** WebBrowserComponent implementation, frontend
    bindings, and IPC logic.\
-   **Source/Processors:** The AudioProcessor integration layer. Manages
    resources and threads.\
-   **Tests:** Unit tests (Catch2/GoogleTest) and benchmarks.

## 3. REAL-TIME SAFETY (THE IRON LAWS)

These rules apply strictly to `processBlock`, `process`,
`getNextAudioBlock`, and all functions in the audio-thread call stack.

### 3.1 Memory Management (Zero Allocation)

**PROHIBITED:**\
`new`, `malloc`, `std::vector::resize`, `std::vector::push_back`,
`std::string` creation/concatenation, `std::function` creation.

**MANDATORY:**\
All resources (buffers, grain pools, delay lines) MUST be pre-allocated
in `prepareToPlay` or the constructor.

**PATTERN:**\
Use `.reserve()` on vectors during initialization.\
Use object pools (free lists) for polyphonic voice management (e.g.,
granular synthesis).

### 3.2 Thread Synchronization (Wait-Free)

**PROHIBITED:**\
`std::mutex`, `juce::CriticalSection`, `std::promise` in the audio
thread, or any blocking lock.

**MANDATORY:**\
Use `std::atomic<T>` for control parameters.\
Use `juce::AbstractFIFO` or `melatonin::SpscQueue` for lock-free
cross-thread messaging.

### 3.3 Numeric Stability

**MANDATORY:**\
Instantiate `juce::ScopedNoDenormals noDenormals;` as the first line of
`processBlock`.

**GUIDELINE:**\
Prefer `float` over `double` for audio paths unless a filter topology
demands double precision.

------------------------------------------------------------------------

## 4. JUCE 8 WEBVIEW INTEGRATION

### 4.1 Windows Backend Reliability (CRITICAL)

You MUST configure the WebBrowserComponent to avoid backend fallback and
sandbox failures on Windows.

**BACKEND:** Always use `Backend::webview2`.\
**USER DATA:** You MUST explicitly set a writeable User Data Folder.

``` cpp
auto options = juce::WebBrowserComponent::Options()
    .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
    .withWinWebView2Options(
        juce::WebBrowserComponent::Options::WinWebView2()
            .withUserDataFolder(
                juce::File::getSpecialLocation(
                    juce::File::SpecialLocationType::tempDirectory)));
```

**LINKING:**\
`JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (project-specific
requirement).

### 4.2 IPC & Event Strategy

**PROHIBITED:**\
Calling `emitEventIfBrowserIsVisible` from the audio thread.

**PATTERN --- Rate-Limited Polling:**\
- **Audio Thread:** Write analysis data to a lock-free FIFO or atomic.\
- **Message Thread:** A `juce::Timer` (30--60 Hz) or VBlankAttachment
drains the FIFO and calls `emitEventIfBrowserIsVisible`.

**BINDINGS:**\
Use `withNativeFunction` for JS → C++.\
The JS completion callback MUST ALWAYS be executed.

### 4.3 Development Workflow

Use dual-source loading:

``` cpp
#if JUCE_DEBUG
    loadURL("http://localhost:3000");
#else
    loadFromResourceProviderRoot(); // BinaryData bundle
#endif
```

------------------------------------------------------------------------

## 5. DSP & ALGORITHMIC PATTERNS

### 5.1 Modern C++20 Interfaces

**BUFFER VIEW:**\
Use `std::span<float>` for internal DSP process methods.\
Do NOT pass `juce::AudioBuffer<float>` into DSP engines.

``` cpp
std::span<float> channelData(buffer.getWritePointer(ch), buffer.getNumSamples());
engine.process(channelData);
```

**AUDIOBLOCK:**\
Use `juce::dsp::AudioBlock` ONLY for JUCE DSP module interactions.

### 5.2 Interpolation & Warping (Indie / Dream Pop Aesthetic)

**ALGORITHM:**\
Use `juce::CatmullRomInterpolator` for fractional delay and
pitch-warping.

**AVOID:**\
Linear interpolation for modulation-heavy or pitch-shifted audio.

**CONTEXT:**\
For tape-like warping, modulate read pointer speed using filtered noise
or LFOs.

### 5.3 Parameter Smoothing

**TOPOLOGY:**\
Use `juce::SmoothedValue` for gain/mix changes.\
Use one-pole exponential smoothing for musical parameters (delay time,
filters, pitch).

------------------------------------------------------------------------

## 6. TESTING & VERIFICATION

-   Use Catch2/GoogleTest for DSP unit tests.
-   Implement Impulse Response tests for reverbs/delays.
-   Assert numerical correctness with epsilon = `1e-5`.
-   Include golden-master comparisons for high-level algorithms.

------------------------------------------------------------------------

## 7. CODE STYLE

-   Prefer `std::unique_ptr` for ownership.
-   Avoid `std::shared_ptr` in DSP execution paths.
-   Mark non-mutating methods as `const` and `noexcept`.
-   Wrap all project code in a namespace (e.g., `threadbare::dsp`).
