# Repository Guidelines

## Project Structure & Modules
- Root `CMakeLists.txt` builds `installer/`, `scanner/`, and optional `patchfinder/` as separate MO2 plugins; shared helpers live in `installer/lib/` and `share/`.
- UI assets live in `installer/resources/` and `scanner/resources/`; translations are `.ts` files in each module and packaged `.qm` files under `package/translations/`.
- Tests sit in `tests/` with XML fixtures in `tests/moduleconf/`; packaging output lands in `package/plugins/`.
- Dependency setup depends on the selected preset; see `BUILDING.md` for mob, vcpkg, and source-built MO2 dependencies.

## Build, Test, and Development Commands
- Before configuring, building, packaging, or testing, read `BUILDING.md` for current commands, profile rules, and prerequisites.
- Validate C++ changes using the standalone `tests/` project and build-script changes using `tests/build/profile-tests.ps1`.
- Keep validation outputs isolated from live MO2: root plugin builds can write directly to `MO2_PLUGIN_TARGET_DIR` even without an install command.
- `GLOSSARY.md` defines Patch Finder domain terms; use it when changing suggestion or dismissal behavior.

## Coding Style & Naming Conventions
- C++17/20 with Qt6; follow WebKit `.clang-format`: 2-space indents, aligned consecutive assignments, 120-column limit. `.editorconfig` enforces 2-space indents for `.cpp/.h/.ui` and final newlines.
- Classes use PascalCase (`FomodInstallerWindow`), member functions and locals use lowerCamelCase; prefer descriptive names over abbreviations.
- Keep UI strings in translation files; place shared helpers in `share/` to avoid duplication.

### Header File Patterns
- Use `#pragma once` for include guards (preferred over `#ifndef` guards)
- Member variables use `m` prefix: `mOrganizer`, `mFomodPath`, `mViewModel`
- Use `[[nodiscard]]` attribute on getters and pure functions
- Organize includes: local project includes first, then library includes, then Qt includes

### Pragma Regions (.cpp files)
Use `#pragma region` to organize implementation files into logical sections. Common regions:
- `Initialization` - Constructor, init(), setup functions
- `Settings` - Configuration getters/setters
- `Helpers` - Private utility functions
- `ViewModel Lifecycle` - State management
- `Traversal Functions` - Iteration logic
- `Initializers` - Object creation
- `Group Constraints` / `Plugin Constraints` / `Step Constraints` - Validation logic
- `Navigation/UI` - User interaction handlers
- `Utility` - toString(), logging, misc

Example:
```cpp
#pragma region Initialization
bool MyClass::init(IOrganizer* organizer) { ... }
#pragma endregion

#pragma region Settings
bool MyClass::shouldDoThing() const { ... }
#pragma endregion
```

### Other Conventions
- Use `using namespace MOBase;` and `using namespace std;` at file scope in implementation files
- Prefer `std::shared_ptr` for objects shared across components, `std::unique_ptr` for owned objects
- Use `final` on classes not intended for inheritance
- Use modern C++ features: `std::ranges`, `std::optional`, structured bindings, `[[maybe_unused]]`

## Testing Guidelines
- GoogleTest is fetched in `tests/CMakeLists.txt`; add new suites as `tests/test_<area>.cpp` or under `tests/moduleconf/`.
- Favor small, fixture-backed tests that use existing XML samples; when adding data files, keep them minimal to reduce repo bloat.
- All new logic should have at least one failing test reproduced first, then fixed; ensure `ctest --output-on-failure` passes before PRs.

## Commit & Pull Request Guidelines
- Commit messages in this repo are short, imperative, and single-sentence (e.g., “Clean up debug logs.”). Keep them under ~72 chars and scoped to one change.
- For PRs, include: what changed, why, how to verify (commands or screenshots for UI), linked issues if applicable, and any regression risks. Mark when CI/build artifacts were produced (via `package` target or GH Actions).
