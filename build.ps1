#Requires -Version 7.0
param(
    [string]$Preset = 'beta12',
    [string]$Config,
    [switch]$Configure,
    [switch]$NoPatchFinder
)

$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/cmake/Profile.ps1"
Push-Location $PSScriptRoot
try {
    $profile = Resolve-Profile $Preset $Config $NoPatchFinder
    $receipt = "$($profile.Binary)/fomod-receipt.json"
    if (Test-Path $receipt) { Remove-Item $receipt -Force }
    $needsConfigure = $Configure
    try { $null = Assert-Profile $profile } catch { $needsConfigure = $true }
    if ($needsConfigure) {
        Invoke-CMake --preset $Preset -B $profile.Binary "-DCMAKE_BUILD_TYPE:STRING=$($profile.Config)" `
            "-DFOMOD_PLUS_PRESET:STRING=$Preset" "-DFOMOD_PLUS_BUILD_PATCHFINDER:BOOL=$($profile.PatchFinder)"
    }
    $manifest = Assert-Profile $profile
    # CMake, not PowerShell, applies inherited preset environment to the build.
    $buildResult = [IO.Path]::GetTempFileName()
    try {
        Invoke-CMake --preset $Preset -N "-DFOMOD_BUILD_DIRECTORY:PATH=$($profile.Binary)" `
            "-DFOMOD_BUILD_RESULT:FILEPATH=$buildResult" -C "$PSScriptRoot/cmake/BuildArtifacts.cmake"
        $result = (Get-Content $buildResult -Raw)
        if ($result -cne '0') { throw "Artifact build failed ($result)" }
    } finally {
        Remove-Item -LiteralPath $buildResult -Force
    }
    # Preserve the existing convenience: copy enabled translations only when the
    # configured MO2 translations directory already exists. Never create it.
    if ($manifest.translationDirectory -and (Test-Path $manifest.translationDirectory -PathType Container)) {
        foreach ($artifact in $manifest.artifacts) {
            if ($artifact.destination.StartsWith('translations/')) {
                Copy-Item -LiteralPath $artifact.path -Destination $manifest.translationDirectory -Force
            }
        }
    }
} finally {
    Pop-Location
}
