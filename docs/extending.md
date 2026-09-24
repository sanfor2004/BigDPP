# Editing and Extension Guide

Keep each change small enough that the repository remains buildable. Check the
current phase in [the roadmap](roadmap.md) before adding behavior from a later
phase.

## Current boundaries

```text
Discord callbacks -> Bot adapter -> DPP
Bot adapter -> Local LLM adapter -> Ollama (loopback only)
startup -> Config -> concrete adapters
```

The domain and application layers do not exist yet. Do not put SQL, blocking
work, or large business rules in Discord callbacks. The local LLM is prompt-only
and must not gain Discord credentials or server-management tools implicitly.

## Add a Discord command

Add administrator command definitions and dispatch in `src/admin.cpp` and
`include/bigdpp/admin.hpp`; keep `src/bot.cpp` focused on composition and
foundation/AI dispatch. Then update the command table in `README.md` and the
Discord setup documentation. Keep callbacks asynchronous and non-blocking.
If a command develops reusable validation or business rules, first introduce a
DPP-free application type and tests rather than growing the Discord adapter.

Every command must:

- validate its inputs and reply safely on expected failure;
- avoid logging tokens, IDs that are not needed for diagnosis, or raw errors;
- catch exceptions at the command boundary;
- request no broader Discord permission or intent than its behavior needs.

## Add configuration

Add the field to `Config` or `LocalLlmConfig` in
`include/bigdpp/config.hpp`, parse it in `src/config.cpp`, and add validation
tests in `tests/config_tests.cpp` using `Config::load_from()` with an injected
environment lookup. Add only a safe placeholder to `.env.example`, then update
`docs/configuration.md` and any affected setup guide.

Process environment values take precedence over `.env`. Never add a real token,
password, connection string, guild ID, channel ID, role ID, or user ID to the
repository.

## Add a native dependency

Verify the official API and CMake target, confirm availability at the pinned
vcpkg baseline, add it to `vcpkg.json`, and link it only to the target that needs
it. Document its purpose in `docs/development.md`. The current LLM adapter uses
nlohmann-json and DPP's asynchronous HTTP request queue; it does not need a
separate native HTTP library.

## Verification

For native changes, configure with the repository preset, build `bigdpp`, and
run CTest:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --preset windows-debug
```

Live Discord and networked integration tests are opt-in. Do not require a token,
Discord server, or PostgreSQL instance in the default unit suites.
