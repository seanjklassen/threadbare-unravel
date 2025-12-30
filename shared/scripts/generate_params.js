#!/usr/bin/env node
/**
 * generate_params.js
 * 
 * Generates C++ and JS parameter definitions from a JSON schema.
 * This is the canonical source of truth for plugin parameters.
 * 
 * Usage:
 *   node generate_params.js <input.json> <output.h> <output.js>
 * 
 * Example:
 *   node generate_params.js \
 *     plugins/unravel/config/params.json \
 *     plugins/unravel/Source/UnravelGeneratedParams.h \
 *     plugins/unravel/Source/UI/frontend/src/generated/params.js
 */

const fs = require('fs');
const path = require('path');

// =============================================================================
// Argument Parsing
// =============================================================================
const args = process.argv.slice(2);

if (args.length < 3) {
    console.error('Usage: node generate_params.js <input.json> <output.h> <output.js>');
    process.exit(1);
}

const [inputPath, outputCppPath, outputJsPath] = args;

// =============================================================================
// Read and Parse JSON
// =============================================================================
let schema;
try {
    const jsonContent = fs.readFileSync(inputPath, 'utf8');
    schema = JSON.parse(jsonContent);
} catch (err) {
    console.error(`Error reading ${inputPath}: ${err.message}`);
    process.exit(1);
}

const { plugin, parameters } = schema;

if (!parameters || !Array.isArray(parameters)) {
    console.error('Invalid schema: missing "parameters" array');
    process.exit(1);
}

// =============================================================================
// Generate C++ Header
// =============================================================================
function generateCppHeader(plugin, params) {
    const className = `${capitalize(plugin)}GeneratedParams`;
    
    const lines = [
        '// =============================================================================',
        '// AUTO-GENERATED FILE - DO NOT EDIT MANUALLY',
        `// Generated from: ${path.basename(inputPath)}`,
        `// Generated at: ${new Date().toISOString()}`,
        '// =============================================================================',
        '#pragma once',
        '',
        '#include <juce_audio_processors/juce_audio_processors.h>',
        '#include <memory>',
        '#include <vector>',
        '',
        `namespace threadbare::${plugin}`,
        '{',
        '',
        `struct ${className}`,
        '{',
        '    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()',
        '    {',
        '        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;',
        '',
    ];
    
    for (const param of params) {
        if (param.type === 'bool') {
            lines.push(`        params.push_back(std::make_unique<juce::AudioParameterBool>("${param.id}", "${param.name}", ${param.default ? 'true' : 'false'}));`);
        } else if (param.type === 'float') {
            if (param.skewCentre !== undefined) {
                // Parameter with skew
                lines.push(`        {`);
                lines.push(`            auto range = juce::NormalisableRange<float>(${formatFloat(param.min)}f, ${formatFloat(param.max)}f);`);
                lines.push(`            range.setSkewForCentre(${formatFloat(param.skewCentre)}f);`);
                lines.push(`            params.push_back(std::make_unique<juce::AudioParameterFloat>("${param.id}", "${param.name}", range, ${formatFloat(param.default)}f));`);
                lines.push(`        }`);
            } else {
                // Simple parameter
                lines.push(`        params.push_back(std::make_unique<juce::AudioParameterFloat>("${param.id}", "${param.name}", ${formatFloat(param.min)}f, ${formatFloat(param.max)}f, ${formatFloat(param.default)}f));`);
            }
        }
        lines.push('');
    }
    
    lines.push('        return { params.begin(), params.end() };');
    lines.push('    }');
    lines.push('');
    
    // Generate parameter ID constants
    lines.push('    // Parameter ID constants');
    lines.push('    struct IDs');
    lines.push('    {');
    for (const param of params) {
        const constName = camelToScreamingSnake(param.id);
        lines.push(`        static constexpr const char* ${constName} = "${param.id}";`);
    }
    lines.push('    };');
    lines.push('');
    
    // Generate parameter metadata struct
    // Uses 'k' prefix to avoid conflicts with system macros (e.g., SIZE_MAX)
    lines.push('    // Parameter metadata (k-prefixed to avoid macro conflicts)');
    lines.push('    struct Meta');
    lines.push('    {');
    for (const param of params) {
        if (param.type === 'float') {
            const constPrefix = camelToScreamingSnakeWithPrefix(param.id);
            lines.push(`        static constexpr float ${constPrefix}_MIN = ${formatFloat(param.min)}f;`);
            lines.push(`        static constexpr float ${constPrefix}_MAX = ${formatFloat(param.max)}f;`);
            lines.push(`        static constexpr float ${constPrefix}_DEFAULT = ${formatFloat(param.default)}f;`);
            if (param.skewCentre !== undefined) {
                lines.push(`        static constexpr float ${constPrefix}_SKEW_CENTRE = ${formatFloat(param.skewCentre)}f;`);
            }
        } else if (param.type === 'bool') {
            const constPrefix = camelToScreamingSnakeWithPrefix(param.id);
            lines.push(`        static constexpr bool ${constPrefix}_DEFAULT = ${param.default ? 'true' : 'false'};`);
        }
    }
    lines.push('    };');
    lines.push('};');
    lines.push('');
    lines.push(`} // namespace threadbare::${plugin}`);
    lines.push('');
    
    return lines.join('\n');
}

