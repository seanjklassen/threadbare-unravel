#include "BbdChorus.h"

#include <numbers>

namespace threadbare::dsp
{
void BbdChorus::prepare(double newSampleRate, std::size_t maxBlockSize)
{
    sampleRate = std::max(1.0, newSampleRate);
    const auto maxDelaySamples = static_cast<std::size_t>(sampleRate * 0.02); // 20 ms headroom
    delayL.assign(maxDelaySamples + maxBlockSize + 8, 0.0f);
    delayR.assign(maxDelaySamples + maxBlockSize + 8, 0.0f);

    const float w0 = 2.0f * std::numbers::pi_v<float> * 150.0f / static_cast<float>(sampleRate);
    const float cosW0 = std::cos(w0);
    const float sinW0 = std::sin(w0);
    const float alpha = sinW0 / (2.0f * std::sqrt(2.0f));
    const float a0 = 1.0f + alpha;
    subLpB0 = ((1.0f - cosW0) * 0.5f) / a0;
    subLpB1 = (1.0f - cosW0) / a0;
    subLpB2 = subLpB0;
    subLpA1 = (-2.0f * cosW0) / a0;
    subLpA2 = (1.0f - alpha) / a0;

    reset();
}

void BbdChorus::reset() noexcept
{
    std::fill(delayL.begin(), delayL.end(), 0.0f);
    std::fill(delayR.begin(), delayR.end(), 0.0f);
    writeIndex = 0;
    phase1 = 0.0f;
    phase2 = 0.5f;
    noiseState = 0.0f;
    noiseRng = 0x6D2B79F5u;
    subLpS1 = 0.0f;
    subLpS2 = 0.0f;
}

void BbdChorus::process(float* left, float* right, int numSamples) noexcept
{
    if (mode == Mode::off || delayL.empty() || delayR.empty())
        return;

    const float mode1Rate = 0.50f;
    const float mode2Rate = 0.88f;
    const float depthSamples = static_cast<float>(sampleRate * 0.0018); // ~1.8 ms
    const float baseDelaySamples = static_cast<float>(sampleRate * 0.0032); // ~3.2 ms

    for (int i = 0; i < numSamples; ++i)
    {
        float inL = left[i];
        float inR = right[i];
        const float mono = 0.5f * (inL + inR);
        const float subBass = subLpB0 * mono + subLpS1;
        subLpS1 = subLpB1 * mono - subLpA1 * subBass + subLpS2;
        subLpS2 = subLpB2 * mono - subLpA2 * subBass;
        const float upperBand = mono - subBass;

        noiseRng = noiseRng * 1664525u + 1013904223u;
        const float white = (static_cast<float>((noiseRng >> 8) & 0x00FFFFFFu) / static_cast<float>(0x00FFFFFFu)) * 2.0f - 1.0f;
        noiseState = noiseState * 0.9975f + 0.0025f * white;
        const float lfo1 = std::sin(2.0f * std::numbers::pi_v<float> * phase1);
        const float lfo2 = std::sin(2.0f * std::numbers::pi_v<float> * phase2);

        phase1 += mode1Rate / static_cast<float>(sampleRate);
        phase2 += mode2Rate / static_cast<float>(sampleRate);
        if (phase1 >= 1.0f) phase1 -= 1.0f;
        if (phase2 >= 1.0f) phase2 -= 1.0f;

        delayL[writeIndex] = upperBand;
        delayR[writeIndex] = upperBand;

        float depthA = depthSamples * (0.5f + 0.5f * lfo1);
        float depthB = depthSamples * (0.5f + 0.5f * lfo2);
        if (mode == Mode::modeII)
            depthA = depthB;

        const float dL = baseDelaySamples + depthA + noiseState * 0.25f;
        const float dR = baseDelaySamples + ((mode == Mode::modeIPlusII) ? depthB : -depthA) - noiseState * 0.25f;

        float wetL = readDelaySample(delayL, dL, writeIndex);
        float wetR = readDelaySample(delayR, dR, writeIndex);

        const float wetGain = 0.65f * stereoWidth;
        const float dryGain = 1.0f - wetGain;
        const float midWet = 0.5f * (wetL + wetR);
        const float outWetL = midWet + stereoWidth * (wetL - midWet);
        const float outWetR = midWet + stereoWidth * (wetR - midWet);
        left[i] = subBass + wetGain * outWetL + dryGain * (inL - subBass);
        right[i] = subBass + wetGain * outWetR + dryGain * (inR - subBass);

        writeIndex = (writeIndex + 1) % delayL.size();
    }
}

float BbdChorus::readDelaySample(const std::vector<float>& line, float delaySamples, std::size_t writeHead) const noexcept
{
    const float readPos = static_cast<float>(writeHead) - delaySamples;
    const float wrapped = readPos < 0.0f ? readPos + static_cast<float>(line.size()) : readPos;

    const auto i1 = static_cast<std::size_t>(wrapped) % line.size();
    const auto i0 = (i1 + line.size() - 1) % line.size();
    const auto i2 = (i1 + 1) % line.size();
    const auto i3 = (i1 + 2) % line.size();
    const float frac = wrapped - static_cast<float>(static_cast<std::size_t>(wrapped));

    // Cubic Hermite interpolation.
    const float y0 = line[i0];
    const float y1 = line[i1];
    const float y2 = line[i2];
    const float y3 = line[i3];
    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}
} // namespace threadbare::dsp
