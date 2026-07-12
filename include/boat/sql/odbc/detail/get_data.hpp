// Andrew Naplavkov

#ifndef BOAT_SQL_ODBC_GET_DATA_HPP
#define BOAT_SQL_ODBC_GET_DATA_HPP

#include <boat/db/adapted/adapted.hpp>
#include <boat/sql/odbc/detail/utility.hpp>

namespace boat::sql::odbc {

template <arithmetic T, SQLSMALLINT type>
db::variant get(stmt_ptr const& stmt, SQLUSMALLINT col)
{
    T val;
    SQLLEN ind = 0;
    check(SQLGetData(stmt.get(), col, type, &val, sizeof val, &ind), stmt);
    boat::check(ind != SQL_NO_TOTAL, "SQL_NO_TOTAL");
    if (ind == SQL_NULL_DATA)
        return {};
    return val;
}

inline db::variant get_text(stmt_ptr const& stmt, SQLUSMALLINT col)
{
    thread_local std::vector<SQLWCHAR> buf(2048);
    std::vector<SQLWCHAR> val;
    SQLLEN num_bytes = buf.size() * sizeof(SQLWCHAR);
    SQLLEN ind = 0;
    SQLRETURN ec;
    do {
        ec = SQLGetData(
            stmt.get(), col, SQL_C_WCHAR, buf.data(), num_bytes, &ind);
        check(ec, stmt);
        if (ind == SQL_NULL_DATA)
            return {};
        if (ind == SQL_NO_TOTAL || ind >= num_bytes)
            val.append_range(std::span{buf.data(), buf.size() - 1});
        else if (ind > 0)
            val.append_range(std::span{buf.data(), ind / sizeof(SQLWCHAR)});
    } while (ec == SQL_SUCCESS_WITH_INFO);
    return std::span{val.data(), val.size()} | unicode::utf8;
}

inline db::variant get_blob(stmt_ptr const& stmt, SQLUSMALLINT col)
{
    thread_local blob buf(4096, {});
    blob val;
    SQLLEN ind = 0;
    SQLRETURN ec;
    do {
        ec = SQLGetData(
            stmt.get(), col, SQL_C_BINARY, buf.data(), buf.size(), &ind);
        check(ec, stmt);
        if (ind == SQL_NULL_DATA)
            return {};
        if (ind == SQL_NO_TOTAL || ind >= SQLLEN(buf.size()))
            val.append_range(buf);
        else if (ind > 0)
            val.append_range(std::span{buf.data(), size_t(ind)});
    } while (ec == SQL_SUCCESS_WITH_INFO);
    return val;
}

inline db::variant get_data(stmt_ptr const& stmt, SQLUSMALLINT col)
{
    SQLLEN type;
    check(SQLColAttribute(stmt.get(), col, SQL_DESC_TYPE, 0, 0, 0, &type),
          stmt);
    switch (type) {
        case SQL_BIGINT:
        case SQL_BIT:
        case SQL_INTEGER:
        case SQL_SMALLINT:
        case SQL_TINYINT:
            return get<int64_t, SQL_C_SBIGINT>(stmt, col);
        case SQL_DECIMAL:
        case SQL_DOUBLE:
        case SQL_FLOAT:
        case SQL_NUMERIC:
        case SQL_REAL:
            return get<double, SQL_C_DOUBLE>(stmt, col);
        case SQL_CHAR:
        case SQL_LONGVARCHAR:
        case SQL_VARCHAR:
        case SQL_WCHAR:
        case SQL_WLONGVARCHAR:
        case SQL_WVARCHAR:
            return get_text(stmt, col);
        case SQL_BINARY:
        case SQL_LONGVARBINARY:
        case SQL_VARBINARY:
            return get_blob(stmt, col);
    }
    throw std::runtime_error{concat(name(stmt, col), " ", type)};
}

}  // namespace boat::sql::odbc

#endif  // BOAT_SQL_ODBC_GET_DATA_HPP
