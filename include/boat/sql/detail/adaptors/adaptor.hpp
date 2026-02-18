// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_ADAPTOR_HPP
#define BOAT_SQL_ADAPTORS_ADAPTOR_HPP

#include <boat/db/query.hpp>
#include <boat/sql/vocabulary.hpp>

namespace boat::sql::adaptors {

template <class>
struct type_name;

struct type {
    std::string name;
    int length;
    int epsg;
};

struct adaptor {
    virtual ~adaptor() = default;
    virtual bool init(std::string_view dbms, column const&) = 0;
    virtual type to_type(std::string_view dbms) const = 0;
    virtual void select(db::query&) const = 0;
    virtual void insert(db::query&, pfr::variant) const = 0;
};

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_ADAPTOR_HPP
