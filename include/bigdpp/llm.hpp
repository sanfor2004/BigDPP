#pragma once

#include "bigdpp/config.hpp"

#include <dpp/cluster.h>

#include <functional>
#include <string>

namespace bigdpp {

class LocalLlmRuntime final {
public:
	LocalLlmRuntime() = delete;
	explicit LocalLlmRuntime(LocalLlmConfig config);
	~LocalLlmRuntime();

	LocalLlmRuntime(const LocalLlmRuntime&) = delete;
	LocalLlmRuntime& operator=(const LocalLlmRuntime&) = delete;

	[[nodiscard]] bool start(std::string& error);

private:
	LocalLlmConfig config_;
#ifdef _WIN32
	void* process_handle_{nullptr};
#else
	int process_id_{-1};
#endif
};

struct LocalLlmResult final {
	bool success{false};
	std::string response;
	std::string error;
};

class LocalLlmClient final {
public:
	using Completion = std::function<void(LocalLlmResult)>;

	LocalLlmClient(dpp::cluster& cluster, LocalLlmConfig config);

	void ask(std::string prompt, Completion completion) const;

private:
	dpp::cluster& cluster_;
	LocalLlmConfig config_;
};

} // namespace bigdpp
