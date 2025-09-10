// Andrew Naplavkov

#ifndef BOAT_DB_LIBPQ_UTILITY_HPP
#define BOAT_DB_LIBPQ_UTILITY_HPP

#include <libpq-fe.h>
#include <stdexcept>

namespace boat::db::libpq {

// https://github.com/postgres/postgres/blob/master/src/include/catalog/pg_type.dat
constexpr Oid bool_oid = 16;
constexpr Oid bytea_oid = 17;  //< variable-length, binary values escaped
constexpr Oid name_oid = 19;
constexpr Oid int8_oid = 20;
constexpr Oid int2_oid = 21;
constexpr Oid int4_oid = 23;
constexpr Oid text_oid = 25;  //< variable-length, no limit specified
constexpr Oid float4_oid = 700;
constexpr Oid float8_oid = 701;
constexpr Oid bpchar_oid = 1042;  //< 'char(length)' fixed storage, blank-padded
constexpr Oid varchar_oid = 1043;  //< 'varchar(length)', variable storage
constexpr Oid numeric_oid = 1700;

constexpr int text_fmt = 0;
constexpr int binary_fmt = 1;

inline void check(bool success, auto& dbc)
    requires requires { PQerrorMessage(dbc.get()); }
{
    if (!success)
        throw std::runtime_error(dbc ? PQerrorMessage(dbc.get()) : "libpq");
}

}  // namespace boat::db::libpq

#endif  // BOAT_DB_LIBPQ_UTILITY_HPP
