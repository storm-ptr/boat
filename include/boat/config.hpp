// Andrew Naplavkov

#ifndef BOAT_CONFIG_HPP
#define BOAT_CONFIG_HPP

#include <boat/sql/odbc/drivers.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <regex>
#include <unordered_map>

namespace boat::config {

constexpr auto password =
#if defined(BOAT_TEST_PASSWORD)
    BOOST_PP_STRINGIZE(BOAT_TEST_PASSWORD)
#else
    "E207cGYM"
#endif
        ;

constexpr auto host = "192.168.31.129";

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

constexpr auto postgresql_host =
#if defined(BOAT_TEST_POSTGRESQL_HOST)
    BOOST_PP_STRINGIZE(BOAT_TEST_POSTGRESQL_HOST)
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

inline static auto const postgresql_address = boat::concat(  //
    "postgresql://postgres:",
    password,
    "@",
    postgresql_host,
    "/postgres?client_encoding=UTF8");

inline static auto const postgresql_gdal_address = boat::concat(  //
    "pg:host='",
    postgresql_host,
    "' dbname='postgres' user='postgres' password='",
    password,
    "'");

namespace detail {

inline auto odbc_drivers(std::initializer_list<std::string> servers)
{
    static auto const regex = std::regex(R"(\b\d+(?:\.\d+)?\b)");
    constexpr auto priority = [](std::string const& lo) {
        auto match = std::cmatch{};
        auto ver =
            std::regex_search(lo.data(), lo.data() + lo.size(), match, regex)
                ? std::stod(match.str())
                : 0.;
        return std::tuple{lo.contains("x64"), lo.contains("unicode"), ver};
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
         detail::odbc_drivers({"mysql", "postgresql", "sql server"})) {
        if (srv == "mysql")
            ret.push_back(boat::concat(  //
                "odbc://root:",
                password,
                "@",
                mysql_host,
                "/mysql?MULTI_STATEMENTS=1&DRIVER=",
                drv));
        else if (srv == "postgresql")
            ret.push_back(boat::concat(  //
                "odbc://postgres:",
                password,
                "@",
                postgresql_host,
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
                "mssql:driver=",
                drv,
                ";Encrypt=no;server=",
                mssql_host,
                ";database=master;uid=sa;pwd=",
                password);
    return {};
}

}  // namespace boat::config

#endif  // BOAT_CONFIG_HPP
