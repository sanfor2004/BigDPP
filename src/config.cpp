#include "bigdpp/config.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
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

std::string dotenv_value(const std::filesystem::path& dotenv_path, std::string_view key) {
	std::ifstream dotenv{dotenv_path};
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

std::filesystem::path discover_dotenv_path() {
	std::error_code error;
	std::filesystem::path directory = std::filesystem::current_path(error);
	if (error) {
		return ".env";
	}

	while (true) {
		const std::filesystem::path candidate = directory / ".env";
		if (std::filesystem::is_regular_file(candidate, error)) {
			return candidate;
		}

		if (std::filesystem::exists(directory / ".git", error)) {
			break;
		}

		const std::filesystem::path parent = directory.parent_path();
		if (parent == directory) {
			break;
		}
		directory = parent;
	}

	return ".env";
}

std::string environment_value(std::string_view key) {
#ifdef _WIN32
	char* raw_value = nullptr;
	size_t value_length = 0;
	const std::string key_string{key};
	if (_dupenv_s(&raw_value, &value_length, key_string.c_str()) != 0 || raw_value == nullptr) {
		return {};
	}

	std::string value{raw_value};
	std::free(raw_value);
	return value;
#else
	const std::string key_string{key};
	if (const char* value = std::getenv(key_string.c_str()); value != nullptr && *value != '\0') {
		return value;
	}
	return {};
#endif
}

std::string setting_value(
	std::string_view key,
	const std::filesystem::path& dotenv_path,
	const std::function<std::string(std::string_view)>& environment_lookup) {
	const std::string environment = environment_lookup(key);
	return environment.empty() ? dotenv_value(dotenv_path, key) : environment;
}

std::optional<std::uint64_t> parse_id(const std::string& value, const std::string_view field_name) {
	if (value.empty()) {
		return std::nullopt;
	}

	std::uint64_t guild_id = 0;
	const auto result = std::from_chars(value.data(), value.data() + value.size(), guild_id);
	if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || guild_id == 0) {
		throw std::runtime_error{std::string{field_name} + " must be a non-zero decimal Discord ID"};
	}

	return guild_id;
}

bool parse_bool(const std::string& value, const std::string_view field_name) {
	std::string normalized = value;
	std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](const unsigned char character) {
		return static_cast<char>(std::tolower(character));
	});

	if (normalized == "1" || normalized == "true" || normalized == "yes") {
		return true;
	}
	if (normalized.empty() || normalized == "0" || normalized == "false" || normalized == "no") {
		return false;
	}
	throw std::runtime_error{std::string{field_name} + " must be true or false"};
}

bool is_loopback_http_url(const std::string& value) {
	constexpr std::string_view scheme = "http://";
	if (value.rfind(scheme, 0) != 0) {
		return false;
	}

	const auto authority_start = scheme.size();
	const auto authority_end = value.find_first_of("/?#", authority_start);
	const std::string authority = value.substr(
		authority_start,
		authority_end == std::string::npos ? std::string::npos : authority_end - authority_start);
	if (authority.empty() || authority.find('@') != std::string::npos) {
		return false;
	}

	const auto port_separator = authority.find(':');
	const std::string host = authority.substr(0, port_separator);
	if (host != "localhost" && host != "127.0.0.1") {
		return false;
	}
	if (port_separator == std::string::npos) {
		return true;
	}
	if (authority.find(':', port_separator + 1) != std::string::npos) {
		return false;
	}

	const std::string port_text = authority.substr(port_separator + 1);
	std::uint32_t port = 0;
	const auto result = std::from_chars(port_text.data(), port_text.data() + port_text.size(), port);
	return result.ec == std::errc{} && result.ptr == port_text.data() + port_text.size() &&
		port > 0 && port <= 65535;
}

} // namespace

Config Config::load() {
	return load_from(discover_dotenv_path(), [](const std::string_view key) { return environment_value(key); });
}

Config Config::load_from(
	const std::filesystem::path& dotenv_path,
	const std::function<std::string(std::string_view)>& environment_lookup) {
	Config config;
	config.discord_token = setting_value("DISCORD_TOKEN", dotenv_path, environment_lookup);
	if (config.discord_token.empty()) {
		throw std::runtime_error{"DISCORD_TOKEN is required; set it in the environment or .env"};
	}

	config.development_guild_id = parse_id(
		setting_value("DEVELOPMENT_GUILD_ID", dotenv_path, environment_lookup),
		"DEVELOPMENT_GUILD_ID");

	config.local_llm.enabled = parse_bool(
		setting_value("BIGDPP_LLM_ENABLED", dotenv_path, environment_lookup),
		"BIGDPP_LLM_ENABLED");
	const std::string configured_llm_autostart = setting_value(
		"BIGDPP_LLM_AUTOSTART", dotenv_path, environment_lookup);
	if (!configured_llm_autostart.empty()) {
		config.local_llm.auto_start = parse_bool(configured_llm_autostart, "BIGDPP_LLM_AUTOSTART");
	}
	const std::string configured_llm_url = setting_value(
		"BIGDPP_LLM_BASE_URL", dotenv_path, environment_lookup);
	if (!configured_llm_url.empty()) {
		config.local_llm.base_url = configured_llm_url;
	}
	if (!is_loopback_http_url(config.local_llm.base_url)) {
		throw std::runtime_error{"BIGDPP_LLM_BASE_URL must be a loopback http://localhost or http://127.0.0.1 URL"};
	}

	const std::string configured_llm_model = setting_value(
		"BIGDPP_LLM_MODEL", dotenv_path, environment_lookup);
	if (!configured_llm_model.empty()) {
		config.local_llm.model = configured_llm_model;
	}
	if (config.local_llm.enabled && config.local_llm.model.empty()) {
		throw std::runtime_error{"BIGDPP_LLM_MODEL is required when BIGDPP_LLM_ENABLED is true"};
	}

	const std::string configured_llm_executable = setting_value(
		"BIGDPP_LLM_EXECUTABLE", dotenv_path, environment_lookup);
	if (!configured_llm_executable.empty()) {
		config.local_llm.executable = configured_llm_executable;
	}
	if (config.local_llm.executable.empty() || config.local_llm.executable.find('"') != std::string::npos) {
		throw std::runtime_error{"BIGDPP_LLM_EXECUTABLE must be a non-empty path without quote characters"};
	}

	const std::string configured_ai_channel = setting_value(
		"BIGDPP_AI_CHANNEL_NAME", dotenv_path, environment_lookup);
	if (!configured_ai_channel.empty()) {
		config.local_llm.channel_name = configured_ai_channel;
	}
	if (config.local_llm.channel_name.empty() || config.local_llm.channel_name.size() > 100) {
		throw std::runtime_error{"BIGDPP_AI_CHANNEL_NAME must contain between 1 and 100 characters"};
	}

	return config;
}

} // namespace bigdpp
