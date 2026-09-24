#include "bigdpp/config.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

class TemporaryDotenv final {
public:
	TemporaryDotenv()
		: path_{std::filesystem::temp_directory_path() / "bigdpp-config-tests.env"} {}

	~TemporaryDotenv() {
		std::error_code error;
		std::filesystem::remove(path_, error);
	}

	void write(const std::string& contents) const {
		std::ofstream file{path_};
		assert(file);
		file << contents;
	}

	[[nodiscard]] const std::filesystem::path& path() const noexcept {
		return path_;
	}

private:
	std::filesystem::path path_;
};

std::function<std::string(std::string_view)> environment(
	const std::map<std::string, std::string>& values) {
	return [&values](const std::string_view key) {
		const auto found = values.find(std::string{key});
		return found == values.end() ? std::string{} : found->second;
	};
}

template <typename Function>
void expect_failure(Function&& function, const std::string_view expected_fragment) {
	try {
		function();
		assert(false);
	} catch (const std::runtime_error& error) {
		assert(std::string{error.what()}.find(expected_fragment) != std::string::npos);
	}
}

void loads_configuration_and_respects_environment_precedence() {
	TemporaryDotenv dotenv;
	dotenv.write(
		"DISCORD_TOKEN=file-token\n"
		"DEVELOPMENT_GUILD_ID=123456789012345678\n"
		"BIGDPP_LLM_ENABLED=true\n"
		"BIGDPP_LLM_AUTOSTART=false\n"
		"BIGDPP_LLM_BASE_URL=http://localhost:11434\n"
		"BIGDPP_LLM_MODEL=llama3.1\n"
		"BIGDPP_AI_CHANNEL_NAME=ai-agent\n"
		"BIGDPP_LLM_EXECUTABLE=ollama-custom\n");

	const bigdpp::Config config = bigdpp::Config::load_from(
		dotenv.path(),
		environment({{"DISCORD_TOKEN", "environment-token"}}));

	assert(config.discord_token == "environment-token");
	assert(config.development_guild_id == 123456789012345678ULL);
	assert(config.local_llm.enabled);
	assert(!config.local_llm.auto_start);
	assert(config.local_llm.base_url == "http://localhost:11434");
	assert(config.local_llm.model == "llama3.1");
	assert(config.local_llm.channel_name == "ai-agent");
	assert(config.local_llm.executable == "ollama-custom");
}

void loads_development_guild_id() {
	TemporaryDotenv dotenv;
	dotenv.write("DISCORD_TOKEN=token\nDEVELOPMENT_GUILD_ID=123456789012345678\n");

	const bigdpp::Config config = bigdpp::Config::load_from(dotenv.path(), environment({}));
	assert(config.development_guild_id == 123456789012345678ULL);
}

void rejects_missing_token() {
	TemporaryDotenv dotenv;
	dotenv.write("\n");
	expect_failure(
		[&] { static_cast<void>(bigdpp::Config::load_from(dotenv.path(), environment({}))); },
		"DISCORD_TOKEN is required");
}

void rejects_invalid_development_guild_configuration() {
	TemporaryDotenv dotenv;
	dotenv.write("DISCORD_TOKEN=token\nDEVELOPMENT_GUILD_ID=not-an-id\n");
	expect_failure(
		[&] { static_cast<void>(bigdpp::Config::load_from(dotenv.path(), environment({}))); },
		"DEVELOPMENT_GUILD_ID");
}

void rejects_invalid_local_llm_configuration() {
	TemporaryDotenv dotenv;
	dotenv.write("DISCORD_TOKEN=token\nBIGDPP_LLM_BASE_URL=https://example.invalid\n");
	expect_failure(
		[&] { static_cast<void>(bigdpp::Config::load_from(dotenv.path(), environment({}))); },
		"BIGDPP_LLM_BASE_URL");

	dotenv.write("DISCORD_TOKEN=token\nBIGDPP_LLM_BASE_URL=http://localhost:not-a-port\n");
	expect_failure(
		[&] { static_cast<void>(bigdpp::Config::load_from(dotenv.path(), environment({}))); },
		"BIGDPP_LLM_BASE_URL");

	dotenv.write("DISCORD_TOKEN=token\nBIGDPP_LLM_ENABLED=true\nBIGDPP_LLM_MODEL=\n");
	expect_failure(
		[&] { static_cast<void>(bigdpp::Config::load_from(dotenv.path(), environment({}))); },
		"BIGDPP_LLM_MODEL");
}

} // namespace

int main() {
	loads_configuration_and_respects_environment_precedence();
	loads_development_guild_id();
	rejects_missing_token();
	rejects_invalid_development_guild_configuration();
	rejects_invalid_local_llm_configuration();
	return 0;
}
