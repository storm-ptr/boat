// Andrew Naplavkov

#ifndef CATALOG_H
#define CATALOG_H

#include <boat/db/catalog.hpp>
#include <memory>

std::unique_ptr<boat::db::catalog> make_catalog(
    std::string_view address, bool update = false);

#endif  // CATALOG_H
