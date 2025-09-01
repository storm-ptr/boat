// Andrew Naplavkov

#ifndef BOAT_TEST_COMMANDS_HPP
#define BOAT_TEST_COMMANDS_HPP

#include <boat/db/commands.hpp>

inline std::generator<std::unique_ptr<boat::db::command>> commands()
{
    co_yield boat::db::create(
        "postgresql://postgres:E207cGYM@192.168.31.128/"
        "postgres?client_encoding=UTF8");
    co_yield boat::db::create(
        "odbc://sa:E207cGYM@192.168.31.128/master?DRIVER=SQL Server");
    auto cmd = boat::db::create("sqlite:///:memory:");
    cmd->exec("SELECT InitSpatialMetaData('WGS84')");
    co_yield std::move(cmd);
}

#endif  // BOAT_TEST_COMMANDS_HPP
