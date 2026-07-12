// Andrew Naplavkov

#ifndef BOAT_SQL_ODBC_PARAMS_HPP
#define BOAT_SQL_ODBC_PARAMS_HPP

#include <boat/db/adapted/adapted.hpp>
#include <boat/sql/odbc/detail/utility.hpp>
#include <variant>

namespace boat::sql::odbc::params {

template <arithmetic T, SQLSMALLINT c_type_, SQLSMALLINT sql_type_>
struct scalar {
    T val_;

    SQLSMALLINT c_type() const { return c_type_; }
    SQLSMALLINT sql_type() const { return sql_type_; }
    SQLPOINTER value() { return &val_; }
    SQLULEN length() const { return std::numeric_limits<T>::digits10; }
    SQLLEN* indicator() { return 0; }
};

struct text {
    std::basic_string<SQLWCHAR> str_;
    SQLLEN ind_;

    explicit text(std::string_view v)
        : str_(v | unicode::utf<SQLWCHAR>), ind_(str_.size() * sizeof(SQLWCHAR))
    {
    }

    SQLSMALLINT c_type() const { return SQL_C_WCHAR; }
    SQLSMALLINT sql_type() const { return SQL_WVARCHAR; }
    SQLPOINTER value() { return str_.data(); }
    SQLULEN length() const { return ind_; }
    SQLLEN* indicator() { return &ind_; }
};

struct binary {
    std::byte const* ptr_;
    SQLLEN ind_;

    explicit binary(blob_view v) : ptr_(v.data()), ind_(v.size()) {}
    SQLSMALLINT c_type() const { return SQL_C_BINARY; }
    SQLSMALLINT sql_type() const { return SQL_VARBINARY; }
    SQLPOINTER value() { return SQLPOINTER(ptr_); }
    SQLULEN length() const { return ind_; }
    SQLLEN* indicator() { return &ind_; }
};

using integer = scalar<int64_t, SQL_C_SBIGINT, SQL_BIGINT>;
using real = scalar<double, SQL_C_DOUBLE, SQL_DOUBLE>;
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

inline void bind(stmt_ptr const& stmt, SQLUSMALLINT i, param& p)
{
    std::visit(
        [&stmt, i](auto& v) {
            check(SQLBindParameter(  //
                      stmt.get(),
                      i,
                      SQL_PARAM_INPUT,
                      v.c_type(),
                      v.sql_type(),
                      v.length(),
                      0,
                      v.value(),
                      0,
                      v.indicator()),
                  stmt);
        },
        p);
}

}  // namespace boat::sql::odbc::params

#endif  // BOAT_SQL_ODBC_PARAMS_HPP
