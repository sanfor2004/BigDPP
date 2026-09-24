# ADR 0003: Explicit administrator command surface

## Status

Accepted and implemented locally; live Discord verification is pending.

## Context

BigDPP needs to manage a real Discord server, but the local LLM must not become
an unrestricted server-control path. The initial role-only
commands also did not provide a coherent way to manage channels, roles, members,
and bounded message cleanup.

## Decision

Expose server operations as explicit top-level slash commands owned by a
dedicated `AdminService`. A command is authorized only when the caller is the
current guild owner or has Discord Administrator. The bot separately checks the
permission required by the operation, validates cached targets, and enforces
caller and bot role hierarchy.

Destructive operations require a preview followed by `confirm:true`. Inputs are
bounded, allowed mentions are disabled, and outcomes are written to the process
log plus `#mod-log` when that channel exists. `/server-setup` is additive and
never deletes, renames, or overwrites existing server items. The bot requests
specific permissions and does not request Discord Administrator.

`/ask` remains prompt-only. It has no access to `AdminService`, Discord
credentials, or server-management tools.

## Consequences

- The command surface is discoverable and auditable in Discord.
- A server owner or Discord Administrator can use the same explicit controls;
  a user-created role named `Administrator` is not trusted as an identity.
- Live verification must test bot permissions, role placement, command previews,
  confirmation, and failure responses in a dedicated test guild.
- Warnings and audit records are not durable until PostgreSQL persistence is
  implemented.
