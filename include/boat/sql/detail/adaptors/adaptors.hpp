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

inline std::unique_ptr<adaptor> try_create(table const& tbl, column const& col)
{
    using all =
        boost::mpl::list<binary, integer, real, spatial, text, timestamp>;
    auto ret = std::unique_ptr<adaptor>{};
    boost::mpl::for_each<all>([&]<class T>(T v) {
        if (!ret && v.init(tbl, col))
            ret = std::make_unique<T>(std::move(v));
    });
    return ret;
}

inline std::unique_ptr<adaptor> create(table const& tbl, column const& col)
{
    if (auto ret = try_create(tbl, col))
        return ret;
    throw std::runtime_error(concat(col.column_name, " ", col.type_name));
}

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_HPP
