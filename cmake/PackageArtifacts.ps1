#Requires -Version 7.0
param([Parameter(Mandatory)][string]$Manifest, [switch]$Record)
$ErrorActionPreference = 'Stop'
$metadata = Get-Content $Manifest -Raw | ConvertFrom-Json
$receiptPath = Join-Path $metadata.binary 'fomod-receipt.json'
function Get-Hash([string]$Path) {
    if (!(Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Missing artifact or metadata: $Path" }
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}
function Get-ArtifactHashes {
    $hashes = [ordered]@{}
    foreach ($artifact in $metadata.artifacts) {
        if ($artifact.destination -notmatch '^(plugins|translations)/[^/\\:]+$' -or
            $artifact.destination -match '/\.\.?$' -or $hashes.Contains($artifact.destination)) {
            throw "Invalid or duplicate artifact destination: $($artifact.destination)"
        }
        $hashes[$artifact.destination] = Get-Hash $artifact.path
    }
    if ($hashes.Count -eq 0) { throw 'Empty artifact manifest' }
    return $hashes
}
if ($Record) {
    if (Test-Path $receiptPath) { Remove-Item $receiptPath -Force }
    $receipt = @{
        manifest = Get-Hash $Manifest
        cache = Get-Hash (Join-Path $metadata.binary 'CMakeCache.txt')
        artifacts = Get-ArtifactHashes
    }
    $tempReceipt = "$receiptPath.$([guid]::NewGuid()).tmp"
    try {
        $receipt | ConvertTo-Json -Depth 5 | Set-Content $tempReceipt
        Move-Item $tempReceipt $receiptPath -Force
    } finally {
        if (Test-Path $tempReceipt) { Remove-Item $tempReceipt -Force }
    }
    return
}
if (!(Test-Path $receiptPath)) { throw 'Missing successful-build receipt; run build.ps1 (without -SkipBuild)' }
$receipt = Get-Content $receiptPath -Raw | ConvertFrom-Json -AsHashtable
if ($receipt.manifest -ne (Get-Hash $Manifest) -or
    $receipt.cache -ne (Get-Hash (Join-Path $metadata.binary 'CMakeCache.txt'))) {
    throw 'Stale build receipt: manifest/cache mismatch; rebuild'
}
$hashes = Get-ArtifactHashes
if ($receipt.artifacts.Count -ne $hashes.Count) { throw 'Stale build receipt: artifact count mismatch' }
foreach ($key in $hashes.Keys) {
    if ($receipt.artifacts[$key] -ne $hashes[$key]) { throw "Stale build receipt: artifact hash mismatch: $key" }
}
# Never clean the source package or a caller-supplied staging path. Only this unique owned
# directory is disposable. Previous outputs are moved into it for rollback at publication.
$work = Join-Path $metadata.source ".fomod-package-$([guid]::NewGuid())"
$stage = Join-Path $work 'stage'
$archive = Join-Path $work 'mo2-fomod-plus.7z'
$output = Join-Path $metadata.source 'package'
$outputArchive = Join-Path $metadata.source 'mo2-fomod-plus.7z'
$oldPackage = $false; $oldArchive = $false; $newPackage = $false; $newArchive = $false
$keepRecovery = $false
try {
    New-Item -ItemType Directory "$stage/plugins", "$stage/translations" | Out-Null
    foreach ($artifact in $metadata.artifacts) {
        $destination = Join-Path $stage $artifact.destination
        Copy-Item -LiteralPath $artifact.path -Destination $destination
        if ((Get-Hash $destination) -ne $hashes[$artifact.destination]) { throw 'Artifact changed during staging; rebuild' }
    }
    Push-Location $stage
    try {
        & 7z a $archive ./plugins ./translations | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "7z failed ($LASTEXITCODE)" }
        if (!(Test-Path $archive -PathType Leaf)) { throw '7z did not create an archive' }
    } finally { Pop-Location }
    try {
        if (Test-Path $output) { Move-Item $output "$work/previous-package"; $oldPackage = $true }
        if (Test-Path $outputArchive) { Move-Item $outputArchive "$work/previous.7z"; $oldArchive = $true }
        Move-Item $stage $output; $newPackage = $true
        Move-Item $archive $outputArchive; $newArchive = $true
    } catch {
        try {
            if ($newArchive) { Move-Item $outputArchive "$work/failed.7z" }
            if ($newPackage) { Move-Item $output "$work/failed-package" }
            if ($oldPackage) { Move-Item "$work/previous-package" $output }
            if ($oldArchive) { Move-Item "$work/previous.7z" $outputArchive }
        } catch {
            $keepRecovery = $true
            Write-Warning "Publication rollback failed; preserved recovery files at $work"
        }
        throw
    }
} finally {
    if (!$keepRecovery -and (Test-Path $work)) { Remove-Item -LiteralPath $work -Recurse -Force }
}
