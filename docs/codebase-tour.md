# Codebase Tour

This guide explains the code that exists today. BigDPP is currently a Phase 1
Discord foundation: it loads local configuration, initializes logging, connects
to Discord through DPP, registers two slash commands, and routes interactions to
their handlers. PostgreSQL, moderation, announcements, and server diagnostics
are planned but are not implemented.

## Repository map

```text
BigDPP/
|-- CMakeLists.txt                 Build targets and test registration
|-- vcpkg.json                    Pinned C++ dependency manifest
|-- .env.example                  Safe local configuration template
|-- include/bigdpp/
|   |-- application/              DPP-free application behavior
|   |-- config/                   Configuration public API
|   |-- discord/                  Discord adapter public API and commands
|   `-- logging/                  Logging public API
|-- src/
|   |-- application/              Application implementations
|   |-- config/                   Environment and .env loading
|   |-- discord/                  DPP cluster, routing, and commands
|   |-- logging/                  spdlog setup
|   `-- main.cpp                  Composition root and process boundary
|-- tests/                        Fast, offline Catch2 tests
`-- docs/                         Architecture, setup, editing, and operations guides
```

Public headers live under `include/bigdpp/<module>/`; implementations live in
the matching `src/<module>/` directory. All project code uses the `bigdpp`
namespace.

## Executable startup flow

`src/main.cpp` is the composition root. Startup follows this sequence:

```text
main
  -> Config::from_environment()
       -> parse .env in the current working directory
       -> overlay real process environment variables
       -> normalize and validate settings
  -> logging::initialize(LOG_LEVEL)
  -> construct discord::Bot
       -> require DISCORD_TOKEN
       -> create dpp::cluster with Guilds intent
       -> construct and register command handlers
       -> attach DPP callbacks
  -> Bot::run()
       -> start the Gateway connection and wait
```

The process returns `0` after a normal DPP shutdown. A `ConfigError` or other
startup exception is caught in `main`, logged as critical, and returns `1`.
The token is consumed by DPP but is never deliberately written to a log.

## Configuration module

Files:

- `include/bigdpp/config/config.hpp`
- `src/config/config.cpp`

`Config::from_environment()` calls `Config::load()` with the process environment
and a `.env` path relative to the current working directory. The small built-in
dotenv reader supports:

- blank lines and full-line comments beginning with `#`
- `KEY=value` entries
- surrounding whitespace
- values surrounded by matching single or double quotes

It does not implement shell expansion, `export KEY=...`, multiline values, or
inline comments. A non-empty process variable wins over the same key in `.env`.
An absent or empty value is treated as unset.

`Config::validate()` checks the environment name, log level, and optional
development guild ID. The Discord token is required only when `Bot` is
constructed, via `require_discord_token()`. `DATABASE_URL` is loaded but unused
until the PostgreSQL phase.

See [Configuration Reference](configuration.md) for the complete variable table.

## Logging module

Files:

- `include/bigdpp/logging/logging.hpp`
- `src/logging/logging.cpp`

`logging::initialize()` maps the configured level through spdlog and installs a
timestamped text pattern. DPP log events are translated to matching spdlog
levels in `src/discord/bot.cpp`. Logs currently cover startup, Gateway state,
command registration, command dispatch, and caught failures.

Do not add configuration dumps, interaction payload dumps, authorization
headers, or message content to logs. Future structured context should identify
the operation and result without exposing secrets or unnecessary user data.

## Discord adapter

### Bot ownership and callbacks

Files:

- `include/bigdpp/discord/bot.hpp`
- `src/discord/bot.cpp`

`Bot` uses a private implementation (`Bot::Impl`) so the public header does not
expose DPP types. The implementation owns, in declaration order:

- the `dpp::cluster`
- the `CommandRouter`
- an optional development guild snowflake
- the monotonic startup time
- an atomic command-registration guard

The cluster requests only `dpp::i_guilds`. Three callbacks are registered:

- DPP log events are forwarded to spdlog.
- slash-command events are sent to `CommandRouter::dispatch()`.
- the ready event logs connection data and starts bulk command registration.

