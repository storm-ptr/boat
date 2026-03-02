// Andrew Naplavkov

#ifndef BOAT_DB_AGENT_HPP
#define BOAT_DB_AGENT_HPP

#include <boat/db/meta.hpp>
#include <boat/db/rowset.hpp>

namespace boat::db {

struct agent {
    virtual ~agent() = default;

    virtual std::vector<layer> get_layers() = 0;

    virtual table describe(std::string_view schema_name,
                           std::string_view table_name) = 0;

    virtual rowset select(table const&, page const&) = 0;

    virtual rowset select(table const&, bbox const&) = 0;

    virtual void insert(table const&, rowset const&) = 0;

    virtual table create(table const&) = 0;

    virtual void drop(std::string_view schema_name,
                      std::string_view table_name) = 0;
};

}  // namespace boat::db

#endif  // BOAT_DB_AGENT_HPP
