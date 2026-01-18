// =============================================================================
// AUTO-GENERATED FILE - DO NOT EDIT MANUALLY
// Generated from: params.json
// Generated at: 2026-01-12T13:59:00.367Z
// =============================================================================

/**
 * Parameter definitions for the frontend.
 * Import this module to get parameter metadata (min, max, default, etc.)
 */

export const PARAMS = {
  puckX: {
    id: 'puckX',
    name: 'Puck X',
    type: 'float',
    min: -1,
    max: 1,
    default: 0
  },
  puckY: {
    id: 'puckY',
    name: 'Puck Y',
    type: 'float',
    min: -1,
    max: 1,
    default: 0
  },
  mix: {
    id: 'mix',
    name: 'Mix',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.5
  },
  size: {
    id: 'size',
    name: 'Size',
    type: 'float',
    min: 0.5,
    max: 2,
    default: 1
  },
  decay: {
    id: 'decay',
    name: 'Decay',
    type: 'float',
    min: 0.4,
    max: 50,
    default: 5,
    skewCentre: 2,
    unit: 's'
  },
  tone: {
    id: 'tone',
    name: 'Brightness',
    type: 'float',
    min: -1,
    max: 1,
    default: 0
  },
  drift: {
    id: 'drift',
    name: 'Drift',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.2
  },
  ghost: {
    id: 'ghost',
    name: 'Ghost',
    type: 'float',
    min: 0,
    max: 1,
    default: 0.15
  },
  duck: {
    id: 'duck',
    name: 'Duck',
    type: 'float',
    min: 0,
    max: 1,
    default: 0
  },
  erPreDelay: {
    id: 'erPreDelay',
    name: 'Distance',
    type: 'float',
    min: 0,
    max: 100,
    default: 0,
    unit: 'ms'
  },
  freeze: {
    id: 'freeze',
    name: 'Freeze',
    type: 'bool',
    default: false
  },
  output: {
    id: 'output',
    name: 'Output',
    type: 'float',
    min: -24,
    max: 12,
    default: 0,
    unit: 'dB'
  }
};

/** Parameter IDs in definition order */
export const PARAM_IDS = [
  'puckX',
  'puckY',
  'mix',
  'size',
  'decay',
  'tone',
  'drift',
  'ghost',
  'duck',
  'erPreDelay',
  'freeze',
  'output',
];

/**
 * Get parameter metadata by ID
 * @param {string} id - Parameter ID
 * @returns {object|undefined} Parameter metadata or undefined if not found
 */
export const getParam = (id) => PARAMS[id];

export default PARAMS;
