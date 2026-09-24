#include "bigdpp/bot.hpp"

#include <dpp/cache.h>

#include <array>
#include <charconv>
#include <iostream>
#include <algorithm>
#include <memory>
#include <shared_mutex>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace bigdpp {

Bot::Bot(
	std::string token,
	const std::optional<std::uint64_t> development_guild_id,
	LocalLlmConfig local_llm_config)
	: cluster_{std::move(token), dpp::i_guilds | dpp::i_guild_members},
	  admin_service_{cluster_},
	  local_llm_config_{std::move(local_llm_config)},
	  local_llm_runtime_{local_llm_config_},
	  local_llm_client_{cluster_, local_llm_config_} {
	if (development_guild_id.has_value()) {
		development_guild_id_ = dpp::snowflake{*development_guild_id};
	}

	cluster_.on_log(dpp::utility::cout_logger());
	cluster_.on_ready([this](const dpp::ready_t&) { mark_ready(); });
	cluster_.on_guild_create([this](const dpp::guild_create_t& event) {
		if (!local_llm_config_.enabled) {
			return;
		}
		try {
			ensure_ai_channel(event.created.id);
		} catch (const std::exception& error) {
			std::cerr << "BigDPP AI channel setup failed: " << error.what() << "\n";
		}
	});
	cluster_.on_slashcommand([this](const dpp::slashcommand_t& event) { handle_command(event); });
}

void Bot::run() {
	if (local_llm_config_.enabled && local_llm_config_.auto_start) {
		std::string error;
		if (local_llm_runtime_.start(error)) {
			std::cout << "BigDPP local AI runtime started\n";
		} else {
			std::cerr << "BigDPP local AI runtime could not start: " << error << "\n";
		}
	}
	cluster_.start(dpp::st_wait);
}

void Bot::mark_ready() {
	register_commands();
	if (local_llm_config_.enabled) {
		try {
			ensure_ai_channels();
		} catch (const std::exception& error) {
			std::cerr << "BigDPP AI channel setup failed: " << error.what() << "\n";
		}
	}
}

void Bot::register_commands() {
	if (commands_registered_.exchange(true)) {
		return;
	}

	std::vector<dpp::slashcommand> commands;
	commands.emplace_back("ping", "Check that BigDPP is reachable", cluster_.me.id);
	commands.emplace_back("status", "Show basic BigDPP connection status", cluster_.me.id);
	dpp::slashcommand ask{"ask", "Ask BigDPP's local AI assistant", cluster_.me.id};
	dpp::command_option question{
		dpp::co_string,
		"question",
		"The question to ask",
		true};
	question.set_max_length(2000);
	ask.add_option(question);
	commands.push_back(std::move(ask));

	const auto admin_commands = admin_service_.commands(cluster_.me.id);
	commands.insert(commands.end(), admin_commands.begin(), admin_commands.end());

	auto completion = [this](const dpp::confirmation_callback_t& result) {
		if (result.is_error()) {
			commands_registered_.store(false);
			std::cerr << "BigDPP command registration failed\n";
			return;
		}

		std::cout << "BigDPP slash commands registered\n";
	};

	if (development_guild_id_.has_value()) {
		cluster_.guild_bulk_command_create(commands, *development_guild_id_, completion);
	} else {
		cluster_.global_bulk_command_create(commands, completion);
	}
}

void Bot::ensure_ai_channels() {
	std::vector<dpp::snowflake> guild_ids;
	auto* cache = dpp::get_guild_cache();
	{
		std::shared_lock lock{cache->get_mutex()};
		guild_ids.reserve(cache->get_container().size());
		for (const auto& [guild_id, guild] : cache->get_container()) {
			if (guild != nullptr) {
				guild_ids.push_back(guild_id);
			}
		}
	}

	for (const dpp::snowflake guild_id : guild_ids) {
		ensure_ai_channel(guild_id);
	}
}

