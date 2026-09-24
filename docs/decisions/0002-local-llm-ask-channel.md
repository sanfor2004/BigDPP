# ADR 0002: Local LLM Adapter and Dedicated Ask Channel

## Status

Accepted

## Context

The server needs a shared `/ask` command that any member can use without
exposing Discord credentials or server state to a hosted AI provider. Discord
interactions must also remain responsive while a local model may take several
seconds to answer.

## Decision

Use Ollama's local HTTP `POST /api/chat` endpoint with non-streaming responses.
BigDPP sends one bounded user question plus a fixed prompt describing the
assistant's limits. The endpoint is configuration-backed but validated to
`http://localhost` or `http://127.0.0.1` only.

When enabled, BigDPP discovers or creates one text channel named by
`BIGDPP_AI_CHANNEL_NAME` (default `ai-agent`) per connected guild. `/ask` is
accepted only in that channel, but no member permission gate is applied. The
command acknowledges immediately, submits the model request through DPP's
asynchronous raw HTTP queue, and edits the original Discord response when the
result arrives. Generated Discord mentions are disabled and oversized output
is bounded before it is sent.

## Consequences

The bot needs `Manage Channels` during initial channel creation in addition to
the normal view/send/application-command permissions. Channel creation is a
real Discord mutation, so startup logs its failure without terminating the bot;
operators may create the channel manually if the permission is intentionally
not granted.

The current adapter is prompt-only. It does not provide the model with Discord
tools, guild data, or the bot token. Conversation history, persistence, model
streaming, and any AI-triggered Discord operation require a separate design and
authorization review.
