# Development Guide

## Supported baseline

- C++20
- CMake with a target-based configuration
- MSVC on Windows; GCC or Clang in the Linux container/CI
- Ninja or a Visual Studio CMake generator
- vcpkg manifest mode for C++ dependencies

The exact minimum compiler and CMake versions will be recorded after Phase 0 is verified in CI.

## Current Windows toolchain

This workstation has Visual Studio Build Tools 2026 with MSVC 14.51.36231. The persistent user `PATH` includes:

- the x64-hosted MSVC compiler directory
- Visual Studio developer-command tools
- Visual Studio's bundled CMake
- Visual Studio's bundled Ninja

Open a new terminal to receive the updated `PATH`.

`cl.exe` also needs Visual Studio's SDK include and library environment. Start a configured shell before compiling:

```powershell
cmd.exe /k VsDevCmd.bat -arch=x64 -host_arch=x64
```

Inside that developer shell, these checks should succeed:

```bat
cl
cmake --version
ninja --version
```

The compiler printing its banner and then reporting that no source files were supplied is a successful availability check.

## Native build flow

Set `VCPKG_ROOT` to a vcpkg installation, then configure with its toolchain. On Windows:

```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build
ctest --test-dir build --output-on-failure
```

On the current workstation, Visual Studio's bundled vcpkg root is `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\vcpkg`. Build presets may replace repeated command-line options once portable Windows and Linux presets can be verified. Machine-specific absolute paths must not be committed.

## Dependency strategy

Use a checked-in `vcpkg.json` manifest and a pinned registry baseline for reproducibility. Add dependencies only in the phase that uses them:

| Dependency | Earliest phase | Purpose |
| --- | ---: | --- |
| spdlog | 0 | application logging |
| test framework | 0 | unit tests and CTest discovery |
| DPP / D++ | 1 (or a Phase 0 link smoke test) | Discord Gateway and REST API |
| libpqxx | 2 | PostgreSQL client |

DPP's supported CMake package target is expected to be `dpp::dpp`; verify it against the selected vcpkg baseline before committing the manifest. Do not add Redis or a `.env` parsing library by default. Production configuration comes from the process environment; Docker Compose may use a local uncommitted env file to populate it.

## Configuration

The initial required variables are expected to be:

```text
DISCORD_TOKEN=
DATABASE_URL=
LOG_LEVEL=info
ENVIRONMENT=development
```

Requirements become phase-aware: Phase 0 tests configuration validation without contacting services, Phase 1 requires `DISCORD_TOKEN`, and Phase 2 requires `DATABASE_URL`. Never put a real secret in `.env.example`, test fixtures, command history, logs, screenshots, or documentation.

## Build-tree policy

Use out-of-source build directories such as `build/`. Do not commit:

- compiler output
- CMake cache/generated files
- `vcpkg_installed/`
- downloaded tools or dependency source trees
- logs, coverage output, or test artifacts
- `.env` or credentials

## Test categories

- **Unit:** default, fast, no network, no Discord, no PostgreSQL.
- **Integration:** opt-in adapter tests, including PostgreSQL with isolated data.
- **Live smoke:** manual/controlled test-guild checks; never part of the default test command.

## Adding a dependency

Before adding one:

1. Identify the concrete capability it supplies.
2. Check the official maintained API and CMake target.
3. Verify Windows and Linux availability in the selected vcpkg baseline.
4. Prefer a narrow, mature dependency over a broad framework.
5. Record meaningful trade-offs in `docs/decisions/`.
6. Build and test from a clean dependency state when practical.
