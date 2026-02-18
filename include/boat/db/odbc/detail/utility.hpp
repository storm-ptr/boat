// Andrew Naplavkov

#ifndef BOAT_DB_ODBC_UTILITY_HPP
#define BOAT_DB_ODBC_UTILITY_HPP

#if __has_include(<windows.h>)
#include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <array>
#include <boat/detail/unicode.hpp>

namespace boat::db::odbc {

template <SQLSMALLINT type>
struct deleter {
    void operator()(SQLHANDLE h) const
    {
        if constexpr (SQL_HANDLE_DBC == type)
            SQLDisconnect(h);
        if constexpr (SQL_HANDLE_STMT == type)
            SQLFreeStmt(h, SQL_CLOSE);
        SQLFreeHandle(type, h);
    }
};

template <SQLSMALLINT type>
using unique_ptr = std::unique_ptr<void, deleter<type>>;

using env_ptr = unique_ptr<SQL_HANDLE_ENV>;
using dbc_ptr = unique_ptr<SQL_HANDLE_DBC>;
using stmt_ptr = unique_ptr<SQL_HANDLE_STMT>;

template <SQLSMALLINT type>
void check(SQLRETURN ec, unique_ptr<type> const& ptr)
{
    auto os = std::basic_ostringstream<SQLWCHAR>{};
    if (ptr && (SQL_ERROR == ec || SQL_SUCCESS_WITH_INFO == ec)) {
        auto row = SQLSMALLINT{};
        auto state = std::array<SQLWCHAR, 6>{};
        auto code = SQLINTEGER{};
        auto buf = std::array<SQLWCHAR, SQL_MAX_MESSAGE_LENGTH>{};
        while (SQL_SUCCEEDED(SQLGetDiagRecW(  //
            type,
            ptr.get(),
            ++row,
            state.data(),
            &code,
            buf.data(),
            SQLSMALLINT(buf.size()),
            0)))
            os << std::basic_string_view{buf.data()};
    }
    if (SQL_SUCCEEDED(ec))
        return;
    auto msg = os.view() | unicode::utf8;
    throw std::runtime_error{msg.empty() ? "odbc" : msg};
}

template <SQLSMALLINT type_out, SQLSMALLINT type_in>
auto alloc(unique_ptr<type_in> const& ptr)
{
    SQLHANDLE out;
    check(SQLAllocHandle(type_out, ptr.get(), &out), ptr);
    return unique_ptr<type_out>{out};
}

inline std::string info(dbc_ptr const& dbc, SQLUSMALLINT key)
{
    auto buf = std::array<SQLWCHAR, SQL_MAX_MESSAGE_LENGTH>{};
    auto sz = SQLSMALLINT(buf.size());
    check(SQLGetInfoW(dbc.get(), key, buf.data(), sz, 0), dbc);
    return std::basic_string_view{buf.data()} | unicode::utf8;
}

inline std::string name(stmt_ptr const& stmt, SQLUSMALLINT col)
{
    auto key = SQL_DESC_NAME;
    auto buf = std::array<SQLWCHAR, SQL_MAX_MESSAGE_LENGTH>{};
    auto sz = SQLSMALLINT(buf.size());
    check(SQLColAttributeW(stmt.get(), col, key, buf.data(), sz, 0, 0), stmt);
    return std::basic_string_view{buf.data()} | unicode::utf8;
}

}  // namespace boat::db::odbc

#endif  // BOAT_DB_ODBC_UTILITY_HPP
