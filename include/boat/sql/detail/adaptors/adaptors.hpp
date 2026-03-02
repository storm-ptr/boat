// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_HPP
#define BOAT_SQL_ADAPTORS_HPP

#include <boat/sql/detail/adaptors/binary.hpp>
#include <boat/sql/detail/adaptors/integer.hpp>
#include <boat/sql/detail/adaptors/real.hpp>
#include <boat/sql/detail/adaptors/spatial.hpp>
#include <boat/sql/detail/adaptors/text.hpp>
#include <boat/sql/detail/adaptors/timestamp.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/list.hpp>

namespace boat::sql::adaptors {

using all = boost::mpl::list<binary, integer, real, spatial, text, timestamp>;

constexpr auto parse(std::string_view dbms)
{
    return [dbms](db::column col) {
        col.type_name = to_lower(col.type_name);
        boost::mpl::for_each<all>([&]<class T>(T v) {
            if (col.kind.empty()) {
                v.init(dbms, col);
                col.kind = v.parse();
            }
        });
        return col;
    };
}

inline auto try_make(std::string_view dbms, db::column const& col)
{
    auto ret = std::unique_ptr<adaptor>{};
    boost::mpl::for_each<all>([&]<class T>(T v) {
        if (!ret && v.init(dbms, col))
            ret = std::make_unique<T>(std::move(v));
    });
    return ret;
}

inline auto make(std::string_view dbms, db::column const& col)
{
    if (auto ret = try_make(dbms, col))
        return ret;
    throw std::runtime_error(concat(col.column_name, " ", col.type_name));
}

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_HPP
