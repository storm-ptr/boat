// Andrew Naplavkov

#ifndef BOAT_TEST_COMMANDS_HPP
#define BOAT_TEST_COMMANDS_HPP

#include <boat/address.hpp>
#include <boat/sql/commands.hpp>
#include <cstdlib>

inline std::generator<std::unique_ptr<boat::db::command>> commands()
{
    auto selected = std::getenv("BOAT_TEST_DB");
    auto enabled = [selected](std::string_view db) {
        return selected == nullptr || db == selected;
    };
    if (enabled("mysql"))
        co_yield boat::sql::make_command(boat::config::mysql_address);
    if (enabled("postgres"))
        co_yield boat::sql::make_command(boat::config::postgres_address);
    if (enabled("sqlite"))
        co_yield boat::sql::make_command("sqlite:///:memory:");
    if (enabled("odbc"))
        for (auto adr : boat::config::odbc_address())
            co_yield boat::sql::make_command(adr);
}

#endif  // BOAT_TEST_COMMANDS_HPP