The atomic guard prevents repeated ready events from issuing duplicate bulk
registrations. If registration fails, the callback resets the guard so a later
ready event may retry.

When `DEVELOPMENT_GUILD_ID` is present, commands are bulk-registered to that
guild for fast development updates. Otherwise they are bulk-registered globally.
Bulk registration replaces the application commands in the selected scope with
the definitions BigDPP currently knows about.

### Command contract and router

Files:

- `include/bigdpp/discord/command.hpp`
- `include/bigdpp/discord/command_router.hpp`
- `src/discord/command_router.cpp`

Every command implements `ICommand`:

- `name()` returns the dispatch key.
- `definition(application_id)` supplies Discord registration metadata.
- `execute(event)` handles the interaction.

`CommandRouter::add()` takes exclusive ownership with `std::unique_ptr`. It
rejects null handlers and duplicate names. `definitions()` sorts handlers by
name before producing DPP command definitions, making registration ordering
deterministic even though storage uses an unordered map.

`dispatch()` looks up the incoming command name. Unknown commands receive a
safe ephemeral response. Exceptions are caught at this adapter boundary,
logged, and converted to a generic ephemeral failure response so one command
cannot terminate the bot process.

Handlers must respond within Discord's interaction deadline. The current
handlers do only in-memory work and reply immediately. A future handler that
does network, database, or long-running work must acknowledge/defer promptly
and dispatch blocking work away from the Gateway callback thread.

## Implemented commands

### `/ping`

Files:

- `include/bigdpp/discord/commands/ping_command.hpp`
- `src/discord/commands/ping_command.cpp`

This is the smallest end-to-end health check. Its definition describes the
command, and execution immediately replies `Pong!`.

### `/status`

Files:

- `include/bigdpp/discord/commands/status_command.hpp`
- `src/discord/commands/status_command.cpp`
- `include/bigdpp/application/status.hpp`
- `src/application/status.cpp`

The handler converts Discord/runtime state into the DPP-free `StatusInfo` value:

- the build version comes from the CMake project version
- uptime uses `std::chrono::steady_clock`, so wall-clock changes do not affect it
- guild name and member count come from DPP's guild cache when available
- Gateway latency comes from the event's shard when available

`application::format_status()` renders the response. Keeping formatting outside
the DPP handler lets the default test suite verify output without a Discord
connection. Missing cache data is represented honestly as `Unavailable`, zero
members, or an omitted latency line.

## Build targets and dependencies

`CMakeLists.txt` defines these targets:

| Target | Kind | Responsibility | Direct private dependencies |
| --- | --- | --- | --- |
| `bigdpp_core` / `bigdpp::core` | library | configuration, logging, application behavior | spdlog |
| `bigdpp_discord` / `bigdpp::discord` | library | DPP adapter, router, commands | core, DPP, spdlog |
| `bigdpp` | executable | composition root | core, Discord, spdlog |
| `bigdpp_tests` | executable | offline unit tests | core, Catch2 |

All project-owned targets require C++20 and enable warning level 4 on MSVC or
`-Wall -Wextra -Wpedantic` elsewhere. `BIGDPP_VERSION` is injected into the
executable from the CMake project version.

The vcpkg manifest directly declares Catch2, DPP, and spdlog. The manifest's
baseline pins dependency resolution; generated `vcpkg_installed/` trees are not
committed.

## Tests

`tests/config_tests.cpp` verifies defaults, precedence, dotenv parsing,
validation, and the Discord-token startup requirement.

`tests/status_tests.cpp` verifies stable status rendering with both complete and
missing Discord cache data. These tests do not connect to Discord and do not
read a developer's real `.env` file.

Catch2 discovers individual test cases through CTest. See
[Development Guide](development.md) for commands and [Editing Guide](extending.md)
for where new tests belong.

## Implemented versus planned

Implemented behavior is limited to the configuration/logging foundation,
Discord connection and registration, `/ping`, and `/status`. The architectural
boundaries intentionally leave room for future application services and
infrastructure adapters, but those folders should be added only with a real use
case. The [Delivery Roadmap](roadmap.md) is the source of truth for future phases.
