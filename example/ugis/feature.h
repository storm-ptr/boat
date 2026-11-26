// Andrew Naplavkov

#ifndef FEATURE_H
#define FEATURE_H

#include <QString>
#include <boat/geometry/vocabulary.hpp>
#include <stop_token>

namespace boat::gui::caches {
struct cache;
}

class QPointF;
struct leaf;
struct viewport;

struct feature {
    boat::geometry::geographic::variant geom;
    ptrdiff_t layer;
    QString attributes;
};

std::vector<feature> identify_features(
    std::vector<leaf> const& layers,
    QPointF cursor,
    viewport const&,
    boat::gui::caches::cache*,
    int query_limit,
    int feature_limit,
    std::stop_token);

#endif  // FEATURE_H
