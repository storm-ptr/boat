// Andrew Naplavkov

#ifndef BOAT_SQL_DIALECTS_HPP
#define BOAT_SQL_DIALECTS_HPP

#include <boat/sql/detail/dialects/mssql.hpp>
#include <boat/sql/detail/dialects/mysql.hpp>
#include <boat/sql/detail/dialects/postgresql.hpp>
#include <boat/sql/detail/dialects/sqlite.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/container.hpp>

namespace boat::sql::dialects {

inline dialect const& find(std::string_view dbms_name)
{
    static constexpr auto all =
        boost::fusion::list<mssql, mysql, postgresql, sqlite>{};
    auto ptr = static_cast<dialect const*>(nullptr);
    auto dbms = dbms_name | unicode::lower | unicode::string<char>;
    boost::fusion::for_each(all, [&](auto& dial) {
        if (!ptr && dial.match(dbms))
            ptr = &dial;
    });
    return ptr ? *ptr : throw std::runtime_error(dbms);
}

}  // namespace boat::sql::dialects

#endif  // BOAT_SQL_DIALECTS_HPP
