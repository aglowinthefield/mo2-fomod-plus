$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$fixture = Join-Path ([IO.Path]::GetTempPath()) "fomod-profile-test-$([guid]::NewGuid())"
New-Item -ItemType Directory $fixture | Out-Null
try {
    Copy-Item "$repo/build.ps1", "$repo/package.ps1" $fixture
    Copy-Item "$repo/cmake" "$fixture/cmake" -Recurse
    @'
{"version":3,"configurePresets":[{"name":"beta12","generator":"Ninja","binaryDir":"${sourceDir}/out","cacheVariables":{"CMAKE_BUILD_TYPE":"Release"},"environment":{"FIXTURE_BUILD_TOKEN":"from-preset"}}]}
'@ | Set-Content "$fixture/CMakePresets.json"
    @'
cmake_minimum_required(VERSION 3.30)
project(profile_fixture NONE)
option(FOMOD_PLUS_BUILD_PATCHFINDER "" ON)
file(WRITE "${CMAKE_BINARY_DIR}/configured.txt" "${CMAKE_BUILD_TYPE}")
file(APPEND "${CMAKE_BINARY_DIR}/configure-count.txt" "configured\n")
foreach(name installer scanner patch_finder)
  if(name STREQUAL "patch_finder" AND NOT FOMOD_PLUS_BUILD_PATCHFINDER)
    continue()
  endif()
  set(target fomod_plus_${name})
  add_custom_target(${target}_fixture
    BYPRODUCTS "${CMAKE_BINARY_DIR}/${target}.dll"
    COMMAND ${CMAKE_COMMAND} "-DREQUIRE_PRESET_ENV=${FOMOD_PLUS_PRESET}" -P "${CMAKE_SOURCE_DIR}/require-env.cmake"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/payload.txt" "${CMAKE_BINARY_DIR}/${target}.dll")
  add_library(${target} SHARED IMPORTED GLOBAL)
  set_target_properties(${target} PROPERTIES IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/${target}.dll")
  add_dependencies(${target} ${target}_fixture)
  file(WRITE "${CMAKE_BINARY_DIR}/${target}_en.qm" "translation")
  if(name STREQUAL "installer")
    set_property(TARGET ${target} PROPERTY FOMOD_PLUS_TRANSLATIONS "${CMAKE_BINARY_DIR}/${target}_en.qm")
  endif()
endforeach()
include(cmake/BuildProfile.cmake)
'@ | Set-Content "$fixture/CMakeLists.txt"
    'payload' | Set-Content "$fixture/payload.txt"
    @'
if(REQUIRE_PRESET_ENV AND NOT "$ENV{FIXTURE_BUILD_TOKEN}" STREQUAL "from-preset")
  message(FATAL_ERROR "Build must inherit the configure-preset environment")
endif()
'@ | Set-Content "$fixture/require-env.cmake"
    Push-Location $fixture
    & pwsh -NoProfile -File "$fixture/build.ps1"
    if ($LASTEXITCODE -ne 0) { throw 'Public build command failed for actual named preset beta12' }
    if ((Get-Content "$fixture/out/configured.txt") -ne 'Release') { throw 'Omitted Config did not follow preset' }
    Write-Host 'PASS: named preset and omitted configuration'
    $script:checks = 1
    function Assert($condition, $message) {
        if (!$condition) { throw $message }
    }
    function Run([string]$scriptName, [string[]]$arguments, [bool]$success = $true) {
        $output = & pwsh -NoProfile -File "$fixture/$scriptName.ps1" @arguments 2>&1
        $code = $LASTEXITCODE
        $script:lastOutput = $output -join "`n"
        if (($code -eq 0) -ne $success) { throw "Unexpected exit $code from $scriptName $arguments`n$output" }
        $script:checks++
    }
    # Named local preset: CMake (not PowerShell) expands inheritance, environment and macros.
    @'
{"version":3,"configurePresets":[{"name":"local","inherits":"beta12","binaryDir":"$env{FIXTURE_OUTPUT}/${presetName}","environment":{"FIXTURE_OUTPUT":"${sourceDir}/inherited"},"cacheVariables":{"CMAKE_BUILD_TYPE":"MinSizeRel"}},{"name":"shared","inherits":"beta12"},{"name":"multi","inherits":"beta12","generator":"Ninja Multi-Config","binaryDir":"${sourceDir}/multi"}]}
'@ | Set-Content "$fixture/CMakeUserPresets.json"
    New-Item -ItemType Directory "$fixture/elsewhere" | Out-Null
    Set-Location "$fixture/elsewhere"
    Run build @('-Preset','local')
    Assert ((Get-Content "$fixture/inherited/local/configured.txt") -eq 'MinSizeRel') 'Local inherited preset was not resolved'
    $count = @(Get-Content "$fixture/inherited/local/configure-count.txt").Count
    Run build @('-Preset','local','-Config','MinSizeRel')
    Assert (@(Get-Content "$fixture/inherited/local/configure-count.txt").Count -eq $count) 'Matching identity unnecessarily reconfigured'
    Run build @('-Preset','local','-Configure')
    Assert (@(Get-Content "$fixture/inherited/local/configure-count.txt").Count -gt $count) 'Configure did not force configure'
    Run build @('-Preset','local','-Config','Debug')
    Assert ((Get-Content "$fixture/inherited/local-Debug/configured.txt") -eq 'Debug') 'Ninja configuration was not changed'
    Assert ((Get-Content "$fixture/inherited/local/configured.txt") -eq 'MinSizeRel') 'Default binary directory was overwritten'
    Run build @('-Preset','vs2022-windows') $false
    Run build @('-Preset','multi') $false
    $cwd = (Get-Location).Path
    try { & "$fixture/build.ps1" -Preset nonexistent; throw 'Expected error' } catch {
        Assert ($_.Exception.Message -ne 'Expected error') 'Unknown preset unexpectedly succeeded'
    }
    Assert ((Get-Location).Path -eq $cwd) 'Build did not restore cwd on error'
    Run package @('-Preset','local','-SkipBuild')
    Assert (Test-Path "$fixture/package/plugins/fomod_plus_patch_finder.dll") 'Patch Finder missing from enabled package'
    $cacheHash = (Get-FileHash "$fixture/inherited/local/CMakeCache.txt").Hash
    $originalProject = Get-Content "$fixture/CMakeLists.txt" -Raw
    'message(FATAL_ERROR "Configure must not execute")' | Set-Content "$fixture/CMakeLists.txt"
    # Poison the build itself as well as regeneration. Artifact-only must run neither.
    $ninja = "$fixture/inherited/local/build.ninja"
    $originalNinja = Get-Content $ninja -Raw
    'this is deliberately invalid ninja syntax' | Set-Content $ninja
    & cmake --build "$fixture/inherited/local" --target fomod_artifacts 2>&1 | Out-Null
    Assert ($LASTEXITCODE -ne 0) 'Poisoned direct build should fail before any target can run'
    # A preflight failure cannot run receipt invalidation; the last verified build is still packageable.
    Run package @('-Preset','local','-SkipBuild')
    Assert ((Get-FileHash "$fixture/inherited/local/CMakeCache.txt").Hash -eq $cacheHash) 'SkipBuild mutated cache'
    Run package @('-Preset','local','-SkipBuild','-NoPatchFinder') $false
    Run package @('-Preset','local','-SkipBuild','-Configure') $false
    $originalProject | Set-Content "$fixture/CMakeLists.txt"
    $originalNinja | Set-Content $ninja
    Run build @('-Preset','local','-NoPatchFinder')
    Assert (Test-Path "$fixture/inherited/local/fomod_plus_patch_finder.dll") 'Fixture needs stale disabled DLL'
    Assert (Test-Path "$fixture/inherited/local/fomod_plus_patch_finder_en.qm") 'Fixture needs stale disabled translation'
    Run package @('-Preset','local','-SkipBuild','-NoPatchFinder')
    Assert (!(Test-Path "$fixture/package/plugins/fomod_plus_patch_finder.dll")) 'Disabled stale DLL packaged'
    Assert (!(Test-Path "$fixture/package/translations/fomod_plus_patch_finder_en.qm")) 'Disabled stale translation packaged'
    $listing = & 7z l "$fixture/mo2-fomod-plus.7z"
    Assert ($LASTEXITCODE -eq 0 -and ($listing -match 'plugins') -and ($listing -match 'translations')) 'Archive layout incorrect'
    Assert (!($listing -match 'patch_finder')) 'Disabled target included in archive'
    $archiveHash = (Get-FileHash "$fixture/mo2-fomod-plus.7z").Hash
    $packageHash = (Get-FileHash "$fixture/package/plugins/fomod_plus_installer.dll").Hash
    $dll = "$fixture/inherited/local/fomod_plus_installer.dll"
    'tampered' | Set-Content $dll
    Run package @('-Preset','local','-SkipBuild','-NoPatchFinder') $false
    Assert ((Get-FileHash "$fixture/mo2-fomod-plus.7z").Hash -eq $archiveHash) 'Failed validation replaced old archive'
    Remove-Item $dll
    Run package @('-Preset','local','-SkipBuild','-NoPatchFinder') $false
    Run build @('-Preset','local','-NoPatchFinder')
    Remove-Item "$fixture/inherited/local/fomod-receipt.json"
    Run package @('-Preset','local','-SkipBuild','-NoPatchFinder') $false
    Run build @('-Preset','local','-NoPatchFinder')
    # A real failed artifact command must invalidate a formerly successful receipt.
    Remove-Item "$fixture/payload.txt"
    Run build @('-Preset','local','-NoPatchFinder') $false
    Assert (!(Test-Path "$fixture/inherited/local/fomod-receipt.json")) 'Failed build retained receipt'
    Run package @('-Preset','local','-SkipBuild','-NoPatchFinder') $false
    'payload' | Set-Content "$fixture/payload.txt"
    Run build @('-Preset','local','-NoPatchFinder')
    $manifestPath = "$fixture/inherited/local/fomod-profile.json"
    $manifestText = Get-Content $manifestPath -Raw
    ($manifestText + ' ') | Set-Content $manifestPath
    Run package @('-Preset','local','-SkipBuild','-NoPatchFinder') $false
    Run build @('-Preset','local','-NoPatchFinder','-Configure')
    Add-Content "$fixture/inherited/local/CMakeCache.txt" '# changed cache'
    Run package @('-Preset','local','-SkipBuild','-NoPatchFinder') $false
    Run package @('-Preset','local','-NoPatchFinder')
    # Shared binaryDir must not allow a different preset to reuse a receipt.
    Run package @('-Preset','shared','-SkipBuild') $false
    Run build @('-Preset','shared')
    Run package @('-Preset','beta12','-SkipBuild') $false
    Run build @('-Preset','beta12')
    # Archive failure preserves both old published outputs; PATH affects only fixture subprocesses.
    $archiveHash = (Get-FileHash "$fixture/mo2-fomod-plus.7z").Hash
    $packageHash = (Get-FileHash "$fixture/package/plugins/fomod_plus_installer.dll").Hash
    New-Item -ItemType Directory "$fixture/failing-tools" | Out-Null
    "@echo off`r`nexit /b 9" | Set-Content "$fixture/failing-tools/7z.cmd"
    $oldPath = $env:PATH
    try {
        $env:PATH = "$fixture/failing-tools;$oldPath"
        Run package @('-Preset','local','-SkipBuild','-NoPatchFinder') $false
        Assert ($script:lastOutput.Contains('7z failed (9)')) 'Failure did not come from the poisoned archiver'
    } finally { $env:PATH = $oldPath }
    Assert ((Get-FileHash "$fixture/mo2-fomod-plus.7z").Hash -eq $archiveHash) 'Archive failure replaced old archive'
    Assert ((Get-FileHash "$fixture/package/plugins/fomod_plus_installer.dll").Hash -eq $packageHash) 'Archive failure replaced old package'
    Assert (@(Get-ChildItem $fixture -Filter '.fomod-package-*').Count -eq 0) 'Staging directory leaked'
    if ($IsWindows) {
        # Fail publication after the old package moves aside, using a real filesystem lock.
        $lock = [IO.File]::Open("$fixture/mo2-fomod-plus.7z", 'Open', 'Read', 'Read')
        try { Run package @('-Preset','local','-SkipBuild','-NoPatchFinder') $false }
        finally { $lock.Dispose() }
        Assert ((Get-FileHash "$fixture/mo2-fomod-plus.7z").Hash -eq $archiveHash) 'Publication failure replaced old archive'
        Assert ((Get-FileHash "$fixture/package/plugins/fomod_plus_installer.dll").Hash -eq $packageHash) 'Publication rollback did not restore old package'
        Assert (@(Get-ChildItem $fixture -Filter '.fomod-package-*').Count -eq 0) 'Publication rollback leaked staging'
    }
    # Direct CI-style CMake configure/build/package has no preset identity requirement.
    & cmake -S $fixture -B "$fixture/direct" -G Ninja -DCMAKE_BUILD_TYPE=Release -DFOMOD_PLUS_BUILD_PATCHFINDER=OFF | Out-Null
    Assert ($LASTEXITCODE -eq 0) 'Direct CMake configure failed'
    & cmake --build "$fixture/direct" --target package | Out-Null
    Assert ($LASTEXITCODE -eq 0) 'Direct CMake package failed'
    $script:checks++
    Write-Host "PASS: $script:checks public-command checks plus profile/artifact/cache/cwd assertions"
} finally {
    Pop-Location
    Remove-Item $fixture -Recurse -Force
}
