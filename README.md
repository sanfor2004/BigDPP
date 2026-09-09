# BigDPP

A C++ Discord operations bot for server maintenance, moderation, diagnostics, announcements, logging, and administrative automation.

## Project status

BigDPP is in **Phase 0 — Repository Foundation**. The native CMake, configuration, logging, and testing foundation is implemented; bot features are not implemented yet.

## Direction

BigDPP targets C++20, DPP/D++, CMake, PostgreSQL/libpqxx, spdlog, and Docker. It is designed as a production-oriented modular monolith for a real Discord server, with read-only diagnostics preceding any administrative mutation.

## Documentation

- [Architecture](docs/architecture.md)
- [Delivery roadmap](docs/roadmap.md)
- [Development guide](docs/development.md)
- [Security and operations baseline](docs/security-and-operations.md)
- [Repository working instructions](AGENTS.md)

## Implemented

- C++20 CMake executable and reusable core target
- environment-backed configuration with validation
- spdlog initialization
- Catch2/CTest configuration tests
- pinned vcpkg manifest
- project scope, architectural boundaries, phased roadmap, and development/security policies

## Planned

- remaining Phase 0 container and CI verification
- Discord connection and basic commands
- PostgreSQL persistence and migrations
- Read-only server diagnostics
- Moderation, announcements, audit events, and advanced maintenance in later phases

See the [roadmap](docs/roadmap.md) for phase acceptance criteria. Planned features are intentionally not presented as available commands.

## Build on Windows

Open a new terminal, then activate the Visual Studio developer environment:

```powershell
cmd.exe /k VsDevCmd.bat -arch=x64 -host_arch=x64
```

In that developer shell, set `VCPKG_ROOT` to your vcpkg installation and run:

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build
ctest --test-dir build --output-on-failure
build\bigdpp.exe
```

On the current development machine, Visual Studio's bundled vcpkg root is `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\vcpkg`.
