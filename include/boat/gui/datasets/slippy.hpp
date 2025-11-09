// Andrew Naplavkov

#ifndef BOAT_GUI_DATASETS_SLIPPY_HPP
#define BOAT_GUI_DATASETS_SLIPPY_HPP

#include <boat/detail/curl.hpp>
#include <boat/geometry/slippy.hpp>
#include <boat/geometry/wkb.hpp>
#include <boat/gui/datasets/dataset.hpp>
#include <boost/algorithm/string.hpp>

namespace boat::gui::datasets {

class slippy : public dataset {
    std::string usr_;
    std::string url_;
    std::shared_ptr<caches::cache> cache_;

public:
    slippy(std::string usr,
           std::string url,
           std::shared_ptr<caches::cache> const& cache)
        : usr_(std::move(usr)), url_(std::move(url)), cache_(std::move(cache))
    {
    }

    std::vector<layer> layers() override { return {{"slippy"}}; }

    std::generator<feature> features(layer,
                                     geometry::geographic::grid grid,
                                     double resolution) override
    {
        auto tiles = std::unordered_map<std::string, geometry::tile>{};
        auto queue = curl{usr_};
        for (auto t : geometry::slippy::coverage(grid, resolution)) {
            auto url = url_;
            url = boost::replace_first_copy(url, "{x}", to_chars(t.x));
            url = boost::replace_first_copy(url, "{y}", to_chars(t.y));
            url = boost::replace_first_copy(url, "{z}", to_chars(t.z));
            auto img = cache_ ? cache_->get(url) : std::any{};
            if (img.has_value())
                co_yield feature{std::in_place_type<raster>,
                                 std::any_cast<blob>(std::move(img)),
                                 geometry::slippy::to_matrix(t),
                                 geometry::slippy::epsg};
            else {
                tiles.insert({url, t});
                queue.push(url);
            }
        }
        while (queue.size()) {
            auto [url, img] = queue.pop();
            if (cache_)
                cache_->put(url, img);
            co_yield feature{std::in_place_type<raster>,
                             std::move(img),
                             geometry::slippy::to_matrix(tiles.at(url)),
                             geometry::slippy::epsg};
        }
    }
};

}  // namespace boat::gui::datasets

#endif  // BOAT_GUI_DATASETS_SLIPPY_HPP
