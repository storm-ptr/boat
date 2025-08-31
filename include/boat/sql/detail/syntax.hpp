// Andrew Naplavkov

#ifndef BOAT_SQL_SYNTAX_HPP
#define BOAT_SQL_SYNTAX_HPP

#include <boat/db/query.hpp>
#include <boat/geometry/algorithm.hpp>
#include <boat/sql/detail/adaptors/adaptors.hpp>
#include <boat/sql/detail/utility.hpp>

namespace boat::sql {

struct id {
    table const& tbl;

    friend db::query& operator<<(db::query& out, id const& in)
    {
        if (!in.tbl.schema_name.empty())
            out << db::id{in.tbl.schema_name} << ".";
        return out << db::id{in.tbl.table_name};
    }
};

template <range_of<index_key> R>
struct index_spec {
    R const& idx;

    friend db::query& operator<<(db::query& out, index_spec const& in)
    {
        auto sep = "(";
        for (auto& key : in.idx)
            out << std::exchange(sep, ", ") << db::id{key.column_name}
                << (key.descending ? " desc" : "");
        return out << ")";
    }
};

struct select_list {
    table const& tbl;
    std::vector<std::string> const& column_names;

    void print(db::query& out, range_of<column> auto&& cols) const
    {
        auto sep = "";
        for (auto const& col : cols) {
            out << std::exchange(sep, ", ");
            adaptors::create(tbl, col)->select(out);
        }
    }

    friend db::query& operator<<(db::query& out, select_list const& in)
    {
        auto to_column = [&](auto& column_name) {
            return *std::ranges::find(
                in.tbl.columns, column_name, &column::column_name);
        };
        in.column_names.empty()
            ? in.print(out, in.tbl.columns)
            : in.print(out, in.column_names | std::views::transform(to_column));
        return out;
    }
};

struct order_by {
    table const& tbl;
    std::vector<order_key> const& order_keys;

    void print(db::query& out, auto&& keys) const
    {
        auto sep = "\n order by ";
        for (auto& key : keys)
            out << std::exchange(sep, ", ") << db::id{tbl.table_name} << "."
                << db::id{key.column_name} << (key.descending ? " desc" : "");
    }

    friend db::query& operator<<(db::query& out, order_by const& in)
    {
        if (in.order_keys.empty())
            for (auto idx : in.tbl.indices() | std::views::filter(orderable) |
                                std::views::take(1))
                in.print(out, idx);
        else
            in.print(out, in.order_keys);
        return out;
    }
};

struct polygon {
    table const& tbl;
    column const& col;
    double xmin;
    double ymin;
    double xmax;
    double ymax;

    friend db::query& operator<<(db::query& out, polygon const& in)
    {
        auto var = pfr::to_variant(geometry::to_polygon(
            geometry::cartesian::box{{in.xmin, in.ymin}, {in.xmax, in.ymax}}));
        adaptors::create(in.tbl, in.col)->insert(out, std::move(var));
        return out;
    }
};

}  // namespace boat::sql

#endif  // BOAT_SQL_SYNTAX_HPP
