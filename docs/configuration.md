# Configuration Reference

BigDPP reads the Discord token once during startup. A non-empty process
environment variable takes precedence; otherwise the executable reads
`DISCORD_TOKEN` from `.env` in the process working directory. Run the executable
from the repository root when you rely on the local file.

## Variables

| Variable | Required now | Default | Purpose |
| --- | --- | --- | --- |
| `DISCORD_TOKEN` | Yes, when starting the bot | none | Authenticates the DPP cluster. Never log or commit it. |
| `DEVELOPMENT_GUILD_ID` | No | none | Registers slash commands in one test guild instead of globally. Must be a non-zero decimal Discord snowflake. |

Copy the example rather than creating a new format:

```powershell
Copy-Item .env.example .env
```

Then populate only local values:

```dotenv
DISCORD_TOKEN=replace-with-local-test-token
DEVELOPMENT_GUILD_ID=123456789012345678
```

`.env` and `.env.*` are ignored by Git, except for the safe `.env.example`
template. Confirm with `git status` before committing.

## Precedence and empty values

For the Discord token, BigDPP uses this order:

1. a non-empty process environment variable
2. a non-empty value in `.env`
3. no value; startup fails safely

An empty process value does not mask a non-empty `.env` value. Empty optional
values are treated as absent.

## Dotenv syntax

Supported:

```dotenv
# full-line comment
DISCORD_TOKEN='quoted-token'
DEVELOPMENT_GUILD_ID=123456789012345678
```

Not supported:

```dotenv
NAME=${OTHER_NAME}
MULTILINE="first
second"
```

Quoted values must start and end with the same quote character. Everything
inside those quotes is used literally; escape processing is not performed.

## Validation and failure behavior

An invalid `DEVELOPMENT_GUILD_ID` or a missing Discord token produces a startup
error. `main()` catches startup failures, writes a short message, and exits with
status `1`.

## Production guidance

Provide secrets through the deployment environment or secret manager rather
than shipping `.env`. Use a separate bot application for testing, keep the bot's
permissions minimal, and rotate the token immediately if it is exposed. Do not
print a `Config` object or log raw configuration values.
