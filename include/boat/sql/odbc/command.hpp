// Andrew Naplavkov

#ifndef BOAT_SQL_ODBC_COMMAND_HPP
#define BOAT_SQL_ODBC_COMMAND_HPP

#include <boat/db/command.hpp>
#include <boat/detail/config.hpp>
#include <boat/sql/odbc/detail/get_data.hpp>
#include <boat/sql/odbc/detail/params.hpp>

namespace boat::sql::odbc {

class command : public db::command {
    env_ptr env_;
    dbc_ptr dbc_;
    stmt_ptr stmt_;
    char id_quote_;
    std::string dbms_;
    std::basic_string<SQLWCHAR> sql_;

public:
    explicit command(std::string_view connection)
    {
        env_ = alloc<SQL_HANDLE_ENV>(env_);
        check(
            SQLSetEnvAttr(
                env_.get(), SQL_ATTR_ODBC_VERSION, SQLPOINTER(SQL_OV_ODBC3), 0),
            env_);
        dbc_ = alloc<SQL_HANDLE_DBC>(env_);
        check(SQLSetConnectAttr(  //
                  dbc_.get(),
                  SQL_ATTR_LOGIN_TIMEOUT,
                  SQLPOINTER(std::chrono::seconds{timeout}.count()),
                  0),
              dbc_);
        SQLSMALLINT len;
        check(SQLDriverConnectW(  //
                  dbc_.get(),
                  0,
                  unicode::utf<SQLWCHAR>(connection).data(),
                  SQL_NTS,
                  0,
                  0,
                  &len,
                  SQL_DRIVER_NOPROMPT),
              dbc_);
        stmt_ = alloc<SQL_HANDLE_STMT>(dbc_);
        id_quote_ = info(dbc_, SQL_IDENTIFIER_QUOTE_CHAR).at(0);
        dbms_ = to_lower(info(dbc_, SQL_DBMS_NAME));
    }

    db::rowset exec(db::query const& qry) override
    {
        check(SQLFreeStmt(stmt_.get(), SQL_RESET_PARAMS), stmt_);
        auto ret = db::rowset{};
        auto txt = qry.text(id_quote_, param_mark()) | unicode::utf<SQLWCHAR>;
        if (txt != sql_) {
            check(SQLPrepareW(stmt_.get(), txt.data(), SQL_NTS), stmt_);
            sql_ = std::move(txt);
        }
        auto ps = std::vector<params::param>{};
        ps.reserve(std::ranges::distance(qry.params()));
        for (auto var : qry.params())
            ps.push_back(params::make(var));
        for (size_t i{}; i < ps.size(); ++i)
            params::bind(stmt_, SQLUSMALLINT(i + 1), ps[i]);
        check(SQLSetStmtAttr(  //
                  stmt_.get(),
                  SQL_ATTR_QUERY_TIMEOUT,
                  SQLPOINTER(std::chrono::seconds{timeout}.count()),
                  0),
              stmt_);
        auto ec = SQLExecute(stmt_.get());
        for (; SQL_NO_DATA != ec; ec = SQLMoreResults(stmt_.get())) {
            check(ec, stmt_);
            ret = {};
            SQLSMALLINT num_cols;
            check(SQLNumResultCols(stmt_.get(), &num_cols), stmt_);
            if (num_cols) {
                for (int i{}; i < num_cols; ++i)
                    ret.columns.push_back(name(stmt_, i + 1));
                ec = SQLFetch(stmt_.get());
                for (; SQL_NO_DATA != ec; ec = SQLFetch(stmt_.get())) {
                    check(ec, stmt_);
                    auto& row = ret.rows.emplace_back(num_cols);
                    for (int i{}; i < num_cols; ++i)
                        row[i] = get_data(stmt_, i + 1);
                }
            }
        }
        return ret;
    }

    void set_autocommit(bool on) override
    {
        if (on)
            check(SQLEndTran(SQL_HANDLE_DBC, dbc_.get(), SQL_ROLLBACK), dbc_);
        auto val = SQLPOINTER(on ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF);
        check(SQLSetConnectAttr(dbc_.get(), SQL_ATTR_AUTOCOMMIT, val, 0), dbc_);
    }

    void commit() override
    {
        check(SQLEndTran(SQL_HANDLE_DBC, dbc_.get(), SQL_COMMIT), dbc_);
    }

    char id_quote() override { return id_quote_; }
    std::string param_mark() override { return "?"; }
    std::string dbms() override { return dbms_; }
};

}  // namespace boat::sql::odbc

#endif  // BOAT_SQL_ODBC_COMMAND_HPP
