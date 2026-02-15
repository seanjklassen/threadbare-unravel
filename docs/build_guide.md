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

## Installer Builds

Release builds for packaging should disable auto-copy:

```bash
cmake -B build-release -DCMAKE_BUILD_TYPE=Release \
  -DTHREADBARE_COPY_PLUGIN_AFTER_BUILD=OFF \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build-release --config Release
```

### macOS (Packaging Script)

```bash
./scripts/build-installer.sh macos --sign
```

### Windows (PowerShell)

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-installer.ps1 -Sign
```

### CI signing: Apple .p12 and GitHub secrets

For macOS signing and notarization in GitHub Actions, the runner needs your Developer ID certs in a keychain. The workflow imports them from two secrets: a base64-encoded .p12 and its password.

**1. Export a .p12 on your Mac**

- Open **Keychain Access** and select the **login** keychain.
- Under **My Certificates**, select **Developer ID Application: Your Name (TEAMID)** (expand to show the private key).
- **File → Export** → save as e.g. `DeveloperID.p12`, format **Personal Information Exchange (.p12)**.
- Set an export password and remember it.
- Repeat for **Developer ID Installer** (or export both certs in one .p12: select both, then File → Export; one .p12 with both is fine).

**2. Base64-encode the .p12**

In Terminal:

```bash
base64 -i DeveloperID.p12 | pbcopy
```

(Or write to a file: `base64 -i DeveloperID.p12 -o p12-base64.txt`)

**3. Add GitHub repository secrets**

Repo → **Settings** → **Secrets and variables** → **Actions** → **New repository secret**:

| Secret name | Value |
|-------------|--------|
| `APPLE_CERTIFICATE_P12` | The entire base64 string (paste from clipboard or from `p12-base64.txt`). |
| `APPLE_CERTIFICATE_PASSWORD` | The password you set when exporting the .p12. |

Also ensure these are set (certificate *names* and notarization): `APPLE_DEVELOPER_ID_APP`, `APPLE_DEVELOPER_ID_INSTALLER`, `APPLE_ID`, `APPLE_APP_SPECIFIC_PASSWORD`, `APPLE_TEAM_ID`.

The workflow will decode the .p12, import it into a temporary keychain, and use it for `codesign` and `productsign`; signing only runs when `APPLE_CERTIFICATE_P12` is set.

### CI signing: Azure Artifact Signing (Windows)

The Windows job uses [Azure Artifact Signing](https://github.com/Azure/artifact-signing-action). You do **not** need `AZURE_SIGNING_DLIB` or `AZURE_METADATA_JSON`; the workflow uses the official action and the account/profile names from `installer/windows/azure-metadata.json`. You only need an Azure AD app and three GitHub secrets.

**1. Create an App Registration (Azure Portal)**

- Go to [Azure Portal](https://portal.azure.com) → **Microsoft Entra ID** (or Azure Active Directory) → **App registrations** → **New registration**.
- Name it (e.g. `threadbare-github-signing`), leave supported account type as default, no redirect URI needed → **Register**.

**2. Create a client secret**

- In the app → **Certificates & secrets** → **New client secret** → add description, choose expiry → **Add**. Copy the **Value** immediately (you can’t see it again); this is `AZURE_CLIENT_SECRET`.

**3. Get Tenant ID and Client ID**

- **Overview** of the app: copy **Application (client) ID** → `AZURE_CLIENT_ID`.
- **Overview**: copy **Directory (tenant) ID** → `AZURE_TENANT_ID`.

**4. Assign the app permission to sign**

- Go to your **Artifact Signing** account (e.g. **threadbare-design**) in the portal.
- **Access control (IAM)** → **Add** → **Add role assignment**.
- Role: **Artifact Signing Certificate Profile Signer** (or “Code Signing Certificate User” if that’s what your tenant shows). **Next** → **Members** → **+ Select members** → choose the app you created → **Review + assign**.

**5. Add GitHub repository secrets**

Repo → **Settings** → **Secrets and variables** → **Actions** → **New repository secret**:

| Secret name | Value |
|-------------|--------|
| `AZURE_TENANT_ID` | Directory (tenant) ID from step 3. |
| `AZURE_CLIENT_ID` | Application (client) ID from step 3. |
| `AZURE_CLIENT_SECRET` | The client secret value from step 2. |

The workflow signs the Windows installer only when `AZURE_CLIENT_ID` is set. Endpoint and signing account/profile are fixed in the workflow to match `installer/windows/azure-metadata.json` (threadbare-design, unravel-code-signing, East US).

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
