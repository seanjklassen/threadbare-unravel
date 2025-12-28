# Unravel Tuning Cheatsheet

Reference for the designer-facing constants defined in `Source/UnravelTuning.h`. Keep tweaks within these guardrails to preserve the "memory cloud" voicing.

## FDN
- `kNumLines = 8`: musical sweet spot for CPU vs depth.
- `kBaseDelaysMs`: {31, 37, 41, 53, 61, 71, 83, 97}; prime-ish to dodge metallic modes.
- `kSizeMin = 0.5`, `kSizeMax = 2.0`: tighter tanks vs widescreen washes.
- `kAvgDelayMs = 59.125`: average delay time for simplified feedback calculation.

## Decay
- `kT60Min = 0.4s`, `kT60Max = 50s`: short verbs to near-infinite pads.
- `kPuckYMultiplier` clamps decay morph to ~÷3 … ×3.

## Damping
- `kLowCutoffHz = 400 Hz`: aggressive darkening (underwater effect).
- `kMidCutoffHz = 8 kHz`, `kHighCutoffHz = 16 kHz`: tone slider anchors.
- `kLoopHighPassHz = 100 Hz`: keeps LF mud out of the tank.

## Modulation
- `kMinRateHz = 0.1`, `kMaxRateHz = 3.0`: slow drift to obvious modulation.
- `kMaxDepthSamples = 100`: extreme tape warble at 48kHz; creates heavy detune.

## Ghost
- `kHistorySeconds = 1.2`: extended memory buffer (750ms lookback + safety margin).
- `kGrainMinSec = 0.05`, `kGrainMaxSec = 0.30`: dense to smooth textures.
- `kDetuneSemi = 0.2`: ±20 cents wobble.
- `kShimmerSemi = 12` with `kShimmerProbability = 25%`: obvious sparkle.
- `kMinGainDb = -24`, `kMaxGainDb = -6`: ghost sits against FDN input.
- `kReverseProbability = 0.25`: 25% reverse grains at ghost=1.0.
- `kReverseGainReduction = 0.75`: -2.5dB for reverse grains.
- `kMinLookbackMs = 150`, `kMaxLookbackMs = 750`: puck X memory proximity.
- `kMinPanWidth = 0.3`, `kMaxPanWidth = 0.85`: stereo width scaling.
- `kFreezeShimmerProbability = 0.40`: increased shimmer when frozen.

## Freeze
- `kLoopBufferSeconds = 5.0`: multi-head loop buffer duration.
- `kNumReadHeads = 6`: smooth blend without excessive CPU.
- `kTransitionSeconds = 0.3`: glide into/out of freeze.
- `kHeadDetuneCents = 6.0`: subtle per-head pitch variation.
- `kHeadModRateMin = 0.03`, `kHeadModRateMax = 0.12`: slow drift rates.
- `kLoopWarmingCoef = 0.15`: ≈1.5kHz cutoff for warmth.
- `kFrozenFeedback = 1.0`: true infinite sustain.
- `kRampTimeSec = 0.05`: fast ramp for freeze transitions.

## Ducking
- `kAttackSec = 0.01`, `kReleaseSec = 0.25`: pump timing.
- `kMinWetFactor = 0.15`: even full duck never fully dries out.

## PuckMapping
- `kDecayYFactor = 3.0`: ensures top of pad feels 3× longer.
- `kGhostYBonus = 0.3`, `kDriftYBonus = 0.25`: extra haze & motion at high Y.

## Metering
- `kAttackSec = 0.01`, `kReleaseSec = 0.10`: envelope follower for orb visualization.

## Safety
- `kAntiDenormal = 1e-18`: injects microscopic noise to keep FDN CPU-stable.
