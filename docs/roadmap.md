# Delivery Roadmap

## Planning principles

- Deliver the smallest production-shaped vertical slice in each phase.
- Keep the bot deployable and diagnostics read-only before enabling administrative mutations.
- A phase is complete only when its acceptance criteria pass and its documentation matches reality.
- Planned items are not advertised as implemented.

## Phase 0 — Repository foundation

**Status:** native foundation complete; container and CI verification deferred to
Phase 8 production hardening.

Completed:

- C++20 target-based CMake executable and core library
- validated environment configuration foundation
- spdlog initialization
- pinned vcpkg manifest
- Catch2/CTest unit-test foundation
- ignore/example/formatting files and planning documentation
- successful MSVC build, tests, valid-startup smoke test, and invalid-config smoke test

Deferred hardening work:

- Linux/container build verification and Docker foundation
- CI for clean Windows/Linux configuration, build, and tests
- confirm the final supported compiler matrix from CI evidence

Outcome:

- the native Windows project configures and builds through the pinned vcpkg
  manifest
- configuration failures are reported clearly with a non-zero exit status
- unit tests run through CTest with project warnings enabled
- CMake 3.25 is the current minimum and DPP was introduced in Phase 1
- clean Linux/container and CI evidence remain explicitly unverified

## Phase 1 — Discord foundation

**Status:** implemented locally; awaiting live test-guild verification.

Completed locally:

- DPP 10.1.5 cluster adapter using only the standard Guilds intent
- ready and DPP log handlers
- command registry/router with exception containment
- development-guild or global bulk registration
- `/ping` and `/status`
- DPP-free status formatting and tests
- ignored `.env` loading with process-environment precedence
- missing-token and development-guild-ID validation

Remaining verification:

- connect with the owner's test application token
- register commands in the configured test guild
- exercise `/ping` and `/status` through Discord
- observe reconnect behavior before production use

Deliverables:

- DPP cluster adapter and graceful lifetime ownership
- required-intents configuration
- ready handler
- slash-command definition/registration strategy
- command registry/router
- `/ping` and `/status`
- startup, connection, command, and failure logging

Acceptance criteria:

- the bot connects using an environment token without logging it
- command registration is deterministic and documented for development versus production guilds
- `/ping` and `/status` respond within Discord interaction constraints
- `/status` reports only actually available version, uptime, guild/member, and latency data
- handler/service behavior is unit-tested without a live Discord connection
- a callback failure is contained and produces a safe response/log entry

## Phase 2 — PostgreSQL foundation

**Status:** planned.

Deliverables:

- connection management and health check
- ordered migrations and schema-history tracking
- guild registration and settings repository
- PostgreSQL integration-test fixture
- Docker Compose services for BigDPP and PostgreSQL with persistent storage

Acceptance criteria:

- SQL exists only in migrations and PostgreSQL adapters
- queries are parameterized
- migrations apply automatically or through one documented release command and are safe to re-run
- startup/database failure behavior is explicit
- integration tests run separately from the default fast unit suite

## Phase 3 — Read-only server diagnostics

**Status:** planned.

First rules:

- BigDPP permission and role-hierarchy checks
- `@everyone` privileged-permission audit
- Administrator role detection
- empty category detection
- bot visibility/inaccessible-channel findings

Acceptance criteria:

- `/server-check` delegates to independently testable rules
- findings use stable codes, severity, evidence, and presentation fields
- scan output is bounded/paginated safely for Discord
- scans make no server mutations
- permission calculations and edge cases have unit tests

## Phase 4 — Moderation foundation

**Status:** planned.

Deliver warnings, timeout/untimeout, purge, and lock/unlock before expanding to kick/ban. Every operation uses a shared authorization pipeline: caller permission, bot permission, hierarchy, target, execution, persistence, audit, response.

Acceptance criteria include atomic/consistent action recording, explicit partial-failure handling, audit messages, reason validation, bounded purge behavior, and test coverage for authorization decisions.

## Phase 5 — Announcements

**Status:** planned.

Start with validated immediate announcements, explicit allowed mentions, configured/default destinations, embeds, and database logging. Then add persisted scheduled jobs with timezone-aware input, atomic claiming, retries, and terminal job states. Do not schedule with `sleep()`.

## Phase 6 — Configurable Discord event audit

**Status:** planned.

Add opt-in categories for joins, leaves, message deletes/edits, role/channel changes, bans, and unbans. Define content/privacy retention rules before storing message data.

## Phase 7 — Advanced maintenance

**Status:** planned.

Add evidence-based stale channels/roles, duplicate-role heuristics, ignored findings, scan history, and guided fixes. Detection and mutation remain separate; fixes re-check current state and authorization.

## Phase 8 — Production hardening

**Status:** planned.

Add graceful shutdown, health checks, CI, release builds, container hardening, backup/restore documentation, operational runbooks, broader tests, and honest production metrics.

## First deployment gates

Before enabling BigDPP on the approximately 450-member server:

1. Use a test guild for command and permission validation.
2. Request the minimum intents and permissions.
3. Deploy read-only commands first.
4. Configure application logs and a restricted audit channel.
5. Verify restart/reconnect behavior.
6. Back up relevant configuration before enabling mutations.
7. Enable destructive commands individually after test evidence and admin review.

## Recommended next slice

Complete Phase 1 live verification in a dedicated test guild:

1. Start the bot with a test application token and `DEVELOPMENT_GUILD_ID`.
2. Confirm registration and exercise `/ping` and `/status` through Discord.
3. Observe disconnect/reconnect behavior and record the result before starting
   Phase 2.
