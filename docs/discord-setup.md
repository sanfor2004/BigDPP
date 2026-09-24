# Discord Application Setup

BigDPP uses a Discord Gateway connection through DPP. It does not need a public interactions endpoint or an HTTP tunnel.

## 1. Create the application

1. Open the [Discord Developer Portal](https://discord.com/developers/applications).
2. Select **New Application** and name it `BigDPP`.
3. Open the **Bot** page. Newly created applications normally already have a bot user.
4. Under **Token**, generate or reset the bot token.
5. Put the token only in the local ignored `.env` file:

```text
DISCORD_TOKEN=paste-token-here
```

Never paste the token into source code, documentation, chat, screenshots, Git commits, or logs. Reset it immediately if it is exposed.

From the repository root, create the local file from the safe template:

```powershell
Copy-Item .env.example .env
```

## 2. Configure installation

BigDPP is a server-operations bot, so use the **Guild Install** context.

Under the guild-install default settings, select:

- `applications.commands` scope
- `bot` scope
- **View Channels** permission
- **Send Messages** permission
- **Read Message History** permission for `/messages-purge` and normal channel use
- **Manage Channels** for channel setup, editing, locking, and access changes
- **Manage Roles** for role setup and role/member role changes
- **Manage Messages** for `/messages-purge`
- **Moderate Members** for warnings and timeouts
- **Kick Members** for `/member-kick`
- **Ban Members** for `/member-ban` and `/member-unban`

Do not select Administrator for the bot. BigDPP checks the bot permission needed
for each operation at runtime. Install it in a dedicated test server first,
and place its bot role above roles and members it must manage.

Copy the installation link and install BigDPP into a dedicated test server first—not the production server.

## 3. Gateway intents

BigDPP requests the standard **Guilds** and **Guild Members** intents so it can
evaluate role hierarchy and target members. No other privileged Gateway intent
is required. In the Developer Portal, enable:

- Server Members Intent

Leave these disabled:

- Presence Intent
- Message Content Intent

The Server Members Intent is required by the current administrator command
surface for cached member and role-hierarchy checks.

## 4. Configure fast development commands

Guild-scoped commands update quickly and are preferable during development.

1. Enable Discord Developer Mode in your Discord client.
2. Right-click the test server and copy its server ID.
3. Add it to `.env`:

```text
DEVELOPMENT_GUILD_ID=your-test-server-id
```

When this value is absent, BigDPP registers commands globally. Global command availability may not be immediate.

## 5. Run BigDPP

After building, run from the repository root so BigDPP can locate `.env`:

```powershell
.\out\build\windows-debug\bigdpp.exe
```

Successful startup logs Gateway connection progress, the ready event, and command-registration outcome. Test:

- `/ping` — returns `Pong!`
- `/status` — reports the bot's online identity
- `/ask question:<text>` — asks the local AI in `#ai-agent`
- `/server-setup confirm:true` — additive baseline; creates missing categories, channels, roles, and a restricted `#mod-log`
- `/server-info` and `/audit-log` — inspect the server and the audit destination
- `/server-edit`, channel commands, role commands, member commands, and `/messages-purge` — explicit administrative operations

Administrative commands authorize the current Discord guild owner or a member
with Discord Administrator; a user-created role named `Administrator` is not
trusted. Validation responses disable generated mentions and are ephemeral;
deferred operations are acknowledged and completed asynchronously. Commands
enforce role hierarchy and write outcomes to the process log and `#mod-log` when
that channel exists. Destructive commands first show a preview and require
`confirm:true`. `/server-setup` never deletes, renames, or overwrites existing
items. See the [command reference](commands.md) for the complete parameter and
permission table.

## Local AI setup

Install Ollama locally, pull the model named by `BIGDPP_LLM_MODEL`, and enable
the feature in `.env`. BigDPP uses Ollama's loopback API with the configured
local model:

```text
BIGDPP_LLM_ENABLED=true
BIGDPP_LLM_AUTOSTART=true
BIGDPP_LLM_BASE_URL=http://127.0.0.1:11434
BIGDPP_LLM_MODEL=llama3.2
```

Download the model before starting BigDPP:

```powershell
ollama pull llama3.2
```

With `BIGDPP_LLM_AUTOSTART=true`, BigDPP starts `ollama serve` automatically.
Set `BIGDPP_LLM_AUTOSTART=false` when Ollama is managed separately, and set
`BIGDPP_LLM_EXECUTABLE` if `ollama` is not on the process `PATH`.

On ready, BigDPP finds or creates one text channel named by
`BIGDPP_AI_CHANNEL_NAME` (default: `ai-agent`) in each guild it is connected
to. Every member can use `/ask` there; the command is rejected in other
channels. BigDPP sends only the question to the loopback Ollama endpoint and
disables Discord mentions in generated replies.

Stop the process with `Ctrl+C`. Graceful signal handling will be strengthened during production hardening.

## Troubleshooting

- **Missing token:** confirm `DISCORD_TOKEN` is non-empty in `.env` and run from the repository root.
- **Commands do not appear:** set `DEVELOPMENT_GUILD_ID` to the test server ID and confirm the app was installed with `applications.commands`.
- **Gateway rejects intents:** enable Server Members Intent in the Developer Portal; BigDPP uses it for member and role-hierarchy checks.
- **Bot cannot reply:** verify View Channels and Send Messages, including channel-level permission overrides.

## Official references

- [Discord: Building your first Discord bot](https://docs.discord.com/developers/quick-start/getting-started)
- [Discord: Application commands](https://docs.discord.com/developers/interactions/application-commands)
- [Discord: OAuth2 scopes and bot permissions](https://docs.discord.com/developers/platform/oauth2-and-permissions)
- [DPP 10.1.5 API reference](https://dpp.dev/10.1.5/)
