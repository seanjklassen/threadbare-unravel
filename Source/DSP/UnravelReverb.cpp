#include "UnravelReverb.h"

#include <algorithm>
#include <cmath>

namespace threadbare::dsp
{

void UnravelReverb::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = static_cast<int>(spec.sampleRate);
    
    // Allocate 2 seconds of buffer space for each delay line
    const auto bufferSize = static_cast<std::size_t>(2.0 * sampleRate);
    
    for (std::size_t i = 0; i < kNumLines; ++i)
    {
        delayLines[i].resize(bufferSize);
        std::fill(delayLines[i].begin(), delayLines[i].end(), 0.0f);
        writeIndices[i] = 0;
    }
}

void UnravelReverb::reset() noexcept
{
    for (std::size_t i = 0; i < kNumLines; ++i)
    {
        std::fill(delayLines[i].begin(), delayLines[i].end(), 0.0f);
        writeIndices[i] = 0;
    }
}

void UnravelReverb::process(std::span<float> left, 
                            std::span<float> right, 
                            UnravelState& state) noexcept
{
    if (delayLines[0].empty() || left.size() != right.size())
        return;
    
    juce::ScopedNoDenormals noDenormals;
    
    const auto numSamples = left.size();
    const int bufferSize = static_cast<int>(delayLines[0].size());
    
    // Map puckY (-1.0 to 1.0) to feedback gain (0.0 to 0.85)
    const float puckY = juce::jlimit(-1.0f, 1.0f, state.puckY);
    const float feedback = juce::jmap(puckY, -1.0f, 1.0f, 0.0f, 0.85f);
    
    // Pre-calculate delay offsets in samples for each line
    std::array<int, kNumLines> delayOffsets;
    for (std::size_t i = 0; i < kNumLines; ++i)
    {
        const float delayMs = threadbare::tuning::Fdn::kBaseDelaysMs[i];
        delayOffsets[i] = static_cast<int>(delayMs * 0.001f * sampleRate);
    }
    
    // Temporary storage for FDN processing
    std::array<float, kNumLines> readOutputs;
    std::array<float, kNumLines> nextInputs;
    
    for (std::size_t sample = 0; sample < numSamples; ++sample)
    {
        const float inputL = left[sample];
        const float inputR = right[sample];
        const float monoInput = 0.5f * (inputL + inputR);
        
        // Step A: Read from all 8 delay lines
        for (std::size_t i = 0; i < kNumLines; ++i)
        {
            int readIndex = writeIndices[i] - delayOffsets[i];
            if (readIndex < 0)
                readIndex += bufferSize;
            
            readOutputs[i] = delayLines[i][static_cast<std::size_t>(readIndex)];
        }
        
        // Step B: The Matrix - Simple mixing approach
        // Calculate sum of all reads for cross-mixing
        float sumOfReads = 0.0f;
        for (std::size_t i = 0; i < kNumLines; ++i)
            sumOfReads += readOutputs[i];
        
        // Mix with input and apply feedback matrix
        // Using a simple Householder-style reflection: mix in inverted sum + self
        constexpr float mixCoeff = -0.2f;
        for (std::size_t i = 0; i < kNumLines; ++i)
        {
            const float crossMix = sumOfReads * mixCoeff + readOutputs[i];
            nextInputs[i] = monoInput + (crossMix * feedback);
        }
        
        // Step C: Write with safety clipping
        for (std::size_t i = 0; i < kNumLines; ++i)
        {
            delayLines[i][static_cast<std::size_t>(writeIndices[i])] = std::tanh(nextInputs[i]);
            
            // Advance write index with wrapping
            ++writeIndices[i];
            if (writeIndices[i] >= bufferSize)
                writeIndices[i] = 0;
        }
        
        // Step D: Output - odd lines to L, even lines to R
        float outputL = 0.0f;
        float outputR = 0.0f;
        
        for (std::size_t i = 0; i < kNumLines; ++i)
        {
            if (i % 2 == 0)
                outputL += readOutputs[i];
            else
                outputR += readOutputs[i];
        }
        
        // Scale output and mix with dry signal
        constexpr float outputScale = 0.5f;
        left[sample] = inputL + (outputL * outputScale);
        right[sample] = inputR + (outputR * outputScale);
    }
    
    // Simple metering
    state.inLevel = 0.0f;
    state.tailLevel = 0.0f;
}

} // namespace threadbare::dsp
