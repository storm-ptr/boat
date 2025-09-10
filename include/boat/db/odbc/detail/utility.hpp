// Andrew Naplavkov

#ifndef BOAT_DB_ODBC_UTILITY_HPP
#define BOAT_DB_ODBC_UTILITY_HPP

#if __has_include(<windows.h>)
#include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <boat/detail/unicode.hpp>

namespace boat::db::odbc {

template <SQLSMALLINT type>
struct deleter {
    void operator()(SQLHANDLE hdl) const
    {
        if constexpr (SQL_HANDLE_DBC == type)
            SQLDisconnect(hdl);
        if constexpr (SQL_HANDLE_STMT == type)
            SQLFreeStmt(hdl, SQL_CLOSE);
        SQLFreeHandle(type, hdl);
    }
};

template <SQLSMALLINT type>
using handle = std::unique_ptr<void, deleter<type>>;

using handle_env = handle<SQL_HANDLE_ENV>;
using handle_dbc = handle<SQL_HANDLE_DBC>;
using handle_stmt = handle<SQL_HANDLE_STMT>;

template <SQLSMALLINT type>
void check(SQLRETURN rc, handle<type> const& hdl)
{
    auto os = std::basic_ostringstream<SQLWCHAR>{};
    if (hdl && (SQL_ERROR == rc || SQL_SUCCESS_WITH_INFO == rc)) {
        auto row = SQLSMALLINT{};
        auto state = std::array<SQLWCHAR, 6>{};
        auto code = SQLINTEGER{};
        auto buf = std::array<SQLWCHAR, SQL_MAX_MESSAGE_LENGTH>{};
        while (SQL_SUCCEEDED(SQLGetDiagRecW(type,
                                            hdl.get(),
                                            ++row,
                                            state.data(),
                                            &code,
                                            buf.data(),
                                            SQLSMALLINT(buf.size()),
                                            0)))
            os << std::basic_string_view{buf.data()};
    }
    if (SQL_SUCCEEDED(rc))
        return;
    auto msg = os.view() | unicode::string<char>;
    throw std::runtime_error{msg.empty() ? "odbc" : msg};
}

template <SQLSMALLINT type_out, SQLSMALLINT type_in>
auto alloc(handle<type_in> const& hdl)
{
    SQLHANDLE out;
    check(SQLAllocHandle(type_out, hdl.get(), &out), hdl);
    return handle<type_out>{out};
}

inline std::string info(handle_dbc const& hdl, SQLUSMALLINT key)
{
    auto buf = std::array<SQLWCHAR, SQL_MAX_MESSAGE_LENGTH>{};
    auto sz = SQLSMALLINT(buf.size());
    check(SQLGetInfoW(hdl.get(), key, buf.data(), sz, 0), hdl);
    return std::basic_string_view{buf.data()} | unicode::string<char>;
}

inline std::string name(handle_stmt const& hdl, SQLUSMALLINT col)
{
    auto key = SQL_DESC_NAME;
    auto buf = std::array<SQLWCHAR, SQL_MAX_MESSAGE_LENGTH>{};
    auto sz = SQLSMALLINT(buf.size());
    check(SQLColAttributeW(hdl.get(), col, key, buf.data(), sz, 0, 0), hdl);
    return std::basic_string_view{buf.data()} | unicode::string<char>;
}

}  // namespace boat::db::odbc

#endif  // BOAT_DB_ODBC_UTILITY_HPP
