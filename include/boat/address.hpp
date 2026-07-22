// Andrew Naplavkov

#ifndef BOAT_ADDRESS_HPP
#define BOAT_ADDRESS_HPP

#include <boat/sql/odbc/drivers.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <regex>
#include <unordered_map>

namespace boat::config {

constexpr auto password =
#if defined(BOAT_TEST_PASSWORD)
    BOOST_PP_STRINGIZE(BOAT_TEST_PASSWORD)
#else
    "E207cGYM!"
#endif
        ;

constexpr auto host = "192.168.31.131";

constexpr auto mssql_host =
#if defined(BOAT_TEST_MSSQL_HOST)
    BOOST_PP_STRINGIZE(BOAT_TEST_MSSQL_HOST)
#else
    host
#endif
        ;

constexpr auto mysql_host =
#if defined(BOAT_TEST_MYSQL_HOST)
    BOOST_PP_STRINGIZE(BOAT_TEST_MYSQL_HOST)
#else
    host
#endif
        ;

constexpr auto postgres_host =
#if defined(BOAT_TEST_POSTGRES_HOST)
    BOOST_PP_STRINGIZE(BOAT_TEST_POSTGRES_HOST)
#else
    host
#endif
        ;

inline static auto const mysql_address =
    boat::concat("mysql://root:", password, "@", mysql_host, "/mysql");

inline static auto const mysql_gdal_address = boat::concat(  //
    "mysql:mysql,user=root,password=",
    password,
    ",host=",
    mysql_host,
    ",port=3306");

inline static auto const postgres_address = boat::concat(  //
    "postgres://postgres:",
    password,
    "@",
    postgres_host,
    "/postgres?client_encoding=UTF8");

inline static auto const postgres_gdal_address = boat::concat(  //
    "postgresql://postgres:",
    password,
    "@",
    postgres_host,
    "/postgres");

namespace detail {

inline auto odbc_drivers(std::initializer_list<std::string> servers)
{
    static auto const regex = std::regex(R"(\b\d+(?:\.\d+)?\b)");
    constexpr auto priority = [](std::string const& lo) {
        auto match = std::cmatch{};
        return std::tuple{
            lo.contains("x64"),
            lo.contains("unicode"),
            std::regex_search(lo.data(), lo.data() + lo.size(), match, regex)
                ? std::stod(match.str())
                : 0.};
    };
    auto ret = std::unordered_map<std::string, std::string>{};
    for (auto drv : sql::odbc::drivers()) {
        auto lo = to_lower(drv);
        for (auto srv : servers)
            if (lo.contains(srv) &&
                (!ret.contains(srv) ||
                 priority(lo) > priority(to_lower(ret[srv]))))
                ret[srv] = drv;
    }
    return ret;
}

}  // namespace detail

inline std::vector<std::string> odbc_address()
{
    auto ret = std::vector<std::string>{};
    for (auto [srv, drv] :
         detail::odbc_drivers({"mysql", "postgres", "sql server"})) {
        if (srv == "mysql")
            ret.push_back(boat::concat(  //
                "odbc://root:",
                password,
                "@",
                mysql_host,
                "/mysql?MULTI_STATEMENTS=1&DRIVER=",
                drv));
        else if (srv == "postgres")
            ret.push_back(boat::concat(  //
                "odbc://postgres:",
                password,
                "@",
                postgres_host,
                "/postgres?BoolsAsChar=0&DRIVER=",
                drv));
        else if (srv == "sql server")
            ret.push_back(boat::concat(  //
                "odbc://sa:",
                password,
                "@",
                mssql_host,
                "/master?Encrypt=no&DRIVER=",
                drv));
    }
    return ret;
}

inline std::string mssql_gdal_address()
{
    for (auto [srv, drv] : detail::odbc_drivers({"sql server"}))
        if (srv == "sql server")
            return boat::concat(  //
                "mssql:driver={",
                drv,
                "};Encrypt=no;server=",
                mssql_host,
                ";database=master;uid=sa;pwd=",
                password);
    return {};
}

}  // namespace boat::config

#endif  // BOAT_ADDRESS_HPP
