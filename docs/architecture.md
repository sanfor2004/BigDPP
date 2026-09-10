# Architecture

## Current status

BigDPP is a modular monolith in Phase 1. The repository currently implements a
configuration/logging core, a DPP Discord adapter, a command router, and the
`/ping` and `/status` commands. PostgreSQL, moderation, announcements, audit
events, scheduling, and server diagnostics are planned and must not be treated
as available behavior.

The architecture below distinguishes current code from the intended boundaries
for future phases.

## Architectural style

BigDPP is one long-running process with explicit internal module boundaries.
This keeps deployment and recovery understandable for one real Discord server
while preserving independently testable application behavior.

The primary rule is dependency direction:

```text
Discord Gateway / interaction
             |
             v
      Discord adapter
             |
             v
    Application behavior  ---> project-owned values/results
             |
             v
           Domain

External adapters ---> application ports / domain
Composition root ---> constructs all concrete objects
```

The current `/status` path demonstrates this direction: the DPP handler gathers
Discord data, converts it to `application::StatusInfo`, and calls a DPP-free
formatter that unit tests exercise offline.

## Implemented modules

### Configuration and logging core

`bigdpp::config` loads `.env` plus process environment values and validates
startup settings. `bigdpp::logging` initializes spdlog. These are infrastructure
concerns packaged in `bigdpp_core` because both the executable and application
foundation use that target.

### Application

`bigdpp::application` currently contains `StatusInfo` and status formatting.
Application code must remain independent of DPP so use cases and rules can be
tested without Discord. As real use cases appear, this layer will coordinate
them and depend on narrow ports for external operations.

### Discord adapter

`bigdpp::discord` owns DPP-specific concerns:

- cluster lifetime and the Guilds Gateway intent
- ready, log, and slash-command callbacks
- guild-scoped or global bulk command registration
- command definition and dispatch
- conversion from DPP state into application inputs
- Discord interaction responses and exception containment

The adapter is compiled as `bigdpp_discord`. DPP does not leak into the public
`Bot` header because the class uses a private implementation.

### Composition root

`src/main.cpp` loads validated configuration, initializes logging, creates the
Discord bot, and starts it. It is also the process exception boundary. Object
construction belongs here; business rules do not.

## Current runtime paths

Startup:

```text
main -> Config -> logging -> Bot::Impl -> dpp::cluster -> blocking Gateway start
```

Ready and registration:

```text
DPP ready event -> atomic one-time guard -> sorted command definitions
                -> guild bulk create OR global bulk create
                -> completion callback logs success/failure
```

Interaction:

```text
DPP slash event -> CommandRouter -> ICommand implementation
                               -> optional application behavior
                               -> Discord reply
```

Unknown commands and thrown handler failures are converted to ephemeral safe
responses. The router logs the failure but does not expose exception text to the
Discord user.

See [Codebase Tour](codebase-tour.md) for file-level detail.

## Ownership and lifetime

`main` owns a `Bot` on the stack. `Bot` exclusively owns `Bot::Impl` with
`std::unique_ptr`; the implementation owns the DPP cluster, router, commands,
configuration-derived guild ID, and runtime state. The router exclusively owns
each command with `std::unique_ptr<ICommand>`.

DPP callbacks capture the implementation through `this`. They remain valid
while `cluster_.start(dpp::st_wait)` is running because the `Bot` and its
implementation outlive the cluster event loop. Future asynchronous additions
must preserve equally explicit lifetimes and avoid owning raw pointers.

## Threading and asynchronous boundaries

DPP invokes Gateway callbacks. They must validate and dispatch quickly; blocking
database work or long-running computation must not run on that callback thread.
The current callbacks only route, read cached state, format small strings, reply,
or initiate asynchronous DPP registration.

The concrete database/job execution model will be selected when those phases
begin, after verifying DPP and libpqxx concurrency requirements for the chosen
versions. Scheduled work will use persisted state and an injectable clock, not
sleeping event callbacks.

## Planned boundaries

### Domain

Domain code will contain stable business concepts and rules such as maintenance
findings, permission decisions, moderation requests, announcement state, and
project-owned identifiers. It must not include DPP, pqxx, spdlog, or operating-
system headers.

### Application services and ports

Future services will coordinate maintenance scans, moderation, announcements,
auditing, guild settings, and scheduled jobs. A port is added only when a real
use case needs an external boundary, such as a guild repository, audit sink,
clock, scheduler, or Discord operation.

Expected validation failures should use typed results/errors. Infrastructure
exceptions are translated at adapter, command, job, or process boundaries.

### PostgreSQL infrastructure

Beginning in Phase 2, PostgreSQL adapters will own SQL and transaction
boundaries. Application services will not construct SQL. Queries will be
parameterized, and ordered migrations will be immutable after release and
tracked in schema history.

Initial persistence areas are expected to include guild settings, moderation
records, announcements/jobs, audit settings, and ignored maintenance findings.
The schema will be designed only as each implemented use case requires it.

### Maintenance

Read-only server diagnostics begin in Phase 3. Independent rules will evaluate
a project-owned guild snapshot and return stable finding codes, severity,
evidence, and presentation data. Detection and mutation remain separate. Any
future guided fix must re-check authorization and current state, then require
explicit confirmation.

## Error and reliability model

- Expected invalid input or business rejection uses typed results/errors.
- One command, event, or job failure must not terminate the process.
- Adapter failures are logged with safe operational context.
- User replies do not reveal secrets, exception details, or database internals.
- Transient retries must be bounded and respect Discord/DPP rate-limit behavior.
- Command handlers defer before slow work and never block with `sleep()`.

The current router and `main` implement the command and process exception
boundaries. More specific typed errors will arrive with application use cases.

## Observability

Current logs record startup, environment label, DPP events, Gateway readiness,
registration, command execution, and failures. Secrets and message content are
excluded.

Future structured logs should include operation name, safe guild context,
result, duration, and an operation ID. Discord audit messages are a separate
product feature, not a replacement for application logs. Metrics are added only
for measured operational needs.

## Deployment model

Today BigDPP runs as one native process connected to Discord. The planned first
production-like topology is one BigDPP container and one PostgreSQL service with
persistent storage. Redis, microservices, Kubernetes, a dashboard, and unrelated
bot features remain intentionally excluded.

## Source layout rule

Create directories only when code is added:

```text
include/bigdpp/<module>/
src/<module>/
tests/
migrations/          # Phase 2+
docs/decisions/      # when an ADR is required
```

Do not commit empty placeholder trees. Public headers and matching source files
use lower-case names under the `bigdpp` namespace.

For concrete change recipes and verification steps, see
[Editing and Extension Guide](extending.md).
