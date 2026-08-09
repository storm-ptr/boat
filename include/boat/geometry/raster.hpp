// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_RASTER_HPP
#define BOAT_GEOMETRY_RASTER_HPP

#include <boat/geometry/algorithm.hpp>
#include <boat/geometry/detail/fibonacci.hpp>
#include <boat/geometry/transform.hpp>
#include <boost/qvm/map_vec_mat.hpp>

namespace boat::geometry {

geographic::grid geographic_interpolate(  //
    int width,
    int height,
    matrix const& mat,
    srs_params auto const& crs,
    size_t num_points)
{
    auto tf = transformation(crs);
    auto fwd = transform(srs_forward(tf), mat_inverse(mat));
    auto inv = transform(mat_forward(mat), srs_inverse(tf));
    auto sentinel = [&](auto& ll) {
        auto xy = fwd(ll).transform(cartesian{});
        return !xy || !boost::geometry::covered_by(
                          *xy, cartesian::box{{}, {width * 1., height * 1.}});
    };
    auto points =
        inv(box_area_interpolate(geographic::box{{}, {width * 1., height * 1.}},
                                 num_points) |
            std::ranges::to<geographic::multi_point>())
            .value_or(geographic::multi_point{});
    auto ret = geographic::grid{};
    for (auto fib : geographic_fibonacci_levels) {
        auto indices = std::unordered_set<size_t>{};
        for (auto& p : points)
            for (auto i : fib.nearests(p, sentinel)) {
                if (!indices.insert(i).second)
                    break;
                if (indices.size() > points.size() * 2)
                    return ret;
            }
        if (indices.empty())
            continue;
        auto& lvl = ret[numbers::earth::sqrt_area / std::sqrt(fib.num_points)];
        lvl.resize(indices.size());
        for (auto [i, j] : indices | std::views::enumerate)
            lvl[i] = fib[j];
    }
    return ret;
}

inline matrix affine(int width, int height, cartesian::segment const& mid_pixel)
{
    namespace qvm = boost::qvm;
    auto const& [a, b] = mid_pixel;
    auto scale = boost::geometry::distance(a, b);
    return qvm::translation_mat(qvm::vec{{a.x(), a.y()}}) *
           qvm::rotz_mat<3>(.5 * numbers::pi - boost::geometry::azimuth(a, b)) *
           qvm::diag_mat(qvm::vec{{scale, scale, 1.}}) *
           qvm::translation_mat(-qvm::vec{{width * .5, height * .5}}) *
           qvm::diag_mat(qvm::vec{{1., -1., 1.}}) *
           qvm::translation_mat(-qvm::vec{{0., height * 1.}});
}

inline matrix affine(int width, int height, cartesian::box const& mbr)
{
    return boost::qvm::inverse(
        boost::geometry::strategy::transform::
            map_transformer<double, 2, 2, true, false>{mbr, width, height}
                .matrix());
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_RASTER_HPP
