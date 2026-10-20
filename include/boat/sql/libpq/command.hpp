// Andrew Naplavkov

#ifndef BOAT_SQL_LIBPQ_COMMAND_HPP
#define BOAT_SQL_LIBPQ_COMMAND_HPP

#include <boat/db/command.hpp>
#include <boat/sql/libpq/detail/fetch.hpp>
#include <boat/sql/libpq/detail/params.hpp>

namespace boat::sql::libpq {

class command : public db::command {
    unique_ptr<PGconn, PQfinish> dbc_;

public:
    explicit command(char const* connection) : dbc_(PQconnectdb(connection))
    {
        check(dbc_ && PQstatus(dbc_.get()) == CONNECTION_OK, dbc_.get());
    }

    db::rowset exec(db::query const& qry, std::stop_token tok = {}) override
    {
        auto cxl = unique_ptr<PGcancel, PQfreeCancel>{PQgetCancel(dbc_.get())};
        check(!!cxl, dbc_.get());
        char err[256]{};
        auto stop = std::stop_callback{
            tok, [&] { PQcancel(cxl.get(), err, sizeof(err)); }};
        auto txt = qry.text(id_quote(), param_mark());
        auto ps = qry.params() | std::views::transform(params::make) |
                  std::ranges::to<std::vector>();
        if (ps.empty()) {
            auto res =
                unique_ptr<PGresult, PQclear>{PQexec(dbc_.get(), txt.data())};
            return fetch(res.get());
        }
        auto types = std::vector<Oid>(ps.size());
        auto values = std::vector<char const*>(ps.size());
        auto lengths = std::vector<int>(ps.size());
        auto formats = std::vector<int>(ps.size());
        for (size_t i{}; i < ps.size(); ++i) {
            auto& p = ps[i];
            types[i] = params::type(p);
            values[i] = params::value(p);
            lengths[i] = params::length(p);
            formats[i] = params::format(p);
        }
        auto res = unique_ptr<PGresult, PQclear>{PQexecParams(  //
            dbc_.get(),
            txt.data(),
            int(ps.size()),
            types.data(),
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
