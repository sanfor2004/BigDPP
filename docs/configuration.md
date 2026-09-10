# Configuration Reference

BigDPP reads configuration once during startup. For local development it reads
`.env` from the process working directory; real environment variables take
precedence over `.env` values. Run the executable from the repository root when
you rely on the local file.

## Variables

| Variable | Required now | Default | Purpose |
| --- | --- | --- | --- |
| `DISCORD_TOKEN` | Yes, when starting the bot | none | Authenticates the DPP cluster. Never log or commit it. |
| `DEVELOPMENT_GUILD_ID` | No | none | Registers slash commands in one test guild instead of globally. Must be a non-zero decimal Discord snowflake. |
| `LOG_LEVEL` | No | `info` | spdlog threshold: `trace`, `debug`, `info`, `warn`, `error`, `critical`, or `off`. Input is normalized to lowercase. |
| `ENVIRONMENT` | No | `development` | Non-empty deployment label written to the startup log. It does not currently switch behavior. |
| `DATABASE_URL` | No; reserved for Phase 2 | none | Loaded into configuration but not consumed by the current application. |

Copy the example rather than creating a new format:

```powershell
Copy-Item .env.example .env
```

Then populate only local values:

```dotenv
DISCORD_TOKEN=replace-with-local-test-token
DEVELOPMENT_GUILD_ID=123456789012345678
LOG_LEVEL=debug
ENVIRONMENT=development
```

`.env` and `.env.*` are ignored by Git, except for the safe `.env.example`
template. Confirm with `git status` before committing.

## Precedence and empty values

For each setting, BigDPP uses this order:

1. a non-empty process environment variable
2. a non-empty value in `.env`
3. the code default, when one exists

An empty process value does not mask a non-empty `.env` value. Empty optional
values are treated as absent.

## Dotenv syntax

Supported:

```dotenv
# full-line comment
LOG_LEVEL = INFO
ENVIRONMENT="local development"
DISCORD_TOKEN='quoted-token'
```

Not supported:

```dotenv
export LOG_LEVEL=debug
NAME=${OTHER_NAME}
MULTILINE="first
second"
```

Quoted values must start and end with the same quote character. Everything
inside those quotes is used literally; escape processing is not performed.

## Validation and failure behavior

Malformed dotenv entries, invalid keys, unterminated quotes, unsupported log
levels, empty environment names, and invalid development guild IDs produce a
`ConfigError`. A missing Discord token produces a `ConfigError` when the Discord
bot is constructed. `main()` catches these failures, logs a short critical
message, and exits with status `1`.

## Production guidance

Provide secrets through the deployment environment or secret manager rather
than shipping `.env`. Use a separate bot application for testing, keep the bot's
permissions minimal, and rotate the token immediately if it is exposed. Do not
print a `Config` object or log raw configuration values.
