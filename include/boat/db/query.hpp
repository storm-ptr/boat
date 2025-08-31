// Andrew Naplavkov

#ifndef BOAT_DB_QUERY_HPP
#define BOAT_DB_QUERY_HPP

#include <boat/detail/unicode.hpp>
#include <boat/pfr/variant.hpp>
#include <boost/algorithm/string.hpp>
#include <functional>
#include <generator>
#include <vector>

namespace boat::db {

struct id : std::string {
    using std::string::string;
    explicit id(std::string name) { assign(std::move(name)); }
};

class query : std::vector<std::variant<std::string, id, pfr::variant>> {
public:
    query(auto&&... vals)
        requires requires { ((*this << vals), ...); }
    {
        ((*this << std::forward<decltype(vals)>(vals)), ...);
    }

    query& operator<<(std::convertible_to<std::string> auto&& sql)
    {
        emplace_back(std::in_place_type<std::string>,
                     std::forward<decltype(sql)>(sql));
        return *this;
    }

    query& operator<<(id name)
    {
        push_back(std::move(name));
        return *this;
    }

    query& operator<<(pfr::variant param)
    {
        push_back(std::move(param));
        return *this;
    }

    std::string sql(char id_quote, std::string_view param_mark) const
    {
        auto os = std::ostringstream{};
        os.imbue(std::locale::classic());
        os.precision(std::numeric_limits<double>::digits10);
        auto num_params = 0;
        auto param_vis = overloaded{
            [&](pfr::null) { os << "null"; },
            [&](arithmetic auto v) { os << v; },
            [&](std::string_view v) { os << unicode::quoted(v, '\''); },
            [&](blob_view v) { os << "x'" << hex{v} << "'"; }};
        auto item_vis = overloaded{
            [&](std::string const& v) { os << v; },
            [&](id const& v) { os << unicode::quoted(v, id_quote); },
            [&](pfr::variant const& v) {
                if (param_mark.empty() or not v)
                    std::visit(param_vis, v);
                else
                    os << boost::replace_first_copy(
                        std::string{param_mark}, "{}", to_chars(++num_params));
            }};
        for (auto& item : *this)
            std::visit(item_vis, item);
        return std::move(os).str();
    }

    std::generator<std::reference_wrapper<pfr::variant const>> params() const
    {
        for (auto& item : *this)
            if (auto* ptr = std::get_if<pfr::variant>(&item); ptr and *ptr)
                co_yield std::cref(*ptr);
    }
};

}  // namespace boat::db

#endif  // BOAT_DB_QUERY_HPP
