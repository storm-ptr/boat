// Andrew Naplavkov

#ifndef BOAT_DB_META_HPP
#define BOAT_DB_META_HPP

#include <boat/geometry/vocabulary.hpp>
#include <cstdint>
#include <ranges>
#include <string>
#include <vector>

namespace boat::db {

struct source {
    std::string source_name;
    std::string address;
};

struct layer {
    std::string schema_name;
    std::string table_name;
    std::string column_name;
    bool raster;

    friend bool operator==(layer const&, layer const&) = default;
};

struct column {
    std::string kind;
    std::string column_name;
    std::string type_name;  ///< Lowercase.
    int length;
    int srid;
    int epsg;
    std::string wkt;
    std::string proj4;
};

inline bool has_coord_sys(column const& col)
{
    return col.epsg > 0 || !col.wkt.empty() || !col.proj4.empty();
}

struct index_key {
    std::string schema_name;
    std::string index_name;
    std::string column_name;
    bool descending;
    bool partial;
    bool primary;
    bool unique;
    int ordinal;  ///< One-based.
};

struct table {
    std::string dbms;  ///< Lowercase.
    std::string schema_name;
    std::string table_name;
    std::vector<column> columns;
    std::vector<index_key> index_keys;
};

inline auto indices(table const& tbl)
{
    return tbl.index_keys | std::views::chunk_by([](auto& lhs, auto& rhs) {
               return lhs.ordinal < rhs.ordinal;
           });
}

struct bbox {
    std::vector<std::string> select_list;
    std::string layer_column;
    geometry::cartesian::box bounds;  //< Layer column's SRS.
    int limit;
};

struct order_key {
    std::string column_name;
    bool descending;
};

struct page {
    std::vector<std::string> select_list;
    std::vector<order_key> order_by;
    size_t offset;
    int limit;
};

struct band {
    std::string color_name;  ///< Lowercase.
    std::string type_name;   ///< Lowercase.
};

struct raster {
    std::string schema_name;
    std::string table_name;
    std::string column_name;
    std::vector<band> bands;
    geometry::raster::point extent;
    double xorig;
    double yorig;
    double xscale;
    double yscale;
    double xskew;
    double yskew;
    int srid;
    int epsg;
    std::string wkt;
    std::string proj4;
};

/// Maps pixels to spatial coordinates.
inline geometry::matrix affine(raster const& r)
{
    return {
        {{r.xscale, r.xskew, r.xorig},
         {r.yskew, r.yscale, r.yorig},
         {0., 0., 1.}}};
}

}  // namespace boat::db

#endif  // BOAT_DB_META_HPP
