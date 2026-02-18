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

inline auto try_make(std::string_view dbms, column const& col)
{
    using all =
        boost::mpl::list<binary, integer, real, spatial, text, timestamp>;
    auto ret = std::unique_ptr<adaptor>{};
    boost::mpl::for_each<all>([&]<class T>(T v) {
        if (!ret && v.init(dbms, col))
            ret = std::make_unique<T>(std::move(v));
    });
    return ret;
}

inline auto make(std::string_view dbms, column const& col)
{
    if (auto ret = try_make(dbms, col))
        return ret;
    throw std::runtime_error(concat(col.column_name, " ", col.lcase_type));
}

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_HPP
