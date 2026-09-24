# Security and Operations Baseline

## Deployment posture

BigDPP will operate on a real Discord server, so development defaults must be conservative. The current local build exposes explicit administrative slash commands, but live verification and production enablement must happen in a separate test guild first.

## Secrets

- Read secrets from environment variables.
- Fail startup clearly when a phase-required secret is absent.
- Never log configuration values wholesale.
- Redact tokens, database credentials, authorization headers, and signed URLs.
- Use separate Discord credentials and databases for development and production.
- Rotate a token immediately if it enters Git history or logs.

The optional local AI integration is also loopback-only. `/ask` sends only the
member's bounded question to Ollama, gives the model no Discord credentials or
tools, and disables generated Discord mentions. The command is open to members
of the dedicated AI channel; it does not grant any administrative capability.

## Discord authorization

Discord command visibility is not an authorization boundary. Each administrative use case must verify:

1. caller identity and required permissions
2. BigDPP's required permission
3. guild and target validity
4. caller-to-target and bot-to-target role hierarchy
5. command-specific constraints

State can change between validation and execution. Handle Discord rejection safely and record an audit outcome; do not report success before the API operation succeeds.

BigDPP should not request Administrator. The active administrator command
surface authorizes only the current guild owner or a member whose Discord base
permissions include Administrator. Every action checks the bot permission it
needs, validates cached guild/target state, and re-checks role hierarchy.
Privileged intents must be enabled only when an implemented feature requires
them; Server Members is currently required for hierarchy checks.

## Dangerous operations

- `/server-setup` is additive: it creates missing baseline items and never deletes, renames, or overwrites existing items.
- Destructive commands show a preview and require explicit `confirm:true`; inputs are bounded, including purge at 100 messages and timeouts at 40320 minutes.
- Bound bulk operations such as purge; no natural-language server-control path is exposed.
- Disable mass mentions unless an authorized announcement explicitly requests a permitted role/everyone mention.
- Record administrative outcomes in the process log and in `#mod-log` when that channel exists. Durable PostgreSQL audit history remains planned.

Validation failures are returned ephemerally. Mutations that require a Discord
callback are deferred and completed asynchronously; their successful result is
not necessarily ephemeral. See the [command reference](commands.md) for the
current operation-by-operation confirmation rules.

## Database

- Use parameterized statements and least-privilege database credentials.
- Apply versioned migrations; do not edit production schemas manually.
- Back up before destructive migrations and document restore testing.
- Treat migrations as immutable once deployed.
- Define retention before persisting message content or other privacy-sensitive event data.

## Reliability boundaries

- One command, event, or scheduled job failure must not crash the process.
- Use bounded retries with backoff only for transient failures.
- Make scheduled-job claiming recoverable after process termination.
- Respect DPP/Discord rate-limit behavior rather than adding blind retry loops.
- Log operation IDs and outcomes without sensitive payloads.
- Support graceful shutdown before production deployment.

## Initial production checklist

- test guild validation complete
- least-privilege bot role and correct hierarchy verified
- required intents documented and enabled
- administrative command set reviewed and tested in a dedicated guild
- audit channel access restricted
- application log destination and retention configured
- PostgreSQL backup and restore procedure tested when Phase 2 begins
- restart, reconnect, and unavailable-database behavior exercised
- rollback procedure documented for each newly enabled mutating feature
