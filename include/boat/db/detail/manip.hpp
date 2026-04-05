// Andrew Naplavkov

#ifndef BOAT_DB_MANIP_HPP
#define BOAT_DB_MANIP_HPP

#include <boat/db/variant.hpp>
#include <boat/detail/unicode.hpp>
#include <cctype>

namespace boat::db {

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

}  // namespace boat::db

#endif  // BOAT_DB_MANIP_HPP
