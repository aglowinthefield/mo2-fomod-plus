# Building

## Presets

`CMakePresets.json` defines several configure presets. Machine-specific paths
can be overridden in a local, git-ignored `CMakeUserPresets.json`.

| Preset    | Dependencies source                                  |
|-----------|------------------------------------------------------|
| `default` | A locally-built `modorganizer_super` tree            |
| `beta`    | A locally-built MO2 beta source tree                 |
| `vcpkg`   | vcpkg registry (`mo2-uibase` pinned in `vcpkg.json`) |
| `beta12`  | Source-built uibase/archive + Qt 6.11 (see below)    |

### `beta12` preset

There is no official MO2 "beta12" uibase, and the vcpkg registry has no exact
match, so this preset builds against uibase/archive compiled locally from the
MO2 source. It reads three environment variables:

| Variable           | Example                                              |
|--------------------|------------------------------------------------------|
| `QT_ROOT`          | `C:/Qt/6.11.1/msvc2022_64`                            |
| `MO2_DEPS_INSTALL` | `D:/var/mo2-beta12-libs/install` (uibase + archive)  |
| `MO2_CMAKE_COMMON` | `D:/Mod.Organizer-2.5.3beta12-src/cmake_common`      |

beta12's uibase needs **C++23** (`std::generator`), so `MO2_CMAKE_COMMON` must
point at a `cmake_common` that sets `CXX_STANDARD 23` (the in-tree one from a
beta12 source tree, or the `v2.6.0-dev.1` tag of `ModOrganizer2/cmake_common`).

To produce `MO2_DEPS_INSTALL`, build uibase and archive from source (run each
inside a VS dev shell, with vcpkg providing spdlog/7zip):

```pwsh
cmake -S <mo2-src>/uibase -B build-uibase -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows -DVCPKG_MANIFEST_FEATURES=standalone `
  "-Dmo2-cmake_DIR=<mo2-src>/cmake_common" "-DCMAKE_PREFIX_PATH=$env:QT_ROOT" `
  "-DQt6_DIR=$env:QT_ROOT/lib/cmake/Qt6" -DCMAKE_INSTALL_PREFIX=<install> -DBUILD_TESTING=OFF
cmake --build build-uibase --target install
# repeat for <mo2-src>/archive (drop the standalone/testing flags)
```

Then build from a VS developer shell (PowerShell 7 and Ninja on PATH):

```pwsh
./build.ps1                         # beta12, preset's RelWithDebInfo
./build.ps1 -Preset beta12 -Config Debug
./package.ps1 -Preset beta12 -NoPatchFinder
./package.ps1 -Preset beta12 -NoPatchFinder -SkipBuild
```

### Build profiles and packaging

The scripts can be invoked by absolute path from any working directory and restore
that directory even on failure. `-Preset` is an actual configure preset name,
including names in `CMakeUserPresets.json`; the old `vs2022-windows*` aliases are
not supported. The default is `beta12`. CMake itself resolves inheritance,
preset environment variables and macros through a read-only `--preset ... -N -C`
initial-cache probe. PowerShell contains no preset map or inheritance resolver.
Build execution also runs inside CMake's resolved preset environment, so inherited
`PATH`, `INCLUDE`, `LIB` and custom-command variables reach Ninja and its children.

Omitting `-Config` uses the selected preset's `CMAKE_BUILD_TYPE`. An explicit
matching configuration keeps its `binaryDir`; a different configuration uses
`<resolved-binaryDir>-<Config>` and configures the actual `CMAKE_BUILD_TYPE` there.
For example, beta12 Debug uses `build-beta12-Debug`, not release objects with a
`--config Debug` label. Configuration names must contain only letters, digits,
underscore, plus or hyphen. Only **single-config Ninja** is supported; multi-config
and other generators fail explicitly. Presets must provide a nonempty build type.
`-Configure` forces configuration; missing or mismatched preset/configuration/
Patch Finder identity also triggers configuration. Use `-Configure` after changing
other preset cache settings (dependency locations, toolchains, etc.).

`-NoPatchFinder` configures `FOMOD_PLUS_BUILD_PATCHFINDER=OFF`, not merely a
shorter build target list. `cmake/BuildProfile.cmake` owns the enabled targets and
generates `fomod-profile.json` with exact target-file and translation paths.
Translations use the MO2 helper's `<target>_en.qm` output convention; a target
with nonstandard outputs can set its `FOMOD_PLUS_TRANSLATIONS` property. The
convention is verified against CI's `cmake_common` tags `v2.5.3-beta.2` and
`v2.6.0-dev.1`. No
recursive build-directory translation search is performed.

`fomod_artifacts` builds the enabled plugins and writes `fomod-receipt.json` only
after success. The receipt hashes the manifest, CMake cache and every artifact.
A `build.ps1` attempt invalidates the previous receipt before configuration/build.
Direct CMake builds invalidate it when Ninja starts the target commands; a Ninja
preflight failure (such as an invalid build file) cannot run that invalidation.
In that case the last verified artifacts remain packageable. `package.ps1 -SkipBuild` is
strictly artifact-only: the read-only preset probe is allowed, but no project
configuration, compilation, build target or cache mutation occurs. It requires a
matching successful receipt and unchanged artifacts; missing outputs, mismatched
profiles and stale hashes fail with a rebuild instruction. It deliberately does
not check whether source files have changed: it packages the last verified build,
not a claim that current source has been compiled. `-SkipBuild -Configure` is an
error. Do not run simultaneous builds/packages against the same profile/output.

