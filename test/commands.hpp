// Andrew Naplavkov

#ifndef BOAT_TEST_COMMANDS_HPP
#define BOAT_TEST_COMMANDS_HPP

#include <boat/address.hpp>
#include <boat/sql/commands.hpp>

inline std::generator<std::unique_ptr<boat::db::command>> commands()
{
    co_yield boat::sql::make_command(boat::config::mysql_address);
    co_yield boat::sql::make_command(boat::config::postgres_address);
    co_yield boat::sql::make_command("sqlite:///:memory:");
    for (auto adr : boat::config::odbc_address())
        co_yield boat::sql::make_command(adr);
}

#endif  // BOAT_TEST_COMMANDS_HPP
