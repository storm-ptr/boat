// Andrew Naplavkov

#ifndef BOAT_SQL_IO_HPP
#define BOAT_SQL_IO_HPP

#include <boat/pfr/io.hpp>
#include <boat/sql/vocabulary.hpp>

namespace boat::sql {

auto& operator<<(ostream auto& out, table const& in)
{
    auto rs = pfr::rowset{{
        in.schema_name.empty() ? in.table_name
                               : concat(in.schema_name, ".", in.table_name),
        in.dbms_name,
        "length",
        "srid",
    }};
    for (auto& col : in.columns)
        rs.rows.push_back({
            col.column_name,
            col.type_name,
            col.length > 0 ? pfr::variant{col.length} : pfr::variant{},
            col.srid > 0 ? pfr::variant{col.srid} : pfr::variant{},
        });
    for (auto idx : in.indices()) {
        auto key = std::ranges::begin(idx);
        auto spatial = any_geo(in.columns, key->column_name);
        rs.columns.push_back(concat(key->partial   ? "partial"
                                    : key->primary ? "primary"
                                    : spatial      ? "spatial"
                                    : key->unique  ? "unique"
                                                   : "index",
                                    "(",
                                    std::ranges::size(idx),
                                    ")"));
        for (auto [col, row] : std::views::zip(in.columns, rs.rows)) {
            key = std::ranges::find(
                idx, col.column_name, &index_key::column_name);
            row.push_back(
                key == std::ranges::end(idx)
                    ? pfr::variant{}
                    : pfr::variant{key->ordinal * (key->descending ? -1 : 1)});
        }
    }
    return out << rs;
}

}  // namespace boat::sql

#endif  // BOAT_SQL_IO_HPP
