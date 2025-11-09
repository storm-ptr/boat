// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_RASTER_HPP
#define BOAT_GEOMETRY_RASTER_HPP

#include <boat/geometry/distribution.hpp>
#include <boat/geometry/transform.hpp>
#include <boost/qvm/map_vec_mat.hpp>

namespace boat::geometry {

inline matrix to_matrix(int width, int height, cartesian::segment const& pixel)
{
    namespace qvm = boost::qvm;
    auto const& [a, b] = pixel;
    auto scale = boost::geometry::distance(a, b);
    return qvm::translation_mat(qvm::vec{{a.x(), a.y()}}) *
           qvm::rotz_mat<3>(.5 * numbers::pi - boost::geometry::azimuth(a, b)) *
           qvm::diag_mat(qvm::vec{{scale, scale, 1.}}) *
           qvm::translation_mat(-qvm::vec{{width * .5, height * .5}}) *
           qvm::diag_mat(qvm::vec{{1., -1., 1.}}) *
           qvm::translation_mat(-qvm::vec{{0., height * 1.}});
}

geographic::grid tessellation(int width,
                              int height,
                              size_t num_points,
                              matrix const& mat,
                              srs_spec auto const& srs)
{
    auto tf = transformation(srs);
    auto fwd = transform(srs_forward(tf), mat_inverse(mat));
    auto inv = transform(mat_forward(mat), srs_inverse(tf));
    auto wnd = cartesian::box{{}, {width * 1., height * 1.}};
    auto sentinel = [&](auto const& ll) {
        auto xy = fwd(ll);
        return !xy || !boost::geometry::covered_by(
                          cartesian::point{xy->x(), xy->y()}, wnd);
    };
    auto lls = geographic::multi_point{};
    for (auto xy : box_fibonacci(wnd, num_points))
        if (auto ll = inv(geographic::point{xy.x(), xy.y()}))
            lls.push_back(*ll);
    auto ret = geographic::grid{};
    for (auto z : std::views::iota(0, 40)) {
        auto fib = geographic_fibonacci{static_cast<size_t>(std::pow(2, z))};
        auto indices = std::unordered_set<size_t>{};
        for (auto& ll : lls)
            for (auto i : fib.nearests(ll, sentinel)) {
                if (!indices.insert(i).second)
                    break;
                if (indices.size() > lls.size())
                    return ret;
            }
        if (indices.empty())
            continue;
        auto& level =
            ret[numbers::earth::sqrt_area / std::sqrt(fib.num_points)];
        level.resize(indices.size());
        for (auto [i, j] : indices | std::views::enumerate)
            level[i] = fib[j];
    }
    return ret;
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_RASTER_HPP
