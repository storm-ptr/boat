// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_SPATIAL_HPP
#define BOAT_SQL_ADAPTORS_SPATIAL_HPP

#include <boat/geometry/concepts.hpp>
#include <boat/sql/detail/adaptors/adaptor.hpp>
#include <boat/sql/detail/utility.hpp>

namespace boat::sql::adaptors {

template <geometry::ogc99 T>
struct type_name<T> {
    static constexpr auto value = "geometry";
};

class spatial : public adaptor {
protected:
    std::string_view dbms_;
    column const* col_;

public:
    bool init(std::string_view dbms, column const& col) override
    {
        dbms_ = dbms;
        col_ = &col;
        return geo(col);
    }

    type to_type(std::string_view dbms) const override
    {
        return dbms.contains(mssql_dbms) ? type{"geography", -1, 4326}
                                         : type{"geometry", 0, col_->epsg};
    }

    void select(db::query& qry) const override
    {
        if (dbms_.contains(mssql_dbms))
            qry << db::id{col_->column_name} << ".STAsBinary() "
                << db::id{col_->column_name};
        else if (dbms_.contains(mysql_dbms))
            qry << "ST_AsBinary(" << db::id{col_->column_name}
                << ", 'axis-order=long-lat') " << db::id{col_->column_name};
        else
            qry << "ST_AsBinary(" << db::id{col_->column_name} << ") "
                << db::id{col_->column_name};
    }

    void insert(db::query& qry, pfr::variant var) const override
    {
        if (dbms_.contains(mssql_dbms))
            qry << col_->lcase_type << "::STGeomFromWKB(" << std::move(var)
                << ", " << to_chars(col_->srid) << ")";
        else if (dbms_.contains(mysql_dbms))
            qry << "ST_GeomFromWKB(" << std::move(var) << ", "
                << to_chars(col_->srid) << ", 'axis-order=long-lat')";
        else if (dbms_.contains(postgresql_dbms) &&
                 col_->lcase_type == "geography")
            qry << "ST_GeogFromWKB(" << std::move(var) << ")";
        else
            qry << "ST_GeomFromWKB(" << std::move(var) << ", "
                << to_chars(col_->srid) << ")";
    }
};

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_SPATIAL_HPP
