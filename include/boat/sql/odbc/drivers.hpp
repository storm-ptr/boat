// Andrew Naplavkov

#ifndef BOAT_SQL_ODBC_DRIVERS_HPP
#define BOAT_SQL_ODBC_DRIVERS_HPP

#include <boat/detail/unicode.hpp>
#if __has_include(<sql.h>)
#include <boat/sql/odbc/detail/utility.hpp>
#endif
#include <generator>

namespace boat::sql::odbc {

inline std::generator<std::string> drivers()
{
#if __has_include(<sql.h>)
    auto env = env_ptr{};
    env = alloc<SQL_HANDLE_ENV>(env);
    check(SQLSetEnvAttr(
              env.get(), SQL_ATTR_ODBC_VERSION, SQLPOINTER(SQL_OV_ODBC3), 0),
          env);
    std::array<SQLWCHAR, SQL_MAX_MESSAGE_LENGTH> desc, attr;
    SQLSMALLINT desc_len, attr_len;
    while (SQL_SUCCEEDED(SQLDriversW(  //
        env.get(),
        SQL_FETCH_NEXT,
        desc.data(),
        SQL_MAX_MESSAGE_LENGTH,
        &desc_len,
        attr.data(),
        SQL_MAX_MESSAGE_LENGTH,
        &attr_len)))
        co_yield std::span(desc.data(), desc_len) | unicode::utf8;

#else
    co_return;
#endif
}

}  // namespace boat::sql::odbc

#endif  // BOAT_SQL_ODBC_DRIVERS_HPP
