#include <dpp/dpp.h>

#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>

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

std::string token_from_dotenv() {
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
		if (separator == std::string::npos ||
			trim(line.substr(0, separator)) != "DISCORD_TOKEN") {
			continue;
		}

		return unquote(trim(line.substr(separator + 1)));
	}

	return {};
}

std::string token_from_environment() {
#ifdef _WIN32
	char* raw_value = nullptr;
	size_t value_length = 0;
	if (_dupenv_s(&raw_value, &value_length, "DISCORD_TOKEN") != 0 || raw_value == nullptr) {
		return {};
	}

	std::string value{raw_value};
	std::free(raw_value);
	return value;
#else
	if (const char* value = std::getenv("DISCORD_TOKEN"); value != nullptr && *value != '\0') {
		return value;
	}
	return {};
#endif
}

std::string discord_token() {
	const std::string environment_value = token_from_environment();
	return environment_value.empty() ? token_from_dotenv() : environment_value;
}

} // namespace

int main() {
	try {
		const std::string token = discord_token();
		if (token.empty()) {
			std::cerr << "DISCORD_TOKEN is required; set it in the environment or .env\n";
			return 1;
		}

		dpp::cluster bot{token, dpp::i_guilds};
		bot.on_log(dpp::utility::cout_logger());
		bot.on_ready([&bot](const dpp::ready_t&) {
			std::cout << "Hello From " << bot.me.username << "!\n";
		});
		bot.start(dpp::st_wait);
	} catch (const std::exception& error) {
		std::cerr << "BigDPP failed: " << error.what() << "\n";
		return 1;
	}
}