// =============================================================================
// Generate JS Module
// =============================================================================
function generateJsModule(plugin, params) {
    const lines = [
        '// =============================================================================',
        '// AUTO-GENERATED FILE - DO NOT EDIT MANUALLY',
        `// Generated from: ${path.basename(inputPath)}`,
        `// Generated at: ${new Date().toISOString()}`,
        '// =============================================================================',
        '',
        '/**',
        ' * Parameter definitions for the frontend.',
        ' * Import this module to get parameter metadata (min, max, default, etc.)',
        ' */',
        '',
        'export const PARAMS = {',
    ];
    
    for (let i = 0; i < params.length; i++) {
        const param = params[i];
        const isLast = i === params.length - 1;
        const comma = isLast ? '' : ',';
        
        if (param.type === 'bool') {
            lines.push(`  ${param.id}: {`);
            lines.push(`    id: '${param.id}',`);
            lines.push(`    name: '${param.name}',`);
            lines.push(`    type: 'bool',`);
            lines.push(`    default: ${param.default}`);
            lines.push(`  }${comma}`);
        } else if (param.type === 'float') {
            lines.push(`  ${param.id}: {`);
            lines.push(`    id: '${param.id}',`);
            lines.push(`    name: '${param.name}',`);
            lines.push(`    type: 'float',`);
            lines.push(`    min: ${param.min},`);
            lines.push(`    max: ${param.max},`);
            lines.push(`    default: ${param.default}${param.skewCentre !== undefined ? ',' : (param.unit ? ',' : '')}`);
            if (param.skewCentre !== undefined) {
                lines.push(`    skewCentre: ${param.skewCentre}${param.unit ? ',' : ''}`);
            }
            if (param.unit) {
                lines.push(`    unit: '${param.unit}'`);
            }
            lines.push(`  }${comma}`);
        }
    }
    
    lines.push('};');
    lines.push('');
    
    // Export array of parameter IDs in order
    lines.push('/** Parameter IDs in definition order */');
    lines.push('export const PARAM_IDS = [');
    for (const param of params) {
        lines.push(`  '${param.id}',`);
    }
    lines.push('];');
    lines.push('');
    
    // Export helper function to get metadata
    lines.push('/**');
    lines.push(' * Get parameter metadata by ID');
    lines.push(' * @param {string} id - Parameter ID');
    lines.push(' * @returns {object|undefined} Parameter metadata or undefined if not found');
    lines.push(' */');
    lines.push('export const getParam = (id) => PARAMS[id];');
    lines.push('');
    
    // Export default
    lines.push('export default PARAMS;');
    lines.push('');
    
    return lines.join('\n');
}

// =============================================================================
// Utility Functions
// =============================================================================
function capitalize(str) {
    return str.charAt(0).toUpperCase() + str.slice(1);
}

function camelToScreamingSnake(str) {
    return str
        .replace(/([a-z])([A-Z])/g, '$1_$2')
        .toUpperCase();
}

// Safe version with 'k' prefix to avoid conflicts with system macros (e.g., SIZE_MAX)
function camelToScreamingSnakeWithPrefix(str) {
    return 'k' + camelToScreamingSnake(str);
}

function formatFloat(num) {
    // Ensure floats are formatted consistently
    const str = String(num);
    if (!str.includes('.')) {
        return str + '.0';
    }
    return str;
}

// =============================================================================
// Write Output Files
// =============================================================================
function ensureDir(filePath) {
    const dir = path.dirname(filePath);
    if (!fs.existsSync(dir)) {
        fs.mkdirSync(dir, { recursive: true });
    }
}

try {
    // Generate C++ header
    const cppContent = generateCppHeader(plugin, parameters);
    ensureDir(outputCppPath);
    fs.writeFileSync(outputCppPath, cppContent);
    console.log(`Generated: ${outputCppPath}`);
    
    // Generate JS module
    const jsContent = generateJsModule(plugin, parameters);
    ensureDir(outputJsPath);
    fs.writeFileSync(outputJsPath, jsContent);
    console.log(`Generated: ${outputJsPath}`);
    
} catch (err) {
    console.error(`Error writing output files: ${err.message}`);
    process.exit(1);
}

console.log('Parameter generation complete.');

