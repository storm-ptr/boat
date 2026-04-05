// Andrew Naplavkov

#ifndef BOAT_DB_IO_HPP
#define BOAT_DB_IO_HPP

#include <boat/db/detail/manip.hpp>
#include <boat/db/meta.hpp>
#include <boat/db/rowset.hpp>

namespace boat::db {

auto& operator<<(ostream auto& out, rowset const& in)
{
    using char_t = std::decay_t<decltype(out)>::char_type;
    auto sizes = std::vector<size_t>(in.columns.size());
    auto cols = in.columns | std::ranges::to<std::vector<variant>>();
    auto line = std::vector<variant>(cols.size());
    manip{sizes, cols}.resize(out);
    for (auto& row : in)
        manip{sizes, row}.resize(out);
    out << '\n'
        << std::setfill<char_t>(' ') << manip{sizes, cols}
        << std::setfill<char_t>('-') << manip{sizes, line}
        << std::setfill<char_t>(' ');
    for (auto& row : in)
        out << manip{sizes, row};
    return out;
}

auto& operator<<(ostream auto& out, table const& in)
{
    auto rs = rowset{
        {concat(
             in.schema_name, in.schema_name.empty() ? "" : ".", in.table_name),
         in.dbms}};
    for (auto& col : in.columns) {
        auto suf = col.srid > 0 ? col.srid : col.length > 0 ? col.length : 0;
        rs.rows.push_back({
            col.column_name,
            suf > 0 ? concat(col.type_name, ":", suf) : col.type_name,
        });
    }
    for (auto idx : in.indices()) {
        auto key = std::ranges::begin(idx);
        auto spatial = std::ranges::any_of(in.columns, [&](auto& col) {
            return col.epsg > 0 && col.column_name == key->column_name;
        });
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
                    ? variant{}
                    : variant{key->ordinal * (key->descending ? -1 : 1)});
        }
    }
    return out << rs;
}

auto& operator<<(ostream auto& out, raster const& in)
{
    out << "{ name: " << in.schema_name << (in.schema_name.empty() ? "" : ".")
        << in.table_name << "." << in.column_name << "\n, bands: {";
    for (auto sep = ""; auto& band : in.bands)
        out << std::exchange(sep, ", ") << band.color_name << ":"
            << band.type_name;
    return out << "}\n, width: " << in.width << "\n, height: " << in.height
               << "\n, xorig: " << in.xorig << "\n, yorig: " << in.yorig
               << "\n, xscale: " << in.xscale << "\n, yscale: " << in.yscale
               << "\n, xskew: " << in.xskew << "\n, yskew: " << in.yskew
               << "\n, epsg: " << in.epsg << " }\n";
}

}  // namespace boat::db

#endif  // BOAT_DB_IO_HPP
