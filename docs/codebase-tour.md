# Codebase Tour

BigDPP is currently in Phase 1, Discord Foundation. The implemented runtime
loads a Discord token securely, connects through DPP, registers two slash
commands, and handles their responses. Server mutations, persistence,
diagnostics, and AI control are planned rather than available.

## Repository map

```text
BigDPP/
|-- CMakeLists.txt                 Build target and dependency wiring
|-- .env.example                  Safe local configuration template
|-- include/bigdpp/config.hpp     Startup configuration contract
|-- include/bigdpp/bot.hpp        Discord adapter contract
|-- src/config.cpp                Environment and .env loading
|-- src/bot.cpp                   DPP cluster and slash-command handling
|-- src/main.cpp                  Composition root and process boundary
`-- docs/                         Architecture, setup, and operations guides
```

## Startup flow

```text
main
  -> Config::load()
       -> read process environment
       -> read .env when a process value is absent
       -> validate DISCORD_TOKEN and DEVELOPMENT_GUILD_ID
  -> construct Bot
       -> create dpp::cluster with the Guilds intent
       -> attach ready, log, and slash-command handlers
  -> Bot::run()
       -> start the Gateway connection and wait
```

The token is passed to DPP but is never deliberately logged. Startup failures
are caught at the process boundary and reported without configuration values.

## Configuration

`Config::load()` supports `DISCORD_TOKEN` and the optional
`DEVELOPMENT_GUILD_ID`. A non-empty process environment value takes precedence
over `.env`. The dotenv reader supports blank lines, full-line comments,
surrounding whitespace, and matching single or double quotes. It does not
implement shell expansion, `export`, inline comments, or multiline values.

## Discord adapter

`Bot` owns the DPP cluster and its callbacks. It requests only the standard
Guilds intent. On the first ready event it bulk-registers `/ping` and `/status`
either in `DEVELOPMENT_GUILD_ID` or globally. A later ready event does not
duplicate registration. Registration failures are logged and can be retried on
a later ready event.

Slash-command handling is intentionally small and non-blocking:

- `/ping` replies `Pong!`.
- `/status` reports the bot's online username.
- Unknown commands receive an ephemeral safe response.
- Exceptions are caught at the command boundary so one interaction cannot stop
  the process.

## Boundaries for the next phases

`main.cpp` constructs concrete objects and remains the composition root. Future
server operations should be added as application services and narrow Discord
commands, not as a growing collection of callbacks in `main.cpp`.

Before adding mutations, the project needs live Phase 1 verification in a
dedicated test guild. Administrative operations must then add caller and bot
permission checks, role-hierarchy checks, explicit confirmation for dangerous
actions, bounded inputs, and audit-safe logging. AI control will be a later
adapter that can call only those explicitly allowlisted operations.

See the [roadmap](roadmap.md), [security baseline](security-and-operations.md),
and [editing guide](extending.md) for the current boundaries.
