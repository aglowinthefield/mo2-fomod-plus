# Repository Guidelines

## Project Structure & Modules
- Root `CMakeLists.txt` builds two MO2 plugins: `installer/` (`fomod_plus_installer`) and `scanner/` (`fomod_plus_scanner`); both share helpers in `installer/lib/` and `share/`.
- UI assets live in `installer/resources/` and `scanner/resources/`; translations are `.ts` files in each module and packaged `.qm` files under `package/translations/`.
- Tests sit in `tests/` with XML fixtures in `tests/moduleconf/`; packaging output lands in `package/plugins/`.
- Clone this repo inside `build/modorganizer_super/<repo>` of your mob checkout so MO2 headers and dependencies resolve.

## Build, Test, and Development Commands
- Configure (uses local paths in `CMakePresets.json`):  
  `cmake --preset vs2022-windows`
- Build (RelWithDebInfo by default):  
  `cmake --build vsbuild --config RelWithDebInfo`
- Run tests after a Debug build for clearer asserts:  
  `cmake --build vsbuild --config Debug --target runTests`  
  `ctest --test-dir vsbuild --config Debug --output-on-failure`
- Install compiled plugins into the preset install prefix (or the MO2 plugin dir if `MO2_PLUGIN_TARGET_DIR` is set):  
  `cmake --install vsbuild --config RelWithDebInfo`
- Package distributables (copies DLLs and translations into `package/`):  
  `cmake --build vsbuild --config RelWithDebInfo --target package`

## Coding Style & Naming Conventions
- C++17/20 with Qt6; follow WebKit `.clang-format`: 2-space indents, aligned consecutive assignments, 120-column limit. `.editorconfig` enforces 2-space indents for `.cpp/.h/.ui` and final newlines.
- Classes use PascalCase (`FomodInstallerWindow`), member functions and locals use lowerCamelCase; prefer descriptive names over abbreviations.
- Keep UI strings in translation files; place shared helpers in `share/` to avoid duplication.

## Testing Guidelines
- GoogleTest is fetched in `tests/CMakeLists.txt`; add new suites as `tests/test_<area>.cpp` or under `tests/moduleconf/`.
- Favor small, fixture-backed tests that use existing XML samples; when adding data files, keep them minimal to reduce repo bloat.
- All new logic should have at least one failing test reproduced first, then fixed; ensure `ctest --output-on-failure` passes before PRs.

## Commit & Pull Request Guidelines
- Commit messages in this repo are short, imperative, and single-sentence (e.g., “Clean up debug logs.”). Keep them under ~72 chars and scoped to one change.
- For PRs, include: what changed, why, how to verify (commands or screenshots for UI), linked issues if applicable, and any regression risks. Mark when CI/build artifacts were produced (via `package` target or GH Actions).
