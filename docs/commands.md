# Command Reference

BigDPP registers the foundation, local-AI, and administrator commands when the
first ready event completes. With `DEVELOPMENT_GUILD_ID`, they are registered
in that guild; otherwise they are registered globally. Global command updates
can take longer to appear.

## Foundation and AI

| Command | Parameters | Behavior |
| --- | --- | --- |
| `/ping` | none | Replies `Pong!`. |
| `/status` | none | Reports the bot's online username. |
| `/ask` | `question` required | Sends the question to the configured local Ollama model. The command is available only when `BIGDPP_LLM_ENABLED=true` and only in the configured AI channel. Replies are non-streaming, bounded before sending, and have Discord mention parsing disabled. |

`/ask` is prompt-only. It does not receive Discord data, credentials, or
server-management tools. BigDPP discovers or creates the configured AI text
channel for each connected guild when the feature is enabled.

## Administrator authorization

Every command in the administrator section must be used in a guild whose state
is available in BigDPP's cache. The caller must be the current guild owner or
have Discord Administrator in that guild. BigDPP then checks the operation's
own bot permission, validates the target, and enforces bot and caller role
hierarchy where a member or role is targeted.

Commands that can make a destructive or high-impact change first return a
preview. Re-run the same command with `confirm:true` to execute it. Command
responses disable allowed mentions. Immediate validation responses are
ephemeral; deferred Discord operations are acknowledged and completed through
callbacks.

## Server and channel commands

| Command | Parameters | Required bot permission | Confirmation |
| --- | --- | --- | --- |
| `/server-info` | none | none beyond access to the guild cache | no |
| `/server-edit` | `name` or `description` (at least one) | Manage Guild | yes |
| `/server-setup` | `confirm` | Manage Channels and Manage Roles | yes |
| `/channel-create` | `name`, `type` (`text`, `voice`, or `category`); optional `category`, `topic`, `reason` | Manage Channels | no |
| `/channel-edit` | `channel`; optional `name`, `topic`, `slowmode` (0-21600), `nsfw`, `reason` | Manage Channels | no |
| `/channel-delete` | `channel`; optional `reason` | Manage Channels | yes |
| `/channel-lock` | `channel`; optional `reason` | Manage Channels | yes |
| `/channel-unlock` | `channel`; optional `reason` | Manage Channels | yes |
| `/channel-access` | `channel`, `mode` (`public` or `private`); optional `reason` | Manage Channels | yes |

`/server-setup` is additive. It creates missing `INFORMATION`, `COMMUNITY`,
and `STAFF` categories; `welcome`, `rules`, `general`, `ai-agent`, and
`mod-log` text channels; and the baseline roles `Server Owner`,
`Administrator`, `Moderator`, `Member`, and `AI User`. The created roles have
no permissions, and `mod-log` is restricted to the bot by its initial
overwrites. Existing items are reused; they are not renamed, deleted, or
overwritten. The setup command uses the literal `ai-agent` channel name even
when `BIGDPP_AI_CHANNEL_NAME` is customized.

## Role commands

| Command | Parameters | Required bot permission | Confirmation |
| --- | --- | --- | --- |
| `/role-create` | `name`; optional `color`, `profile` (`cosmetic` or `moderator`), `reason` | Manage Roles | no |
| `/create-role` | `name`; optional `color`, `reason` | Manage Roles | no |
| `/role-edit` | `role` ID; optional `name`, `color`, `profile`; optional `reason` | Manage Roles | yes |
| `/role-delete` | `role` ID; optional `reason` | Manage Roles | yes |
| `/role-add` | `user`, `role` ID; optional `reason` | Manage Roles | no |
| `/role-remove` | `user`, `role` ID; optional `reason` | Manage Roles | no |

Cosmetic roles receive no permissions. The moderator profile grants only
Manage Messages and Moderate Members; no command profile grants Administrator.
Managed integration roles, `@everyone`, and roles at or above BigDPP's highest
role cannot be edited, deleted, or assigned by these commands.

## Member and message commands

| Command | Parameters | Required bot permission | Confirmation |
| --- | --- | --- | --- |
| `/member-warn` | `user`, `reason` | Moderate Members | no |
| `/member-timeout` | `user`, `minutes` (1-40320), `reason` | Moderate Members | yes |
| `/member-untimeout` | `user`; optional `reason` | Moderate Members | no |
| `/member-kick` | `user`, `reason` | Kick Members | yes |
| `/member-ban` | `user`, `reason`; optional `delete_days` (0-7) | Ban Members | yes |
| `/member-unban` | user ID; optional `reason` | Ban Members | yes |
| `/messages-purge` | `amount` (1-100), `reason`; optional `channel` | Manage Messages | yes |
| `/audit-log` | none | none beyond administrator authorization | no |

`/messages-purge` defaults to the invoking channel and accepts only a cached
text channel in the current guild. `/member-warn` records an audit line in the
process log and in `#mod-log` when that channel exists; warnings and audit
records are not persisted in PostgreSQL yet. `/audit-log` reports this current
storage boundary.

## Audit and limitations

Administrative outcomes are written to the process log and, when available, a
text channel named `mod-log`. The command schemas accept optional `reason`
values, but durable moderation history, database persistence, scheduled work,
and LLM-driven server-control tools are not implemented. Live Discord
verification of the complete command surface is still pending.
