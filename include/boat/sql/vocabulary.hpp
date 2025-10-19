// Andrew Naplavkov

#ifndef BOAT_SQL_VOCABULARY_HPP
#define BOAT_SQL_VOCABULARY_HPP

#include <cstdint>
#include <ranges>
#include <string>
#include <vector>

namespace boat::sql {

struct layer {
    std::string schema_name;
    std::string table_name;
    std::string column_name;
};

struct column {
    std::string column_name;
    std::string type_name;
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
    std::string dbms_name;
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

struct order_key {
    std::string column_name;
    bool descending;
};

struct page {
    std::vector<std::string> select_list;
    std::vector<order_key> order_by;
    int64_t offset;
    int limit;
};

struct overlap {
    std::vector<std::string> select_list;
    std::string spatial_column;
    double xmin;
    double ymin;
    double xmax;
    double ymax;
    int limit;
};

}  // namespace boat::sql

#endif  // BOAT_SQL_VOCABULARY_HPP
