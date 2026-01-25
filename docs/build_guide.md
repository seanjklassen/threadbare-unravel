# Unravel Plugin Build Guide

## Prerequisites

- CMake (3.22+)
- C++20 compatible compiler (Clang on macOS)
- JUCE 8 (included via CMake FetchContent)

## Quick Build (Most Common)

After making code changes, rebuild and install with:

```bash
cd /Users/seanklassen-mercury/threadbare-unravel
cmake --build build --config Release
```

This will:
1. Compile changed source files
2. Link the plugin binaries
3. Automatically install to system plugin folders:
   - **AU**: `~/Library/Audio/Plug-Ins/Components/unravel.component`
   - **VST3**: `~/Library/Audio/Plug-Ins/VST3/unravel.vst3`
   - **Standalone**: `build/plugins/unravel/ThreadbareUnravel_artefacts/Release/Standalone/unravel.app`

## Full Clean Build

If you need to rebuild everything from scratch:

```bash
cd /Users/seanklassen-mercury/threadbare-unravel
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Frontend Only

If you only changed frontend files (`plugins/unravel/Source/UI/frontend/`):

```bash
# 1. Build frontend assets
cd plugins/unravel/Source/UI/frontend
npm run build

# 2. Force regenerate resources and rebuild plugin
cd /Users/seanklassen-mercury/threadbare-unravel
cmake --build build --target UnravelResources --config Release -- -B
cmake --build build --config Release
```

## Parameter Changes

If you modified `plugins/unravel/config/params.json`:

```bash
# 1. Regenerate C++ and JS parameter files
node shared/scripts/generate_params.js \
  plugins/unravel/config/params.json \
  plugins/unravel/Source/UnravelGeneratedParams.h \
  plugins/unravel/Source/UI/frontend/src/generated/params.js

# 2. Rebuild frontend (params.js changed)
cd plugins/unravel/Source/UI/frontend
npm run build

# 3. Rebuild plugin
cd /Users/seanklassen-mercury/threadbare-unravel
cmake --build build --target UnravelResources --config Release -- -B
cmake --build build --config Release
```

## Testing in DAW

After building:

1. **Close your DAW completely** (or the standalone app)
2. **Reopen the DAW** - most DAWs rescan plugins on launch
3. If changes don't appear, manually rescan plugins in DAW preferences

### Standalone App Location

```
/Users/seanklassen-mercury/threadbare-unravel/build/plugins/unravel/ThreadbareUnravel_artefacts/Release/Standalone/unravel.app
```

## Build Targets

| Target | Description |
|--------|-------------|
| `ThreadbareUnravel` | Shared code library |
| `ThreadbareUnravel_Standalone` | Standalone application |
| `ThreadbareUnravel_AU` | Audio Unit plugin |
| `ThreadbareUnravel_VST3` | VST3 plugin |
| `UnravelResources` | Frontend binary resources |

Build a specific target:
```bash
cmake --build build --target ThreadbareUnravel_VST3 --config Release
```

## Troubleshooting

### Plugin not updating in DAW
- Ensure DAW is fully closed before rebuilding
- Try rescanning plugins in DAW preferences
- Check that the install paths match where your DAW looks for plugins

### Frontend changes not appearing
- Make sure to run `npm run build` in the frontend folder
- Rebuild `UnravelResources` target with `-B` flag to force regeneration
- Rebuild the main plugin after resources

### Build errors after pulling changes
- Try a clean build (delete `build/` folder and reconfigure)
- Check that no header files are missing includes
