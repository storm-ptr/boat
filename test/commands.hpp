// Andrew Naplavkov

#ifndef BOAT_TEST_COMMANDS_HPP
#define BOAT_TEST_COMMANDS_HPP

#include <boat/db/commands.hpp>
#include <boost/preprocessor/stringize.hpp>

inline std::generator<std::unique_ptr<boat::db::command>> commands()
{
    constexpr auto password =
#if defined(BOAT_TEST_PASSWORD)
        BOOST_PP_STRINGIZE(BOAT_TEST_PASSWORD)
#else
        "E207cGYM"
#endif
            ;
    constexpr auto host = "192.168.31.128";
    constexpr auto mssql_host =
#if defined(BOAT_TEST_MSSQL_HOST)
        BOOST_PP_STRINGIZE(BOAT_TEST_MSSQL_HOST)
#else
        host
#endif
            ;
    constexpr auto postgresql_host =
#if defined(BOAT_TEST_POSTGRESQL_HOST)
        BOOST_PP_STRINGIZE(BOAT_TEST_POSTGRESQL_HOST)
#else
        host
#endif
            ;
    co_yield boat::db::create(boat::concat("postgresql://postgres:",
                                           password,
                                           "@",
                                           postgresql_host,
                                           "/postgres?client_encoding=UTF8"));
    co_yield boat::db::create(boat::concat(
        "odbc://sa:", password, "@", mssql_host, "/master?DRIVER=SQL Server"));
    auto cmd = boat::db::create("sqlite:///:memory:");
    cmd->exec("SELECT InitSpatialMetaData('WGS84')");
    co_yield std::move(cmd);
}

#endif  // BOAT_TEST_COMMANDS_HPP
