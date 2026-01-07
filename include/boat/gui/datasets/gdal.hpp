// Andrew Naplavkov

#ifndef BOAT_GUI_DATASETS_GDAL_HPP
#define BOAT_GUI_DATASETS_GDAL_HPP

#include <boat/gdal/make_image.hpp>
#include <boat/gdal/raster.hpp>
#include <boat/geometry/tile.hpp>
#include <boat/gui/datasets/dataset.hpp>
#include <boost/gil/extension/io/png.hpp>

namespace boat::gui::datasets {

class gdal : public dataset {
    std::string file_;
    std::shared_ptr<caches::cache> cache_;

public:
    gdal(std::string file, std::shared_ptr<caches::cache> const& cache)
        : file_(std::move(file)), cache_(std::move(cache))
    {
    }

    std::vector<qualified_name> layers() override { return {{"raster"}}; }

    std::generator<feature> features(  //
        qualified_name,
        geometry::geographic::grid grid,
        double resolution) override
    {
        namespace gil = boost::gil;
        namespace qvm = boost::qvm;
        constexpr auto limit = 512;
        auto opt = std::optional<boat::gdal::raster>{};
        auto ref = [&] {
            return std::cref(opt ? *opt : opt.emplace(file_.data()));
        };
        auto m = get_or_invoke(
            cache_.get(), file_, [&] { return ref().get().get_meta(); });
        auto srs = boost::geometry::srs::epsg{m.epsg};
        auto tiles = geometry::covers(
            grid, resolution, limit, m.width, m.height, m.affine, srs);
        for (auto& t : tiles) {
            auto mbr = boat::geometry::envelope(m.width, m.height, t);
            auto a = mbr.min_corner(), b = mbr.max_corner();
            auto scale =
                boat::geometry::downscaling_factor(m.width, m.height, t.z);
            auto key = std::tuple{file_, t.z, t.y, t.x};
            auto img = get_or_invoke(cache_.get(), key, [&] {
                auto x = static_cast<int>(a.x());
                auto y = static_cast<int>(a.y());
                auto w = static_cast<int>(b.x() - a.x());
                auto h = static_cast<int>(b.y() - a.y());
                auto ret = boat::gdal::make_image(
                    w / scale,
                    h / scale,
                    m.bands | std::views::transform(&boat::gdal::band::color));
                boost::variant2::visit(
                    [&](auto& v) { ref().get().io(x, y, w, h, gil::view(v)); },
                    ret);
                return ret;
            });
            auto os =
                std::ostringstream{std::ios_base::out | std::ios_base::binary};
            gil::write_view(os, gil::view(img), gil::png_tag());
            auto str = std::move(os).str();
            auto mat =  //
                m.affine * qvm::translation_mat(qvm::vec{{a.x(), a.y()}}) *
                qvm::diag_mat(qvm::vec{{scale * 1., scale * 1., 1.}});
            co_yield feature{
                std::in_place_type<raster>,
                blob{as_bytes(str.data()), str.size()},
                mat,
                m.epsg,
            };
        }
    }
};

}  // namespace boat::gui::datasets

#endif  // BOAT_GUI_DATASETS_GDAL_HPP
