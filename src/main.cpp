#include <dpp/dpp.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include "token.h"

int main() {
	try {
		const std::string token = DISCORD_TOKEN;
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
