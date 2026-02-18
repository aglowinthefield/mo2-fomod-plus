param(
    [string]$Preset = "vs2022-windows",
    [string]$Config = "RelWithDebInfo",
    [switch]$Configure,
    [switch]$SkipBuild,
    [switch]$NoPatchFinder
)

$ErrorActionPreference = "Stop"

# Build first (delegates to build.ps1)
if (-not $SkipBuild) {
    $buildArgs = @{ Preset = $Preset; Config = $Config }
    if ($Configure) { $buildArgs.Configure = $true }
    if ($NoPatchFinder) { $buildArgs.NoPatchFinder = $true }
    & "$PSScriptRoot/build.ps1" @buildArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$buildDir = switch ($Preset) {
    "vs2022-windows"       { "vsbuild" }
    "vs2022-windows-beta"  { "vsbuild-beta" }
    "vs2022-windows-vcpkg" { "vsbuild-vcpkg" }
    default                { "vsbuild" }
}

# Run the package target
Write-Host "Packaging..." -ForegroundColor Cyan
cmake --build "$PSScriptRoot/$buildDir" --config $Config --target package
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Remove patch finder from package if skipped
if ($NoPatchFinder) {
    $pfDll = "$PSScriptRoot/package/plugins/fomod_plus_patch_finder.dll"
    if (Test-Path $pfDll) { Remove-Item $pfDll }
    $pfQm = Get-ChildItem -Filter "fomod_plus_patch_finder*" -Path "$PSScriptRoot/package/translations" -ErrorAction SilentlyContinue
    foreach ($f in $pfQm) { Remove-Item $f.FullName }
    # Also remove the patch wizard translations
    $pwQm = Get-ChildItem -Filter "fomod_plus_patch_wizard*" -Path "$PSScriptRoot/package/translations" -ErrorAction SilentlyContinue
    foreach ($f in $pwQm) { Remove-Item $f.FullName }
}

# Create 7z archive
$packageDir = "$PSScriptRoot/package"
$archivePath = "$PSScriptRoot/mo2-fomod-plus.7z"
if (Test-Path $archivePath) { Remove-Item $archivePath }

Write-Host "Creating archive..." -ForegroundColor Cyan
Push-Location $packageDir
7z a $archivePath ./plugins ./translations
$exitCode = $LASTEXITCODE
Pop-Location
if ($exitCode -ne 0) { exit $exitCode }

Write-Host "Created $archivePath" -ForegroundColor Green
