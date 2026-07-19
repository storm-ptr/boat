// Andrew Naplavkov

#ifndef BOAT_SQL_LIBPQ_PARAMS_HPP
#define BOAT_SQL_LIBPQ_PARAMS_HPP

#include <boat/db/adapted/adapted.hpp>
#include <boat/sql/libpq/detail/utility.hpp>

namespace boat::sql::libpq::params {

template <arithmetic T, Oid oid>
    requires(!mixed(std::endian::native))
struct scalar {
    T val;

    explicit scalar(T val_) : val(val_)
    {
        if constexpr (std::endian::native != std::endian::big)
            val = byteswap(val);
    }

    Oid type() const { return oid; }
    char const* value() const { return as_chars(&val); }
    int length() const { return sizeof val; }
    int format() const { return binary_fmt; }
};

template <class T, Oid oid, int fmt>
struct array {
    T view;

    Oid type() const { return oid; }
    char const* value() const { return as_chars(view.data()); }
    int length() const { return int(view.size()); }
    int format() const { return fmt; }
};

using integer = scalar<int64_t, int8_oid>;
using real = scalar<double, float8_oid>;
using text = array<std::string_view, text_oid, text_fmt>;
using binary = array<blob_view, bytea_oid, binary_fmt>;
using param = std::variant<integer, real, text, binary>;

inline param make(db::variant const& var)
{
    constexpr auto vis = overloaded{
        [](db::null) -> param { throw std::logic_error("null param"); },
        [](int64_t v) { return param(std::in_place_type<integer>, v); },
        [](double v) { return param(std::in_place_type<real>, v); },
        [](std::string_view v) { return param(std::in_place_type<text>, v); },
        [](blob_view v) { return param(std::in_place_type<binary>, v); },
    };
    return std::visit(vis, var);
}

inline Oid type(param const& p)
{
    return std::visit([](auto& v) { return v.type(); }, p);
}

inline char const* value(param const& p)
{
    return std::visit([](auto& v) { return v.value(); }, p);
}

inline int length(param const& p)
{
    return std::visit([](auto& v) { return v.length(); }, p);
}

inline int format(param const& p)
{
    return std::visit([](auto& v) { return v.format(); }, p);
}

}  // namespace boat::sql::libpq::params

#endif  // BOAT_SQL_LIBPQ_PARAMS_HPP
