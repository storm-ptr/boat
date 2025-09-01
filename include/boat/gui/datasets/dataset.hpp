// Andrew Naplavkov

#ifndef BOAT_GUI_DATASETS_DATASET_HPP
#define BOAT_GUI_DATASETS_DATASET_HPP

#include <boat/gui/feature.hpp>
#include <generator>

namespace boat::gui::datasets {

using layer = std::vector<std::string>;

struct dataset {
    virtual ~dataset() = default;

    virtual std::vector<layer> layers() = 0;

    virtual std::generator<feature> features(layer,
                                             geometry::geographic::grid,
                                             double resolution) = 0;
};

}  // namespace boat::gui::datasets

#endif  // BOAT_GUI_DATASETS_DATASET_HPP
