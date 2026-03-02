// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_BINARY_HPP
#define BOAT_SQL_ADAPTORS_BINARY_HPP

#include <boat/sql/detail/adaptors/adaptor.hpp>
#include <boat/sql/detail/utility.hpp>

namespace boat::sql::adaptors {

struct binary : impl<blob> {
    std::string_view parse() const override
    {
        if (any({"binary",
                 "binary large object",
                 "binary varying",
                 "blob",
                 "varbinary"},
                same(type())) ||
            is_mysql(dbms_) && type().contains("blob") ||
            is_postgres(dbms_) && type() == "bytea" ||
            is_sqlite(dbms_) && type().contains("blob"))
            return kind_;
        return {};
    }

    db::column migrate(std::string_view dbms) const override
    {
        return {.kind{kind_},
                .column_name{col_->column_name},
                .type_name{is_mssql(dbms)      ? "varbinary"
                           : is_postgres(dbms) ? "bytea"
                                               : "blob"},
                .length = is_mssql(dbms) ? -1 : 0};
    }
};

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_BINARY_HPP
