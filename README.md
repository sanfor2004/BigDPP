# BigDPP

A C++ Discord operations bot for server maintenance, moderation, diagnostics, announcements, logging, and administrative automation.

## Project status

BigDPP is in **Phase 1 — Discord Foundation**. The native foundation and initial DPP integration are implemented; live Discord verification awaits local credentials.

## Direction

BigDPP targets C++20, DPP/D++, CMake, PostgreSQL/libpqxx, spdlog, and Docker. It is designed as a production-oriented modular monolith for a real Discord server, with read-only diagnostics preceding any administrative mutation.

## Documentation

- [Start here: codebase tour](docs/codebase-tour.md)
- [Architecture](docs/architecture.md)
- [Development guide](docs/development.md)
- [Editing and extension guide](docs/extending.md)
- [Configuration reference](docs/configuration.md)
- [Discord application setup](docs/discord-setup.md)
- [Security and operations baseline](docs/security-and-operations.md)
- [Delivery roadmap](docs/roadmap.md)
- [Repository working instructions](AGENTS.md)

If you are new to the project, read the codebase tour first, follow the
development guide to build and test, then use the editing guide before adding a
command or module.

## Implemented

- C++20 CMake executable and reusable core target
- environment-backed configuration with validation
- spdlog initialization
- Catch2/CTest configuration tests
- pinned vcpkg manifest
- DPP 10.1.5 Gateway adapter using only the standard Guilds intent
- command registry/router with contained error handling
- `/ping` and `/status` commands
- development-guild or global bulk command registration
- secure ignored `.env` fallback with process-environment precedence
- project scope, architectural boundaries, phased roadmap, and development/security policies

## Planned

- live test-guild verification of Phase 1
- remaining container and CI verification
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

Before starting the bot, follow the [Discord application setup](docs/discord-setup.md) and put the token in the ignored `.env` file. Never commit the token.

## Commands

| Command | Status | Purpose |
| --- | --- | --- |
| `/ping` | Implemented | Verify that BigDPP responds |
| `/status` | Implemented | Show version, uptime, guild/member cache data, and latency |
