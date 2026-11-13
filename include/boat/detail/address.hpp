// Andrew Naplavkov

#ifndef BOAT_ADDRESS_HPP
#define BOAT_ADDRESS_HPP

#include <boat/sql/odbc/drivers.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <format>
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
    std::format("mysql://root:{}@{}/mysql", password, mysql_host);

inline static auto const mysql_gdal_address = std::format(
    "mysql:mysql,user=root,password={},host={},port=3306",
    password,
    mysql_host);

inline static auto const postgres_address = std::format(
    "postgres://postgres:{}@{}/postgres?client_encoding=UTF8",
    password,
    postgres_host);

inline static auto const postgres_gdal_address = std::format(
    "postgresql://postgres:{}@{}/postgres",
    password,
    postgres_host);

inline auto odbc_drivers(std::initializer_list<std::string> servers)
{
    static auto const regex =
        std::regex(R"(\b(\d+)(?:\.(\d+))?(?:\.(\d+))?\b)");
    constexpr auto priority = [](std::string const& lo) {
        auto match = std::cmatch{};
        std::regex_search(lo.data(), lo.data() + lo.size(), match, regex);
        auto part = [&](size_t i) {
            return match[i].matched ? std::stoi(match.str(i)) : 0;
        };
        return std::tuple{
            lo.contains("x64"),
            lo.contains("unicode"),
            std::tuple{part(1), part(2), part(3)}};
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

inline std::vector<std::string> odbc_address()
{
    auto ret = std::vector<std::string>{};
    for (auto [srv, drv] : odbc_drivers({"mysql", "postgres"})) {
        if (srv == "mysql")
            ret.push_back(std::format(
                "odbc://root:{}@{}/mysql?MULTI_STATEMENTS=1"
                "&INITSTMT=SET time_zone='+00:00'&DRIVER={}",
                password,
                mysql_host,
                drv));
        else if (srv == "postgres")
            ret.push_back(std::format(
                "odbc://postgres:{}@{}/postgres?BoolsAsChar=0&DRIVER={}",
                password,
                postgres_host,
                drv));
    }
    return ret;
}

inline std::string mssql_gdal_address()
{
#if defined(BOAT_TEST_MSSQL_HOST)
    for (auto [srv, drv] : odbc_drivers({"sql server"}))
        if (srv == "sql server")
            return std::format(
                "mssql:driver={{{}}};Encrypt=no;server={};"
                "database=master;uid=sa;pwd={}",
                drv,
                mssql_host,
                password);
#endif
    return {};
}

}  // namespace boat::config

#endif  // BOAT_ADDRESS_HPP
