#include "bigdpp/config/config.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

namespace bigdpp::config {
namespace {

[[nodiscard]] std::optional<std::string> read_environment(const char* name) {
#ifdef _WIN32
    char* value = nullptr;
    std::size_t value_size = 0;
    if (_dupenv_s(&value, &value_size, name) != 0) {
        throw ConfigError{"Unable to read process environment"};
    }

    const std::unique_ptr<char, decltype(&std::free)> owned_value{value, &std::free};
#else
    const char* value = std::getenv(name);
#endif

    if (value == nullptr || *value == '\0') {
        return std::nullopt;
    }

    return std::string{value};
}

[[nodiscard]] std::string lowercase(std::string value) {
    std::ranges::transform(value, value.begin(), [](const unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

} // namespace

Config Config::from_environment() {
    Config config;

    if (auto environment = read_environment("ENVIRONMENT")) {
        config.environment = std::move(*environment);
    }
    if (auto log_level = read_environment("LOG_LEVEL")) {
        config.log_level = lowercase(std::move(*log_level));
    }

    config.discord_token = read_environment("DISCORD_TOKEN");
    config.database_url = read_environment("DATABASE_URL");
    config.validate();
    return config;
}

void Config::validate() const {
    if (environment.empty()) {
        throw ConfigError{"ENVIRONMENT must not be empty"};
    }

    constexpr std::array<std::string_view, 7> valid_levels{
        "trace", "debug", "info", "warn", "error", "critical", "off"};

    if (std::ranges::find(valid_levels, log_level) == valid_levels.end()) {
        throw ConfigError{"LOG_LEVEL must be one of: trace, debug, info, warn, error, critical, off"};
    }
}

} // namespace bigdpp::config
