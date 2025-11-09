// Andrew Naplavkov

#ifndef BOAT_DB_LIBMYSQL_UTILITY_HPP
#define BOAT_DB_LIBMYSQL_UTILITY_HPP

#include <mysql.h>
#include <boat/pfr/variant.hpp>

namespace boat::db::libmysql {

struct buffer {
    std::string str;
    bool null;
    unsigned long len;
};

constexpr auto error = overloaded{
    [](MYSQL* dbc) { return mysql_error(dbc); },
    [](MYSQL_STMT* stmt) { return mysql_stmt_error(stmt); },
};

void check(bool success, auto* ptr)
    requires requires { error(ptr); }
{
    if (!success)
        throw std::runtime_error(ptr ? error(ptr) : "libmysql");
}

void check(bool success, auto& ptr)
    requires requires { check(success, ptr.get()); }
{
    check(success, ptr.get());
}

inline MYSQL_BIND to_bind(pfr::variant const& var)
{
    auto ret = MYSQL_BIND{};
    auto vis = overloaded{
        [&](pfr::null const&) { ret.buffer_type = MYSQL_TYPE_NULL; },
        [&](int64_t const& v) {
            ret.buffer_type = MYSQL_TYPE_LONGLONG;
            ret.buffer = const_cast<int64_t*>(&v);
        },
        [&](double const& v) {
            ret.buffer_type = MYSQL_TYPE_DOUBLE;
            ret.buffer = const_cast<double*>(&v);
        },
        [&](std::string const& v) {
            ret.buffer_type = MYSQL_TYPE_STRING;
            ret.buffer = const_cast<char*>(v.data());
            ret.buffer_length = static_cast<unsigned long>(v.size());
        },
        [&](blob const& v) {
            ret.buffer_type = MYSQL_TYPE_BLOB;
            ret.buffer = const_cast<std::byte*>(v.data());
            ret.buffer_length = static_cast<unsigned long>(v.size());
        },
    };
    std::visit(vis, var);
    return ret;
}

inline pfr::variant get_value(MYSQL_FIELD& fld,
                              char const* ptr,
                              unsigned long len)
{
    if (!ptr)
        return {};
    switch (fld.type) {
        case MYSQL_TYPE_NULL:
            return {};
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_LONGLONG:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_TINY:
            return from_chars<int64_t>(ptr, len);
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_DOUBLE:
        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_NEWDECIMAL:
            return from_chars<double>(ptr, len);
        case MYSQL_TYPE_BLOB:
        case MYSQL_TYPE_LONG_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_TINY_BLOB:
            if (63u == fld.charsetnr)
                return pfr::variant{
                    std::in_place_type<blob>, as_bytes(ptr), len};
            [[fallthrough]];
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VAR_STRING:
        case MYSQL_TYPE_VARCHAR:
            return pfr::variant{std::in_place_type<std::string>, ptr, len};
    }
    throw std::runtime_error{concat(fld.name, " ", fld.type)};
}

}  // namespace boat::db::libmysql

#endif  // BOAT_DB_LIBMYSQL_UTILITY_HPP
