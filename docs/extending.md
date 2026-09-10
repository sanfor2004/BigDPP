# Editing and Extension Guide

Use this guide when changing BigDPP. Keep each change small enough that the
repository remains buildable, and do not implement features from later roadmap
phases while Phase 1 verification remains open.

## Before editing

1. Read [Architecture](architecture.md) and check the current phase in the
   [Delivery Roadmap](roadmap.md).
2. Inspect `git status` and preserve unrelated work.
3. Identify the boundary being changed: domain/application, Discord adapter,
   infrastructure, or composition root.
4. Confirm the installed dependency version and its maintained API before using
   a new DPP, spdlog, Catch2, or CMake feature.
5. Choose the smallest coherent change and decide how it will be verified.

## Add a slash command

For a simple immediate command, add a final class under
`include/bigdpp/discord/commands/` and its implementation under the matching
`src/discord/commands/` path.

```cpp
// include/bigdpp/discord/commands/example_command.hpp
#pragma once

#include "bigdpp/discord/command.hpp"

namespace bigdpp::discord {

class ExampleCommand final : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] dpp::slashcommand definition(dpp::snowflake application_id) const override;
    void execute(const dpp::slashcommand_t& event) const override;
};

} // namespace bigdpp::discord
```

```cpp
// src/discord/commands/example_command.cpp
#include "bigdpp/discord/commands/example_command.hpp"

namespace bigdpp::discord {

std::string_view ExampleCommand::name() const noexcept {
    return "example";
}

dpp::slashcommand ExampleCommand::definition(const dpp::snowflake application_id) const {
    return dpp::slashcommand{"example", "Describe the command", application_id};
}

void ExampleCommand::execute(const dpp::slashcommand_t& event) const {
    event.reply("Example response");
}

} // namespace bigdpp::discord
```

Then make three wiring changes:

1. Add the `.cpp` file to `bigdpp_discord` in `CMakeLists.txt`.
2. Include the command header in `src/discord/bot.cpp`.
3. Construct it with `router_.add(std::make_unique<ExampleCommand>());` in
   `Bot::Impl`.

The string returned by `name()` must equal the slash-command name in
`definition()`. Duplicate router names fail during startup. Command definitions
are bulk-registered, so removing a handler also removes that command from the
selected registration scope the next time registration succeeds.

Keep DPP-specific extraction and reply formatting in the handler. If the command
contains rules or reusable behavior, move that behavior into an application
service or DPP-free function and inject it into the handler. Do not put SQL,
substantial authorization rules, or blocking work in `execute()`.

## Add application behavior

Application code belongs in:

```text
include/bigdpp/application/<feature>.hpp
src/application/<feature>.cpp
tests/<feature>_tests.cpp
```

Application APIs should use project-owned values and typed results rather than
DPP objects. This makes rules testable without Discord. Add the implementation
to `bigdpp_core`; add the test source to `bigdpp_tests`.

Use `application/status.*` as the current example: the Discord handler gathers
adapter data, while a DPP-free function formats it and is covered by unit tests.

If behavior depends on time, accept a clock or time value rather than calling
the wall clock deep inside the service. Tests must not wait with sleeps.

## Add configuration

When a new implemented feature needs configuration:

1. Add a field to `Config` in `include/bigdpp/config/config.hpp`.
2. Read it in `Config::load()` and validate it in `Config::validate()` or in a
   narrowly named `require_*` accessor when it is required only by one runtime
   component.
3. Add precedence, valid, invalid, and missing-value tests to
   `tests/config_tests.cpp` as appropriate.
4. Add a safe placeholder to `.env.example`.
5. Update [Configuration Reference](configuration.md) and any affected setup
   guide.

Never add a real token, password, connection string, guild ID, channel ID, role
ID, or user ID to source, tests, examples, or documentation.

## Add a library dependency

Do not introduce a dependency merely to avoid a small amount of project code.
When a concrete capability requires one:

1. Verify the official API and CMake target for the selected version.
2. Confirm that version is available at the pinned vcpkg baseline on Windows
   and Linux.
3. Add it to `vcpkg.json`.
4. Use `find_package(... CONFIG REQUIRED)` and link its imported target to only
   the project target that needs it.
5. Document its purpose in `docs/development.md`; add an ADR under
   `docs/decisions/` if the choice changes architecture or operations.
6. Configure, build, and test from a clean dependency state when practical.

Never set global include or link directories.

## Add a module or infrastructure adapter

Add a new abstraction only when it isolates an external boundary, enables a
focused test, or has more than one meaningful behavior. Keep dependency arrows
directed inward:

```text
Discord command -> application service -> domain/value types
external adapter -> application port and domain/value types
main.cpp -> concrete construction and wiring
```

Domain code must not include DPP, pqxx, spdlog, or operating-system headers.
Future SQL belongs only in PostgreSQL adapters and migrations. Keep `main.cpp`
small: it may construct and connect objects, but it should not implement a use
case.

## Error handling

- Use typed results/errors for expected validation or business failures.
- Reserve exceptions for exceptional failures.
- Catch exceptions at command, event, job, and process boundaries.
- Log enough context to diagnose the operation, but never secrets or raw
  database details.
- Send users actionable, safe messages; do not expose exception text.

The router already provides a last-resort command exception boundary. A handler
may still catch a narrower adapter failure when it can return a more useful safe
response.

## Asynchronous and Discord rules

DPP event callbacks must remain non-blocking. Current commands reply immediately
and do no I/O. For future slow operations:

1. acknowledge or defer the interaction before Discord's deadline
2. move blocking work away from the Gateway callback thread
3. keep ownership explicit for any state captured by asynchronous callbacks
4. translate completion or failure into one final interaction response

Do not use `sleep()` as a scheduler. Persistent scheduling belongs to a later
application service and infrastructure worker with an injectable clock.

## Tests to add

- Put pure rule and application-service tests in the default offline suite.
- Use fakes for narrow application ports.
- Add a regression test with a bug fix when practical.
- Keep networked Discord/PostgreSQL tests opt-in and out of default CTest.
- Assert behavior, not DPP implementation details, whenever the application
  boundary permits it.

Catch2 test cases are automatically exposed through CTest. New test `.cpp` files
must still be listed in the `bigdpp_tests` target.

## Documentation to update

Update documentation in the same change when behavior, setup, configuration, or
architecture changes:

- user-visible commands: `README.md`, this guide if the pattern changes, and
  Discord setup/smoke instructions
- configuration: `.env.example` and `docs/configuration.md`
- architectural decisions: `docs/architecture.md` and, when consequential, an
  ADR under `docs/decisions/`
- phase status: `docs/roadmap.md`
- operational or permission changes: `docs/security-and-operations.md` and
  `docs/discord-setup.md`

Describe planned behavior as planned. Do not list a command as available until
its implementation and verification state are clear.

## Required verification

From a configured developer shell:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build
ctest --test-dir build --output-on-failure
```

For documentation-only edits, the same configure/build/test sequence confirms
that documented commands and repository state have not drifted. Run a live
Discord smoke test only when credentials and a dedicated test guild are
available; it is never part of the default unit suite.

Before handing off, report changed files, configure/build/test results, warnings
introduced, blockers, and the smallest recommended next step.
