// Andrew Naplavkov

#ifndef BOAT_SQL_LIBPQ_COMMAND_HPP
#define BOAT_SQL_LIBPQ_COMMAND_HPP

#include <boat/db/command.hpp>
#include <boat/sql/libpq/detail/get_value.hpp>
#include <boat/sql/libpq/detail/params.hpp>

namespace boat::sql::libpq {

class command : public db::command {
public:
    explicit command(char const* connection) : dbc_(PQconnectdb(connection))
    {
        check(dbc_ && PQstatus(dbc_.get()) == CONNECTION_OK, dbc_);
    }

    db::rowset exec(db::query const& qry) override
    {
        auto txt = qry.text(id_quote(), param_mark());
        auto ps = std::vector<params::param>{};
        ps.reserve(std::ranges::distance(qry.params()));
        for (auto var : qry.params())
            ps.push_back(params::make(var));
        if (ps.empty()) {
            auto res =
                unique_ptr<PGresult, PQclear>{PQexec(dbc_.get(), txt.data())};
            return fetch(res.get());
        }
        if (prepared_ != txt) {
            auto types = std::vector<Oid>(ps.size());
            for (size_t i{}; i < ps.size(); ++i)
                types[i] = params::type(ps[i]);
            auto res = unique_ptr<PGresult, PQclear>{PQprepare(  //
                dbc_.get(),
                "",
                txt.data(),
                static_cast<int>(types.size()),
                types.data())};
            auto ec = PQresultStatus(res.get());
            check(ec == PGRES_COMMAND_OK || ec == PGRES_TUPLES_OK, dbc_);
            prepared_ = txt;
        }
        auto values = std::vector<char const*>(ps.size());
        auto lengths = std::vector<int>(ps.size());
        auto formats = std::vector<int>(ps.size());
        for (size_t i{}; i < ps.size(); ++i) {
            auto& p = ps[i];
            values[i] = params::value(p);
            lengths[i] = params::length(p);
            formats[i] = params::format(p);
        }
        auto res = unique_ptr<PGresult, PQclear>{PQexecPrepared(  //
            dbc_.get(),
            "",
            int(ps.size()),
            values.data(),
            lengths.data(),
            formats.data(),
            text_fmt)};
        return fetch(res.get());
    }

    void set_autocommit(bool on) override { exec(on ? "rollback;" : "begin;"); }
    void commit() override { exec("commit;begin;"); }
    char id_quote() override { return '"'; }
    std::string param_mark() override { return "${}"; }
    std::string dbms() override { return "postgres"; }

private:
    unique_ptr<PGconn, PQfinish> dbc_;
    std::string prepared_;

    db::rowset fetch(PGresult* res)
    {
        auto ec = PQresultStatus(res);
        check(ec == PGRES_COMMAND_OK || ec == PGRES_TUPLES_OK, dbc_);
        int cols = PQnfields(res);
        int rows = PQntuples(res);
        auto ret = db::rowset{.columns{static_cast<size_t>(cols)},
                              .rows{static_cast<size_t>(rows)}};
        for (int col{}; col < cols; ++col)
            ret.columns[col] = PQfname(res, col);
        for (int row{}; row < rows; ++row) {
            ret.rows[row].resize(cols);
            for (int col{}; col < cols; ++col)
                ret.rows[row][col] = get_value(res, row, col);
        }
        return ret;
    }
};

}  // namespace boat::sql::libpq

#endif  // BOAT_SQL_LIBPQ_COMMAND_HPP
