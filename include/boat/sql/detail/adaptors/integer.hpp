// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_INTEGER_HPP
#define BOAT_SQL_ADAPTORS_INTEGER_HPP

#include <boat/sql/detail/adaptors/adaptor.hpp>
#include <boat/sql/detail/utility.hpp>

namespace boat::sql::adaptors {

template <std::integral T>
struct type_name<T> {
    static constexpr auto value = "bigint";
};

class integer : public adaptor {
    std::string_view col_;

public:
    bool init(std::string_view dbms, column const& col) override
    {
        col_ = col.column_name;
        return any({"bigint", "boolean", "int", "integer", "smallint"},
                   equal(col.lcase_type)) ||
               (dbms.contains(mssql_dbms) &&
                any({"bit", "tinyint"}, equal(col.lcase_type))) ||
               (dbms.contains(mysql_dbms) &&
                any({"mediumint", "tinyint"}, equal(col.lcase_type))) ||
               (dbms.contains(postgresql_dbms) &&
                any({"bigserial", "smallserial", "serial"},
                    equal(col.lcase_type)));
    }

    type type_cast(std::string_view) const override { return {"bigint"}; }

    void select(db::query& qry) const override { qry << db::id{col_}; }

    void insert(db::query& qry, pfr::variant var) const override
    {
        qry << std::move(var);
    }
};

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_INTEGER_HPP
