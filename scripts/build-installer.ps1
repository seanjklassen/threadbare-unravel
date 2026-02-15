param(
    [switch]$Sign,
    [switch]$DryRun
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $RepoRoot "build-release"
$StagingDir = Join-Path $RepoRoot "staging"
$OutputDir = Join-Path $RepoRoot "Output"
$Python = $env:PYTHON_BIN
if (-not $Python) { $Python = "python" }
$InnoSetup = $env:INNO_SETUP_PATH
if (-not $InnoSetup) { $InnoSetup = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" }

function Run([string]$Cmd, [string[]]$Args) {
    Write-Host "+ $Cmd $($Args -join ' ')"
    if (-not $DryRun) {
        & $Cmd @Args
    }
}

if (-not $DryRun) {
    if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
    if (Test-Path $StagingDir) { Remove-Item -Recurse -Force $StagingDir }
    if (Test-Path $OutputDir) { Remove-Item -Recurse -Force $OutputDir }
}
Run "New-Item" @("-Path", "$StagingDir\VST3", "-ItemType", "Directory", "-Force") 
Run "New-Item" @("-Path", "$OutputDir", "-ItemType", "Directory", "-Force")

Run "cmake" @("-B", $BuildDir, "-DCMAKE_BUILD_TYPE=Release", "-DTHREADBARE_COPY_PLUGIN_AFTER_BUILD=OFF")
Run "cmake" @("--build", $BuildDir, "--config", "Release")

Run "cmake" @("-P", "$RepoRoot\scripts\resolve-artefacts.cmake", "-DOUT_FILE=$StagingDir\artefacts.json", "-DBUILD_DIR=$BuildDir")
Run $Python @("$RepoRoot\scripts\copy-artefacts.py", "$StagingDir\artefacts.json", "$StagingDir")

if (-not (Test-Path $InnoSetup)) {
    throw "Inno Setup not found at $InnoSetup. Set INNO_SETUP_PATH."
}
Run $InnoSetup @("$RepoRoot\installer\windows\Installer.iss")

if ($Sign) {
    if (-not $env:AZURE_SIGNING_DLIB -or -not $env:AZURE_METADATA_JSON) {
        throw "AZURE_SIGNING_DLIB and AZURE_METADATA_JSON must be set to sign."
    }
    $InstallerExe = Join-Path $OutputDir "Unravel-Installer.exe"
    Run "$RepoRoot\installer\windows\sign.bat" @("$InstallerExe")
    Run "signtool" @("verify", "/pa", "/v", $InstallerExe)
}
