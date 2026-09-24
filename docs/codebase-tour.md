# Codebase Tour

BigDPP is currently in Phase 1, Discord Foundation. The implemented runtime
loads a Discord token securely, connects through DPP, registers foundation, AI,
and administrator slash commands, and handles their responses. With local LLM
mode enabled, it also manages one AI text channel per guild and answers `/ask`
through Ollama. Durable persistence and diagnostics are planned; explicit
Discord server mutations are available through `AdminService`.

## Repository map

```text
BigDPP/
|-- CMakeLists.txt                 Build target and dependency wiring
|-- .env.example                  Safe local configuration template
|-- include/bigdpp/config.hpp     Startup configuration contract
|-- include/bigdpp/bot.hpp        Discord adapter contract
|-- include/bigdpp/admin.hpp      Administrator command service contract
|-- include/bigdpp/llm.hpp        Local LLM adapter contract
|-- src/config.cpp                Environment and .env loading
|-- src/bot.cpp                   DPP cluster and slash-command handling
|-- src/admin.cpp                 Administrator authorization and operations
|-- src/llm.cpp                   Asynchronous loopback Ollama client
|-- src/main.cpp                  Composition root and process boundary
`-- docs/                         Architecture, setup, and operations guides
```

## Startup flow

```text
main
  -> Config::load()
       -> read process environment
       -> read .env when a process value is absent
       -> validate Discord and local-LLM settings
  -> construct Bot
       -> create dpp::cluster with Guilds and Guild Members intents
       -> attach ready, log, and slash-command handlers
  -> Bot::run()
       -> optionally start the local Ollama runtime
       -> start the Gateway connection and wait
       -> ready event registers commands and prepares AI channels
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

`Bot` owns the DPP cluster and its callbacks. It requests the Guilds and Guild
Members intents. On the first ready event it bulk-registers foundation, AI, and
administrator commands either in `DEVELOPMENT_GUILD_ID` or globally. A later
ready event does not duplicate registration. Registration failures are logged
and can be retried on a later ready event. When enabled, ready handling also
discovers or creates the configured AI text channel.

Slash-command handling is intentionally small and non-blocking:

- `/ping` replies `Pong!`.
- `/status` reports the bot's online username.
- `/ask question:<text>` defers, calls the configured local model, and edits the
  response; it is accepted only in the configured AI channel.
- Unknown commands receive an ephemeral safe response.
- Administrator commands are routed to `AdminService`, which authorizes the
  guild owner or Discord Administrator, checks bot permissions and hierarchy,
  requires confirmation for destructive actions, and audits outcomes.
- Exceptions are caught at the command boundary so one interaction cannot stop
  the process.

The local LLM adapter calls Ollama's non-streaming `/api/chat` endpoint through
DPP's raw request queue. It sends a single user question and a fixed assistant
instruction; it does not receive Discord data or tools.

The LLM adapter uses nlohmann-json to encode and parse the Ollama request and
response. No external control or tool bridge is connected to the model.

## Boundaries for the next phases

`main.cpp` constructs concrete objects and remains the composition root.
Administrator operations are kept in `AdminService`, not in a growing
collection of callbacks in `main.cpp`.

The current administrator operations need live verification in a dedicated test
guild. New operations must preserve caller and bot permission checks,
role-hierarchy checks, explicit confirmation for dangerous actions, bounded
inputs, and audit-safe logging. The local AI remains prompt-only until an
explicit, separately authorized tool boundary is designed.

See the [roadmap](roadmap.md), [security baseline](security-and-operations.md),
and [editing guide](extending.md) for the current boundaries.
