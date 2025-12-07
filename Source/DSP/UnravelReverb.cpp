#include "UnravelReverb.h"

#include <algorithm>
#include <juce_dsp/juce_dsp.h>

namespace threadbare::dsp
{

void UnravelReverb::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = static_cast<int>(spec.sampleRate);
    
    // Allocate 2 seconds of buffer space
    const auto bufferSize = static_cast<std::size_t>(2.0 * sampleRate);
    
    delayBufferL.resize(bufferSize);
    delayBufferR.resize(bufferSize);
    
    // Zero the buffers
    std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
    std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
    
    writeIndex = 0;
}

void UnravelReverb::reset() noexcept
{
    std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
    std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
    writeIndex = 0;
}

void UnravelReverb::process(std::span<float> left, 
                            std::span<float> right, 
                            UnravelState& state) noexcept
{
    if (delayBufferL.empty() || left.size() != right.size())
        return;
    
    juce::ScopedNoDenormals noDenormals;
    
    const auto numSamples = left.size();
    const int bufferSize = static_cast<int>(delayBufferL.size());
    
    // Fixed delay time: 500ms = 24000 samples at 48kHz
    constexpr int delayTimeSamples = 24000;
    
    // Map puckY (-1.0 to 1.0) to feedback gain (0.0 to 0.9)
    const float puckY = juce::jlimit(-1.0f, 1.0f, state.puckY);
    const float feedback = juce::jmap(puckY, -1.0f, 1.0f, 0.0f, 0.9f);
    
    for (std::size_t i = 0; i < numSamples; ++i)
    {
        const float inputL = left[i];
        const float inputR = right[i];
        
        // Calculate read index with wrapping
        int readIndex = writeIndex - delayTimeSamples;
        if (readIndex < 0)
            readIndex += bufferSize;
        
        // Read delayed samples
        const float delayedL = delayBufferL[static_cast<std::size_t>(readIndex)];
        const float delayedR = delayBufferR[static_cast<std::size_t>(readIndex)];
        
        // Feedback loop: mix input with delayed signal
        float newL = inputL + (delayedL * feedback);
        float newR = inputR + (delayedR * feedback);
        
        // Soft clipping to prevent runaway feedback (The Iron Law: No Explosions)
        newL = std::tanh(newL);
        newR = std::tanh(newR);
        
        // Write to buffer
        delayBufferL[static_cast<std::size_t>(writeIndex)] = newL;
        delayBufferR[static_cast<std::size_t>(writeIndex)] = newR;
        
        // Output: input + (delayed * 0.5)
        left[i] = inputL + (delayedL * 0.5f);
        right[i] = inputR + (delayedR * 0.5f);
        
        // Advance write index with wrapping
        ++writeIndex;
        if (writeIndex >= bufferSize)
            writeIndex = 0;
    }
    
    // Simple metering (optional, keeps state updated)
    state.inLevel = 0.0f;
    state.tailLevel = 0.0f;
}

} // namespace threadbare::dsp
