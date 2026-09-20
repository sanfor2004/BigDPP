#pragma once

#include <dpp/dpp.h>

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>

namespace bigdpp {

class Bot final {
public:
	Bot(std::string token, std::optional<std::uint64_t> development_guild_id);

	void run();

private:
	void register_commands();
	void handle_command(const dpp::slashcommand_t& event);

	dpp::cluster cluster_;
	std::optional<dpp::snowflake> development_guild_id_;
	std::atomic_bool commands_registered_{false};
};

} // namespace bigdpp
