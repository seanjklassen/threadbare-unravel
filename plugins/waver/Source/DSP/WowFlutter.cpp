#include "WowFlutter.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace threadbare::dsp
{

void WowFlutter::prepare(double sr, std::size_t /*maxBlockSize*/) noexcept
{
    sampleRate = std::max(1.0, sr);
    const int maxDelaySamples = static_cast<int>(sampleRate * 0.04) + 4;
    delaySize = maxDelaySamples;
    delayL.assign(static_cast<std::size_t>(delaySize), 0.0f);
    delayR.assign(static_cast<std::size_t>(delaySize), 0.0f);
    writePos = 0;

    wowInc = 1.0f / static_cast<float>(sampleRate);
    flutterInc = 25.0f / static_cast<float>(sampleRate);
    wowPhase = 0.0f;
    flutterPhase = 0.0f;
    noiseLpZ = 0.0f;

    transitionDelayTarget = 0.0f;
    transitionDelayCurrent = 0.0f;
    constexpr float smoothHz = 8.0f;
    transitionDelayCoeff = 1.0f - std::exp(-2.0f * std::numbers::pi_v<float> * smoothHz
                                           / static_cast<float>(sampleRate));

    constexpr float xoverCutoff = 100.0f;
    const float srF = static_cast<float>(sampleRate);
    const float w0 = 2.0f * std::numbers::pi_v<float> * xoverCutoff / srF;
    const float cosW0 = std::cos(w0);
    const float sinW0 = std::sin(w0);
    const float alpha = sinW0 / std::sqrt(2.0f); // Q = 1/sqrt(2) for Butterworth
    const float a0 = 1.0f + alpha;
    const float lpB0 = ((1.0f - cosW0) * 0.5f) / a0;
    const float lpB1 = (1.0f - cosW0) / a0;
    const float lpA1 = (-2.0f * cosW0) / a0;
    const float lpA2 = (1.0f - alpha) / a0;
    xoverL = { lpB0, lpB1, lpB0, lpA1, lpA2, 0.0f, 0.0f };
    xoverR = { lpB0, lpB1, lpB0, lpA1, lpA2, 0.0f, 0.0f };
}

void WowFlutter::reset() noexcept
{
    std::fill(delayL.begin(), delayL.end(), 0.0f);
    std::fill(delayR.begin(), delayR.end(), 0.0f);
    writePos = 0;
    wowPhase = 0.0f;
    flutterPhase = 0.0f;
    noiseLpZ = 0.0f;
    transitionDelayTarget = 0.0f;
    transitionDelayCurrent = 0.0f;
    xoverL.s1 = xoverL.s2 = 0.0f;
    xoverR.s1 = xoverR.s2 = 0.0f;
}

void WowFlutter::setWowDepth(float depth01) noexcept
{
    wowDepthMs = std::clamp(depth01, 0.0f, 1.0f) * 3.0f;
}

void WowFlutter::setFlutterDepth(float depth01) noexcept
{
    flutterDepthMs = std::clamp(depth01, 0.0f, 1.0f) * 0.15f;
}

void WowFlutter::setAge(float age) noexcept
{
    ageParam = std::clamp(age, 0.0f, 1.0f);
}

void WowFlutter::setTransitionDelay(float delayMs) noexcept
{
    transitionDelayTarget = std::clamp(delayMs, 0.0f, 30.0f);
}

void WowFlutter::process(float* left, float* right, int numSamples) noexcept
{
    if (delaySize < 4)
        return;

    constexpr float twoPi = 2.0f * std::numbers::pi_v<float>;
    const float srF = static_cast<float>(sampleRate);
    const float stochasticScale = 0.3f + ageParam * 0.7f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = left[i];
        const float inR = right[i];

        // Extract sub-bass via 2nd-order Butterworth LP (transposed-direct-form II).
        const float subL = xoverL.b0 * inL + xoverL.s1;
        xoverL.s1 = xoverL.b1 * inL - xoverL.a1 * subL + xoverL.s2;
        xoverL.s2 = xoverL.b2 * inL - xoverL.a2 * subL;

        const float subR = xoverR.b0 * inR + xoverR.s1;
        xoverR.s1 = xoverR.b1 * inR - xoverR.a1 * subR + xoverR.s2;
        xoverR.s2 = xoverR.b2 * inR - xoverR.a2 * subR;

        // Only the upper band enters the modulated delay.
        delayL[static_cast<std::size_t>(writePos)] = inL - subL;
        delayR[static_cast<std::size_t>(writePos)] = inR - subR;

        const float noise = nextNoise() * stochasticScale;

        const float wowMod = std::sin(twoPi * wowPhase) + noise * 0.3f;
        const float flutterMod = std::sin(twoPi * flutterPhase) + noise * 0.15f;

        transitionDelayCurrent += transitionDelayCoeff * (transitionDelayTarget - transitionDelayCurrent);

        const float baseDelayMs = wowDepthMs * 1.35f + flutterDepthMs * 1.25f + transitionDelayCurrent;
        const float totalDelayMs = baseDelayMs + wowDepthMs * wowMod + flutterDepthMs * flutterMod;
        const float delaySamples = std::max(1.0f, totalDelayMs * 0.001f * srF);

        const int intDelay = static_cast<int>(delaySamples);
        const float frac = delaySamples - static_cast<float>(intDelay);
        const int readPos = ((writePos - intDelay - 1) + delaySize * 4) % delaySize;

        // Recombine: clean sub-bass + modulated upper band.
        left[i] = subL + cubicHermite(delayL.data(), delaySize, frac, readPos);
        right[i] = subR + cubicHermite(delayR.data(), delaySize, frac, readPos);

        wowPhase += wowInc;
        if (wowPhase >= 1.0f)
            wowPhase -= 1.0f;
        flutterPhase += flutterInc;
        if (flutterPhase >= 1.0f)
            flutterPhase -= 1.0f;

        writePos = (writePos + 1) % delaySize;
    }
}

float WowFlutter::cubicHermite(const float* buf, int size, float frac, int index) const noexcept
{
    auto at = [&](int offset) -> float {
        return buf[static_cast<std::size_t>(((index + offset) % size + size) % size)];
    };

    const float y0 = at(-1);
    const float y1 = at(0);
    const float y2 = at(1);
    const float y3 = at(2);

    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

float WowFlutter::nextNoise() noexcept
{
    rngState = rngState * 1664525u + 1013904223u;
    const float raw = (static_cast<float>((rngState >> 8) & 0xFFFFu) / 65535.0f) * 2.0f - 1.0f;
    // One-pole LP at ~3Hz for slow noise.
    const float coeff = 1.0f - std::exp(-2.0f * std::numbers::pi_v<float> * 3.0f / static_cast<float>(sampleRate));
    noiseLpZ += coeff * (raw - noiseLpZ);
    return noiseLpZ;
}

} // namespace threadbare::dsp
