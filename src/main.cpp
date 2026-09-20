#include <exception>
#include <iostream>
#include "bigdpp/bot.hpp"
#include "bigdpp/config.hpp"

int main() {
	try {
		const bigdpp::Config config = bigdpp::Config::load();
		bigdpp::Bot bot{config.discord_token, config.development_guild_id};
		bot.run();
	} catch (const std::exception& error) {
		std::cerr << "BigDPP failed: " << error.what() << "\n";
		return 1;
	}
}
