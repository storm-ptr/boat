// Andrew Naplavkov

#ifndef BOAT_SQL_MANIP_HPP
#define BOAT_SQL_MANIP_HPP

#include <boat/db/query.hpp>
#include <boat/sql/detail/adaptors/adaptors.hpp>

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

template <range_of<index_key> T>
struct index_spec {
    T const& idx;

    friend db::query& operator<<(db::query& out, index_spec const& in)
    {
        for (auto sep = "("; auto& key : in.idx)
            out << std::exchange(sep, ", ") << db::id{key.column_name}
                << (key.descending ? " desc" : "");
        return out << ")";
    }
};

struct create_indices {
    table const& tbl;

    friend db::query& operator<<(db::query& out, create_indices const& in)
    {
        auto i = 0;
        for (auto idx : in.tbl.indices() | std::views::filter(constructible)) {
            auto key = std::ranges::begin(idx);
            auto spatial = has_geo(in.tbl.columns, key->column_name);
            if (spatial && std::ranges::size(idx) > 1u)
                continue;
            auto type = spatial ? "spatial" : key->unique ? "unique" : "";
            if (key->primary)
                out << "\n alter table " << id{in.tbl} << " add primary key ";
            else
                out << "\n create " << type << " index _" << to_chars(++i)
                    << " on " << id{in.tbl} << " ";
            out << index_spec{idx} << ";";
        }
        return out;
    }
};

struct select_list {
    table const& tbl;
    std::span<std::string const> cols;

    void print(db::query& out, range_of<column> auto&& cols) const
    {
        for (auto sep = ""; auto const& col : cols) {
            out << std::exchange(sep, ", ");
            adaptors::make(tbl.lcase_dbms, col)->select(out);
        }
    }

    friend db::query& operator<<(db::query& out, select_list const& in)
    {
        auto to_column = [&](auto& column_name) {
            return *std::ranges::find(
                in.tbl.columns, column_name, &column::column_name);
        };
        in.cols.empty()
            ? in.print(out, in.tbl.columns)
            : in.print(out, in.cols | std::views::transform(to_column));
        return out;
    }
};

struct order_by {
    table const& tbl;
    std::span<order_key const> keys;

    void print(db::query& out, auto&& keys) const
    {
        for (auto sep = "\n order by "; auto& key : keys)
            out << std::exchange(sep, ", ") << db::id{tbl.table_name} << "."
                << db::id{key.column_name} << (key.descending ? " desc" : "");
    }

    friend db::query& operator<<(db::query& out, order_by const& in)
    {
        if (in.keys.empty())
            for (auto idx : in.tbl.indices() | std::views::filter(orderable) |
                                std::views::take(1))
                in.print(out, idx);
        else
            in.print(out, in.keys);
        return out;
    }
};

struct rect {
    std::string_view dbms;
    column const& col;
    double xmin;
    double ymin;
    double xmax;
    double ymax;

    friend db::query& operator<<(db::query& out, rect const& in)
    {
        auto var = pfr::to_variant(geometry::cartesian::polygon{{
            {in.xmin, in.ymin},
            {in.xmax, in.ymin},
            {in.xmax, in.ymax},
            {in.xmin, in.ymax},
            {in.xmin, in.ymin},
        }});
        adaptors::make(in.dbms, in.col)->insert(out, std::move(var));
        return out;
    }
};

}  // namespace boat::sql

#endif  // BOAT_SQL_MANIP_HPP
