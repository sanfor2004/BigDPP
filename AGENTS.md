# BigDPP Repository Instructions

These instructions apply to the entire repository.

## Mission

Build BigDPP as a production-oriented, long-running Discord operations bot for a real server of approximately 450 members. Prefer a reliable modular monolith over a demo-style collection of callbacks or premature distributed services.

## Current phase

The repository is in Phase 1: Discord Foundation. Do not implement moderation, announcements, PostgreSQL persistence, or server diagnostics until the Discord foundation is buildable and verified.

Before changing code:

1. Inspect the repository and current phase.
2. Select the smallest coherent change.
3. Check affected architectural boundaries.
4. Verify third-party APIs against the installed or selected version.

After changing code:

1. Configure and build the project.
2. Run relevant tests.
3. Resolve compiler warnings introduced by the change.
4. Update documentation when behavior, setup, or architecture changes.
5. Report changed files, verification results, blockers, and the next step.

Every implementation step must leave the repository buildable.

## Technology constraints

- Use C++20 unless a later documented decision raises the minimum.
- Use target-based modern CMake. Do not set global include or link directories.
- Use DPP/D++ for Discord integration.
- Use PostgreSQL through libpqxx when persistence begins.
- Use spdlog for application logging.
- Use Docker Compose for the production-like local stack.
- Use vcpkg manifest mode for declared C++ dependencies unless an ADR documents a change.
- Do not add Redis until a measured requirement exists.
- Do not add a web dashboard, microservices, Kubernetes, music, economy, games, leveling, or AI-chat features during the initial roadmap.

Do not introduce a dependency without documenting its purpose and confirming its maintained API and CMake target.

## Architecture boundaries

Keep dependencies directed inward:

```text
Discord handlers/commands -> application services -> domain
database/Discord adapters -> application ports and domain
main/composition root -> all concrete implementations
```

- Domain code must not include DPP, pqxx, spdlog, or operating-system headers.
- Discord handlers translate DPP events into application inputs and format application results.
- Commands must not contain SQL or substantial business rules.
- SQL belongs in PostgreSQL repository implementations and migrations.
- Application services coordinate use cases and depend on narrow interfaces at infrastructure boundaries.
- `main.cpp` is the composition root and should remain small.
- Keep event callbacks non-blocking. Do not perform blocking database or long-running work on a Discord event thread.
- Avoid global mutable state, owning raw pointers, and singleton service locators.
- Use RAII and explicit ownership. Prefer values and references; use smart pointers only when ownership semantics require them.
- Catch failures at command, event, job, and process boundaries so one failed operation cannot terminate the bot.

Add abstractions when they isolate a real boundary, enable focused testing, or support more than one concrete behavior. Do not create pass-through layers for appearance.

## Security and safety

- Never commit or log tokens, passwords, connection strings, or production IDs.
- Load deployment secrets from environment variables and validate required configuration at startup.
- Use parameterized SQL exclusively.
- Enforce caller permission, bot permission, target validity, and role hierarchy inside the application before moderation actions.
- Request only required Discord intents and permissions; never default to Administrator.
- Prevent accidental mass mentions. Allowed-mention behavior must be explicit.
- Diagnostics are read-only by default. Any destructive maintenance action requires admin review and explicit confirmation.
- Do not hardcode guild, channel, role, or user IDs.

## Coding conventions

- Use the `bigdpp` namespace and lower-case source filenames.
- Put public headers under `include/bigdpp/<module>/` and implementations under the matching `src/<module>/` directory.
- Prefer cohesive types, meaningful names, `const`, scoped enums, and `std::chrono` for time and durations.
- Keep Discord snowflakes out of the domain as DPP types; use a project-owned ID type or value representation at the boundary.
- Represent expected validation failures as typed results/errors. Reserve exceptions for exceptional failures and translate them at system boundaries.
- Do not block asynchronous code with `sleep()` for scheduling.
- Comments should explain decisions, invariants, and non-obvious behavior—not restate code.
- Format consistently and keep warnings enabled for project-owned targets.

## Testing rules

- Test domain rules and application services without connecting to Discord or PostgreSQL.
- Use fakes at application ports and focused integration tests for adapters.
- Any bug fix should add a regression test when practical.
- Time-dependent behavior must use an injectable clock.
- Scheduler tests must not depend on wall-clock sleeps.
- Networked integration tests must be opt-in and must not run in the default unit-test suite.

## Documentation truthfulness

Separate implemented, planned, and experimental behavior. Do not describe planned commands as available. Update `docs/roadmap.md` when a phase changes state and create an ADR under `docs/decisions/` for consequential architectural changes.

## Git hygiene

- Preserve unrelated user changes.
- Keep generated build trees, vcpkg installations, logs, secrets, and local database data out of Git.
- Do not combine unrelated refactors with feature work.
