# Repository Guidelines

## Project Structure & Module Organization
- `src/`: C++ sources organized by module (e.g., `board/`, `core/`, `gui/`, `web/`). Executable target is lowercase `openboard` (CMake).
- `resources/`: Qt assets (`.qrc`, images, `.ui` forms, `i18n` translations, styles).
- `plugins/`: Optional plugin code (e.g., `cffadaptor`).
- `cmake/`: Dependency and version helpers used by the top‑level `CMakeLists.txt`.
- Build metadata: `CMakeLists.txt`, `OpenBoard.pro` (Qt/qmake), `version.txt`, `release_scripts/`.

## Build, Test, and Development Commands
- Configure (CMake, Qt6 preferred):
  - `cmake -S . -B build -DQT_VERSION=6 -DCMAKE_BUILD_TYPE=Release`
- Build:
  - `cmake --build build -j$(nproc)`
- Run locally (from repo root after build):
  - `./build/openboard`
- Package (from `build/`):
  - `cpack -G DEB` or `cpack -G RPM`
- Alternative (Qt Creator/qmake):
  - `qmake OpenBoard.pro && make -j$(nproc)`

## Coding Style & Naming Conventions
- Language: C++20 with Qt 5.15/6 APIs; prefer Qt types (`QString`, `QVector`).
- Indentation: 4 spaces; braces on the same line as declarations.
- Naming: Classes PascalCase (often `UB…` prefix), methods/variables camelCase, files pair as `Name.h`/`Name.cpp` within the relevant module folder.
- Const‑correctness: prefer `const` and `const&` parameters; use `nullptr`.
- Formatting: no enforced tool in repo; keep diffs minimal and consistent with nearby code.

## Testing Guidelines
- Frameworks: No unit tests currently in tree. Validate changes by running the app and exercising affected tools (Board, Web, Document views).
- If adding tests, mirror structure under `tests/` and use Qt Test or GoogleTest; name tests after the class/module (e.g., `core_SettingsTest.cpp`).

## Commit & Pull Request Guidelines
- Commits: Imperative present tense, concise title (<72 chars). Optionally tag scope (e.g., `core: …`) and reference issues (`#123`).
- PRs: Include a clear description, linked issues, platforms tested (Windows/macOS/Linux), reproduction and verification steps, and screenshots/GIFs for UI changes.
- CI/Build: Ensure CMake config completes and the app launches (`./build/openboard`).

## Security & Configuration Tips
- Dependencies: OpenSSL, Poppler, QuaZip, FFmpeg, PipeWire (Linux), libevdev (Linux). Provide platform packages or paths as needed.
- CMake options: `-DQT_VERSION=6` selects Qt6; set `-DCMAKE_INSTALL_PREFIX=/usr` or `/opt` for packaging.
