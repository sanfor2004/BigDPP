#pragma once

#include <optional>
#include <stdexcept>
#include <string>

namespace bigdpp::config {

class ConfigError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct Config final {
    std::string environment{"development"};
    std::string log_level{"info"};
    std::optional<std::string> discord_token;
    std::optional<std::string> database_url;

    [[nodiscard]] static Config from_environment();
    void validate() const;
};

} // namespace bigdpp::config
