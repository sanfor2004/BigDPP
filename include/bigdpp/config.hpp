#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace bigdpp {

struct Config final {
	std::string discord_token;
	std::optional<std::uint64_t> development_guild_id;

	[[nodiscard]] static Config load();
};

} // namespace bigdpp
