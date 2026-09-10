#include <dpp/dpp.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
	std::string read_discord_token() {
	#ifdef _WIN32
		char* raw_value = nullptr;
		std::size_t value_size = 0;

		if (_dupenv_s(&raw_value, &value_size, "DISCORD_TOKEN") != 0) {
			throw std::runtime_error{"Unable to read DISCORD_TOKEN"};
		}

		const std::unique_ptr<char, decltype(&std::free)> value{
			raw_value,
			&std::free
		};
		return value ? std::string{value.get()} : std::string{};
	#else
		const char* value = std::getenv("DISCORD_TOKEN");
		return value ? std::string{value} : std::string{};
	#endif
	}
}

int main() {
	try {
		const std::string token = read_discord_token();
		if (token.empty()) {
			std::cerr<<"DISCORD TOKEN IS REQUIRE";
			return 1;
		}
		dpp::cluster bot{token, dpp::i_guilds};
		bot.on_log(dpp::utility::cout_logger());
		bot.on_ready([&bot](const dpp::ready_t&){
				std::cout << "Hello From " << bot.me.username << "!\n";
		});
		bot.start(dpp::st_wait);
	} catch (const std::exception& error) {
		std::cerr <<"BigDPP failed: "<<error.what() <<"\n";
		return 1;

	}
}
