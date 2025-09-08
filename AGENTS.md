# Repository Guidelines

## Project Structure & Module Organization
- `src/`: C++ sources by module — `board/`, `core/`, `gui/`, `web/`. Executable target: `openboard` (CMake).
- `resources/`: Qt assets (`.qrc`, images, `.ui` forms, `i18n`, styles).
- `plugins/`: Optional plugins (e.g., `cffadaptor`).
- `cmake/`: CMake helpers for dependencies and versioning.
- Build metadata: `CMakeLists.txt`, `OpenBoard.pro`, `version.txt`, `release_scripts/`.
- `tests/`: Mirror `src/` when adding tests (e.g., `tests/core/core_SettingsTest.cpp`).

## Build, Test, and Development Commands
- Configure (Qt6 preferred): `cmake -S . -B build -DQT_VERSION=6 -DCMAKE_BUILD_TYPE=Release`
- Build: `cmake --build build -j$(nproc)`
- Run locally: `./build/openboard`
- Package (from `build/`): `cpack -G DEB` or `cpack -G RPM`
- Alternative (qmake/Qt Creator): `qmake OpenBoard.pro && make -j$(nproc)`
- Tips: Provide dependency paths to CMake if required; set `-DCMAKE_INSTALL_PREFIX=/usr` or `/opt` for packaging.

## Coding Style & Naming Conventions
- C++20 with Qt 5.15/6; prefer Qt types (`QString`, `QVector`), RAII, and `const`/`const&`; use `nullptr`; avoid raw pointers when possible.
- Indentation 4 spaces; braces on the same line.
- Naming: Classes PascalCase (often `UB…`), methods/variables camelCase; files `Name.h`/`Name.cpp` within the module folder.
- Keep diffs minimal and consistent with nearby code.

## Testing Guidelines
- Current: no unit tests checked in; validate by launching and exercising Board/Web/Document views.
- Adding tests: use Qt Test or GoogleTest; mirror `src/` under `tests/`; integrate with CMake and run via `ctest` from `build/`.

## Commit & Pull Request Guidelines
- Commits: imperative, concise (<72 chars), scoped prefix (e.g., `core:`), reference issues (`#123`).
- PRs: include clear description, linked issues, platforms tested (Windows/macOS/Linux), reproduction and verification steps, and screenshots/GIFs for UI changes.
- CI/build expectation: CMake configuration completes and the app launches (`./build/openboard`).

## Security & Configuration Tips
- Dependencies: OpenSSL, Poppler, QuaZip, FFmpeg, PipeWire (Linux), libevdev (Linux).
- CMake: `-DQT_VERSION=6` selects Qt6; supply system paths where needed.

