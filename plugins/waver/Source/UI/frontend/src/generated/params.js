// =============================================================================
// AUTO-GENERATED FILE - DO NOT EDIT MANUALLY
// Generated from: params.json
// Generated at: 2026-02-25T00:57:45.920Z
// =============================================================================

/**
 * Parameter definitions for the frontend.
 * Import this module to get parameter metadata (min, max, default, etc.)
 */

export const PARAMS = {
  puckX: {
    id: 'puckX',
    name: 'presence',
    type: 'float',
    min: -1,
    max: 1,
    default: 0
  },
  puckY: {
    id: 'puckY',
    name: 'age',
    type: 'float',
    min: -1,
    max: 1,
    default: 0
  },
  blend: {
    id: 'blend',
    name: 'blend',
    type: 'float',
    min: 0.15,
    max: 0.6,
    default: 0.35
  },
  momentSeed: {
    id: 'momentSeed',
    name: 'moment',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.5
  },
  momentTrigger: {
    id: 'momentTrigger',
    name: 'moment trigger',
    type: 'bool',
    default: false
  },
  arpEnabled: {
    id: 'arpEnabled',
    name: 'arp',
    type: 'bool',
    default: false
  },
  outputGain: {
    id: 'outputGain',
    name: 'output',
    type: 'float',
    min: -24,
    max: 12,
    default: 0,
    unit: 'dB'
  },
  qualityMode: {
    id: 'qualityMode',
    name: 'quality',
    type: 'choice',
    options: ['Lite', 'Standard', 'HQ'],
    default: 1
  },
  filterCutoff: {
    id: 'filterCutoff',
    name: 'cutoff',
    type: 'float',
    min: 20,
    max: 20000,
    default: 8000,
    skewCentre: 1000,
    unit: 'Hz'
  },
  filterRes: {
    id: 'filterRes',
    name: 'resonance',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.15
  },
  filterMode: {
    id: 'filterMode',
    name: 'filter',
    type: 'choice',
    options: ['OTA', 'Ladder'],
    default: 0
  },
  filterKeyTrack: {
    id: 'filterKeyTrack',
    name: 'tracking',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.5
  },
  envToFilter: {
    id: 'envToFilter',
    name: 'envelope',
    type: 'float',
    min: -1,
    max: 1,
    default: 0.3
  },
  macroShape: {
    id: 'macroShape',
    name: 'shape',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.5
  },
  dcoSubLevel: {
    id: 'dcoSubLevel',
    name: 'sub',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.2
  },
  dcoSubOctave: {
    id: 'dcoSubOctave',
    name: 'sub octave',
    type: 'choice',
    options: ['-1', '-2'],
    default: 0
  },
  noiseLevel: {
    id: 'noiseLevel',
    name: 'noise',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.1
  },
  noiseColor: {
    id: 'noiseColor',
    name: 'color',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.35
  },
  toyIndex: {
    id: 'toyIndex',
    name: 'fm depth',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.25
  },
  toyRatio: {
    id: 'toyRatio',
    name: 'fm ratio',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.5
  },
  layerDco: {
    id: 'layerDco',
    name: 'analog',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.7
  },
  layerToy: {
    id: 'layerToy',
    name: 'toy',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.2
  },
  layerOrgan: {
    id: 'layerOrgan',
    name: 'organ',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.3
  },
  organ16: {
    id: 'organ16',
    name: '16\'',
    type: 'float',
    min: 0,
    max: 8,
    default: 5
  },
  organ8: {
    id: 'organ8',
    name: '8\'',
    type: 'float',
    min: 0,
    max: 8,
    default: 4
  },
  organ4: {
    id: 'organ4',
    name: '4\'',
    type: 'float',
    min: 0,
    max: 8,
    default: 2
  },
  organMix: {
    id: 'organMix',
    name: 'mixture',
    type: 'float',
    min: 0,
    max: 8,
    default: 3
  },
  lfoRate: {
    id: 'lfoRate',
    name: 'rate',
    type: 'float',
    min: 0.1,
    max: 30,
    default: 3,
    unit: 'Hz'
  },
  lfoShape: {
    id: 'lfoShape',
    name: 'shape',
    type: 'choice',
    options: ['Tri', 'Sin', 'Sq', 'S&H'],
    default: 0
  },
  lfoToVibrato: {
    id: 'lfoToVibrato',
    name: 'vibrato',
    type: 'float',
    min: 0,
    max: 50,
    default: 0,
    unit: 'cents'
  },
  lfoToPwm: {
    id: 'lfoToPwm',
    name: 'pwm',
    type: 'float',
    min: 0,
    max: 1,
    default: 0
  },
  chorusMode: {
    id: 'chorusMode',
    name: 'chorus',
    type: 'choice',
    options: ['Off', 'I', 'II', 'I+II'],
    default: 1
  },
  driftAmount: {
    id: 'driftAmount',
    name: 'drift',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.3
  },
  stereoWidth: {
    id: 'stereoWidth',
    name: 'width',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.8
  },
  portaTime: {
    id: 'portaTime',
    name: 'glide',
    type: 'float',
    min: 0,
    max: 2000,
    default: 0,
    unit: 'ms'
  },
  portaMode: {
    id: 'portaMode',
    name: 'glide mode',
    type: 'choice',
    options: ['Legato', 'Always'],
    default: 0
  },
  envAttack: {
    id: 'envAttack',
    name: 'attack',
    type: 'float',
    min: 0.001,
    max: 5,
    default: 0.01,
    skewCentre: 0.05,
    unit: 's'
  },
  envDecay: {
    id: 'envDecay',
    name: 'decay',
    type: 'float',
    min: 0.001,
    max: 10,
    default: 0.2,
    skewCentre: 0.08,
    unit: 's'
  },
  envSustain: {
    id: 'envSustain',
    name: 'sustain',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.7
  },
  envRelease: {
    id: 'envRelease',
    name: 'release',
    type: 'float',
    min: 0.001,
    max: 15,
    default: 0.4,
    skewCentre: 0.2,
    unit: 's'
  },
  driveGain: {
    id: 'driveGain',
    name: 'grit',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.2
  },
  tapeSat: {
    id: 'tapeSat',
    name: 'saturation',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.3
  },
  wowDepth: {
    id: 'wowDepth',
    name: 'wow',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.15
  },
  flutterDepth: {
    id: 'flutterDepth',
    name: 'flutter',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.1
  },
  hissLevel: {
    id: 'hissLevel',
    name: 'hiss',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.1
  },
  humFreq: {
    id: 'humFreq',
    name: 'mains',
    type: 'choice',
    options: ['50', '60'],
    default: 1
  },
  printMix: {
    id: 'printMix',
    name: 'print mix',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.75
  }
};

