#pragma once

#include "bigdpp/admin.hpp"
#include "bigdpp/config.hpp"
#include "bigdpp/llm.hpp"

#include <dpp/dpp.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace bigdpp {

class Bot final {
public:
	Bot(
		std::string token,
		std::optional<std::uint64_t> development_guild_id,
		LocalLlmConfig local_llm_config);

	void run();

private:
	void mark_ready();
	void register_commands();
	void ensure_ai_channels();
	void ensure_ai_channel(dpp::snowflake guild_id);
	void handle_command(const dpp::slashcommand_t& event);
	void handle_ask(const dpp::slashcommand_t& event);
	void handle_server_setup(const dpp::slashcommand_t& event);
	void handle_create_role(const dpp::slashcommand_t& event);
	void complete_ask(const dpp::slashcommand_t& event, LocalLlmResult result);
	[[nodiscard]] bool is_guild_owner(const dpp::slashcommand_t& event) const;
	[[nodiscard]] bool bot_can_manage_roles(dpp::snowflake guild_id) const;
	[[nodiscard]] bool is_ai_channel(dpp::snowflake guild_id, dpp::snowflake channel_id) const;

	dpp::cluster cluster_;
	AdminService admin_service_;
	LocalLlmConfig local_llm_config_;
	LocalLlmRuntime local_llm_runtime_;
	LocalLlmClient local_llm_client_;
	std::optional<dpp::snowflake> development_guild_id_;
	std::atomic_bool commands_registered_{false};
	mutable std::mutex ai_channel_mutex_;
	std::unordered_set<std::uint64_t> ai_channel_ids_;
	std::unordered_set<std::uint64_t> ai_channel_setup_guilds_;
};

} // namespace bigdpp
