// Andrew Naplavkov

#ifndef BOAT_SQL_IO_HPP
#define BOAT_SQL_IO_HPP

#include <boat/pfr/io.hpp>
#include <boat/sql/detail/utility.hpp>
#include <boat/sql/vocabulary.hpp>

namespace boat::sql {

auto& operator<<(ostream auto& out, table const& in)
{
    auto rs = pfr::rowset{{
        in.schema_name.empty()  //
            ? in.table_name
            : concat(in.schema_name, ".", in.table_name),
        in.lcase_dbms,
    }};
    for (auto& col : in.columns) {
        auto suf = col.srid > 0 ? col.srid : col.length > 0 ? col.length : 0;
        rs.rows.push_back({
            col.column_name,
            suf > 0 ? concat(col.lcase_type, ":", suf) : col.lcase_type,
        });
    }
    for (auto idx : in.indices()) {
        auto key = std::ranges::begin(idx);
        auto spatial = has_geo(in.columns, key->column_name);
        rs.columns.push_back(concat(  //
            key->partial   ? "part:"
            : key->primary ? "pk:"
            : spatial      ? "spat:"
            : key->unique  ? "uniq:"
                           : "idx:",
            std::ranges::size(idx)));
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
