#include "bigdpp/llm.hpp"

#include <nlohmann/json.hpp>

#include <exception>
#include <utility>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace bigdpp {

LocalLlmRuntime::LocalLlmRuntime(LocalLlmConfig config)
	: config_{std::move(config)} {}

LocalLlmRuntime::~LocalLlmRuntime() {
#ifdef _WIN32
	if (process_handle_ != nullptr) {
		const auto handle = static_cast<HANDLE>(process_handle_);
		if (WaitForSingleObject(handle, 0) == WAIT_TIMEOUT) {
			TerminateProcess(handle, 0);
			WaitForSingleObject(handle, 2000);
		}
		CloseHandle(handle);
	}
#else
	if (process_id_ > 0) {
		const auto process_id = static_cast<pid_t>(process_id_);
		if (kill(process_id, 0) == 0) {
			kill(process_id, SIGTERM);
			waitpid(process_id, nullptr, 0);
		}
	}
#endif
}

bool LocalLlmRuntime::start(std::string& error) {
	if (!config_.enabled || !config_.auto_start) {
		return true;
	}

#ifdef _WIN32
	std::string command_line = "\"" + config_.executable + "\" serve";
	STARTUPINFOA startup_info{};
	startup_info.cb = sizeof(startup_info);
	PROCESS_INFORMATION process_info{};
	if (CreateProcessA(
		nullptr,
		command_line.data(),
		nullptr,
		nullptr,
		FALSE,
		CREATE_NO_WINDOW,
		nullptr,
		nullptr,
		&startup_info,
		&process_info) == 0) {
		error = "could not start " + config_.executable + " serve (Windows error " +
			std::to_string(GetLastError()) + ")";
		return false;
	}

	CloseHandle(process_info.hThread);
	process_handle_ = process_info.hProcess;
	return true;
#else
	const pid_t child = fork();
	if (child < 0) {
		error = "could not fork " + config_.executable + " serve";
		return false;
	}
	if (child == 0) {
		execlp(config_.executable.c_str(), config_.executable.c_str(), "serve", nullptr);
		_exit(127);
	}
	process_id_ = static_cast<int>(child);
	return true;
#endif
}

LocalLlmClient::LocalLlmClient(dpp::cluster& cluster, LocalLlmConfig config)
	: cluster_{cluster}, config_{std::move(config)} {}

void LocalLlmClient::ask(std::string prompt, Completion completion) const {
	if (!completion) {
		return;
	}

	std::string endpoint = config_.base_url;
	if (!endpoint.empty() && endpoint.back() == '/') {
		endpoint.pop_back();
	}
	endpoint += "/api/chat";
	const nlohmann::json request_body{
		{"model", config_.model},
		{"messages", nlohmann::json::array({
			{
				{"role", "system"},
				{"content", "You are BigDPP's local Discord assistant. Answer the user's question directly and clearly. You cannot see server data or perform Discord actions. Keep the answer below 1800 characters."},
			},
			{
				{"role", "user"},
				{"content", std::move(prompt)},
			},
		})},
		{"stream", false},
		{"options", {{"num_predict", 384}}},
	};

	try {
		cluster_.request(
			endpoint,
			dpp::m_post,
			[completion](const dpp::http_request_completion_t& result) mutable {
				if (result.error != dpp::h_success || result.status < 200 || result.status >= 300) {
					completion({false, {}, "The local AI service did not return a successful response."});
					return;
				}

				try {
					const nlohmann::json response = nlohmann::json::parse(result.body);
					const std::string answer = response.at("message").at("content").get<std::string>();
					if (answer.empty()) {
						completion({false, {}, "The local AI service returned an empty response."});
						return;
					}
					completion({true, answer, {}});
				} catch (const std::exception&) {
					completion({false, {}, "The local AI service returned an invalid response."});
				}
			},
			request_body.dump(),
			"application/json");
	} catch (const std::exception&) {
		completion({false, {}, "The local AI service request could not be started."});
	}
}

} // namespace bigdpp
