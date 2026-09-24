#include "bigdpp/admin.hpp"

#include <dpp/cache.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <ctime>
#include <iostream>
#include <memory>
#include <sstream>
#include <string_view>
#include <utility>

namespace bigdpp {

namespace {

constexpr std::size_t max_reason_length = 512;
constexpr std::size_t max_channel_topic_length = 1024;
constexpr std::int64_t max_purge_messages = 100;
constexpr std::int64_t max_timeout_minutes = 40320;

dpp::command_option string_option(
	const std::string& name,
	const std::string& description,
	const bool required,
	const std::uint16_t max_length = 0) {
	dpp::command_option option{dpp::co_string, name, description, required};
	if (max_length > 0) {
		option.set_max_length(max_length);
	}
	return option;
}

dpp::command_option user_option(const std::string& name, const std::string& description) {
	return dpp::command_option{dpp::co_user, name, description, true};
}

dpp::command_option channel_option(const std::string& name, const std::string& description, const bool required) {
	return dpp::command_option{dpp::co_channel, name, description, required};
}

dpp::command_option confirm_option() {
	return dpp::command_option{dpp::co_boolean, "confirm", "Confirm this change", false};
}

std::string sanitize_for_log(std::string value) {
	std::replace(value.begin(), value.end(), '@', '_');
	if (value.size() > max_reason_length) {
		value.resize(max_reason_length);
		value += "...";
	}
	return value;
}

std::string command_name(const dpp::slashcommand_t& event) {
	return event.command.get_command_name();
}

bool is_text_channel(const dpp::channel& channel) {
	return channel.get_type() == dpp::CHANNEL_TEXT;
}

} // namespace

AdminService::AdminService(dpp::cluster& cluster)
	: cluster_{cluster} {}

std::vector<dpp::slashcommand> AdminService::commands(const dpp::snowflake application_id) const {
	std::vector<dpp::slashcommand> result;
	result.emplace_back("server-info", "Show server information", application_id);

	dpp::slashcommand server_edit{"server-edit", "Edit basic server details", application_id};
	server_edit.add_option(string_option("name", "New server name", false, 100));
	server_edit.add_option(string_option("description", "New server description", false, 120));
	server_edit.add_option(confirm_option());
	result.push_back(std::move(server_edit));

	dpp::slashcommand setup{"server-setup", "Create the additive BigDPP server baseline", application_id};
	setup.add_option(confirm_option());
	result.push_back(std::move(setup));

	dpp::slashcommand channel_create{"channel-create", "Create a server channel", application_id};
	channel_create.add_option(string_option("name", "Channel name", true, 100));
	channel_create.add_option(string_option("type", "text, voice, or category", true, 16));
	channel_create.add_option(channel_option("category", "Optional parent category", false));
	channel_create.add_option(string_option("topic", "Optional topic", false, max_channel_topic_length));
	channel_create.add_option(string_option("reason", "Audit reason", false, max_reason_length));
	result.push_back(std::move(channel_create));

	dpp::slashcommand channel_edit{"channel-edit", "Edit a server channel", application_id};
	channel_edit.add_option(channel_option("channel", "Channel to edit", true));
	channel_edit.add_option(string_option("name", "New channel name", false, 100));
	channel_edit.add_option(string_option("topic", "New topic", false, max_channel_topic_length));
	channel_edit.add_option(dpp::command_option{dpp::co_integer, "slowmode", "Slowmode seconds (0-21600)", false});
	channel_edit.add_option(dpp::command_option{dpp::co_boolean, "nsfw", "Whether the channel is age restricted", false});
	channel_edit.add_option(string_option("reason", "Audit reason", false, max_reason_length));
	result.push_back(std::move(channel_edit));

	dpp::slashcommand channel_delete{"channel-delete", "Delete a server channel", application_id};
	channel_delete.add_option(channel_option("channel", "Channel to delete", true));
	channel_delete.add_option(string_option("reason", "Audit reason", false, max_reason_length));
	channel_delete.add_option(confirm_option());
	result.push_back(std::move(channel_delete));

	for (const std::pair command : std::array{
		std::pair{"channel-lock", "Stop members from sending messages"},
		std::pair{"channel-unlock", "Allow members to send messages"}}) {
		dpp::slashcommand command_definition{command.first, command.second, application_id};
		command_definition.add_option(channel_option("channel", "Channel to change", true));
		command_definition.add_option(string_option("reason", "Audit reason", false, max_reason_length));
		command_definition.add_option(confirm_option());
		result.push_back(std::move(command_definition));
	}

	dpp::slashcommand channel_access{"channel-access", "Set public or private channel access", application_id};
	channel_access.add_option(channel_option("channel", "Channel to change", true));
	channel_access.add_option(string_option("mode", "public or private", true, 16));
	channel_access.add_option(string_option("reason", "Audit reason", false, max_reason_length));
	channel_access.add_option(confirm_option());
	result.push_back(std::move(channel_access));

	dpp::slashcommand role_create{"role-create", "Create a server role", application_id};
	role_create.add_option(string_option("name", "Role name", true, 100));
	role_create.add_option(string_option("color", "Six-digit hex color, for example #5865F2", false, 7));
	role_create.add_option(string_option("profile", "cosmetic or moderator", false, 16));
	role_create.add_option(string_option("reason", "Audit reason", false, max_reason_length));
	result.push_back(std::move(role_create));

	dpp::slashcommand legacy_role_create{"create-role", "Create a cosmetic server role", application_id};
	legacy_role_create.add_option(string_option("name", "Role name", true, 100));
	legacy_role_create.add_option(string_option("color", "Six-digit hex color, for example #5865F2", false, 7));
	legacy_role_create.add_option(string_option("reason", "Audit reason", false, max_reason_length));
	result.push_back(std::move(legacy_role_create));

	dpp::slashcommand role_edit{"role-edit", "Edit a server role", application_id};
	role_edit.add_option(string_option("role", "Role ID", true, 22));
	role_edit.add_option(string_option("name", "New role name", false, 100));
	role_edit.add_option(string_option("color", "Six-digit hex color", false, 7));
	role_edit.add_option(string_option("profile", "cosmetic or moderator", false, 16));
	role_edit.add_option(string_option("reason", "Audit reason", false, max_reason_length));
	role_edit.add_option(confirm_option());
	result.push_back(std::move(role_edit));

	dpp::slashcommand role_delete{"role-delete", "Delete a server role", application_id};
	role_delete.add_option(string_option("role", "Role ID", true, 22));
	role_delete.add_option(string_option("reason", "Audit reason", false, max_reason_length));
	role_delete.add_option(confirm_option());
	result.push_back(std::move(role_delete));

	for (const std::pair command : std::array{
		std::pair{"role-add", "Add a role to a member"},
		std::pair{"role-remove", "Remove a role from a member"}}) {
		dpp::slashcommand command_definition{command.first, command.second, application_id};
		command_definition.add_option(user_option("user", "Member to change"));
		command_definition.add_option(string_option("role", "Role ID", true, 22));
		command_definition.add_option(string_option("reason", "Audit reason", false, max_reason_length));
		result.push_back(std::move(command_definition));
	}

	dpp::slashcommand warn{"member-warn", "Record a member warning", application_id};
	warn.add_option(user_option("user", "Member to warn"));
	warn.add_option(string_option("reason", "Warning reason", true, max_reason_length));
	result.push_back(std::move(warn));

	dpp::slashcommand timeout{"member-timeout", "Timeout a member", application_id};
	timeout.add_option(user_option("user", "Member to timeout"));
	timeout.add_option(dpp::command_option{dpp::co_integer, "minutes", "Timeout duration, 1-40320 minutes", true});
	timeout.add_option(string_option("reason", "Timeout reason", true, max_reason_length));
	timeout.add_option(confirm_option());
	result.push_back(std::move(timeout));

	dpp::slashcommand untimeout{"member-untimeout", "Remove a member timeout", application_id};
	untimeout.add_option(user_option("user", "Member to restore"));
	untimeout.add_option(string_option("reason", "Reason", false, max_reason_length));
	result.push_back(std::move(untimeout));

	for (const std::pair command : std::array{
		std::pair{"member-kick", "Kick a member"},
		std::pair{"member-ban", "Ban a member"}}) {
		dpp::slashcommand command_definition{command.first, command.second, application_id};
		command_definition.add_option(user_option("user", "Member to target"));
		command_definition.add_option(string_option("reason", "Reason", true, max_reason_length));
		if (std::string_view{command.first} == "member-ban") {
			auto delete_days = dpp::command_option{dpp::co_integer, "delete_days", "Delete 0-7 days of recent messages", false};
			delete_days.set_min_value(0).set_max_value(7);
			command_definition.add_option(delete_days);
		}
		command_definition.add_option(confirm_option());
		result.push_back(std::move(command_definition));
	}

	dpp::slashcommand unban{"member-unban", "Unban a user", application_id};
	unban.add_option(string_option("user", "User ID", true, 22));
	unban.add_option(string_option("reason", "Reason", false, max_reason_length));
	unban.add_option(confirm_option());
	result.push_back(std::move(unban));

	dpp::slashcommand purge{"messages-purge", "Delete recent messages", application_id};
	purge.add_option(dpp::command_option{dpp::co_integer, "amount", "Number of messages, 1-100", true});
	purge.add_option(channel_option("channel", "Channel to purge", false));
	purge.add_option(string_option("reason", "Reason", true, max_reason_length));
	purge.add_option(confirm_option());
	result.push_back(std::move(purge));

	dpp::slashcommand audit{"audit-log", "Show where BigDPP records admin actions", application_id};
	result.push_back(std::move(audit));

	return result;
}

bool AdminService::handle(const dpp::slashcommand_t& event) {
	const std::string name = command_name(event);
	if (name == "server-setup") {
		handle_server_setup(event);
	} else if (name == "server-info") {
		handle_server_info(event);
	} else if (name == "server-edit") {
		handle_server_edit(event);
	} else if (name == "channel-create") {
		handle_channel_create(event);
	} else if (name == "channel-edit") {
		handle_channel_edit(event);
	} else if (name == "channel-delete") {
		handle_channel_delete(event);
	} else if (name == "channel-lock") {
		handle_channel_lock(event, true);
	} else if (name == "channel-unlock") {
		handle_channel_lock(event, false);
	} else if (name == "channel-access") {
		handle_channel_access(event);
	} else if (name == "role-create" || name == "create-role") {
		handle_role_create(event);
	} else if (name == "role-edit") {
		handle_role_edit(event);
	} else if (name == "role-delete") {
		handle_role_delete(event);
	} else if (name == "role-add") {
		handle_role_member(event, true);
	} else if (name == "role-remove") {
		handle_role_member(event, false);
	} else if (name == "member-warn") {
		handle_member_warn(event);
	} else if (name == "member-timeout") {
		handle_member_timeout(event, false);
	} else if (name == "member-untimeout") {
		handle_member_timeout(event, true);
	} else if (name == "member-kick") {
		handle_member_kick(event);
	} else if (name == "member-ban") {
		handle_member_ban(event, false);
	} else if (name == "member-unban") {
		handle_member_ban(event, true);
	} else if (name == "messages-purge") {
		handle_messages_purge(event);
	} else if (name == "audit-log") {
		handle_audit_log(event);
	} else {
		return false;
	}
	return true;
}

bool AdminService::authorize(
	const dpp::slashcommand_t& event,
	const dpp::permission required_permission,
	std::string& error) const {
	if (!event.command.guild_id) {
		error = "This command can only be used inside a server.";
		return false;
	}

	const auto* guild = dpp::find_guild(event.command.guild_id);
	if (guild == nullptr) {
		error = "The server is not available in BigDPP's cache yet.";
		return false;
	}

	const dpp::permission caller_permissions = guild->base_permissions(event.command.member);
	const bool is_owner = guild->owner_id == event.command.usr.id;
	if (!is_owner && !caller_permissions.has(dpp::p_administrator)) {
		error = "Only the server owner or a member with Discord Administrator can use this command.";
		return false;
	}

	if (required_permission && !guild->base_permissions(&cluster_.me).has(required_permission)) {
		error = "BigDPP does not have the required Discord permission for this action.";
		return false;
	}
	return true;
}

std::optional<dpp::snowflake> AdminService::parameter_snowflake(
	const dpp::slashcommand_t& event,
	const std::string& name) const {
	const dpp::command_value value = event.get_parameter(name);
	if (const auto* id = std::get_if<dpp::snowflake>(&value); id != nullptr) {
		return *id;
	}
	if (const auto* text = std::get_if<std::string>(&value); text != nullptr && !text->empty()) {
		std::uint64_t parsed = 0;
		const auto result = std::from_chars(text->data(), text->data() + text->size(), parsed);
		if (result.ec == std::errc{} && result.ptr == text->data() + text->size() && parsed != 0) {
			return dpp::snowflake{parsed};
		}
	}
	return std::nullopt;
}

std::optional<std::string> AdminService::parameter_string(
	const dpp::slashcommand_t& event,
	const std::string& name) const {
	const dpp::command_value value = event.get_parameter(name);
	if (const auto* text = std::get_if<std::string>(&value); text != nullptr && !text->empty()) {
		return *text;
	}
	return std::nullopt;
}

std::optional<int64_t> AdminService::parameter_integer(
	const dpp::slashcommand_t& event,
	const std::string& name) const {
	const dpp::command_value value = event.get_parameter(name);
	if (const auto* number = std::get_if<int64_t>(&value); number != nullptr) {
		return *number;
	}
	return std::nullopt;
}

std::optional<bool> AdminService::parameter_boolean(
	const dpp::slashcommand_t& event,
	const std::string& name) const {
	const dpp::command_value value = event.get_parameter(name);
	if (const auto* flag = std::get_if<bool>(&value); flag != nullptr) {
		return *flag;
	}
	return std::nullopt;
}

bool AdminService::require_confirmation(
	const dpp::slashcommand_t& event,
	const std::string& preview) const {
	if (parameter_boolean(event, "confirm").value_or(false)) {
		return true;
	}
	respond(event, "Preview only — no changes were made. " + preview + " Re-run with `confirm:true` to execute.");
	return false;
}

std::optional<uint32_t> AdminService::parse_colour(const std::string& value) const {
	std::string digits = value;
	if (!digits.empty() && digits.front() == '#') {
		digits.erase(digits.begin());
	}
	if (digits.size() != 6) {
		return std::nullopt;
	}

	uint32_t colour = 0;
	const auto result = std::from_chars(digits.data(), digits.data() + digits.size(), colour, 16);
	if (result.ec != std::errc{} || result.ptr != digits.data() + digits.size()) {
		return std::nullopt;
	}
	return colour;
}

std::optional<dpp::role> AdminService::find_role(
	const dpp::snowflake guild_id,
	const dpp::snowflake role_id) const {
	const auto* guild = dpp::find_guild(guild_id);
	if (guild == nullptr || std::find(guild->roles.begin(), guild->roles.end(), role_id) == guild->roles.end()) {
		return std::nullopt;
	}
	const auto* role = dpp::find_role(role_id);
	return role == nullptr ? std::nullopt : std::optional<dpp::role>{*role};
}

std::optional<dpp::channel> AdminService::find_channel(
	const dpp::snowflake guild_id,
	const dpp::snowflake channel_id) const {
	const auto* channel = dpp::find_channel(channel_id);
	if (channel == nullptr || channel->guild_id != guild_id) {
		return std::nullopt;
	}
	return *channel;
}

uint16_t AdminService::highest_role_position(
	const dpp::snowflake guild_id,
	const dpp::snowflake user_id) const {
	const auto* guild = dpp::find_guild(guild_id);
	if (guild == nullptr) {
		return 0;
	}

	std::vector<dpp::snowflake> role_ids;
	if (const auto member = guild->members.find(user_id); member != guild->members.end()) {
		role_ids = member->second.get_roles();
	}

	uint16_t highest = 0;
	for (const dpp::snowflake role_id : role_ids) {
		if (const auto* role = dpp::find_role(role_id); role != nullptr) {
			highest = std::max<uint16_t>(highest, role->position);
		}
	}
	return highest;
}

bool AdminService::can_target_member(
	const dpp::slashcommand_t& event,
	const dpp::snowflake target_id,
	std::string& error) const {
	const auto* guild = dpp::find_guild(event.command.guild_id);
	if (guild == nullptr) {
		error = "The server is not available in BigDPP's cache yet.";
		return false;
	}
	if (target_id == guild->owner_id) {
		error = "The guild owner cannot be targeted by the bot.";
		return false;
	}
	if (target_id == cluster_.me.id) {
		error = "BigDPP cannot target itself.";
		return false;
	}

	const uint16_t bot_position = highest_role_position(event.command.guild_id, cluster_.me.id);
	const uint16_t target_position = highest_role_position(event.command.guild_id, target_id);
	if (target_position >= bot_position) {
		error = "That member is at or above BigDPP's highest role.";
		return false;
	}

	const bool owner = guild->owner_id == event.command.usr.id;
	const uint16_t caller_position = highest_role_position(event.command.guild_id, event.command.usr.id);
	if (!owner && target_position >= caller_position) {
		error = "You cannot target a member at or above your highest role.";
		return false;
	}
	return true;
}

bool AdminService::can_manage_role(
	const dpp::snowflake guild_id,
	const dpp::role& role,
	std::string& error) const {
	if (role.guild_id != guild_id) {
		error = "That role does not belong to this server.";
		return false;
	}
	if (role.id == guild_id) {
		error = "The @everyone role cannot be changed by this command.";
		return false;
	}
	if (role.is_managed()) {
		error = "Managed integration roles cannot be changed by this command.";
		return false;
	}
	if (role.position >= highest_role_position(guild_id, cluster_.me.id)) {
		error = "That role is at or above BigDPP's highest role.";
		return false;
	}
	return true;
}

void AdminService::respond(const dpp::slashcommand_t& event, const std::string& content) const {
	event.reply(dpp::message{content}
		.set_flags(dpp::m_ephemeral)
		.set_allowed_mentions(false, false, false));
}

void AdminService::audit(
	const dpp::slashcommand_t& event,
	const std::string& action,
	const std::string& result) const {
	const std::string line = "admin action=" + sanitize_for_log(action) +
		" actor=" + std::to_string(static_cast<std::uint64_t>(event.command.usr.id)) +
		" result=" + sanitize_for_log(result);
	std::clog << "BigDPP " << line << '\n';

	const auto* guild = dpp::find_guild(event.command.guild_id);
	if (guild == nullptr) {
		return;
	}
	for (const dpp::snowflake channel_id : guild->channels) {
		const auto* channel = dpp::find_channel(channel_id);
		if (channel != nullptr && is_text_channel(*channel) && channel->name == "mod-log") {
			cluster_.message_create(dpp::message{channel_id, line}, [](const dpp::confirmation_callback_t&) {});
			return;
		}
	}
}

void AdminService::handle_server_info(const dpp::slashcommand_t& event) {
	std::string error;
	if (!authorize(event, dpp::permission{}, error)) {
		respond(event, error);
		return;
	}
	const auto* guild = dpp::find_guild(event.command.guild_id);
	if (guild == nullptr) {
		respond(event, "The server is not available in BigDPP's cache yet.");
		return;
	}

	std::size_t text_channels = 0;
	std::size_t voice_channels = 0;
	for (const dpp::snowflake channel_id : guild->channels) {
		if (const auto* channel = dpp::find_channel(channel_id); channel != nullptr) {
			if (is_text_channel(*channel)) {
				++text_channels;
			} else if (channel->get_type() == dpp::CHANNEL_VOICE) {
				++voice_channels;
			}
		}
	}

	std::ostringstream output;
	output << "**" << sanitize_for_log(guild->name) << "**\n"
		<< "Owner: `" << static_cast<std::uint64_t>(guild->owner_id) << "`\n"
		<< "Members: " << guild->member_count << "\n"
		<< "Roles: " << guild->roles.size() << "\n"
		<< "Channels: " << text_channels << " text, " << voice_channels << " voice";
	respond(event, output.str());
}

void AdminService::handle_server_edit(const dpp::slashcommand_t& event) {
	std::string error;
	if (!authorize(event, dpp::p_manage_guild, error)) {
		respond(event, error);
		return;
	}
	const auto* guild = dpp::find_guild(event.command.guild_id);
	const auto name = parameter_string(event, "name");
	const auto description = parameter_string(event, "description");
	if (guild == nullptr || (!name.has_value() && !description.has_value())) {
		respond(event, "Provide at least one of `name` or `description`.");
		return;
	}

	std::string preview = "Update the server details";
	if (name.has_value()) {
		preview += "; name → **" + sanitize_for_log(*name) + "**";
	}
	if (description.has_value()) {
		preview += "; description → **" + sanitize_for_log(*description) + "**";
	}
	if (!require_confirmation(event, preview)) {
		return;
	}

	dpp::guild edited = *guild;
	if (name.has_value()) {
		edited.set_name(*name);
	}
	if (description.has_value()) {
		edited.description = *description;
	}
	const dpp::slashcommand_t event_copy = event;
	event.thinking(true);
	cluster_.guild_edit(edited, [this, event_copy](const dpp::confirmation_callback_t& result) {
		if (result.is_error()) {
			audit(event_copy, "server-edit", "failed");
			event_copy.edit_response(dpp::message{"Discord rejected the server update."}
				.set_allowed_mentions(false, false, false));
			return;
		}
		audit(event_copy, "server-edit", "success");
		event_copy.edit_response(dpp::message{"Server details updated."}
			.set_allowed_mentions(false, false, false));
	});
}

void AdminService::handle_channel_create(const dpp::slashcommand_t& event) {
	std::string error;
	if (!authorize(event, dpp::p_manage_channels, error)) {
		respond(event, error);
		return;
	}
	const auto name = parameter_string(event, "name");
	const auto type = parameter_string(event, "type");
	if (!name.has_value() || !type.has_value()) {
		respond(event, "Channel name and type are required.");
		return;
	}

	dpp::channel_type channel_type = dpp::CHANNEL_TEXT;
	if (*type == "voice") {
		channel_type = dpp::CHANNEL_VOICE;
	} else if (*type == "category") {
		channel_type = dpp::CHANNEL_CATEGORY;
	} else if (*type != "text") {
		respond(event, "Type must be `text`, `voice`, or `category`.");
		return;
	}

	const auto* guild = dpp::find_guild(event.command.guild_id);
	if (guild == nullptr) {
		respond(event, "The server is not available in BigDPP's cache yet.");
		return;
	}
	for (const dpp::snowflake channel_id : guild->channels) {
		if (const auto* existing = dpp::find_channel(channel_id); existing != nullptr &&
			existing->name == *name && existing->get_type() == channel_type) {
			respond(event, "A channel with that name and type already exists.");
			return;
		}
	}

	dpp::channel channel;
	channel.set_guild_id(event.command.guild_id).set_name(*name).set_type(channel_type);
	if (const auto category = parameter_snowflake(event, "category"); category.has_value()) {
		const auto parent = find_channel(event.command.guild_id, *category);
		if (!parent.has_value() || parent->get_type() != dpp::CHANNEL_CATEGORY) {
			respond(event, "The selected category was not found in this server.");
			return;
		}
		channel.set_parent_id(*category);
	}
	if (const auto topic = parameter_string(event, "topic"); topic.has_value() && is_text_channel(channel)) {
		channel.set_topic(*topic);
	}

	const dpp::slashcommand_t event_copy = event;
	event.thinking(true);
	cluster_.channel_create(channel, [this, event_copy](const dpp::confirmation_callback_t& result) {
		if (result.is_error()) {
			audit(event_copy, "channel-create", "failed");
			event_copy.edit_response(dpp::message{"Discord rejected the channel creation."}
				.set_allowed_mentions(false, false, false));
			return;
		}
		const auto* created = std::get_if<dpp::channel>(&result.value);
		const std::string created_name = created == nullptr ? "channel" : created->name;
		audit(event_copy, "channel-create", "success " + created_name);
		event_copy.edit_response(dpp::message{"Created channel **" + sanitize_for_log(created_name) + "**."}
			.set_allowed_mentions(false, false, false));
	});
}

void AdminService::handle_channel_edit(const dpp::slashcommand_t& event) {
	std::string error;
	if (!authorize(event, dpp::p_manage_channels, error)) {
		respond(event, error);
		return;
	}
	const auto channel_id = parameter_snowflake(event, "channel");
	const auto channel = channel_id.has_value() ? find_channel(event.command.guild_id, *channel_id) : std::nullopt;
	if (!channel.has_value()) {
		respond(event, "The selected channel was not found in this server.");
		return;
	}

	dpp::channel edited = *channel;
	bool changed = false;
	if (const auto name = parameter_string(event, "name"); name.has_value()) {
		edited.set_name(*name);
		changed = true;
	}
	if (const auto topic = parameter_string(event, "topic"); topic.has_value() && is_text_channel(edited)) {
		edited.set_topic(*topic);
		changed = true;
	}
	if (const auto slowmode = parameter_integer(event, "slowmode"); slowmode.has_value()) {
		if (*slowmode < 0 || *slowmode > 21600) {
			respond(event, "Slowmode must be between 0 and 21600 seconds.");
			return;
		}
		edited.set_rate_limit_per_user(static_cast<uint16_t>(*slowmode));
		changed = true;
	}
	if (const auto nsfw = parameter_boolean(event, "nsfw"); nsfw.has_value() && is_text_channel(edited)) {
		edited.set_nsfw(*nsfw);
		changed = true;
	}
	if (!changed) {
		respond(event, "Provide at least one channel property to change.");
		return;
	}

	const dpp::slashcommand_t event_copy = event;
	event.thinking(true);
	cluster_.channel_edit(edited, [this, event_copy](const dpp::confirmation_callback_t& result) {
		if (result.is_error()) {
			audit(event_copy, "channel-edit", "failed");
			event_copy.edit_response(dpp::message{"Discord rejected the channel update."}
				.set_allowed_mentions(false, false, false));
			return;
		}
		audit(event_copy, "channel-edit", "success");
		event_copy.edit_response(dpp::message{"Channel updated."}
			.set_allowed_mentions(false, false, false));
	});
}

void AdminService::handle_channel_delete(const dpp::slashcommand_t& event) {
	std::string error;
	if (!authorize(event, dpp::p_manage_channels, error)) {
		respond(event, error);
		return;
	}
	const auto channel_id = parameter_snowflake(event, "channel");
	const auto channel = channel_id.has_value() ? find_channel(event.command.guild_id, *channel_id) : std::nullopt;
	if (!channel.has_value()) {
		respond(event, "The selected channel was not found in this server.");
		return;
	}
	if (!require_confirmation(event, "Delete channel **" + sanitize_for_log(channel->name) + "**.")) {
		return;
	}

	const dpp::slashcommand_t event_copy = event;
	event.thinking(true);
	cluster_.channel_delete(channel->id, [this, event_copy](const dpp::confirmation_callback_t& result) {
		if (result.is_error()) {
			audit(event_copy, "channel-delete", "failed");
			event_copy.edit_response(dpp::message{"Discord rejected the channel deletion."}
				.set_allowed_mentions(false, false, false));
			return;
		}
		audit(event_copy, "channel-delete", "success");
		event_copy.edit_response(dpp::message{"Channel deleted."}
			.set_allowed_mentions(false, false, false));
	});
}

void AdminService::handle_channel_lock(const dpp::slashcommand_t& event, const bool lock) {
	std::string error;
	if (!authorize(event, dpp::p_manage_channels, error)) {
		respond(event, error);
		return;
	}
	const auto channel_id = parameter_snowflake(event, "channel");
	const auto channel = channel_id.has_value() ? find_channel(event.command.guild_id, *channel_id) : std::nullopt;
	if (!channel.has_value() || !is_text_channel(*channel)) {
		respond(event, "Select a text channel in this server.");
		return;
	}
	const std::string action = lock ? "Lock" : "Unlock";
	if (!require_confirmation(event, action + " channel **" + sanitize_for_log(channel->name) + "**.")) {
		return;
	}

	const dpp::slashcommand_t event_copy = event;
	event.thinking(true);
	const uint64_t allow = lock ? 0 : dpp::p_send_messages;
	const uint64_t deny = lock ? dpp::p_send_messages : 0;
	cluster_.channel_edit_permissions(
		channel->id,
		event.command.guild_id,
		allow,
		deny,
		false,
		[this, event_copy, action](const dpp::confirmation_callback_t& result) {
			if (result.is_error()) {
				audit(event_copy, "channel-" + action, "failed");
				event_copy.edit_response(dpp::message{"Discord rejected the channel permission update."}
					.set_allowed_mentions(false, false, false));
				return;
			}
			audit(event_copy, "channel-" + action, "success");
			event_copy.edit_response(dpp::message{"Channel " + action + "ed."}
				.set_allowed_mentions(false, false, false));
		});
}

void AdminService::handle_channel_access(const dpp::slashcommand_t& event) {
	std::string error;
	if (!authorize(event, dpp::p_manage_channels, error)) {
		respond(event, error);
		return;
	}
	const auto channel_id = parameter_snowflake(event, "channel");
	const auto channel = channel_id.has_value() ? find_channel(event.command.guild_id, *channel_id) : std::nullopt;
	const auto mode = parameter_string(event, "mode");
	if (!channel.has_value() || !mode.has_value() || (*mode != "public" && *mode != "private")) {
		respond(event, "Select a channel and use mode `public` or `private`.");
		return;
	}
	if (!require_confirmation(event, "Set **" + sanitize_for_log(channel->name) + "** access to **" + *mode + "**.")) {
		return;
	}

	const dpp::slashcommand_t event_copy = event;
	event.thinking(true);
	const bool private_mode = *mode == "private";
	const uint64_t everyone_allow = private_mode ? 0 : (dpp::p_view_channel | dpp::p_send_messages | dpp::p_read_message_history);
	const uint64_t everyone_deny = private_mode ? dpp::p_view_channel : 0;
	cluster_.channel_edit_permissions(
		channel->id,
		event.command.guild_id,
		everyone_allow,
		everyone_deny,
		false,
		[this, event_copy, channel_id = channel->id](const dpp::confirmation_callback_t& result) {
			if (result.is_error()) {
				audit(event_copy, "channel-access", "failed");
				event_copy.edit_response(dpp::message{"Discord rejected the channel access update."}
					.set_allowed_mentions(false, false, false));
				return;
			}
			const uint64_t bot_allow = dpp::p_view_channel | dpp::p_send_messages | dpp::p_read_message_history;
			cluster_.channel_edit_permissions(
				channel_id,
				cluster_.me.id,
				bot_allow,
				0,
				true,
				[this, event_copy](const dpp::confirmation_callback_t& bot_result) {
					if (bot_result.is_error()) {
						audit(event_copy, "channel-access", "partial failure");
						event_copy.edit_response(dpp::message{"The member access update succeeded, but BigDPP could not preserve its own access."}
							.set_allowed_mentions(false, false, false));
						return;
					}
					audit(event_copy, "channel-access", "success");
					event_copy.edit_response(dpp::message{"Channel access updated."}
						.set_allowed_mentions(false, false, false));
				});
		});
}

void AdminService::handle_role_create(const dpp::slashcommand_t& event) {
	std::string error;
	if (!authorize(event, dpp::p_manage_roles, error)) {
		respond(event, error);
		return;
	}
	const auto name = parameter_string(event, "name");
	if (!name.has_value()) {
		respond(event, "Role name is required.");
		return;
	}
	const auto profile = parameter_string(event, "profile").value_or("cosmetic");
	if (profile != "cosmetic" && profile != "moderator") {
		respond(event, "Profile must be `cosmetic` or `moderator`. Administrator permissions are never granted automatically.");
		return;
	}
	const auto colour_text = parameter_string(event, "color").value_or("5865F2");
	const auto colour = parse_colour(colour_text);
	if (!colour.has_value()) {
		respond(event, "Color must be six hexadecimal digits, for example `#5865F2`.");
		return;
	}

	const auto* guild = dpp::find_guild(event.command.guild_id);
	if (guild == nullptr) {
		respond(event, "The server is not available in BigDPP's cache yet.");
		return;
	}
	for (const dpp::snowflake role_id : guild->roles) {
		if (const auto* existing = dpp::find_role(role_id); existing != nullptr && existing->name == *name) {
			respond(event, "A role with that name already exists.");
			return;
		}
	}

	dpp::role role;
	role.guild_id = event.command.guild_id;
	role.set_name(*name).set_color(*colour);
	role.permissions = profile == "moderator"
		? dpp::permission{dpp::p_manage_messages | dpp::p_moderate_members}
		: dpp::permission{};
	role.flags = 0;
	const dpp::slashcommand_t event_copy = event;
	event.thinking(true);
	cluster_.role_create(role, [this, event_copy](const dpp::confirmation_callback_t& result) {
		if (result.is_error()) {
			audit(event_copy, "role-create", "failed");
			event_copy.edit_response(dpp::message{"Discord rejected the role creation."}
				.set_allowed_mentions(false, false, false));
			return;
		}
		const auto* created = std::get_if<dpp::role>(&result.value);
		const std::string name = created == nullptr ? "role" : created->name;
		audit(event_copy, "role-create", "success " + name);
		event_copy.edit_response(dpp::message{"Created role **" + sanitize_for_log(name) + "**."}
			.set_allowed_mentions(false, false, false));
	});
}

void AdminService::handle_role_edit(const dpp::slashcommand_t& event) {
	std::string error;
	if (!authorize(event, dpp::p_manage_roles, error)) {
		respond(event, error);
		return;
	}
	const auto role_id = parameter_snowflake(event, "role");
	const auto role = role_id.has_value() ? find_role(event.command.guild_id, *role_id) : std::nullopt;
	if (!role.has_value() || !can_manage_role(event.command.guild_id, *role, error)) {
		respond(event, error.empty() ? "The selected role was not found in this server." : error);
		return;
	}

	dpp::role edited = *role;
	bool changed = false;
	if (const auto name = parameter_string(event, "name"); name.has_value()) {
		edited.set_name(*name);
		changed = true;
	}
	if (const auto color = parameter_string(event, "color"); color.has_value()) {
		const auto parsed = parse_colour(*color);
		if (!parsed.has_value()) {
			respond(event, "Color must be six hexadecimal digits, for example `#5865F2`.");
			return;
		}
		edited.set_color(*parsed);
		changed = true;
	}
	if (const auto profile = parameter_string(event, "profile"); profile.has_value()) {
		if (*profile == "cosmetic") {
			edited.permissions = dpp::permission{};
		} else if (*profile == "moderator") {
			edited.permissions = dpp::permission{dpp::p_manage_messages | dpp::p_moderate_members};
		} else {
			respond(event, "Profile must be `cosmetic` or `moderator`. Administrator permissions are never granted automatically.");
			return;
		}
		changed = true;
	}
	if (!changed) {
		respond(event, "Provide a name, color, or profile to change.");
		return;
	}
	if (!require_confirmation(event, "Edit role **" + sanitize_for_log(role->name) + "**.")) {
		return;
	}

	const dpp::slashcommand_t event_copy = event;
	event.thinking(true);
	cluster_.role_edit(edited, [this, event_copy](const dpp::confirmation_callback_t& result) {
		if (result.is_error()) {
			audit(event_copy, "role-edit", "failed");
			event_copy.edit_response(dpp::message{"Discord rejected the role update."}
				.set_allowed_mentions(false, false, false));
			return;
		}
		audit(event_copy, "role-edit", "success");
		event_copy.edit_response(dpp::message{"Role updated."}
			.set_allowed_mentions(false, false, false));
	});
}

void AdminService::handle_role_delete(const dpp::slashcommand_t& event) {
	std::string error;
	if (!authorize(event, dpp::p_manage_roles, error)) {
		respond(event, error);
		return;
	}
	const auto role_id = parameter_snowflake(event, "role");
	const auto role = role_id.has_value() ? find_role(event.command.guild_id, *role_id) : std::nullopt;
	if (!role.has_value() || !can_manage_role(event.command.guild_id, *role, error)) {
		respond(event, error.empty() ? "The selected role was not found in this server." : error);
		return;
	}
	if (!require_confirmation(event, "Delete role **" + sanitize_for_log(role->name) + "**.")) {
		return;
	}

	const dpp::slashcommand_t event_copy = event;
	event.thinking(true);
	cluster_.role_delete(event.command.guild_id, role->id, [this, event_copy](const dpp::confirmation_callback_t& result) {
		if (result.is_error()) {
			audit(event_copy, "role-delete", "failed");
			event_copy.edit_response(dpp::message{"Discord rejected the role deletion."}
				.set_allowed_mentions(false, false, false));
			return;
		}
		audit(event_copy, "role-delete", "success");
		event_copy.edit_response(dpp::message{"Role deleted."}
			.set_allowed_mentions(false, false, false));
	});
}

void AdminService::handle_role_member(const dpp::slashcommand_t& event, const bool add) {
	std::string error;
	if (!authorize(event, dpp::p_manage_roles, error)) {
		respond(event, error);
		return;
	}
	const auto user_id = parameter_snowflake(event, "user");
	const auto role_id = parameter_snowflake(event, "role");
	const auto role = role_id.has_value() ? find_role(event.command.guild_id, *role_id) : std::nullopt;
	if (!user_id.has_value() || !role.has_value() || !can_manage_role(event.command.guild_id, *role, error)) {
		respond(event, error.empty() ? "Provide a valid member and manageable role from this server." : error);
		return;
	}
	if (!can_target_member(event, *user_id, error)) {
		respond(event, error);
		return;
	}

	const dpp::slashcommand_t event_copy = event;
	event.thinking(true);
	auto callback = [this, event_copy, add](const dpp::confirmation_callback_t& result) {
		if (result.is_error()) {
			audit(event_copy, add ? "role-add" : "role-remove", "failed");
			event_copy.edit_response(dpp::message{"Discord rejected the member role update."}
				.set_allowed_mentions(false, false, false));
			return;
		}
		audit(event_copy, add ? "role-add" : "role-remove", "success");
		event_copy.edit_response(dpp::message{add ? "Role added to the member." : "Role removed from the member."}
			.set_allowed_mentions(false, false, false));
	};
	if (add) {
		cluster_.guild_member_add_role(event.command.guild_id, *user_id, role->id, std::move(callback));
	} else {
		cluster_.guild_member_remove_role(event.command.guild_id, *user_id, role->id, std::move(callback));
	}
}

void AdminService::handle_member_warn(const dpp::slashcommand_t& event) {
	std::string error;
	if (!authorize(event, dpp::p_moderate_members, error)) {
		respond(event, error);
		return;
	}
	const auto user_id = parameter_snowflake(event, "user");
	const auto reason = parameter_string(event, "reason");
	if (!user_id.has_value() || !reason.has_value()) {
		respond(event, "A member and warning reason are required.");
		return;
	}
	if (!can_target_member(event, *user_id, error)) {
		respond(event, error);
		return;
	}

	audit(event, "member-warn", "user=" + std::to_string(static_cast<std::uint64_t>(*user_id)) +
		" reason=" + *reason);
	respond(event, "Warning recorded in `#mod-log`. This warning is currently stored in Discord's audit channel, not a database.");
}

void AdminService::handle_member_timeout(const dpp::slashcommand_t& event, const bool remove) {
	std::string error;
	if (!authorize(event, dpp::p_moderate_members, error)) {
		respond(event, error);
		return;
	}
	const auto user_id = parameter_snowflake(event, "user");
	if (!user_id.has_value()) {
		respond(event, "A valid member is required.");
		return;
	}
	if (!can_target_member(event, *user_id, error)) {
		respond(event, error);
		return;
	}

	const auto reason = parameter_string(event, "reason").value_or("No reason supplied");
	if (remove) {
		const dpp::slashcommand_t event_copy = event;
		event.thinking(true);
		cluster_.guild_member_timeout_remove(event.command.guild_id, *user_id, [this, event_copy](const dpp::confirmation_callback_t& result) {
			if (result.is_error()) {
				audit(event_copy, "member-untimeout", "failed");
				event_copy.edit_response(dpp::message{"Discord rejected the timeout removal."}
					.set_allowed_mentions(false, false, false));
				return;
			}
			audit(event_copy, "member-untimeout", "success");
			event_copy.edit_response(dpp::message{"Member timeout removed."}
				.set_allowed_mentions(false, false, false));
		});
		return;
	}

	const auto minutes = parameter_integer(event, "minutes");
	if (!minutes.has_value() || *minutes < 1 || *minutes > max_timeout_minutes) {
		respond(event, "Timeout minutes must be between 1 and 40320.");
		return;
	}
	if (!require_confirmation(event, "Timeout member `" + std::to_string(static_cast<std::uint64_t>(*user_id)) +
		"` for " + std::to_string(*minutes) + " minute(s).")) {
		return;
	}

	const std::time_t until = std::time(nullptr) + static_cast<std::time_t>(*minutes * 60);
	const dpp::slashcommand_t event_copy = event;
	event.thinking(true);
	cluster_.guild_member_timeout(event.command.guild_id, *user_id, until, [this, event_copy](const dpp::confirmation_callback_t& result) {
		if (result.is_error()) {
			audit(event_copy, "member-timeout", "failed");
			event_copy.edit_response(dpp::message{"Discord rejected the member timeout."}
				.set_allowed_mentions(false, false, false));
			return;
		}
		audit(event_copy, "member-timeout", "success");
		event_copy.edit_response(dpp::message{"Member timed out."}
			.set_allowed_mentions(false, false, false));
	});
}

void AdminService::handle_member_kick(const dpp::slashcommand_t& event) {
	std::string error;
	if (!authorize(event, dpp::p_kick_members, error)) {
		respond(event, error);
		return;
	}
	const auto user_id = parameter_snowflake(event, "user");
	const auto reason = parameter_string(event, "reason");
	if (!user_id.has_value() || !reason.has_value()) {
		respond(event, "A member and reason are required.");
		return;
	}
	if (!can_target_member(event, *user_id, error)) {
		respond(event, error);
		return;
	}
	if (!require_confirmation(event, "Kick member `" + std::to_string(static_cast<std::uint64_t>(*user_id)) + "`.")) {
		return;
	}

	const dpp::slashcommand_t event_copy = event;
	event.thinking(true);
	cluster_.guild_member_kick(event.command.guild_id, *user_id, [this, event_copy](const dpp::confirmation_callback_t& result) {
		if (result.is_error()) {
			audit(event_copy, "member-kick", "failed");
			event_copy.edit_response(dpp::message{"Discord rejected the kick."}
				.set_allowed_mentions(false, false, false));
			return;
		}
		audit(event_copy, "member-kick", "success");
		event_copy.edit_response(dpp::message{"Member kicked."}
			.set_allowed_mentions(false, false, false));
	});
}

void AdminService::handle_member_ban(const dpp::slashcommand_t& event, const bool remove) {
	std::string error;
	if (!authorize(event, dpp::p_ban_members, error)) {
		respond(event, error);
		return;
	}
	const auto user_id = parameter_snowflake(event, "user");
	if (!user_id.has_value()) {
		respond(event, "A valid user ID is required.");
		return;
	}
	const auto reason = parameter_string(event, "reason").value_or("No reason supplied");
	if (remove) {
		if (!require_confirmation(event, "Unban user `" + std::to_string(static_cast<std::uint64_t>(*user_id)) + "`.")) {
			return;
		}
		const dpp::slashcommand_t event_copy = event;
		event.thinking(true);
		cluster_.guild_ban_delete(event.command.guild_id, *user_id, [this, event_copy](const dpp::confirmation_callback_t& result) {
			if (result.is_error()) {
				audit(event_copy, "member-unban", "failed");
				event_copy.edit_response(dpp::message{"Discord rejected the unban."}
					.set_allowed_mentions(false, false, false));
				return;
			}
			audit(event_copy, "member-unban", "success");
			event_copy.edit_response(dpp::message{"User unbanned."}
				.set_allowed_mentions(false, false, false));
		});
		return;
	}

	if (!can_target_member(event, *user_id, error)) {
		respond(event, error);
		return;
	}
	const auto delete_days = parameter_integer(event, "delete_days").value_or(0);
	if (delete_days < 0 || delete_days > 7) {
		respond(event, "delete_days must be between 0 and 7.");
		return;
	}
	if (!require_confirmation(event, "Ban member `" + std::to_string(static_cast<std::uint64_t>(*user_id)) +
		"` and delete " + std::to_string(delete_days) + " day(s) of messages.")) {
		return;
	}

	const dpp::slashcommand_t event_copy = event;
	event.thinking(true);
	cluster_.guild_ban_add(
		event.command.guild_id,
		*user_id,
		static_cast<uint32_t>(delete_days * 86400),
		[this, event_copy](const dpp::confirmation_callback_t& result) {
			if (result.is_error()) {
				audit(event_copy, "member-ban", "failed");
				event_copy.edit_response(dpp::message{"Discord rejected the ban."}
					.set_allowed_mentions(false, false, false));
				return;
			}
			audit(event_copy, "member-ban", "success");
			event_copy.edit_response(dpp::message{"Member banned."}
				.set_allowed_mentions(false, false, false));
		});
}

void AdminService::handle_messages_purge(const dpp::slashcommand_t& event) {
	std::string error;
	if (!authorize(event, dpp::p_manage_messages, error)) {
		respond(event, error);
		return;
	}
	const auto amount = parameter_integer(event, "amount");
	if (!amount.has_value() || *amount < 1 || *amount > max_purge_messages) {
		respond(event, "Amount must be between 1 and 100 messages.");
		return;
	}
	const dpp::snowflake channel_id = parameter_snowflake(event, "channel").value_or(event.command.channel_id);
	const auto channel = find_channel(event.command.guild_id, channel_id);
	if (!channel.has_value() || !is_text_channel(*channel)) {
		respond(event, "Select a text channel in this server.");
		return;
	}
	const auto reason = parameter_string(event, "reason").value_or("No reason supplied");
	if (!require_confirmation(event, "Delete up to " + std::to_string(*amount) + " recent message(s) in **" +
		sanitize_for_log(channel->name) + "**.")) {
		return;
	}

	const dpp::slashcommand_t event_copy = event;
	event.thinking(true);
	cluster_.messages_get(channel_id, 0, 0, 0, static_cast<uint64_t>(*amount),
		[this, event_copy, channel_id](const dpp::confirmation_callback_t& result) {
			if (result.is_error()) {
				audit(event_copy, "messages-purge", "fetch failed");
				event_copy.edit_response(dpp::message{"Discord rejected the message lookup."}
					.set_allowed_mentions(false, false, false));
				return;
			}
			const auto* messages = std::get_if<dpp::message_map>(&result.value);
			if (messages == nullptr || messages->empty()) {
				audit(event_copy, "messages-purge", "success 0");
				event_copy.edit_response(dpp::message{"No messages were found to purge."}
					.set_allowed_mentions(false, false, false));
				return;
			}

			std::vector<dpp::snowflake> message_ids;
			message_ids.reserve(messages->size());
			for (const auto& [message_id, message] : *messages) {
				(void)message;
				message_ids.push_back(message_id);
			}
			if (message_ids.size() == 1) {
				cluster_.message_delete(message_ids.front(), channel_id, [this, event_copy](const dpp::confirmation_callback_t& delete_result) {
					if (delete_result.is_error()) {
						audit(event_copy, "messages-purge", "delete failed");
						event_copy.edit_response(dpp::message{"Discord rejected the message deletion."}
							.set_allowed_mentions(false, false, false));
						return;
					}
					audit(event_copy, "messages-purge", "success 1");
					event_copy.edit_response(dpp::message{"Deleted 1 message."}
						.set_allowed_mentions(false, false, false));
				});
				return;
			}

			cluster_.message_delete_bulk(message_ids, channel_id, [this, event_copy, count = message_ids.size()](const dpp::confirmation_callback_t& delete_result) {
				if (delete_result.is_error()) {
					audit(event_copy, "messages-purge", "delete failed");
					event_copy.edit_response(dpp::message{"Discord rejected the bulk message deletion."}
						.set_allowed_mentions(false, false, false));
					return;
				}
				audit(event_copy, "messages-purge", "success " + std::to_string(count));
				event_copy.edit_response(dpp::message{"Deleted " + std::to_string(count) + " messages."}
					.set_allowed_mentions(false, false, false));
			});
		});
}

void AdminService::handle_audit_log(const dpp::slashcommand_t& event) {
	std::string error;
	if (!authorize(event, dpp::permission{}, error)) {
		respond(event, error);
		return;
	}
	respond(event, "Administrative actions are recorded in `#mod-log` and in the BigDPP process log.\n"
		"The current Phase 1 build does not persist warnings or audit records in PostgreSQL.");
}

void AdminService::handle_server_setup(const dpp::slashcommand_t& event) {
	std::string error;
	if (!authorize(event, dpp::p_manage_channels | dpp::p_manage_roles, error)) {
		respond(event, error);
		return;
	}
	if (!require_confirmation(event, "Create missing baseline categories, channels, roles, and a restricted `#mod-log`.")) {
		return;
	}

	const auto* guild = dpp::find_guild(event.command.guild_id);
	if (guild == nullptr) {
		respond(event, "The server is not available in BigDPP's cache yet.");
		return;
	}

	struct setup_state final {
		dpp::slashcommand_t event;
		dpp::snowflake guild_id;
		std::array<dpp::snowflake, 3> categories{};
		std::size_t category_index{0};
		std::size_t channel_index{0};
		std::size_t role_index{0};
		std::size_t created_categories{0};
		std::size_t created_channels{0};
		std::size_t created_roles{0};
		std::size_t existing_items{0};
		std::size_t failures{0};
	};

	auto state = std::make_shared<setup_state>();
	state->event = event;
	state->guild_id = event.command.guild_id;

	static constexpr std::array category_names{"INFORMATION", "COMMUNITY", "STAFF"};
	static constexpr std::array role_presets{
		std::pair{"Server Owner", uint32_t{0xF1C40F}},
		std::pair{"Administrator", uint32_t{0xE74C3C}},
		std::pair{"Moderator", uint32_t{0x3498DB}},
		std::pair{"Member", uint32_t{0x2ECC71}},
		std::pair{"AI User", uint32_t{0x9B59B6}},
	};
	struct channel_preset final {
		const char* name;
		std::size_t category_index;
		bool restricted;
	};
	static constexpr std::array channel_presets{
		channel_preset{"welcome", 0, false},
		channel_preset{"rules", 0, false},
		channel_preset{"general", 1, false},
		channel_preset{"ai-agent", 1, false},
		channel_preset{"mod-log", 2, true},
	};

	auto finish = std::make_shared<std::function<void()>>();
	auto create_roles = std::make_shared<std::function<void()>>();
	auto create_channels = std::make_shared<std::function<void()>>();
	auto create_categories = std::make_shared<std::function<void()>>();

	*finish = [this, state]() {
		std::ostringstream output;
		output << "Server setup finished. Created " << state->created_categories << " categor(ies), "
			<< state->created_channels << " channel(s), and " << state->created_roles << " role(s)."
			<< " Existing items: " << state->existing_items << ".";
		if (state->failures > 0) {
			output << " Failures: " << state->failures << ".";
		}
		audit(state->event, "server-setup", output.str());
		state->event.edit_response(dpp::message{output.str()}
			.set_allowed_mentions(false, false, false));
	};

	*create_roles = [this, state, finish, create_roles]() {
		if (state->role_index >= role_presets.size()) {
			(*finish)();
			return;
		}
		const auto [name, colour] = role_presets[state->role_index++];
		const auto* guild_now = dpp::find_guild(state->guild_id);
		const dpp::role* existing_role = nullptr;
		if (guild_now != nullptr) {
			for (const dpp::snowflake role_id : guild_now->roles) {
				if (const auto* role = dpp::find_role(role_id); role != nullptr && role->name == name) {
					existing_role = role;
					break;
				}
			}
		}
		if (existing_role != nullptr) {
			++state->existing_items;
			(*create_roles)();
			return;
		}

		dpp::role role;
		role.guild_id = state->guild_id;
		role.set_name(name).set_color(colour);
		role.permissions = dpp::permission{};
		role.flags = 0;
		cluster_.role_create(role, [this, state, create_roles](const dpp::confirmation_callback_t& result) {
			if (result.is_error()) {
				++state->failures;
			} else {
				++state->created_roles;
			}
			(*create_roles)();
		});
	};

	*create_channels = [this, state, create_channels, create_roles]() {
		if (state->channel_index >= channel_presets.size()) {
			(*create_roles)();
			return;
		}
		const channel_preset preset = channel_presets[state->channel_index++];
		const auto* guild_now = dpp::find_guild(state->guild_id);
		const dpp::channel* existing_channel = nullptr;
		if (guild_now != nullptr) {
			for (const dpp::snowflake channel_id : guild_now->channels) {
				if (const auto* channel = dpp::find_channel(channel_id); channel != nullptr &&
					channel->name == preset.name && is_text_channel(*channel)) {
					existing_channel = channel;
					break;
				}
			}
		}
		if (existing_channel != nullptr) {
			++state->existing_items;
			(*create_channels)();
			return;
		}

		dpp::channel channel;
		channel.set_guild_id(state->guild_id)
			.set_name(preset.name)
			.set_type(dpp::CHANNEL_TEXT)
			.set_parent_id(state->categories[preset.category_index]);
		if (preset.name == std::string_view{"welcome"}) {
			channel.set_topic("Welcome to the server.");
		} else if (preset.name == std::string_view{"rules"}) {
			channel.set_topic("Please read the server rules before participating.");
		} else if (preset.name == std::string_view{"ai-agent"}) {
			channel.set_topic("Ask the local BigDPP AI with /ask.");
		} else if (preset.restricted) {
			channel.set_topic("Private BigDPP administrative audit log.");
			channel.set_permission_overwrite(state->guild_id, dpp::ot_role, 0, dpp::p_view_channel);
			channel.set_permission_overwrite(
				cluster_.me.id,
				dpp::ot_member,
				dpp::p_view_channel | dpp::p_send_messages | dpp::p_read_message_history,
				0);
		}
		cluster_.channel_create(channel, [this, state, create_channels](const dpp::confirmation_callback_t& result) {
			if (result.is_error()) {
				++state->failures;
			} else {
				++state->created_channels;
			}
			(*create_channels)();
		});
	};

	*create_categories = [this, state, create_categories, create_channels]() {
		if (state->category_index >= category_names.size()) {
			(*create_channels)();
			return;
		}
		const std::size_t index = state->category_index++;
		const auto* guild_now = dpp::find_guild(state->guild_id);
		const dpp::channel* existing_category = nullptr;
		if (guild_now != nullptr) {
			for (const dpp::snowflake channel_id : guild_now->channels) {
				if (const auto* channel = dpp::find_channel(channel_id); channel != nullptr &&
					channel->name == category_names[index] && channel->get_type() == dpp::CHANNEL_CATEGORY) {
					existing_category = channel;
					break;
				}
			}
		}
		if (existing_category != nullptr) {
			state->categories[index] = existing_category->id;
			++state->existing_items;
			(*create_categories)();
			return;
		}

		dpp::channel category;
		category.set_guild_id(state->guild_id).set_name(category_names[index]).set_type(dpp::CHANNEL_CATEGORY);
		cluster_.channel_create(category, [this, state, create_categories, index](const dpp::confirmation_callback_t& result) {
			if (result.is_error()) {
				++state->failures;
			} else if (const auto* created = std::get_if<dpp::channel>(&result.value); created != nullptr) {
				state->categories[index] = created->id;
				++state->created_categories;
			} else {
				++state->failures;
			}
			(*create_categories)();
		});
	};

	event.thinking(true);
	(*create_categories)();
}

} // namespace bigdpp
