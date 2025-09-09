# Repository Guidelines

## Project Structure & Module Organization
- `src/`: C++ sources by module — `board/`, `core/`, `gui/`, `web/`. Main target: `openboard` (CMake).
- `resources/`: Qt assets (`.qrc`, images, `.ui`, `i18n`, styles).
- `plugins/`: Optional extensions (e.g., `cffadaptor`).
- `cmake/`: Dependency finders and version helpers.
- Build metadata: `CMakeLists.txt`, `OpenBoard.pro`, `version.txt`, `release_scripts/`.
- `tests/`: Mirror `src/` when adding tests (e.g., `tests/core/core_SettingsTest.cpp`).

## Build, Test, and Development Commands
- Configure: `cmake -S . -B build -DQT_VERSION=6 -DCMAKE_BUILD_TYPE=Release`
  - Use Qt6 by default; pass dependency paths as needed.
- Build: `cmake --build build -j$(nproc)`
- Run: `./build/openboard`
- Package (from `build/`): `cpack -G DEB` or `cpack -G RPM`
- Alternative (qmake): `qmake OpenBoard.pro && make -j$(nproc)`
- Optional prefix for packaging: `-DCMAKE_INSTALL_PREFIX=/usr` or `/opt`.

## Coding Style & Naming Conventions
- Language: C++20 with Qt 5.15/6. Prefer Qt types (`QString`, `QVector`), RAII, `const`/`const&`; use `nullptr`; avoid raw pointers.
- Formatting: 4‑space indent; braces on the same line.
- Naming: Classes PascalCase (often `UB…`); methods/variables camelCase.
- Files: `Name.h`/`Name.cpp` inside the appropriate module folder.
- Diffs: Keep minimal, focused, and consistent with nearby code.

## Testing Guidelines
- Frameworks: Qt Test or GoogleTest.
- Layout: Mirror `src/` under `tests/`; name tests like `module_ComponentTest.cpp`.
- Enable and run: `cmake -S . -B build -DBUILD_TESTING=ON && cmake --build build && (cd build && ctest -V)`.
- Current state: no unit tests committed; validate manually in Board/Web/Document views.

## Commit & Pull Request Guidelines
- Commits: imperative, concise (<72 chars), scoped prefix (e.g., `core:`), reference issues (`#123`).
- PRs: clear description, linked issues, platforms tested (Windows/macOS/Linux), repro/verification steps, and screenshots/GIFs for UI changes.
- CI/build expectation: CMake config completes and the app launches (`./build/openboard`).

## Security & Configuration Tips
- Dependencies: OpenSSL, Poppler, QuaZip, FFmpeg, PipeWire (Linux), libevdev (Linux).
- Qt: `-DQT_VERSION=6` selects Qt6; supply system/library paths when required.
- Packaging: set `-DCMAKE_INSTALL_PREFIX=/usr` or `/opt` for system installs.

