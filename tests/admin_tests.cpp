#include "bigdpp/admin.hpp"

#include <dpp/dpp.h>

#include <algorithm>
#include <cassert>
#include <string>
#include <unordered_set>

int main() {
	dpp::cluster cluster{"test-token", dpp::i_guilds};
	bigdpp::AdminService service{cluster};
	const auto commands = service.commands(dpp::snowflake{1});
	std::unordered_set<std::string> names;
	for (const auto& command : commands) {
		assert(names.insert(command.name).second);
	}

	for (const std::string required : {
		"server-info",
		"server-edit",
		"server-setup",
		"channel-create",
		"channel-edit",
		"channel-delete",
		"channel-lock",
		"channel-unlock",
		"channel-access",
		"role-create",
		"create-role",
		"role-edit",
		"role-delete",
		"role-add",
		"role-remove",
		"member-warn",
		"member-timeout",
		"member-untimeout",
		"member-kick",
		"member-ban",
		"member-unban",
		"messages-purge",
		"audit-log"}) {
		assert(names.contains(required));
	}
	assert(commands.size() >= 23);
}
