# Architecture

## Current status

BigDPP is a small Phase 1 modular monolith. It currently loads startup
configuration, connects to Discord through DPP, registers `/ping` and `/status`,
and responds to those commands. PostgreSQL, moderation, announcements,
diagnostics, and AI control are planned.

## Dependency direction

```text
main.cpp (composition root)
        |
        +--> Config (startup infrastructure)
        |
        `--> Discord Bot adapter --> DPP
```

`main.cpp` constructs the configuration and Discord adapter, then starts the
Gateway. It does not contain command business rules or Discord API operations.
The bot adapter owns DPP-specific callbacks and translates slash commands into
small responses.

## Ownership and runtime

`main` owns `Config` and `Bot` by value. `Bot` owns the DPP cluster and starts it
with the standard Guilds intent. Ready handling bulk-registers the known slash
commands once per connection. A registration failure clears the guard so a
later ready event can retry.

Callbacks must remain quick and non-blocking. Future slow work must acknowledge
the interaction before Discord's deadline and run away from the Gateway event
thread. One command failure is caught and converted into a safe response.

## Security boundaries

- Tokens are loaded from the process environment or ignored `.env` only.
- No guild, channel, role, or user IDs are hardcoded.
- The bot must not request Administrator.
- Future mutations must verify caller permissions, bot permissions, target
  validity, and role hierarchy immediately before execution.
- Destructive actions require explicit confirmation and bounded inputs.
- AI integration may call only explicitly allowlisted application operations; it
  must never receive unrestricted Discord API access.

## Planned expansion

The next work is live verification of the Phase 1 foundation in a dedicated
test guild. Later features should introduce application services and narrow
ports as real boundaries appear. SQL belongs only in PostgreSQL adapters and
migrations when Phase 2 begins. `main.cpp` should remain a composition root,
not a server-management god class.
