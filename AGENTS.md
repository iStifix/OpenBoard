# Repository Guidelines

## Project Structure & Module Organization
- `src/`: C++ sources by module — `board/`, `core/`, `gui/`, `web/`. Main target: `openboard` (CMake).
- `resources/`: Qt assets (`.qrc`, images, `.ui`, `i18n`, styles).
- `plugins/`: Optional extensions (e.g., `cffadaptor`).
- `cmake/`: Dependency finders and versioning helpers.
- Build metadata: `CMakeLists.txt`, `OpenBoard.pro`, `version.txt`, `release_scripts/`.
- `tests/`: Mirror `src/` structure when adding tests (e.g., `tests/core/core_SettingsTest.cpp`).

## Build, Test, and Development Commands
- Configure (Qt6 preferred): `cmake -S . -B build -DQT_VERSION=6 -DCMAKE_BUILD_TYPE=Release`
- Build: `cmake --build build -j$(nproc)`
- Run: `./build/openboard`
- Package (from `build/`): `cpack -G DEB` or `cpack -G RPM`
- Alternative (qmake/Qt Creator): `qmake OpenBoard.pro && make -j$(nproc)`
- Tips: Pass dependency paths to CMake as needed; set `-DCMAKE_INSTALL_PREFIX=/usr` or `/opt` for packaging.

## Coding Style & Naming Conventions
- Language: C++20 with Qt 5.15/6. Prefer Qt types (`QString`, `QVector`), RAII, `const`/`const&`; use `nullptr`; avoid raw pointers where possible.
- Formatting: 4‑space indent; braces on the same line.
- Naming: Classes PascalCase (often `UB…`); methods/variables camelCase; files `Name.h`/`Name.cpp` in the module folder.
- Diffs: Keep minimal, focused, and consistent with nearby code.

## Testing Guidelines
- Current state: no unit tests committed; validate manually in Board/Web/Document views.
- Adding tests: use Qt Test or GoogleTest; mirror `src/` under `tests/`; wire into CMake and run via `ctest` in `build/`.

## Commit & Pull Request Guidelines
- Commits: imperative, concise (<72 chars), scoped prefix (e.g., `core:`), reference issues (`#123`).
- PRs: include a clear description, linked issues, platforms tested (Windows/macOS/Linux), repro/verification steps, and screenshots/GIFs for UI changes.
- CI/build expectation: CMake config completes and the app launches (`./build/openboard`).

## Security & Configuration Tips
- Dependencies: OpenSSL, Poppler, QuaZip, FFmpeg, PipeWire (Linux), libevdev (Linux).
- Qt: `-DQT_VERSION=6` selects Qt6; supply system/library paths when required.
