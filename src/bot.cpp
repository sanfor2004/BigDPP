#include "bigdpp/bot.hpp"

#include <iostream>
#include <utility>
#include <vector>

namespace bigdpp {

Bot::Bot(std::string token, const std::optional<std::uint64_t> development_guild_id)
	: cluster_{std::move(token), dpp::i_guilds} {
	if (development_guild_id.has_value()) {
		development_guild_id_ = dpp::snowflake{*development_guild_id};
	}

	cluster_.on_log(dpp::utility::cout_logger());
	cluster_.on_ready([this](const dpp::ready_t&) { register_commands(); });
	cluster_.on_slashcommand([this](const dpp::slashcommand_t& event) { handle_command(event); });
}

void Bot::run() {
	cluster_.start(dpp::st_wait);
}

void Bot::register_commands() {
	if (commands_registered_.exchange(true)) {
		return;
	}

	std::vector<dpp::slashcommand> commands;
	commands.emplace_back("ping", "Check that BigDPP is reachable", cluster_.me.id);
	commands.emplace_back("status", "Show basic BigDPP connection status", cluster_.me.id);

	auto completion = [this](const dpp::confirmation_callback_t& result) {
		if (result.is_error()) {
			commands_registered_.store(false);
			std::cerr << "BigDPP command registration failed\n";
			return;
		}

		std::cout << "BigDPP slash commands registered\n";
	};

	if (development_guild_id_.has_value()) {
		cluster_.guild_bulk_command_create(commands, *development_guild_id_, completion);
	} else {
		cluster_.global_bulk_command_create(commands, completion);
	}
}

void Bot::handle_command(const dpp::slashcommand_t& event) {
	try {
		const std::string command_name = event.command.get_command_name();
		if (command_name == "ping") {
			event.reply("Pong!");
			return;
		}

		if (command_name == "status") {
			event.reply("BigDPP is online as " + cluster_.me.username + ".");
			return;
		}

		event.reply(dpp::message{"Unknown command"}.set_flags(dpp::m_ephemeral));
	} catch (const std::exception& error) {
		std::cerr << "BigDPP command failed: " << error.what() << "\n";
		event.reply(dpp::message{"The command could not be completed."}.set_flags(dpp::m_ephemeral));
	}
}

} // namespace bigdpp