Packaging requires `7z` on PATH. It copies only manifested artifacts into a fresh
unique staging directory and creates a temporary archive. Only after success are
`package/plugins`, `package/translations` and root `mo2-fomod-plus.7z` published.
The archive contains `plugins/` and `translations/` at top level. Previous outputs
survive validation, staging and archiver failures; publication failures attempt
rollback (if rollback itself fails, recovery files are preserved and reported).
Cleanup is restricted to that invocation's unique staging directory.

Existing plugin output/deployment settings are unchanged; no install is invoked.
The build convenience still copies enabled translations to an **existing** sibling
`translations` directory when `MO2_PLUGIN_TARGET_DIR` is explicitly cached; it
does not create a new deployment directory. Artifact-only packaging never deploys.

Direct CMake/CI use does not require a named preset:

```pwsh
cmake --preset beta12
cmake --build build-beta12 --target fomod_artifacts
cmake --build build-beta12 --target package
# Or configure directly with -S/-B -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
# and the appropriate dependency paths, then build the same targets.
```

The direct `package` target builds dependencies, records their receipt and creates
the distribution including the archive. Use the PowerShell `-SkipBuild` command
when build execution is forbidden. Separate plugin DLLs and CI's MO2 version
policy remain unchanged.

### Build-script regression tests

```pwsh
pwsh -NoProfile -File tests/build/profile-tests.ps1
```

These plain PowerShell assertions copy the public commands and CMake modules into
unique temporary fixture projects; they require no Qt/MO2 dependencies and do not
install or deploy into MO2. They exercise inherited/local/environment presets,
actual Ninja configuration isolation, unknown/unsupported presets, forced and
identity-driven configuration, stale disabled artifacts, poisoned configure/build
files under `-SkipBuild`, missing/changed receipts and artifacts, failed builds,
archive failure preservation, publication rollback, build-time preset environment,
translation-path fallback, cwd restoration, and direct CMake packaging.
The fixture uses real CMake/Ninja custom artifact commands, not a C++ compiler or
mocked internal profile helpers. All 32 public-command checks pass with PowerShell
7.6.9, both CMake **3.30.9** and **4.4.4**, Ninja 1.13.2 and 7-Zip 26.05 on Windows.
The read-only preset probe is covered on both CMake versions, including unchanged
cache hashes under `-SkipBuild`. Recovery-file retention if rollback itself fails
is defensive behavior, not an exercised fixture case.

### C++ regression tests without plugin deployment

The tests can be configured independently of the MO2 plugins, using a matching
Qt installation and a VS developer shell:

```pwsh
cmake -S tests -B build-tests -G Ninja -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_PREFIX_PATH=$env:QT_ROOT"
cmake --build build-tests --target runTests
$env:PATH = "$env:QT_ROOT/bin;$env:PATH"
ctest --test-dir build-tests --output-on-failure
```

This includes XML, FOMOD data, condition evaluation and Patch Finder action/view
regressions. It does not configure or deploy the plugin DLLs. Patch Finder tests
use real temporary database/dismissal files and a narrow MO2 adapter; Windows
file-lock cases exercise failed atomic replacement without changing old files.
Qt dialog interaction inside MO2 still requires a manual smoke test.

The plugins link the source-built import libs but load the MO2 install's own
`uibase.dll`/`archive.dll` at runtime, so the source must match the MO2 build.

## Neovim

C++ editing uses **clangd** plus CMake's generated compilation database. Install
`clangd` and `clang-format` through Mason (or put them on PATH); enable clangd
in your LSP configuration and `clang-format` for C/C++ in your formatter. C/C++
Treesitter parsers provide highlighting, folds, and syntax-aware text objects.

The repository's `.clangd` reads `build-beta12/compile_commands.json`, generated
by the `beta12` preset (and the local `dev` preset). It uses the real Qt/MO2
include paths, compiler definitions, and language standard. Configure/build once
with the instructions above so dependencies and generated Qt headers exist.
If using a different build tree, update `CompilationDatabase` in `.clangd` to
that directory. Do not recreate the include paths by hand.

From the repository root:

```pwsh
nvim .
```

Editing works from a normal terminal with the installed Windows toolchain;
**building still requires the VS developer shell** described above. Formatting
uses `.clang-format`, including the repository's two-space indentation. No
Neovim startup command should build or deploy plugins automatically.

## CI

`.github/workflows/build.yml` builds artifacts for three MO2 lines by compiling
uibase/archive from pinned refs. The "beta11"/"beta12" labels are approximated
with the closest official `modorganizer-uibase` tags (none are published under
those names):

| Artifact | uibase ref       | cmake_common ref | Qt     |
|----------|------------------|------------------|--------|
| beta11   | `v2.5.3-beta.2`  | `v2.5.3-beta.2`  | 6.11.0 |
| beta12   | `v2.6.0-dev.5`   | `v2.6.0-dev.1`   | 6.11.1 |

### Why no 2.5.2 artifact

MO2 2.5.2 predates the standalone CMake/vcpkg build. Its `cmake_common`
(`2_5_x`) `mo2.cmake` requires `BOOST_ROOT`, `FMT_ROOT`, and `PYTHON_ROOT` and
resolves dependencies against a full `modorganizer_super` umbrella layout (Boost,
fmt, googletest each built the umbrella way). That environment is produced by
MO2's classic [`mob`](https://github.com/ModOrganizer2/mob) tool, not by a few CI
steps — and the modern `build-with-mob-action` doesn't fit it either (`2_5_x`
ships no `CMakePresets`, and `mob.ini` pins Qt 6.11). Producing a 2.5.2 artifact
would require a separate, heavyweight mob-based workflow. Given 2.5.2 → 2.5.3 is a
minor bump, the `beta11` build (uibase `2.5.3-beta.2`) is the closest available.