void Bot::ensure_ai_channel(const dpp::snowflake guild_id) {
	{
		std::lock_guard lock{ai_channel_mutex_};
		if (ai_channel_setup_guilds_.contains(static_cast<std::uint64_t>(guild_id))) {
			return;
		}
		ai_channel_setup_guilds_.insert(static_cast<std::uint64_t>(guild_id));
	}

	std::vector<dpp::snowflake> channel_ids;
	if (const auto* guild = dpp::find_guild(guild_id); guild != nullptr) {
		channel_ids = guild->channels;
	}

	for (const dpp::snowflake channel_id : channel_ids) {
		const auto* channel = dpp::find_channel(channel_id);
		if (channel != nullptr && channel->guild_id == guild_id &&
			channel->get_type() == dpp::CHANNEL_TEXT && channel->name == local_llm_config_.channel_name) {
			std::lock_guard lock{ai_channel_mutex_};
			ai_channel_ids_.insert(static_cast<std::uint64_t>(channel_id));
			return;
		}
	}

	dpp::channel channel;
	channel.set_name(local_llm_config_.channel_name)
		.set_type(dpp::CHANNEL_TEXT)
		.set_guild_id(guild_id)
		.set_topic("Ask the local BigDPP AI with /ask.");
	cluster_.channel_create(channel, [this, guild_id](const dpp::confirmation_callback_t& result) {
		if (result.is_error()) {
			std::lock_guard lock{ai_channel_mutex_};
			ai_channel_setup_guilds_.erase(static_cast<std::uint64_t>(guild_id));
			std::cerr << "BigDPP could not create the local AI channel\n";
			return;
		}

		const auto* created = std::get_if<dpp::channel>(&result.value);
		if (created == nullptr) {
			std::lock_guard lock{ai_channel_mutex_};
			ai_channel_setup_guilds_.erase(static_cast<std::uint64_t>(guild_id));
			std::cerr << "BigDPP received an invalid local AI channel response\n";
			return;
		}

		std::lock_guard lock{ai_channel_mutex_};
		ai_channel_ids_.insert(static_cast<std::uint64_t>(created->id));
		std::cout << "BigDPP local AI channel is ready\n";
	});
}

bool Bot::is_ai_channel(const dpp::snowflake guild_id, const dpp::snowflake channel_id) const {
	if (!guild_id || !channel_id) {
		return false;
	}

	{
		std::lock_guard lock{ai_channel_mutex_};
		if (ai_channel_ids_.contains(static_cast<std::uint64_t>(channel_id))) {
			return true;
		}
	}

	const auto* channel = dpp::find_channel(channel_id);
	return channel != nullptr && channel->guild_id == guild_id &&
		channel->get_type() == dpp::CHANNEL_TEXT && channel->name == local_llm_config_.channel_name;
}

bool Bot::is_guild_owner(const dpp::slashcommand_t& event) const {
	if (!event.command.guild_id) {
		return false;
	}

	const auto* guild = dpp::find_guild(event.command.guild_id);
	return guild != nullptr && guild->owner_id == event.command.usr.id;
}

bool Bot::bot_can_manage_roles(const dpp::snowflake guild_id) const {
	const auto* guild = dpp::find_guild(guild_id);
	return guild != nullptr && guild->base_permissions(&cluster_.me).has(dpp::p_manage_roles);
}

void Bot::handle_command(const dpp::slashcommand_t& event) {
	try {
		const std::string command_name = event.command.get_command_name();
		if (command_name == "ping") {
			event.reply("Pong!");
			return;
		}

		if (command_name == "status") {
			event.reply("BigDPP is online as " + cluster_.me.username + ".");
			return;
		}

		if (command_name == "ask") {
			handle_ask(event);
			return;
		}

		if (admin_service_.handle(event)) {
			return;
		}

		event.reply(dpp::message{"Unknown command"}.set_flags(dpp::m_ephemeral));
	} catch (const std::exception& error) {
		std::cerr << "BigDPP command failed: " << error.what() << "\n";
		event.reply(dpp::message{"The command could not be completed."}.set_flags(dpp::m_ephemeral));
	}
}

