// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_SPATIAL_HPP
#define BOAT_SQL_ADAPTORS_SPATIAL_HPP

#include <boat/sql/detail/adaptors/adaptor.hpp>
#include <boat/sql/detail/utility.hpp>

namespace boat::sql::adaptors {

struct spatial : impl<geometry::geographic::variant> {
    std::string_view parse() const override { return geo(*col_) ? kind_ : ""; }

    db::column migrate(std::string_view dbms) const override
    {
        return {.kind{kind_},
                .column_name{col_->column_name},
                .type_name{is_mssql(dbms) ? "geography" : "geometry"},
                .epsg = is_mssql(dbms) ? 4326 : col_->epsg};
    }

    void select(db::query& qry) const override
    {
        auto id = db::id{col_->column_name};
        if (is_mssql(dbms_))
            qry << id << ".STAsBinary() " << id;
        else if (is_mysql(dbms_))
            qry << "ST_AsBinary(" << id << ", 'axis-order=long-lat') " << id;
        else
            qry << "ST_AsBinary(" << id << ") " << id;
    }

    void insert(db::query& qry, db::variant var) const override
    {
        auto srid = to_chars(col_->srid);
        if (is_mssql(dbms_))
            qry << type() << "::STGeomFromWKB(" << std::move(var) << ", "
                << srid << ")";
        else if (is_mysql(dbms_))
            qry << "ST_GeomFromWKB(" << std::move(var) << ", " << srid
                << ", 'axis-order=long-lat')";
        else if (is_postgres(dbms_) && type() == "geography")
            qry << "ST_GeogFromWKB(" << std::move(var) << ")";
        else
            qry << "ST_GeomFromWKB(" << std::move(var) << ", " << srid << ")";
    }
};

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_SPATIAL_HPP
