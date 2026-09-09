#include "bigdpp/logging/logging.hpp"

#include <spdlog/spdlog.h>

#include <stdexcept>
#include <string>

namespace bigdpp::logging {

void initialize(const std::string_view level) {
    const auto parsed_level = spdlog::level::from_str(std::string{level});
    if (parsed_level == spdlog::level::off && level != "off") {
        throw std::invalid_argument{"Unsupported log level"};
    }

    spdlog::set_level(parsed_level);
    spdlog::set_pattern("%Y-%m-%dT%H:%M:%S.%e%z [%l] %v");
}

} // namespace bigdpp::logging
