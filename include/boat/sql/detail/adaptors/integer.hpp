// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_INTEGER_HPP
#define BOAT_SQL_ADAPTORS_INTEGER_HPP

#include <boat/sql/detail/adaptors/adaptor.hpp>
#include <boat/sql/detail/utility.hpp>

namespace boat::sql::adaptors {

struct integer : impl<int> {
    std::string_view parse() const override
    {
        if (any({"bigint", "boolean", "int", "integer", "smallint"},
                same(type())) ||
            is_mssql(dbms_) && any({"bit", "tinyint"}, same(type())) ||
            is_mysql(dbms_) && any({"mediumint", "tinyint"}, same(type())) ||
            is_postgres(dbms_) && type().contains("serial"))
            return kind_;
        return {};
    }

    db::column migrate(std::string_view) const override
    {
        return {.kind{kind_},
                .column_name{col_->column_name},
                .type_name{"bigint"}};
    }
};

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_INTEGER_HPP
