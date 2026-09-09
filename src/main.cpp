#include "bigdpp/config/config.hpp"
#include "bigdpp/logging/logging.hpp"

#include <spdlog/spdlog.h>

#include <exception>

#ifndef BIGDPP_VERSION
#define BIGDPP_VERSION "development"
#endif

int main() {
    try {
        const auto config = bigdpp::config::Config::from_environment();
        bigdpp::logging::initialize(config.log_level);

        spdlog::info(
            "BigDPP {} foundation initialized (environment={})",
            BIGDPP_VERSION,
            config.environment);
        return 0;
    } catch (const bigdpp::config::ConfigError& error) {
        spdlog::critical("Invalid configuration: {}", error.what());
    } catch (const std::exception& error) {
        spdlog::critical("BigDPP startup failed: {}", error.what());
    }

    return 1;
}
