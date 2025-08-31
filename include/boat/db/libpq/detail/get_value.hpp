// Andrew Naplavkov

#ifndef BOAT_DB_LIBPQ_GET_VALUE_HPP
#define BOAT_DB_LIBPQ_GET_VALUE_HPP

#include <boat/db/libpq/detail/utility.hpp>
#include <boat/pfr/variant.hpp>

namespace boat::db::libpq {

template <arithmetic T>
T get(PGresult* res, int row, int col)
{
    T ret;
    auto first = PQgetvalue(res, row, col);
    auto last = first + PQgetlength(res, row, col);
    std::from_chars(first, last, ret);
    return ret;
}

inline pfr::variant get_blob(PGresult* res, int row, int col)
{
    auto ptr = (unsigned char const*)PQgetvalue(res, row, col);
    auto len = (size_t)PQgetlength(res, row, col);
    if (auto mem = unique_ptr<void, PQfreemem>(PQunescapeBytea(ptr, &len)))
        return pfr::variant{std::in_place_type<blob>, as_bytes(mem.get()), len};
    throw std::runtime_error{concat(PQfname(res, col), " PQunescapeBytea")};
}

inline pfr::variant get_value(PGresult* res, int row, int col)
{
    if (PQgetisnull(res, row, col))
        return {};
    if (PQfformat(res, col) != text_fmt)
        throw std::runtime_error{concat(PQfname(res, col), " PQfformat")};
    auto type = PQftype(res, col);
    switch (type) {
        case bool_oid:
            return *PQgetvalue(res, row, col) == 'f' ? 0 : 1;
        case int2_oid:
        case int4_oid:
        case int8_oid:
            return get<int64_t>(res, row, col);
        case float4_oid:
        case float8_oid:
        case numeric_oid:
            return get<double>(res, row, col);
        case bpchar_oid:
        case name_oid:
        case text_oid:
        case varchar_oid:
            return pfr::variant{std::in_place_type<std::string>,
                                PQgetvalue(res, row, col),
                                PQgetlength(res, row, col)};
        case bytea_oid:
            return get_blob(res, row, col);
    }
    throw std::runtime_error{concat(PQfname(res, col), " ", type)};
}

}  // namespace boat::db::libpq

#endif  // BOAT_DB_LIBPQ_GET_VALUE_HPP
