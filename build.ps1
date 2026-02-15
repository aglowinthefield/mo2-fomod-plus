param(
    [string]$Preset = "vs2022-windows",
    [string]$Config = "RelWithDebInfo",
    [switch]$Configure
)

$ErrorActionPreference = "Stop"
$buildDir = switch ($Preset) {
    "vs2022-windows"       { "vsbuild" }
    "vs2022-windows-beta"  { "vsbuild-beta" }
    "vs2022-windows-vcpkg" { "vsbuild-vcpkg" }
    default                { "vsbuild" }
}

# Resolve MO2 plugin dir from preset to find translations dir
$presets = Get-Content "$PSScriptRoot/CMakePresets.json" | ConvertFrom-Json
$pluginDir = ($presets.configurePresets |
    Where-Object { $_.name -eq $Preset }).cacheVariables.MO2_PLUGIN_TARGET_DIR
$translationsDir = if ($pluginDir) { "$(Split-Path $pluginDir -Parent)/translations" } else { $null }

# Configure if requested or build dir doesn't exist
if ($Configure -or !(Test-Path "$PSScriptRoot/$buildDir")) {
    Write-Host "Configuring with preset: $Preset" -ForegroundColor Cyan
    cmake --preset $Preset
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

# Build
Write-Host "Building ($Config)..." -ForegroundColor Cyan
cmake --build "$PSScriptRoot/$buildDir" --config $Config --target fomod_plus_installer fomod_plus_scanner fomod_plus_patch_finder
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Copy .qm files to MO2 translations dir
if ($translationsDir -and (Test-Path $translationsDir)) {
    $qmFiles = Get-ChildItem -Recurse -Filter *.qm "$PSScriptRoot/$buildDir"
    foreach ($qm in $qmFiles) {
        Copy-Item $qm.FullName "$translationsDir/" -Force
        Write-Host "  Copied $($qm.Name) -> translations/" -ForegroundColor DarkGray
    }
    Write-Host "Translations copied to $translationsDir" -ForegroundColor Green
} else {
    Write-Host "Skipping translations (no MO2 translations dir found)" -ForegroundColor Yellow
}

Write-Host "Done!" -ForegroundColor Green