void Bot::handle_create_role(const dpp::slashcommand_t& event) {
	if (!is_guild_owner(event)) {
		event.reply(dpp::message{"Only the Discord server owner can use this command."}
			.set_flags(dpp::m_ephemeral)
			.set_allowed_mentions(false, false, false));
		return;
	}
	if (!bot_can_manage_roles(event.command.guild_id)) {
		event.reply(dpp::message{"I need the Manage Roles permission to create a role."}
			.set_flags(dpp::m_ephemeral)
			.set_allowed_mentions(false, false, false));
		return;
	}

	const dpp::command_value name_value = event.get_parameter("name");
	const auto* name = std::get_if<std::string>(&name_value);
	if (name == nullptr || name->find_first_not_of(" \t\r\n") == std::string::npos) {
		event.reply(dpp::message{"Please provide a role name."}
			.set_flags(dpp::m_ephemeral)
			.set_allowed_mentions(false, false, false));
		return;
	}

	uint32_t colour = 0x5865F2;
	const dpp::command_value color_value = event.get_parameter("color");
	if (const auto* color = std::get_if<std::string>(&color_value); color != nullptr && !color->empty()) {
		std::string digits = *color;
		if (!digits.empty() && digits.front() == '#') {
			digits.erase(digits.begin());
		}
		if (digits.size() != 6) {
			event.reply(dpp::message{"Color must be six hexadecimal digits, for example #5865F2."}
				.set_flags(dpp::m_ephemeral)
				.set_allowed_mentions(false, false, false));
			return;
		}
		const auto parsed = std::from_chars(digits.data(), digits.data() + digits.size(), colour, 16);
		if (parsed.ec != std::errc{} || parsed.ptr != digits.data() + digits.size()) {
			event.reply(dpp::message{"Color must be six hexadecimal digits, for example #5865F2."}
				.set_flags(dpp::m_ephemeral)
				.set_allowed_mentions(false, false, false));
			return;
		}
	}

	const auto* guild = dpp::find_guild(event.command.guild_id);
	if (guild == nullptr) {
		event.reply(dpp::message{"The server is not available in the bot cache yet."}
			.set_flags(dpp::m_ephemeral)
			.set_allowed_mentions(false, false, false));
		return;
	}
	for (const dpp::snowflake role_id : guild->roles) {
		const auto* existing = dpp::find_role(role_id);
		if (existing != nullptr && existing->name == *name) {
			event.reply(dpp::message{"A role with that name already exists."}
				.set_flags(dpp::m_ephemeral)
				.set_allowed_mentions(false, false, false));
			return;
		}
	}

	dpp::role role;
	role.guild_id = event.command.guild_id;
	role.set_name(*name).set_color(colour);
	role.permissions = dpp::permission{};
	role.flags = 0;
	const dpp::slashcommand_t event_copy = event;
	event.thinking(true, [this, event_copy, role](const dpp::confirmation_callback_t& result) {
		if (result.is_error()) {
			event_copy.edit_response(dpp::message{"I could not acknowledge the role request."}
				.set_allowed_mentions(false, false, false));
			return;
		}

		cluster_.role_create(role, [event_copy](const dpp::confirmation_callback_t& role_result) mutable {
			if (role_result.is_error()) {
				event_copy.edit_response(dpp::message{"Discord rejected the role creation request."}
					.set_allowed_mentions(false, false, false));
				return;
			}

			const auto* created = std::get_if<dpp::role>(&role_result.value);
			const std::string created_name = created == nullptr ? "new role" : created->name;
			event_copy.edit_response(dpp::message{"Created the cosmetic role **" + created_name + "**. It has no dangerous permissions."}
				.set_allowed_mentions(false, false, false));
		});
	});
}

void Bot::handle_server_setup(const dpp::slashcommand_t& event) {
	if (!is_guild_owner(event)) {
		event.reply(dpp::message{"Only the Discord server owner can use this command."}
			.set_flags(dpp::m_ephemeral)
			.set_allowed_mentions(false, false, false));
		return;
	}
	if (!bot_can_manage_roles(event.command.guild_id)) {
		event.reply(dpp::message{"I need the Manage Roles permission to prepare the server roles."}
			.set_flags(dpp::m_ephemeral)
			.set_allowed_mentions(false, false, false));
		return;
	}

	const dpp::command_value confirmation = event.get_parameter("confirm");
	const auto* confirmed = std::get_if<bool>(&confirmation);
	if (confirmed == nullptr || !*confirmed) {
		event.reply(dpp::message{"Nothing changed. Run `/server-setup confirm:true` when you are ready."}
			.set_flags(dpp::m_ephemeral)
			.set_allowed_mentions(false, false, false));
		return;
	}

	const auto* guild = dpp::find_guild(event.command.guild_id);
	if (guild == nullptr) {
		event.reply(dpp::message{"The server is not available in the bot cache yet."}
			.set_flags(dpp::m_ephemeral)
			.set_allowed_mentions(false, false, false));
		return;
	}

	struct role_preset final {
		const char* name;
		uint32_t colour;
	};
	static constexpr std::array presets{
		role_preset{"Server Owner", 0xF1C40F},
		role_preset{"Administrator", 0xE74C3C},
		role_preset{"Moderator", 0x3498DB},
		role_preset{"Member", 0x2ECC71},
		role_preset{"AI User", 0x9B59B6},
	};

	struct setup_state final {
		dpp::snowflake guild_id;
		dpp::snowflake owner_id;
		dpp::snowflake owner_role_id;
		std::size_t pending{0};
		std::size_t created{0};
		std::size_t existing{0};
		std::size_t failed{0};
		std::mutex mutex;
		std::vector<std::string> created_names;
	};

	auto state = std::make_shared<setup_state>();
	state->guild_id = event.command.guild_id;
	state->owner_id = event.command.usr.id;
	std::vector<dpp::role> missing_roles;
	for (const role_preset& preset : presets) {
		const auto existing = std::find_if(guild->roles.begin(), guild->roles.end(), [preset](const dpp::snowflake role_id) {
			const auto* role = dpp::find_role(role_id);
			return role != nullptr && role->name == preset.name;
		});
		if (existing != guild->roles.end()) {
			if (preset.name == std::string_view{"Server Owner"}) {
				state->owner_role_id = *existing;
			}
			++state->existing;
			continue;
		}

		dpp::role role;
		role.guild_id = state->guild_id;
		role.set_name(preset.name).set_color(preset.colour);
		role.permissions = dpp::permission{};
		role.flags = 0;
		missing_roles.push_back(std::move(role));
	}

	state->pending = missing_roles.size();
	const dpp::slashcommand_t event_copy = event;
	event.thinking(true, [this, state, event_copy, missing_roles = std::move(missing_roles)](const dpp::confirmation_callback_t& result) mutable {
		if (result.is_error()) {
			event_copy.edit_response(dpp::message{"I could not acknowledge the setup request."}
				.set_allowed_mentions(false, false, false));
			return;
		}

		if (missing_roles.empty()) {
			if (state->owner_role_id) {
				cluster_.guild_member_add_role(
					state->guild_id,
					state->owner_id,
					state->owner_role_id,
					[](const dpp::confirmation_callback_t&) {});
			}
			event_copy.edit_response(dpp::message{"The BigDPP role baseline already exists. No roles were changed."}
				.set_allowed_mentions(false, false, false));
			return;
		}

		for (const dpp::role& role : missing_roles) {
			cluster_.role_create(role, [this, state, event_copy](const dpp::confirmation_callback_t& role_result) mutable {
				std::string created_name;
				if (!role_result.is_error()) {
					if (const auto* created = std::get_if<dpp::role>(&role_result.value); created != nullptr) {
						created_name = created->name;
						if (created_name == "Server Owner") {
							cluster_.guild_member_add_role(
								state->guild_id,
								state->owner_id,
								created->id,
								[](const dpp::confirmation_callback_t&) {});
						}
					}
				}

				bool complete = false;
				{
					std::lock_guard lock{state->mutex};
					if (created_name.empty()) {
						++state->failed;
					} else {
						++state->created;
						state->created_names.push_back(created_name);
					}
					complete = (--state->pending == 0);
				}

				if (!complete) {
					return;
				}

				std::lock_guard lock{state->mutex};
				std::string response = "Server role setup finished. Created " + std::to_string(state->created) +
					" role(s); " + std::to_string(state->existing) + " already existed.";
				if (state->failed > 0) {
					response += " Discord rejected " + std::to_string(state->failed) + " role(s).";
				}
				event_copy.edit_response(dpp::message{std::move(response)}
					.set_allowed_mentions(false, false, false));
			});
		}
	});
}

