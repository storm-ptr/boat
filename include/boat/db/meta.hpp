// Andrew Naplavkov

#ifndef BOAT_DB_META_HPP
#define BOAT_DB_META_HPP

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
};

struct column {
    std::string kind;
    std::string column_name;
    std::string type_name;  //< lower case
    int length;
    int srid;
    int epsg;
};

struct index_key {
    std::string schema_name;
    std::string index_name;
    std::string column_name;
    bool descending;
    bool partial;
    bool primary;
    bool unique;
    int ordinal;
};

struct table {
    std::string dbms;  //< lower case
    std::string schema_name;
    std::string table_name;
    std::vector<column> columns;
    std::vector<index_key> index_keys;

    auto indices() const
    {
        return index_keys | std::views::chunk_by([](auto& lhs, auto& rhs) {
                   return lhs.ordinal < rhs.ordinal;
               });
    }
};

struct bbox {
    std::vector<std::string> select_list;
    std::string layer_column;
    double xmin;
    double ymin;
    double xmax;
    double ymax;
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
    std::string color_name;  //< lower case
    std::string type_name;   //< lower case
};

struct raster {
    std::string schema_name;
    std::string table_name;
    std::string column_name;
    std::vector<band> bands;
    int width;
    int height;
    double xorig;
    double yorig;
    double xscale;
    double yscale;
    double xskew;
    double yskew;
    int epsg;
};

}  // namespace boat::db

#endif  // BOAT_DB_META_HPP
