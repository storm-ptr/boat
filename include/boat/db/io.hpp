// Andrew Naplavkov

#ifndef BOAT_DB_IO_HPP
#define BOAT_DB_IO_HPP

#include <boat/db/meta.hpp>
#include <boat/db/rowset.hpp>
#include <boat/detail/unicode.hpp>
#include <cctype>

namespace boat::db {
namespace detail {

auto format(ostream auto const& fmt, variant const& var)
{
    auto os = std::wostringstream{};
    os.imbue(fmt.getloc());
    os.unsetf(os.flags());
    os.setf(fmt.flags());
    os.precision(fmt.precision());
    auto vis = overloaded{
        [&](null) { os << ""; },
        [&](arithmetic auto v) { os << v; },
        [&](std::string_view v) { os << unicode::io(v); },
        [&](blob_view v) { os << v.size() << " bytes"; },
    };
    std::visit(vis, var);
    return os.view() |
           std::views::transform([](auto c) { return iswspace(c) ? ' ' : c; }) |
           std::views::take(80) |
           unicode::utf<typename std::decay_t<decltype(fmt)>::char_type>;
}

template <range_of<size_t> S, range_of<variant> R>
struct manip {
    S& sizes;
    R& row;

    void resize(ostream auto const& fmt)
    {
        for (auto&& [size, var] : std::views::zip(sizes, row))
            size = std::max<>(size, unicode::num_points(format(fmt, var)));
    }

    friend auto& operator<<(ostream auto& out, manip const& in)
    {
        for (auto&& [size, var] : std::views::zip(in.sizes, in.row)) {
            auto str = format(out, var);
            auto indent = size - unicode::num_points(str);
            out << '|' << std::setw(1) << "";
            if (std::holds_alternative<std::string>(var))
                out << str << std::setw(indent) << "";
            else
                out << std::setw(indent) << "" << str;
            out << std::setw(1) << "";
        }
        return out << "|\n";
    }
};

}  // namespace detail

auto& operator<<(ostream auto& out, rowset const& in)
{
    using char_type = std::decay_t<decltype(out)>::char_type;
    auto sizes = std::vector<size_t>(in.columns.size());
    auto cols = in.columns | std::ranges::to<std::vector<variant>>();
    auto line = std::vector<variant>(cols.size());
    detail::manip{sizes, cols}.resize(out);
    for (auto& row : in)
        detail::manip{sizes, row}.resize(out);
    out << '\n'
        << std::setfill<char_type>(' ') << detail::manip{sizes, cols}
        << std::setfill<char_type>('-') << detail::manip{sizes, line}
        << std::setfill<char_type>(' ');
    for (auto& row : in)
        out << detail::manip{sizes, row};
    return out;
}

auto& operator<<(ostream auto& out, table const& in)
{
    auto rs = rowset{{
        in.schema_name.empty()  //
            ? in.table_name
            : concat(in.schema_name, ".", in.table_name),
        in.dbms,
    }};
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

}  // namespace boat::db

#endif  // BOAT_DB_IO_HPP