void Bot::handle_ask(const dpp::slashcommand_t& event) {
	if (!local_llm_config_.enabled) {
		event.reply(dpp::message{"The local AI is not configured yet."}
			.set_flags(dpp::m_ephemeral)
			.set_allowed_mentions(false, false, false));
		return;
	}

	if (!is_ai_channel(event.command.guild_id, event.command.channel_id)) {
		event.reply(dpp::message{"Please use /ask in the #" + local_llm_config_.channel_name + " channel."}
			.set_flags(dpp::m_ephemeral)
			.set_allowed_mentions(false, false, false));
		return;
	}

	const dpp::command_value value = event.get_parameter("question");
	const auto* question = std::get_if<std::string>(&value);
	if (question == nullptr || question->find_first_not_of(" \t\r\n") == std::string::npos) {
		event.reply(dpp::message{"Please provide a question."}
			.set_flags(dpp::m_ephemeral)
			.set_allowed_mentions(false, false, false));
		return;
	}

	const dpp::slashcommand_t event_copy = event;
	event.thinking(false, [this, event_copy, prompt = *question](const dpp::confirmation_callback_t& result) {
		if (result.is_error()) {
			std::cerr << "BigDPP could not acknowledge /ask\n";
			return;
		}

		try {
			local_llm_client_.ask(prompt, [this, event_copy](LocalLlmResult llm_result) mutable {
				try {
					complete_ask(event_copy, std::move(llm_result));
				} catch (const std::exception& error) {
					std::cerr << "BigDPP /ask completion failed: " << error.what() << "\n";
				}
			});
		} catch (const std::exception& error) {
			std::cerr << "BigDPP /ask request failed: " << error.what() << "\n";
			event_copy.edit_response(dpp::message{"The local AI is unavailable right now. Please try again later."}
				.set_allowed_mentions(false, false, false));
		}
	});
}

void Bot::complete_ask(const dpp::slashcommand_t& event, LocalLlmResult result) {
	if (!result.success) {
		event.edit_response(dpp::message{"The local AI is unavailable right now. Please try again later."}
			.set_allowed_mentions(false, false, false));
		return;
	}

	constexpr std::size_t max_discord_message_size = 1900;
	if (result.response.size() > max_discord_message_size) {
		result.response.resize(max_discord_message_size);
		result.response += "\n...";
	}
	event.edit_response(dpp::message{std::move(result.response)}.set_allowed_mentions(false, false, false));
}

} // namespace bigdpp
