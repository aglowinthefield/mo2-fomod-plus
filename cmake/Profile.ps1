$ErrorActionPreference = 'Stop'

function Invoke-CMake {
    & cmake @args | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "CMake failed ($LASTEXITCODE): $args" }
}

function Resolve-Profile([string]$Preset, [string]$Config, [bool]$NoPatchFinder) {
    $temp = [IO.Path]::GetTempFileName()
    $oldInspect = $env:FOMOD_PROFILE_INSPECT
    $oldConfig = $env:FOMOD_PROFILE_CONFIG
    try {
        $env:FOMOD_PROFILE_INSPECT = $temp
        $env:FOMOD_PROFILE_CONFIG = $Config
        Invoke-CMake --preset $Preset -N -C "$PSScriptRoot/InspectProfile.cmake"
        $lines = Get-Content $temp
        if ($lines.Count -ne 2) { throw 'CMake did not resolve the profile' }
        return [pscustomobject]@{
            Binary = $lines[0]; Config = $lines[1]; Preset = $Preset
            PatchFinder = $(if ($NoPatchFinder) { 'OFF' } else { 'ON' })
        }
    } finally {
        $env:FOMOD_PROFILE_INSPECT = $oldInspect
        $env:FOMOD_PROFILE_CONFIG = $oldConfig
        Remove-Item $temp -Force
    }
}

function Assert-Profile($Profile) {
    $path = "$($Profile.Binary)/fomod-profile.json"
    if (!(Test-Path $path)) { throw 'Missing configured profile; run build.ps1' }
    $manifest = Get-Content $path -Raw | ConvertFrom-Json
    if ($manifest.preset -cne $Profile.Preset -or $manifest.config -cne $Profile.Config -or
        $manifest.patchfinder -ne $Profile.PatchFinder -or $manifest.generator -ne 'Ninja' -or
        $manifest.binary -ne $Profile.Binary -or $manifest.source -ne (Get-Location).Path.Replace('\','/')) {
        throw 'Configured profile mismatch; run build.ps1 -Configure'
    }
    $cache = Get-Content "$($Profile.Binary)/CMakeCache.txt" -Raw
    foreach ($entry in @("CMAKE_BUILD_TYPE:STRING=$($Profile.Config)", 'CMAKE_GENERATOR:INTERNAL=Ninja',
        "FOMOD_PLUS_BUILD_PATCHFINDER:BOOL=$($Profile.PatchFinder)", "FOMOD_PLUS_PRESET:STRING=$($Profile.Preset)",
        "CMAKE_HOME_DIRECTORY:INTERNAL=$($manifest.source)", "CMAKE_CACHEFILE_DIR:INTERNAL=$($Profile.Binary)")) {
        if ($cache -notmatch "(?m)^$([regex]::Escape($entry))\r?$") { throw "Cache profile mismatch: $entry" }
    }
    return $manifest
}
