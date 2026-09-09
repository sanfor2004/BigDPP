# Security and Operations Baseline

## Deployment posture

BigDPP will operate on a real Discord server, so development defaults must be conservative. The initial production rollout is read-only diagnostics. Mutating moderation and maintenance features remain disabled until tested in a separate guild and explicitly enabled.

## Secrets

- Read secrets from environment variables.
- Fail startup clearly when a phase-required secret is absent.
- Never log configuration values wholesale.
- Redact tokens, database credentials, authorization headers, and signed URLs.
- Use separate Discord credentials and databases for development and production.
- Rotate a token immediately if it enters Git history or logs.

## Discord authorization

Discord command visibility is not an authorization boundary. Each administrative use case must verify:

1. caller identity and required permissions
2. BigDPP's required permission
3. guild and target validity
4. caller-to-target and bot-to-target role hierarchy
5. command-specific constraints

State can change between validation and execution. Handle Discord rejection safely and record an audit outcome; do not report success before the API operation succeeds.

BigDPP should not request Administrator. Document each requested permission and the feature that needs it. Privileged intents must be enabled only when an implemented feature requires them.

## Dangerous operations

- Maintenance scans produce findings only.
- A future guided fix must show the exact proposed action, require explicit confirmation, and re-check preconditions immediately before execution.
- Bound bulk operations such as purge, role changes, and announcements.
- Disable mass mentions unless an authorized announcement explicitly requests a permitted role/everyone mention.
- Preserve an immutable moderation/audit record when policy requires it.

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
- read-only feature set selected
- audit channel access restricted
- application log destination and retention configured
- PostgreSQL backup and restore procedure tested when Phase 2 begins
- restart, reconnect, and unavailable-database behavior exercised
- rollback procedure documented for each newly enabled mutating feature
