// Andrew Naplavkov

#ifndef BOAT_DB_COMMAND_HPP
#define BOAT_DB_COMMAND_HPP

#include <boat/db/query.hpp>
#include <boat/db/rowset.hpp>

namespace boat::db {

struct command {
    virtual ~command() = default;
    virtual rowset exec(query const&) = 0;
    virtual void set_autocommit(bool) = 0;
    virtual void commit() = 0;
    virtual char id_quote() = 0;
    virtual std::string param_mark() = 0;
    virtual std::string dbms() = 0;  //< lower case
};

}  // namespace boat::db

#endif  // BOAT_DB_COMMAND_HPP
