# Architecture

## Current status

BigDPP is a small Phase 1 modular monolith. It loads startup configuration,
connects to Discord through DPP, registers the foundation, local AI, and
explicit administrator commands. When enabled, `/ask` uses a loopback Ollama
HTTP adapter and a managed AI text channel. PostgreSQL, durable moderation
history, announcements, and diagnostics remain planned.

## Dependency direction

```text
main.cpp (composition root)
        |
        +--> Config (startup infrastructure)
        |
        +--> Discord Bot adapter --> AdminService --> DPP
        +--> Local LLM adapter --> Ollama HTTP API (loopback only)
```

`main.cpp` constructs the configuration and Discord adapter, then starts the
Gateway. It does not contain command business rules or Discord API operations.
The bot adapter owns DPP-specific callbacks and routes administrative commands
to `AdminService`, which owns authorization, hierarchy validation, confirmation,
and operation-specific DPP calls. The local LLM adapter is prompt-only and
uses DPP's asynchronous HTTP queue for the loopback Ollama request.

## Ownership and runtime

`main` owns `Config` and `Bot` by value. `Bot` owns the DPP cluster and starts it
with the Guilds and Guild Members intents. Ready handling bulk-registers the
known slash commands once per connection. A registration failure clears the
guard so a later ready event can retry.

Callbacks must remain quick and non-blocking. Future slow work must acknowledge
the interaction before Discord's deadline and run away from the Gateway event
thread. One command failure is caught and converted into a safe response.

The `/ask` callback acknowledges first, then edits the deferred reply when
Ollama completes. Generated replies are bounded to Discord's message size and
have all Discord mention parsing disabled.

## Security boundaries

- Tokens are loaded from the process environment or ignored `.env` only.
- No guild, channel, role, or user IDs are hardcoded.
- The bot must not request Administrator.
- Administrative mutations must verify caller permissions, bot permissions,
  target validity, and role hierarchy immediately before execution.
- Destructive actions require explicit confirmation and bounded inputs.
- AI integration is prompt-only in this milestone. It receives no Discord
  token, guild data, or tool access, and its endpoint is restricted to local
  HTTP loopback addresses.

## Planned expansion

The next work is live verification of the Phase 1 foundation and administrator
commands in a dedicated test guild. Later features should introduce application
services and narrow ports as real boundaries appear. SQL belongs only in
PostgreSQL adapters and migrations when Phase 2 begins. `main.cpp` should remain
a composition root, not a server-management god class.