/** Parameter IDs in definition order */
export const PARAM_IDS = [
  'puckX',
  'puckY',
  'blend',
  'momentSeed',
  'momentTrigger',
  'arpEnabled',
  'outputGain',
  'qualityMode',
  'filterCutoff',
  'filterRes',
  'filterMode',
  'filterKeyTrack',
  'envToFilter',
  'macroShape',
  'dcoSubLevel',
  'dcoSubOctave',
  'noiseLevel',
  'noiseColor',
  'toyIndex',
  'toyRatio',
  'layerDco',
  'layerToy',
  'layerOrgan',
  'organ16',
  'organ8',
  'organ4',
  'organMix',
  'lfoRate',
  'lfoShape',
  'lfoToVibrato',
  'lfoToPwm',
  'chorusMode',
  'driftAmount',
  'stereoWidth',
  'portaTime',
  'portaMode',
  'envAttack',
  'envDecay',
  'envSustain',
  'envRelease',
  'driveGain',
  'tapeSat',
  'wowDepth',
  'flutterDepth',
  'hissLevel',
  'humFreq',
  'printMix',
];

/**
 * Get parameter metadata by ID
 * @param {string} id - Parameter ID
 * @returns {object|undefined} Parameter metadata or undefined if not found
 */
export const getParam = (id) => PARAMS[id];

export default PARAMS;
