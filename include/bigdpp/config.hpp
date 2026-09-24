#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace bigdpp {

struct LocalLlmConfig final {
	bool enabled{false};
	bool auto_start{true};
	std::string base_url{"http://127.0.0.1:11434"};
	std::string model;
	std::string channel_name{"ai-agent"};
	std::string executable{"ollama"};
};

struct Config final {
	std::string discord_token;
	std::optional<std::uint64_t> development_guild_id;
	LocalLlmConfig local_llm;

	[[nodiscard]] static Config load();
	[[nodiscard]] static Config load_from(
		const std::filesystem::path& dotenv_path,
		const std::function<std::string(std::string_view)>& environment_lookup);
};

} // namespace bigdpp
