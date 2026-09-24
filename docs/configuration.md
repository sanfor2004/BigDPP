# Configuration Reference

BigDPP reads the Discord token once during startup. A non-empty process
environment variable takes precedence; otherwise the executable reads
`DISCORD_TOKEN` from `.env`. It checks the process working directory and then
repository ancestor directories, so running the executable from a generated
build directory still finds the repository's ignored `.env` file.

## Variables

| Variable | Required now | Default | Purpose |
| --- | --- | --- | --- |
| `DISCORD_TOKEN` | Yes, when starting the bot | none | Authenticates the DPP cluster. Never log or commit it. |
| `DEVELOPMENT_GUILD_ID` | No | none | Registers slash commands in one test guild instead of globally. Must be a non-zero decimal Discord snowflake. |
| `BIGDPP_LLM_ENABLED` | No | `false` | Enables `/ask` and automatic AI-channel setup. |
| `BIGDPP_LLM_AUTOSTART` | No | `true` | Starts `ollama serve` when the local LLM is enabled. |
| `BIGDPP_LLM_BASE_URL` | No | `http://127.0.0.1:11434` | Loopback Ollama HTTP base URL. Only `localhost` and `127.0.0.1` over HTTP are accepted. |
| `BIGDPP_LLM_MODEL` | Required when LLM is enabled | none | Ollama model tag to use, such as `llama3.2`. |
| `BIGDPP_AI_CHANNEL_NAME` | No | `ai-agent` | Text channel where `/ask` is allowed and, when missing, created. |
| `BIGDPP_LLM_EXECUTABLE` | No | `ollama` | Ollama executable name or path used for autostart. |

Server control is enabled through explicit Discord commands rather than a
configuration allowlist. The handler authorizes the current guild owner or a
member with Discord Administrator, checks the bot permission needed by each
operation, and enforces target role hierarchy. The local AI has no server-control
tools and `/ask` cannot execute administrative commands.

Copy the example rather than creating a new format:

```powershell
Copy-Item .env.example .env
```

Then populate only local values:

```dotenv
DISCORD_TOKEN=replace-with-local-test-token
DEVELOPMENT_GUILD_ID=123456789012345678
BIGDPP_LLM_ENABLED=true
BIGDPP_LLM_BASE_URL=http://127.0.0.1:11434
BIGDPP_LLM_MODEL=llama3.2
BIGDPP_AI_CHANNEL_NAME=ai-agent
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
error. When enabled, an invalid local LLM URL, missing model, or invalid AI
channel name also fails startup. `main()` catches startup failures, writes a
short message, and exits with status `1`.

## Production guidance

Provide secrets through the deployment environment or secret manager rather
than shipping `.env`. Use a separate bot application for testing, keep the bot's
permissions minimal, and rotate the token immediately if it is exposed. Do not
print a `Config` object or log raw configuration values.
