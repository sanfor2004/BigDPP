#pragma once

#include <dpp/dpp.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace bigdpp {

class AdminService final {
public:
	explicit AdminService(dpp::cluster& cluster);

	[[nodiscard]] std::vector<dpp::slashcommand> commands(dpp::snowflake application_id) const;
	[[nodiscard]] bool handle(const dpp::slashcommand_t& event);

private:
	[[nodiscard]] bool authorize(
		const dpp::slashcommand_t& event,
		dpp::permission required_permission,
		std::string& error) const;
	[[nodiscard]] bool can_target_member(
		const dpp::slashcommand_t& event,
		dpp::snowflake target_id,
		std::string& error) const;
	[[nodiscard]] bool can_manage_role(
		dpp::snowflake guild_id,
		const dpp::role& role,
		std::string& error) const;
	[[nodiscard]] std::optional<dpp::snowflake> parameter_snowflake(
		const dpp::slashcommand_t& event,
		const std::string& name) const;
	[[nodiscard]] std::optional<std::string> parameter_string(
		const dpp::slashcommand_t& event,
		const std::string& name) const;
	[[nodiscard]] std::optional<int64_t> parameter_integer(
		const dpp::slashcommand_t& event,
		const std::string& name) const;
	[[nodiscard]] std::optional<bool> parameter_boolean(
		const dpp::slashcommand_t& event,
		const std::string& name) const;
	[[nodiscard]] bool require_confirmation(
		const dpp::slashcommand_t& event,
		const std::string& preview) const;
	[[nodiscard]] std::optional<uint32_t> parse_colour(const std::string& value) const;
	[[nodiscard]] std::optional<dpp::role> find_role(
		dpp::snowflake guild_id,
		dpp::snowflake role_id) const;
	[[nodiscard]] std::optional<dpp::channel> find_channel(
		dpp::snowflake guild_id,
		dpp::snowflake channel_id) const;
	[[nodiscard]] uint16_t highest_role_position(
		dpp::snowflake guild_id,
		dpp::snowflake user_id) const;
	void audit(const dpp::slashcommand_t& event, const std::string& action, const std::string& result) const;
	void respond(const dpp::slashcommand_t& event, const std::string& content) const;

	void handle_server_setup(const dpp::slashcommand_t& event);
	void handle_server_info(const dpp::slashcommand_t& event);
	void handle_server_edit(const dpp::slashcommand_t& event);
	void handle_channel_create(const dpp::slashcommand_t& event);
	void handle_channel_edit(const dpp::slashcommand_t& event);
	void handle_channel_delete(const dpp::slashcommand_t& event);
	void handle_channel_lock(const dpp::slashcommand_t& event, bool lock);
	void handle_channel_access(const dpp::slashcommand_t& event);
	void handle_role_create(const dpp::slashcommand_t& event);
	void handle_role_edit(const dpp::slashcommand_t& event);
	void handle_role_delete(const dpp::slashcommand_t& event);
	void handle_role_member(const dpp::slashcommand_t& event, bool add);
	void handle_member_warn(const dpp::slashcommand_t& event);
	void handle_member_timeout(const dpp::slashcommand_t& event, bool remove);
	void handle_member_kick(const dpp::slashcommand_t& event);
	void handle_member_ban(const dpp::slashcommand_t& event, bool remove);
	void handle_messages_purge(const dpp::slashcommand_t& event);
	void handle_audit_log(const dpp::slashcommand_t& event);

	dpp::cluster& cluster_;
};

} // namespace bigdpp
