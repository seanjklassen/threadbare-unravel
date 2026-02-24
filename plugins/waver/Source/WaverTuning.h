#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace threadbare::tuning::waver
{
struct RateDependent
{
    double sampleRate = 44100.0;
    double sampleRateInverse = 1.0 / 44100.0;

    // Ornstein-Uhlenbeck (44.1 kHz baseline in spec).
    float ouAlpha = 0.9991f;
    float ouBeta = 0.042f;

    // Modulation/time-domain helpers.
    float wowMaxDelayMs = 3.5f;
    float flutterMaxDelayMs = 0.15f;
    float chorusDelayMinMs = 1.6f;
    float chorusDelayMaxMs = 8.5f;
    float organClickSeconds = 0.008f;

    // One-pole helper coefficients derived from SR.
    float dcBlockerR = 0.995f;
    float meterSmoothing = 0.94f;
};

inline constexpr float kDriftMinCents = 2.0f;
inline constexpr float kDriftMaxCents = 8.0f;
inline constexpr float kFilterDriftMin = 0.005f;
inline constexpr float kFilterDriftMax = 0.020f;
inline constexpr float kEnvelopeDriftMin = 0.01f;
inline constexpr float kEnvelopeDriftMax = 0.04f;

inline constexpr float kChorusSplitHz = 150.0f;
inline constexpr float kChorusBandwidthHz = 10000.0f;
inline constexpr float kTapeHeadLowpassHz = 13000.0f;
inline constexpr float kOverdriveMidHz = 1000.0f;
inline constexpr float kOverdriveMidQ = 0.8f;

inline constexpr float kArpRateMinHz = 0.5f;
inline constexpr float kArpRateMaxHz = 32.0f;
inline constexpr float kArpGateMin = 0.05f;
inline constexpr float kArpGateMax = 0.95f;
inline constexpr float kArpSwingMin = 0.0f;
inline constexpr float kArpSwingMax = 0.35f;
inline constexpr std::uint32_t kArpMaxHeldNotes = 16;

inline constexpr float kPuckXToRateExp = 1.5f;
inline constexpr float kPuckYToGateExp = 1.15f;

inline RateDependent recalculate(double sampleRate) noexcept
{
    RateDependent r;
    r.sampleRate = std::max(1.0, sampleRate);
    r.sampleRateInverse = 1.0 / r.sampleRate;

    // Scale OU dynamics relative to 44.1k baseline.
    const double srRatio = r.sampleRate / 44100.0;
    r.ouAlpha = static_cast<float>(std::pow(0.9991, srRatio));
    r.ouBeta = static_cast<float>(0.042 * std::sqrt(srRatio));

    const auto onePoleFromHz = [sr = r.sampleRate](double hz) {
        return static_cast<float>(std::exp((-2.0 * std::numbers::pi * hz) / sr));
    };

    r.dcBlockerR = onePoleFromHz(8.0);
    r.meterSmoothing = onePoleFromHz(12.0);
    return r;
}
} // namespace threadbare::tuning::waver
