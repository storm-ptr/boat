// Andrew Naplavkov

#ifndef BOAT_DB_ODBC_PARAMS_HPP
#define BOAT_DB_ODBC_PARAMS_HPP

#include <boat/db/odbc/detail/utility.hpp>
#include <boat/pfr/variant.hpp>

namespace boat::db::odbc::params {

struct param {
    virtual ~param() = default;
    virtual SQLSMALLINT c_type() = 0;
    virtual SQLSMALLINT sql_type() = 0;
    virtual SQLPOINTER value() = 0;
    virtual SQLULEN length() = 0;
    virtual SQLLEN* indicator() = 0;
};

template <arithmetic T, SQLSMALLINT c_type_, SQLSMALLINT sql_type_>
class scalar : public param {
    T val_;

public:
    explicit scalar(T v) : val_(v) {}
    SQLSMALLINT c_type() override { return c_type_; }
    SQLSMALLINT sql_type() override { return sql_type_; }
    SQLPOINTER value() override { return &val_; }
    SQLULEN length() override { return std::numeric_limits<T>::digits10; }
    SQLLEN* indicator() override { return 0; }
};

class text : public param {
    std::basic_string<SQLWCHAR> str_;
    SQLLEN ind_;

public:
    explicit text(std::string_view v)
        : str_(v | unicode::string<SQLWCHAR>)
        , ind_(str_.size() * sizeof(SQLWCHAR))
    {
    }

    SQLSMALLINT c_type() override { return SQL_C_WCHAR; }
    SQLSMALLINT sql_type() override { return SQL_WVARCHAR; }
    SQLPOINTER value() override { return str_.data(); }
    SQLULEN length() override { return std::max<SQLULEN>(ind_, 1); }
    SQLLEN* indicator() override { return &ind_; }
};

class binary : public param {
    std::byte const* ptr_;
    SQLLEN ind_;

public:
    explicit binary(blob_view v) : ptr_(v.data()), ind_(v.size()) {}
    SQLSMALLINT c_type() override { return SQL_C_BINARY; }
    SQLSMALLINT sql_type() override { return SQL_VARBINARY; }
    SQLPOINTER value() override { return SQLPOINTER(ptr_); }
    SQLULEN length() override { return std::max<SQLULEN>(ind_, 1); }
    SQLLEN* indicator() override { return &ind_; }
};

inline std::unique_ptr<param> create(pfr::variant const& var)
{
    using integer = scalar<int64_t, SQL_C_SBIGINT, SQL_BIGINT>;
    using real = scalar<double, SQL_C_DOUBLE, SQL_DOUBLE>;
    auto ret = std::unique_ptr<param>{};
    auto vis = overloaded{
        [](pfr::null) { throw std::runtime_error{"null param"}; },
        [&](int64_t v) { ret = std::make_unique<integer>(v); },
        [&](double v) { ret = std::make_unique<real>(v); },
        [&](std::string_view v) { ret = std::make_unique<text>(v); },
        [&](blob_view v) { ret = std::make_unique<binary>(v); },
    };
    std::visit(vis, var);
    return ret;
}

}  // namespace boat::db::odbc::params

#endif  // BOAT_DB_ODBC_PARAMS_HPP
