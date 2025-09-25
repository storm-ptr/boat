// Andrew Naplavkov

#ifndef BOAT_GUI_DATASETS_SLIPPY_HPP
#define BOAT_GUI_DATASETS_SLIPPY_HPP

#include <boat/detail/curl.hpp>
#include <boat/detail/slippy.hpp>
#include <boat/geometry/transform.hpp>
#include <boat/geometry/wkb.hpp>
#include <boat/gui/datasets/dataset.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/geometry/srs/epsg.hpp>

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
        using namespace boat::slippy;
        constexpr auto epsg = 3857;
        static auto const fwd = geometry::transformer(
            geometry::forwarder(boost::geometry::srs::projection<
                                boost::geometry::srs::static_epsg<epsg>>{}));
        constexpr auto shape = [](tile const& t) {
            return blob{} << *fwd(geometry::to_polygon(envelope(t)));
        };
        auto tiles = std::unordered_map<std::string, tile>{};
        auto queue = curl{usr_};
        for (auto t : to_tiles(grid, resolution)) {
            auto url = url_;
            url = boost::replace_first_copy(url, "{x}", to_chars(t.x));
            url = boost::replace_first_copy(url, "{y}", to_chars(t.y));
            url = boost::replace_first_copy(url, "{z}", to_chars(t.z));
            auto img = cache_ ? cache_->get(url) : std::any{};
            if (img.has_value())
                co_yield {std::any_cast<blob>(std::move(img)), shape(t), epsg};
            else {
                tiles.insert({url, t});
                queue.push(url);
            }
        }
        while (queue.size()) {
            auto [url, img] = queue.pop();
            if (cache_)
                cache_->put(url, img);
            co_yield {std::move(img), shape(tiles.at(url)), epsg};
        }
    }
};

}  // namespace boat::gui::datasets

#endif  // BOAT_GUI_DATASETS_SLIPPY_HPP
