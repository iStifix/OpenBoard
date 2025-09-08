# Repository Guidelines

## Project Structure & Module Organization
- `src/`: C++ sources by module (`board/`, `core/`, `gui/`, `web/`). Executable target: `openboard` (CMake).
- `resources/`: Qt assets (`.qrc`, images, `.ui` forms, `i18n`, styles).
- `plugins/`: Optional plugins (e.g., `cffadaptor`).
- `cmake/`: Dependency and version helpers.
- Build metadata: `CMakeLists.txt`, `OpenBoard.pro`, `version.txt`, `release_scripts/`.
- Tests: If added, place under `tests/` mirroring module layout.

## Build, Test, and Development Commands
- Configure (Qt6 preferred):
  - `cmake -S . -B build -DQT_VERSION=6 -DCMAKE_BUILD_TYPE=Release`
- Build: `cmake --build build -j$(nproc)`
- Run locally: `./build/openboard`
- Package (from `build/`): `cpack -G DEB` or `cpack -G RPM`
- Alternative (qmake/Qt Creator): `qmake OpenBoard.pro && make -j$(nproc)`

## Coding Style & Naming Conventions
- Language: C++20 with Qt 5.15/6; prefer Qt types (`QString`, `QVector`).
- Indentation: 4 spaces; braces on the same line as declarations.
- Naming: Classes PascalCase (often `UB…`), methods/variables `camelCase`, files `Name.h`/`Name.cpp` in the relevant module folder.
- Practices: Use `const`/`const&`, avoid raw pointers when not needed, use `nullptr`. Keep diffs minimal and consistent with nearby code.

## Testing Guidelines
- Current: No unit tests in tree; validate by running the app and exercising Board/Web/Document views.
- Adding tests: Use Qt Test or GoogleTest; mirror `src/` structure under `tests/`. Example: `core_SettingsTest.cpp`.
- Running tests: Integrate with CMake and invoke your framework (e.g., `ctest`) if added.

## Commit & Pull Request Guidelines
- Commits: Imperative, concise (<72 chars), scoped (e.g., `core:`), reference issues (`#123`).
- PRs: Include a clear description, linked issues, platforms tested (Windows/macOS/Linux), reproduction and verification steps, and screenshots/GIFs for UI changes.
- CI/Build: Ensure CMake config completes and the app launches (`./build/openboard`).

## Security & Configuration Tips
- Dependencies: OpenSSL, Poppler, QuaZip, FFmpeg, PipeWire (Linux), libevdev (Linux).
- CMake: `-DQT_VERSION=6` selects Qt6; set `-DCMAKE_INSTALL_PREFIX=/usr` or `/opt` for packaging.
- Provide system package paths as needed for local builds.

