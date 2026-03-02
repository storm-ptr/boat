// Andrew Naplavkov

#ifndef BOAT_SQL_DIALECTS_HPP
#define BOAT_SQL_DIALECTS_HPP

#include <boat/sql/detail/dialects/mssql.hpp>
#include <boat/sql/detail/dialects/mysql.hpp>
#include <boat/sql/detail/dialects/postgres.hpp>
#include <boat/sql/detail/dialects/sqlite.hpp>

namespace boat::sql::dialects {

inline dialect const& find(std::string_view dbms)
{
    if (is_mssql(dbms)) {
        static const auto ret = mssql{};
        return ret;
    }
    if (is_mysql(dbms)) {
        static const auto ret = mysql{};
        return ret;
    }
    if (is_postgres(dbms)) {
        static const auto ret = postgres{};
        return ret;
    }
    if (is_sqlite(dbms)) {
        static const auto ret = sqlite{};
        return ret;
    }
    throw std::runtime_error(std::string{dbms});
}

}  // namespace boat::sql::dialects

#endif  // BOAT_SQL_DIALECTS_HPP
