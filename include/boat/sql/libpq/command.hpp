// Andrew Naplavkov

#ifndef BOAT_SQL_LIBPQ_COMMAND_HPP
#define BOAT_SQL_LIBPQ_COMMAND_HPP

#include <boat/db/command.hpp>
#include <boat/sql/libpq/detail/fetch.hpp>
#include <boat/sql/libpq/detail/params.hpp>

namespace boat::sql::libpq {

class command : public db::command {
    unique_ptr<PGconn, PQfinish> dbc_;
    std::string prepared_;

public:
    explicit command(char const* connection) : dbc_(PQconnectdb(connection))
    {
        check(dbc_ && PQstatus(dbc_.get()) == CONNECTION_OK, dbc_.get());
    }

    db::rowset exec(db::query const& qry) override
    {
        auto txt = qry.text(id_quote(), param_mark());
        auto ps = qry.params() | std::views::transform(params::make) |
                  std::ranges::to<std::vector>();
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
            check(ec == PGRES_COMMAND_OK || ec == PGRES_TUPLES_OK, dbc_.get());
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
};

}  // namespace boat::sql::libpq

#endif  // BOAT_SQL_LIBPQ_COMMAND_HPP
