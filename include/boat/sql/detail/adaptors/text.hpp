// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_TEXT_HPP
#define BOAT_SQL_ADAPTORS_TEXT_HPP

#include <boat/sql/detail/adaptors/adaptor.hpp>
#include <boat/sql/detail/utility.hpp>

namespace boat::sql::adaptors {

struct text : impl<std::string> {
    std::string_view parse() const override
    {
        if (any({"char",
                 "character",
                 "character large object",
                 "character varying",
                 "clob",
                 "national character",
                 "national character large object",
                 "national character varying",
                 "nchar",
                 "nchar varying",
                 "nclob",
                 "nvarchar",
                 "varchar"},
                same(type())) ||
            is_mysql(dbms_) && type().contains("text") ||
            is_postgres(dbms_) && any({"bpchar", "text"}, same(type())) ||
            is_sqlite(dbms_) && any({"char", "clob", "text"}, in(type())))
            return kind_;
        return {};
    }

    db::column migrate(std::string_view dbms) const override
    {
        return {.kind{kind_},
                .column_name{col_->column_name},
                .type_name{is_mssql(dbms) ? "nvarchar" : "varchar"},
                .length = 250};
    }
};

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_TEXT_HPP
