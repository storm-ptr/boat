// Andrew Naplavkov

#ifndef BOAT_DB_COMMAND_HPP
#define BOAT_DB_COMMAND_HPP

#include <boat/db/query.hpp>
#include <boat/db/rowset.hpp>
#include <stop_token>

namespace boat::db {

struct command {
    virtual ~command() = default;
    virtual rowset exec(query const&, std::stop_token = {}) = 0;
    virtual void set_autocommit(bool) = 0;
    virtual void commit() = 0;
    virtual char id_quote() = 0;

    /// "{}" in the result is replaced with the parameter number.
    virtual std::string param_mark() = 0;

    /// The result is lower-case.
    virtual std::string dbms() = 0;
};

}  // namespace boat::db

#endif  // BOAT_DB_COMMAND_HPP
