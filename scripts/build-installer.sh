#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

PLATFORM="${1:-}"
SIGN=false
NOTARIZE=false
DRY_RUN=false

shift || true
while [[ $# -gt 0 ]]; do
    case $1 in
        --sign) SIGN=true ;;
        --notarize) NOTARIZE=true; SIGN=true ;;
        --dry-run) DRY_RUN=true ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
    shift
done

run() {
    echo "+ $*"
    if [[ "$DRY_RUN" != "true" ]]; then
        "$@"
    fi
}

check_env() {
    local var_name="$1"
    if [[ -z "${!var_name:-}" ]]; then
        echo "Error: $var_name environment variable not set"
        exit 1
    fi
}

build_macos() {
    local product_name="unravel"
    local build_dir="$REPO_ROOT/build-release"
    local staging_dir="$REPO_ROOT/staging"
    local output_dir="$REPO_ROOT/dist"
    local python_bin="${PYTHON_BIN:-python3}"
    local version
    version=$("$python_bin" -c "import json,sys; print(json.load(open(sys.argv[1], encoding='utf-8'))['version'])" "$REPO_ROOT/installer/product.json")

    run rm -rf "$build_dir" "$staging_dir" "$output_dir"
    run mkdir -p "$staging_dir" "$output_dir"

    run cmake -B "$build_dir" \
        -DCMAKE_BUILD_TYPE=Release \
        -DTHREADBARE_COPY_PLUGIN_AFTER_BUILD=OFF \
        -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
    run cmake --build "$build_dir" --config Release

    run cmake \
        -DOUT_FILE="$staging_dir/artefacts.json" \
        -DBUILD_DIR="$build_dir" \
        -P "$REPO_ROOT/scripts/resolve-artefacts.cmake"
    run "$python_bin" "$REPO_ROOT/scripts/copy-artefacts.py" "$staging_dir/artefacts.json" "$staging_dir"

    run dot_clean "$staging_dir"

    if [[ "$SIGN" == "true" ]]; then
        check_env APPLE_DEVELOPER_ID_APP
        check_env APPLE_DEVELOPER_ID_INSTALLER
        run codesign --sign "$APPLE_DEVELOPER_ID_APP" \
            --timestamp --options runtime \
            --entitlements "$REPO_ROOT/installer/macos/Entitlements.plist" \
            --deep --force "$staging_dir/AU/${product_name}.component"
        run codesign --sign "$APPLE_DEVELOPER_ID_APP" \
            --timestamp --options runtime \
            --entitlements "$REPO_ROOT/installer/macos/Entitlements.plist" \
            --deep --force "$staging_dir/VST3/${product_name}.vst3"
        run codesign -vvv --deep --strict "$staging_dir/AU/${product_name}.component"
        run codesign -vvv --deep --strict "$staging_dir/VST3/${product_name}.vst3"
    fi

    run pkgbuild --root "$staging_dir/VST3" \
        --identifier "com.threadbare.${product_name}.vst3" \
        --version "$version" \
        --install-location "/Library/Audio/Plug-Ins/VST3" \
        --scripts "$REPO_ROOT/installer/macos/scripts" \
        "$output_dir/${product_name}-vst3.pkg"
    run pkgbuild --root "$staging_dir/AU" \
        --identifier "com.threadbare.${product_name}.component" \
        --version "$version" \
        --install-location "/Library/Audio/Plug-Ins/Components" \
        --scripts "$REPO_ROOT/installer/macos/scripts" \
        "$output_dir/${product_name}-au.pkg"

    run productbuild \
        --distribution "$REPO_ROOT/installer/macos/Distribution.xml" \
        --resources "$REPO_ROOT/installer/macos/resources" \
        --package-path "$output_dir" \
        "$output_dir/Unravel-${version}-unsigned.pkg"

    if [[ "$SIGN" == "true" ]]; then
        run productsign --sign "$APPLE_DEVELOPER_ID_INSTALLER" \
            --timestamp \
            "$output_dir/Unravel-${version}-unsigned.pkg" \
            "$output_dir/Unravel-${version}.pkg"
        run rm "$output_dir/Unravel-${version}-unsigned.pkg"
        run pkgutil --check-signature "$output_dir/Unravel-${version}.pkg"

        if [[ "$NOTARIZE" == "true" ]]; then
            check_env APPLE_ID
            check_env APPLE_APP_SPECIFIC_PASSWORD
            check_env APPLE_TEAM_ID
            run xcrun notarytool submit "$output_dir/Unravel-${version}.pkg" \
                --apple-id "$APPLE_ID" \
                --password "$APPLE_APP_SPECIFIC_PASSWORD" \
                --team-id "$APPLE_TEAM_ID" \
                --wait
            run xcrun stapler staple "$output_dir/Unravel-${version}.pkg"
            run spctl -vvv --assess --type install "$output_dir/Unravel-${version}.pkg"
        fi
    else
        run mv "$output_dir/Unravel-${version}-unsigned.pkg" "$output_dir/Unravel-${version}.pkg"
    fi

    echo "Output: $output_dir/Unravel-${version}.pkg"
}

case "$PLATFORM" in
    macos|mac|darwin)
        build_macos
        ;;
    windows|win)
        echo "Use PowerShell for Windows builds:"
        echo "  powershell -ExecutionPolicy Bypass -File scripts/build-installer.ps1 -Sign"
        exit 1
        ;;
    *)
        echo "Usage: $0 <macos|windows> [--sign] [--notarize] [--dry-run]"
        exit 1
        ;;
esac
