#include "bigdpp/config.hpp"

#include <charconv>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string_view>

namespace bigdpp {
namespace {

std::string trim(std::string value) {
	const auto first = value.find_first_not_of(" \t\r\n");
	if (first == std::string::npos) {
		return {};
	}

	const auto last = value.find_last_not_of(" \t\r\n");
	return value.substr(first, last - first + 1);
}

std::string unquote(std::string value) {
	if (value.size() >= 2 &&
		((value.front() == '\'' && value.back() == '\'') ||
		 (value.front() == '"' && value.back() == '"'))) {
		return value.substr(1, value.size() - 2);
	}
	return value;
}

std::string dotenv_value(std::string_view key) {
	std::ifstream dotenv{".env"};
	if (!dotenv) {
		return {};
	}

	std::string line;
	while (std::getline(dotenv, line)) {
		line = trim(line);
		if (line.empty() || line.front() == '#') {
			continue;
		}

		const auto separator = line.find('=');
		if (separator == std::string::npos || trim(line.substr(0, separator)) != key) {
			continue;
		}

		return unquote(trim(line.substr(separator + 1)));
	}

	return {};
}

std::string environment_value(const char* key) {
#ifdef _WIN32
	char* raw_value = nullptr;
	size_t value_length = 0;
	if (_dupenv_s(&raw_value, &value_length, key) != 0 || raw_value == nullptr) {
		return {};
	}

	std::string value{raw_value};
	std::free(raw_value);
	return value;
#else
	if (const char* value = std::getenv(key); value != nullptr && *value != '\0') {
		return value;
	}
	return {};
#endif
}

std::string setting_value(const char* key) {
	const std::string environment = environment_value(key);
	return environment.empty() ? dotenv_value(key) : environment;
}

std::optional<std::uint64_t> parse_guild_id(const std::string& value) {
	if (value.empty()) {
		return std::nullopt;
	}

	std::uint64_t guild_id = 0;
	const auto result = std::from_chars(value.data(), value.data() + value.size(), guild_id);
	if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || guild_id == 0) {
		throw std::runtime_error{"DEVELOPMENT_GUILD_ID must be a non-zero decimal Discord ID"};
	}

	return guild_id;
}

} // namespace

Config Config::load() {
	Config config;
	config.discord_token = setting_value("DISCORD_TOKEN");
	if (config.discord_token.empty()) {
		throw std::runtime_error{"DISCORD_TOKEN is required; set it in the environment or .env"};
	}

	config.development_guild_id = parse_guild_id(setting_value("DEVELOPMENT_GUILD_ID"));
	return config;
}

} // namespace bigdpp
