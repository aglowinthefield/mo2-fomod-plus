#Requires -Version 7.0
param(
    [string]$Preset = 'beta12',
    [string]$Config,
    [switch]$Configure,
    [switch]$SkipBuild,
    [switch]$NoPatchFinder
)

$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/cmake/Profile.ps1"
Push-Location $PSScriptRoot
try {
    if ($SkipBuild -and $Configure) { throw '-SkipBuild cannot be combined with -Configure' }
    if (!$SkipBuild) {
        & "$PSScriptRoot/build.ps1" -Preset $Preset -Config $Config -Configure:$Configure -NoPatchFinder:$NoPatchFinder
    }
    $profile = Resolve-Profile $Preset $Config $NoPatchFinder
    $null = Assert-Profile $profile
    & "$PSScriptRoot/cmake/PackageArtifacts.ps1" -Manifest "$($profile.Binary)/fomod-profile.json"
} finally {
    Pop-Location
}
