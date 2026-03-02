// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_REAL_HPP
#define BOAT_SQL_ADAPTORS_REAL_HPP

#include <boat/sql/detail/adaptors/adaptor.hpp>
#include <boat/sql/detail/utility.hpp>

namespace boat::sql::adaptors {

struct real : impl<float> {
    std::string_view parse() const override
    {
        if (any({"decfloat",
                 "decimal",
                 "double precision",
                 "float",
                 "numeric",
                 "real"},
                same(type())) ||
            is_mysql(dbms_) && type() == "double" ||
            is_sqlite(dbms_) && any({"real", "floa", "doub"}, in(type())))
            return kind_;
        return {};
    }

    db::column migrate(std::string_view dbms) const override
    {
        return {.kind{kind_},
                .column_name{col_->column_name},
                .type_name{is_mssql(dbms) ? "float" : "double precision"}};
    }
};

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_REAL_HPP
