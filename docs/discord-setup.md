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

## 2. Configure installation

BigDPP is a server-operations bot, so use the **Guild Install** context.

Under the guild-install default settings, select:

- `applications.commands` scope
- `bot` scope
- **View Channels** permission
- **Send Messages** permission

These are sufficient for the currently implemented `/ping` and `/status` foundation. Do not select Administrator. Later phases will document and request additional permissions only when their corresponding moderation or maintenance features are implemented.

Copy the installation link and install BigDPP into a dedicated test server first—not the production server.

## 3. Gateway intents

Phase 1 requests only Discord's standard **Guilds** intent. No privileged Gateway intent is required, so leave these disabled for now:

- Presence Intent
- Server Members Intent
- Message Content Intent

Future event/onboarding work may require Server Members, but it should be enabled only when that phase is implemented and tested.

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
.\build\bigdpp.exe
```

Successful startup logs Gateway connection progress, the ready event, and command-registration outcome. Test:

- `/ping` — returns `Pong!`
- `/status` — reports version, uptime, guild/member cache information, and Gateway latency

Stop the process with `Ctrl+C`. Graceful signal handling will be strengthened during production hardening.

## Troubleshooting

- **Missing token:** confirm `DISCORD_TOKEN` is non-empty in `.env` and run from the repository root.
- **Commands do not appear:** set `DEVELOPMENT_GUILD_ID` to the test server ID and confirm the app was installed with `applications.commands`.
- **Gateway rejects intents:** Phase 1 should use only Guilds; confirm local code has not been changed to request a privileged intent.
- **Bot cannot reply:** verify View Channels and Send Messages, including channel-level permission overrides.

## Official references

- [Discord: Building your first Discord bot](https://docs.discord.com/developers/quick-start/getting-started)
- [Discord: Application commands](https://docs.discord.com/developers/interactions/application-commands)
- [Discord: OAuth2 scopes and bot permissions](https://docs.discord.com/developers/platform/oauth2-and-permissions)
- [DPP 10.1.5 API reference](https://dpp.dev/10.1.5/)
