# Unravel Tuning Cheatsheet

Reference for the designer-facing constants defined in `src/UnravelTuning.h`. Keep tweaks within these guardrails to preserve the “memory cloud” voicing.

## FDN
- `kNumLines = 8`: musical sweet spot for CPU vs depth.
- `kBaseDelaysMs`: {31, 37, 41, 53, 61, 71, 83, 97}; prime-ish to dodge metallic modes.
- `kSizeMin = 0.5`, `kSizeMax = 2.0`: tighter tanks vs widescreen washes.

## Decay
- `kT60Min = 0.4 s`, `kT60Max = 20 s`: short verbs to infinite-feel pads.
- `kPuckYMultiplier` clamps decay morph to ~÷3 … ×3.

## Damping
- `kLowCutoffHz = 1.5 kHz`, `kMidCutoffHz = 8 kHz`, `kHighCutoffHz = 16 kHz`: tone slider anchors.
- `kLoopHighPassHz = 100 Hz`: keeps LF mud out of the tank.

## Modulation
- `kMinRateHz = 0.05`, `kMaxRateHz = 0.4`: slow drift without chorus wobble.
- `kMaxDepthSamples = 8`: cap for tape-like smears; go lower for calmer tails.

## Ghost
- `kHistorySeconds = 0.75`: grain source window.
- `kGrainMinSec = 0.08`, `kGrainMaxSec = 0.20`: set smear vs jitter.
- `kDetuneSemi = 0.2`: ±20 cents wobble; `kShimmerSemi = 12` with `kShimmerProbability = 5%`.
- `kMinGainDb = -30`, `kMaxGainDb = -12`: couch ghost against FDN input.

## Freeze
- `kFrozenFeedback = 0.999`: almost infinite but stable.
- `kRampTimeSec = 0.05`: glide into/out of freeze without thumps.

## Ducking
- `kAttackSec = 0.01`, `kReleaseSec = 0.25`: pump timing.
- `kMinWetFactor = 0.15`: even full duck never fully dries out.

## PuckMapping
- `kDecayYFactor = 3.0`: ensures top of pad feels 3× longer.
- `kGhostYBonus = 0.3`, `kDriftYBonus = 0.25`: extra haze & motion at high Y.

## Safety
- `kAntiDenormal = 1e-18`: injects microscopic noise to keep FDN CPU-stable.


