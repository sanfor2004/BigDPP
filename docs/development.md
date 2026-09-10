# Development Guide

This guide covers the local configure, build, test, and run loop. For code flow,
read [Codebase Tour](codebase-tour.md); for change recipes, read
[Editing and Extension Guide](extending.md).

## Supported baseline

- C++20
- CMake 3.25 or newer
- vcpkg manifest mode
- MSVC on Windows
- GCC or Clang intended for the future Linux container/CI path
- Ninja or another CMake generator with equivalent target support

The Windows toolchain is verified locally. Linux/compiler support is not final
until container and CI verification is added.

## Dependencies

The checked-in `vcpkg.json` declares dependencies and pins resolution with a
vcpkg baseline.

| Dependency | Current purpose | CMake target |
| --- | --- | --- |
| DPP / D++ 10.1.5 at the verified local baseline | Discord Gateway and REST API | `dpp::dpp` |
| spdlog | application and DPP logging | `spdlog::spdlog` |
| Catch2 3 | offline unit tests and CTest discovery | `Catch2::Catch2WithMain` |

libpqxx is planned for Phase 2 and is not currently declared. Do not add Redis.
See [Editing and Extension Guide](extending.md#add-a-library-dependency) before
introducing another package.

## Windows prerequisites

Install Visual Studio Build Tools with the C++ workload, CMake, Ninja, and a
vcpkg installation. MSVC also needs the Visual Studio SDK environment; merely
having `cl.exe` on `PATH` is not sufficient.

Open a configured developer shell:

```powershell
cmd.exe /k VsDevCmd.bat -arch=x64 -host_arch=x64
```

Inside that shell, verify the tools:

```bat
cl
cmake --version
ninja --version
```

The compiler printing its banner and then reporting that no source files were
supplied is a successful availability check.

## Configure

Set `VCPKG_ROOT` to the vcpkg installation, then configure an out-of-source
Debug build:

```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows
```

On the currently verified workstation, Visual Studio's bundled vcpkg root is:

```text
C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\vcpkg
```

That machine-specific path is documentation only and must not be embedded in
CMake files. The first configure may take time because vcpkg builds the manifest
dependencies.

Useful configuration options:

| Option | Default | Effect |
| --- | --- | --- |
| `BIGDPP_BUILD_TESTS` | `ON` | Builds `bigdpp_tests`, finds Catch2, and registers tests with CTest. |
| `CMAKE_BUILD_TYPE` | generator-dependent | Use `Debug` for development or `Release` for an optimized single-config build. |
| `VCPKG_TARGET_TRIPLET` | environment-dependent | Selects the dependency ABI, such as `x64-windows`. |

If changing the generator, compiler, toolchain file, or triplet, use a different
build directory (for example `build-msvc-debug`) instead of reusing an
incompatible CMake cache.

## Build

```powershell
cmake --build build
```

Build only one target when useful:

```powershell
cmake --build build --target bigdpp_tests
cmake --build build --target bigdpp
```

Project targets compile with `/W4 /permissive-` on MSVC and
`-Wall -Wextra -Wpedantic` on other compilers. Resolve warnings introduced by a
change before handing it off.

## Test

Run the full default offline suite:

```powershell
ctest --test-dir build --output-on-failure
```

List discovered tests without running them:

```powershell
ctest --test-dir build -N
```

Run matching tests:

```powershell
ctest --test-dir build -R configuration --output-on-failure
```

Catch2 registers each `TEST_CASE` as an individual CTest test. The current suite
tests configuration and DPP-free status formatting. It requires no token,
network, Discord server, or PostgreSQL instance.

Networked integration and live Discord smoke tests must remain opt-in and must
not be added to the default unit-test command.

## Configure the bot

Create the ignored local settings file:

```powershell
Copy-Item .env.example .env
```

Set `DISCORD_TOKEN` and preferably `DEVELOPMENT_GUILD_ID` for a dedicated test
guild. See [Configuration Reference](configuration.md) for parsing and
precedence, then complete [Discord Application Setup](discord-setup.md).

Never commit `.env` or put a token on a command line that may be saved in shell
history.

## Run

Run from the repository root so the executable reads `./.env`:

```powershell
.\build\bigdpp.exe
```

Expected successful startup includes logs for initialization, Gateway startup,
the ready event, and command registration. The process waits in the DPP Gateway
loop until stopped. Use `Ctrl+C`; graceful signal handling is still a planned
hardening item.

Do not use production credentials for routine development. Follow the manual
checks in [Discord Application Setup](discord-setup.md) for `/ping` and
`/status`.

## Everyday edit loop

1. Check `git status` and inspect the relevant module.
2. Make one coherent source/test/documentation change.
3. Re-run CMake configure if sources, dependencies, or CMake logic changed.
4. Build the affected target.
5. Run focused tests, then the complete default suite.
6. Review the diff for secrets, generated files, unrelated changes, and stale
   implemented/planned claims.

Formatting is defined by `.clang-format`. Use a compatible `clang-format` on
changed C++ files when available; do not mechanically reformat unrelated code.

## Build-tree policy

Keep generated artifacts out of Git:

- `build/`, `build-*`, and `out/`
- `vcpkg_installed/`
- compiler objects, executables, debug symbols, and CMake cache files
- test, coverage, and runtime output
- `.env`, secrets, and local database data

Do not hand-edit generated build files. Edit `CMakeLists.txt` or `vcpkg.json`,
then configure again.

## Troubleshooting

### CMake cannot find a package

Confirm that the vcpkg toolchain file was supplied during the first configure
and that `VCPKG_ROOT` points to a complete vcpkg installation. Delete nothing
blindly; use a new build directory if the existing cache was configured without
the toolchain or with a different triplet.

### MSVC cannot find standard headers or libraries

Start from a Visual Studio developer shell using `VsDevCmd.bat`. A normal
PowerShell session may find `cl.exe` yet still lack SDK variables.

### The executable says `DISCORD_TOKEN` is required

Run from the repository root, confirm `.env` exists there, and confirm the value
is non-empty. Process environment values take precedence only when non-empty.

### Slash commands do not appear

Set a valid `DEVELOPMENT_GUILD_ID`, ensure the application was installed in that
guild with `applications.commands`, and inspect the registration completion log.
Global registration can take longer to become visible.

### Tests unexpectedly read local configuration

The unit tests call `Config::load()` with an injected environment lookup and a
temporary dotenv path. New configuration tests should keep using this seam and
must not depend on the developer's real `.env`.
