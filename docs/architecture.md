# Architecture

## Status

This document describes the target architecture. The repository is currently in Phase 0 and does not yet implement these modules.

## Architectural style

BigDPP will be a modular monolith: one deployable process with explicit internal boundaries. This is appropriate for the first production server, keeps operations understandable, and still demonstrates service, persistence, asynchronous, and event-driven design.

The important rule is dependency direction, not the number of folders:

```text
Discord Gateway / slash command
              |
              v
       Discord adapter
              |
              v
      Application service
        /      |       \
       v       v        v
    Domain   Ports   Typed results
              ^
              |
 PostgreSQL / Discord / clock / scheduler adapters
```

The domain and application layers must be testable without Discord or PostgreSQL.

## Layers

### Domain

Contains stable business concepts and rules:

- guild configuration values
- maintenance findings and severity
- permission-audit rules
- moderation requests and authorization decisions
- announcement definitions and job state
- project-owned identifiers, timestamps, and validation errors

Domain code has no dependency on DPP, pqxx, spdlog, or environment APIs.

### Application

Implements use cases and orchestration:

- maintenance scans
- moderation actions
- announcements
- audit emission
- guild registration and settings
- scheduled-job claiming and execution

Application services depend on small ports such as guild repositories, audit sinks, clocks, schedulers, and Discord operations. Ports should be introduced only when a use case needs them.

### Discord adapter

Owns DPP-specific concerns:

- cluster lifetime and Gateway event registration
- slash-command definitions and routing
- conversion between DPP objects and application inputs
- interaction acknowledgement, deferred replies, and response formatting
- Discord API operations behind application ports
- mapping DPP failures and rate-limit-aware responses into project errors

DPP callbacks should validate basic interaction shape, dispatch work, and reply. They should not contain persistence or core authorization logic.

### Infrastructure

Owns process and external-system details:

- environment configuration
- spdlog setup and sinks
- PostgreSQL connections, transactions, and repositories
- migration execution
- scheduled-job polling/claiming
- system clock and shutdown handling

### Composition root

`src/main.cpp` loads validated configuration, initializes logging, constructs infrastructure and services, wires Discord handlers, and starts the bot. Object creation belongs here; business logic does not.

## Proposed source layout

Create directories incrementally as code is added:

```text
include/bigdpp/
  application/
  config/
  discord/
  domain/
  infrastructure/
  maintenance/
  moderation/
src/
  application/
  config/
  discord/
  infrastructure/
  maintenance/
  moderation/
tests/
  unit/
  integration/
migrations/
docs/
  decisions/
```

Empty placeholder directory trees should not be committed.

## Command dispatch

A command registry maps a slash-command name to a command handler. A handler is responsible for interaction-specific translation and delegates the use case to an application service. Command registration metadata and execution routing should share one source of truth where practical so definitions cannot silently drift from handlers.

Interaction handling must account for Discord's response deadline. Potentially slow operations should acknowledge or defer first, then complete asynchronously.

## Event processing

Gateway event handlers are adapters. They convert relevant events to project-owned inputs and dispatch them to services. Blocking database calls must not execute on the Gateway callback thread. The concrete execution model will be selected during Phase 1/2 after checking DPP's supported asynchronous APIs and pqxx concurrency requirements.

Ordering and duplicate delivery must be considered per event. Persistent handlers should be designed to tolerate retries where practical.

## Maintenance rules

The scanner runs independent rules against a read-only guild snapshot:

```cpp
enum class Severity { info, warning, critical };

struct Finding {
    Severity severity;
    std::string code;
    std::string title;
    std::string description;
};
```

The final model will also need stable evidence/context fields so a finding can be rendered, stored, ignored, or acted on without parsing human text. Rule codes are stable identifiers. Presentation strings are not identifiers.

Rules report only. Future fixes are separate, authorized commands with precondition re-checks and explicit confirmation.

## Persistence

PostgreSQL repositories own SQL and transaction boundaries. Application services must not assemble SQL. Migrations are ordered, immutable after release, and recorded in a schema-history table.

Initial persistence areas are guilds/settings, warnings and moderation actions, announcement jobs, audit settings, and ignored maintenance findings. Tables should use internal primary keys while Discord snowflakes receive unique/indexed columns appropriate to query patterns.

Scheduled jobs will use persisted state and atomic claiming rather than sleeping threads. A single-instance polling worker is sufficient initially; row locking and leases should make recovery safe before horizontal coordination is considered.

## Error model

Expected failures—invalid input, missing permission, hierarchy conflict, stale Discord IDs, and not-found state—should return typed errors. Infrastructure failures may throw internally but must be translated and logged at adapter/job/process boundaries. User responses expose actionable messages and correlation IDs where useful, never secrets or raw database details.

## Observability

Application logs are structured and include operation/command name, guild ID where safe, result, duration, and an operation ID. Sensitive configuration and message content are excluded by default. Discord audit messages are a configurable product feature and are distinct from diagnostic application logs.

Metrics are added only when measured. Candidate counters include commands processed, command failures, maintenance findings, moderation actions, scheduled-job outcomes, reconnects, and latency.

## Deployment model

The initial production topology is one BigDPP container and one PostgreSQL service with persistent storage. Redis, microservices, multi-region deployment, and Kubernetes are intentionally excluded until a concrete operational requirement justifies them.
