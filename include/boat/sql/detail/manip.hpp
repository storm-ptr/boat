// Andrew Naplavkov

#ifndef BOAT_SQL_MANIP_HPP
#define BOAT_SQL_MANIP_HPP

#include <boat/db/query.hpp>
#include <boat/geometry/algorithm.hpp>
#include <boat/sql/detail/adaptors/adaptors.hpp>

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

struct create_indices {
    db::table const& tbl;

    friend db::query& operator<<(db::query& out, create_indices const& in)
    {
        auto i = 0;
        for (auto idx : in.tbl.indices() | std::views::filter(constructible)) {
            auto key = std::ranges::begin(idx);
            auto spatial = geo(in.tbl.columns, key->column_name);
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
            : in.print(out, in.cols | std::views::transform([&](auto& col) {
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
    db::column const& col;
    double xmin;
    double ymin;
    double xmax;
    double ymax;

    friend db::query& operator<<(db::query& out, rect const& in)
    {
        auto var = db::to_variant(geometry::to_polygon(
            geometry::cartesian::box{{in.xmin, in.ymin}, {in.xmax, in.ymax}}));
        adaptors::make(in.dbms, in.col)->insert(out, std::move(var));
        return out;
    }
};

}  // namespace boat::sql

#endif  // BOAT_SQL_MANIP_HPP
