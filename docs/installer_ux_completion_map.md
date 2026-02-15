# Installer UX Completion Map

This maps `installer_ux_plan` goals to what is implemented on branch `installer-ux-implementation`.

## Status at a glance

- macOS installer pipeline: complete and passing in CI
- Windows installer pipeline: scaffolded, CI-gated until core Windows build issues are resolved
- Extensibility foundation: complete (shared metadata + reusable scripts + CI wiring)

## Plan-to-implementation mapping

- Inventory build behavior and constraints
  - Implemented: `THREADBARE_COPY_PLUGIN_AFTER_BUILD` option in `plugins/unravel/CMakeLists.txt`
  - Implemented: artefact resolution entrypoint using `THREADBARE_ARTEFACTS_OUT`

- Reusable metadata contract
  - Implemented: `installer/product.json`
  - Implemented: bundle IDs and version source consumed by build scripts

- Staging and artefact handling
  - Implemented: `scripts/resolve-artefacts.cmake`
  - Implemented: `scripts/copy-artefacts.py` with fallback discovery for generator-expression paths

- macOS installer templates and scripts
  - Implemented: `installer/macos/Distribution.xml`
  - Implemented: `installer/macos/Entitlements.plist`
  - Implemented: `installer/macos/scripts/preinstall`
  - Implemented: `installer/macos/scripts/postinstall`
  - Implemented: `installer/macos/resources/welcome.html`
  - Implemented: `installer/macos/resources/license.html`

- Windows installer templates and scripts
  - Implemented: `installer/windows/Installer.iss`
  - Implemented: `installer/windows/sign.bat`
  - Implemented: `installer/windows/azure-metadata.json`
  - Note: execution depends on current Windows project compile health

- Build automation
  - Implemented: `scripts/build-installer.sh` (macOS)
  - Implemented: `scripts/build-installer.ps1` (Windows)

- CI automation
  - Implemented: `.github/workflows/installer-build.yml`
  - Implemented: frontend build prerequisite for both jobs
  - Implemented: macOS certificate import fallback to unsigned package
  - Implemented: Windows job gate via repo variable `ENABLE_WINDOWS_INSTALLER_CI`

## Validation evidence

- Passing workflow run (macOS + gated Windows):
  - `https://github.com/seanjklassen/threadbare-unravel/actions/runs/22038861216`

## Remaining items (explicitly deferred)

- Enable Windows installer CI by setting repo variable:
  - `ENABLE_WINDOWS_INSTALLER_CI=true`
- Fix existing Windows compile errors in shared code (`shared/core/WebViewBridge.cpp`) before enabling Windows job.
- Replace placeholder installer copy/branding assets and final legal text.

