// Andrew Naplavkov

#ifndef BOAT_DB_ODBC_GET_DATA_HPP
#define BOAT_DB_ODBC_GET_DATA_HPP

#include <boat/db/odbc/detail/utility.hpp>
#include <boat/pfr/variant.hpp>

namespace boat::db::odbc {

class reader {
    std::basic_string<SQLWCHAR> buf_;
    SQLLEN ind_;

    size_t num_chars() const { return ind_ / sizeof(SQLWCHAR); }
    size_t num_bytes() const { return buf_.size() * sizeof(SQLWCHAR); }

    bool get_data(handle_stmt const& hdl,
                  SQLUSMALLINT col,
                  SQLSMALLINT type,
                  SQLPOINTER ptr,
                  SQLLEN len)
    {
        check(hdl, SQLGetData(hdl.get(), col, type, ptr, len, &ind_));
        if (ind_ == SQL_NO_TOTAL)
            throw std::runtime_error{concat(name(hdl, col), " SQL_NO_TOTAL")};
        return SQL_NULL_DATA != ind_;
    }

    template <arithmetic T, SQLSMALLINT type>
    pfr::variant get(handle_stmt const& hdl, SQLUSMALLINT col)
    {
        T v;
        return get_data(hdl, col, type, &v, sizeof v) ? pfr::variant{v}
                                                      : pfr::variant{};
    }

    pfr::variant get_text(handle_stmt const& hdl, SQLUSMALLINT col)
    {
        if (!get_data(hdl, col, SQL_C_WCHAR, buf_.data(), 0))
            return {};
        buf_.resize(std::max<>(buf_.size(), num_chars() + 1));
        get_data(hdl, col, SQL_C_WCHAR, buf_.data(), num_bytes());
        return std::span{buf_.data(), num_chars()} | unicode::string<char>;
    }

    pfr::variant get_blob(handle_stmt const& hdl, SQLUSMALLINT col)
    {
        if (!get_data(hdl, col, SQL_C_BINARY, buf_.data(), 0))
            return {};
        buf_.resize(std::max<>(buf_.size(), num_chars() + 1));
        get_data(hdl, col, SQL_C_BINARY, buf_.data(), num_bytes());
        return pfr::variant{
            std::in_place_type<blob>, as_bytes(buf_.data()), ind_};
    }

public:
    pfr::variant get_data(handle_stmt const& hdl, SQLUSMALLINT col)
    {
        SQLLEN type;
        check(hdl,
              SQLColAttribute(hdl.get(), col, SQL_DESC_TYPE, 0, 0, 0, &type));
        switch (type) {
            case SQL_BIGINT:
            case SQL_BIT:
            case SQL_INTEGER:
            case SQL_SMALLINT:
            case SQL_TINYINT:
                return get<int64_t, SQL_C_SBIGINT>(hdl, col);
            case SQL_DECIMAL:
            case SQL_DOUBLE:
            case SQL_FLOAT:
            case SQL_NUMERIC:
            case SQL_REAL:
                return get<double, SQL_C_DOUBLE>(hdl, col);
            case SQL_CHAR:
            case SQL_LONGVARCHAR:
            case SQL_VARCHAR:
            case SQL_WCHAR:
            case SQL_WLONGVARCHAR:
            case SQL_WVARCHAR:
                return get_text(hdl, col);
            case SQL_BINARY:
            case SQL_LONGVARBINARY:
            case SQL_VARBINARY:
                return get_blob(hdl, col);
        }
        throw std::runtime_error{concat(name(hdl, col), " ", type)};
    }
};

}  // namespace boat::db::odbc

#endif  // BOAT_DB_ODBC_GET_DATA_HPP
