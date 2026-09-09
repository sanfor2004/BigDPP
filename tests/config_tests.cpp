#include "bigdpp/config/config.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

TEST_CASE("default configuration is valid") {
    const bigdpp::config::Config config;

    REQUIRE_NOTHROW(config.validate());
    CHECK(config.environment == "development");
    CHECK(config.log_level == "info");
    CHECK_FALSE(config.discord_token.has_value());
    CHECK_FALSE(config.database_url.has_value());
}

TEST_CASE("configuration rejects an empty environment name") {
    auto config = bigdpp::config::Config{};
    config.environment.clear();

    REQUIRE_THROWS_WITH(config.validate(), "ENVIRONMENT must not be empty");
}

TEST_CASE("configuration rejects unsupported log levels") {
    auto config = bigdpp::config::Config{};
    config.log_level = "verbose";

    REQUIRE_THROWS_WITH(
        config.validate(),
        "LOG_LEVEL must be one of: trace, debug, info, warn, error, critical, off");
}

TEST_CASE("configuration accepts every documented log level") {
    for (const auto* level : {"trace", "debug", "info", "warn", "error", "critical", "off"}) {
        auto config = bigdpp::config::Config{};
        config.log_level = level;
        CHECK_NOTHROW(config.validate());
    }
}
