// Andrew Naplavkov

#ifndef BOAT_SQL_MANIP_HPP
#define BOAT_SQL_MANIP_HPP

#include <boat/sql/detail/adaptors/adaptors.hpp>
#include <boost/geometry/algorithms/convert.hpp>

namespace boat::sql {

struct id {
    std::string_view lhs;
    std::string_view rhs;

    id(std::string_view lhs, std::string_view rhs) : lhs(lhs), rhs(rhs) {}
    explicit id(db::table const& tbl) : id{tbl.schema_name, tbl.table_name} {}

    friend db::query& operator<<(db::query& out, id const& in)
    {
        if (!in.lhs.empty())
            out << db::id{in.lhs} << ".";
        return out << db::id{in.rhs};
    }
};

template <range_of<db::index_key> T>
struct index_spec {
    T const& idx;

    friend db::query& operator<<(db::query& out, index_spec const& in)
    {
        for (auto sep{"("}; auto& key : in.idx)
            out << std::exchange(sep, ", ") << db::id{key.column_name}
                << (key.descending ? " desc" : "");
        return out << ")";
    }
};

struct select_list {
    db::table const& tbl;
    std::span<std::string const> cols;

    void print(db::query& out, range_of<db::column> auto&& cols) const
    {
        for (auto sep{""}; auto const& col : cols) {
            out << std::exchange(sep, ", ");
            adaptors::make(tbl.dbms, col)->select(out);
        }
    }

    friend db::query& operator<<(db::query& out, select_list const& in)
    {
        in.cols.empty()
            ? in.print(out, in.tbl.columns)
            : in.print(
                  out, in.cols | std::views::transform([&](auto& col) -> auto& {
                           return find(in.tbl.columns, col);
                       }));
        return out;
    }
};

struct order_by {
    db::table const& tbl;
    std::span<db::order_key const> keys;

    void print(db::query& out, auto&& keys) const
    {
        for (auto sep{"\n order by "}; auto& key : keys)
            out << std::exchange(sep, ", ") << db::id{tbl.table_name} << "."
                << db::id{key.column_name} << (key.descending ? " desc" : "");
    }

    friend db::query& operator<<(db::query& out, order_by const& in)
    {
        if (in.keys.empty())
            for (auto idx : indices(in.tbl) | std::views::filter(orderable) |
                                std::views::take(1))
                in.print(out, idx);
        else
            in.print(out, in.keys);
        return out;
    }
};

struct bbox {
    std::string_view dbms;
    db::column const& col;
    geometry::cartesian::box const& bounds;

    friend db::query& operator<<(db::query& out, bbox const& in)
    {
        auto poly = geometry::cartesian::polygon{};
        boost::geometry::convert(in.bounds, poly);
        adaptors::make(in.dbms, in.col)->insert(out, db::to_variant(poly));
        return out;
    }
};

}  // namespace boat::sql

#endif  // BOAT_SQL_MANIP_HPP
