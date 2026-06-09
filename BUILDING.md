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

Then configure + build:

```pwsh
cmake --preset beta12
cmake --build build-beta12 --config RelWithDebInfo `
  --target fomod_plus_installer fomod_plus_scanner fomod_plus_patch_finder
```

The plugins link the source-built import libs but load the MO2 install's own
`uibase.dll`/`archive.dll` at runtime, so the source must match the MO2 build.

## CI

`.github/workflows/build.yml` builds artifacts for three MO2 lines by compiling
uibase/archive from pinned refs. The "beta11"/"beta12" labels are approximated
with the closest official `modorganizer-uibase` tags (none are published under
those names):

| Artifact | uibase ref       | cmake_common ref | Qt     | Status        |
|----------|------------------|------------------|--------|---------------|
| beta11   | `v2.5.3-beta.2`  | `v2.5.3-beta.2`  | 6.11.0 | green         |
| beta12   | `v2.6.0-dev.5`   | `v2.6.0-dev.1`   | 6.11.1 | green         |
| 2.5.2    | `v2.5.2`         | `2_5_x` branch   | 6.7.3  | best-effort   |

2.5.2 predates the standalone CMake/vcpkg build (old "umbrella" era: needs
Boost via `BOOST_ROOT` + spdlog via classic vcpkg, and the modern
`build-with-mob-action` doesn't fit it either — `2_5_x` has no `CMakePresets`).
Its job is marked `continue-on-error` so it never blocks the modern artifacts.
