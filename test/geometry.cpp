// Andrew Naplavkov

#include <boat/geometry/raster.hpp>
#include <boat/geometry/slippy.hpp>
#include <boat/geometry/wkb.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/container.hpp>
#include <boost/test/unit_test.hpp>
#include "utility.hpp"

using namespace boat::geometry;
using namespace std::literals;

namespace {

auto srs = boost::geometry::srs::epsg{slippy::epsg};
auto tf = transformation(srs);
auto width = 1920;
auto height = 1080;
auto num_points = 100u;

template <ogc99 Geom1, same_tag<Geom1> Geom2>
void check(std::string const& wkt)
{
    auto ll = boost::geometry::from_wkt<Geom1>(wkt);
    auto xy = Geom2{};
    BOOST_CHECK(transform(ll, xy, srs_forward(tf)));
    BOOST_CHECK_NE(wkt, boost::geometry::to_wkt(xy));
    auto wkb = boat::blob{} << xy;
    boat::blob_view{wkb} >> xy;
    BOOST_CHECK(transform(xy, ll, srs_inverse(tf)));
    BOOST_CHECK_EQUAL(wkt, boost::geometry::to_wkt(ll));
}

}  // namespace

BOOST_AUTO_TEST_CASE(geometry_transform_wkb)
{
    // clang-format off
    auto geoms = boost::fusion::make_map<
        geographic::point,
        geographic::linestring,
        geographic::polygon,
        geographic::multi_point,
        geographic::multi_linestring,
        geographic::multi_polygon,
        geographic::geometry_collection>(
        "POINT(30 10)"s,
        "LINESTRING(30 10,10 30,40 40)"s,
        "POLYGON((35 10,45 45,15 40,10 20,35 10),(20 30,35 35,30 20,20 30))"s,
        "MULTIPOINT((10 40),(40 30),(20 20),(30 10))"s,
        "MULTILINESTRING((10 10,20 20,10 40),(40 40,30 30,40 20,30 10))"s,
        "MULTIPOLYGON(((40 40,20 45,45 30,40 40)),((20 35,10 30,10 10,30 5,45 20,20 35),(30 20,20 15,20 25,30 20)))"s,
        "GEOMETRYCOLLECTION(POINT(40 10),LINESTRING(10 10,20 20,10 40),POLYGON((40 40,20 45,45 30,40 40)))"s
    );
    // clang-format on
    boost::fusion::for_each(geoms, [&]<class P>(P const& pair) {
        using geom1 = P::first_type;
        using geom2 = std::variant_alternative_t<variant_index_v<geom1>,
                                                 cartesian::variant>;
        auto const& wkt = pair.second;
        check<geom1, geom2>(wkt);
        check<geographic::variant, cartesian::variant>(wkt);
    });
}

BOOST_AUTO_TEST_CASE(geometry_endian)
{
    std::pair<std::string, std::string> tests[] = {
        {"000000000140000000000000004010000000000000", "POINT(2 4)"},
        {"010100000000000000000000400000000000001040", "POINT(2 4)"},
    };
    for (auto& [hex, wkt] : tests) {
        auto unhex = boost::algorithm::unhex(hex);
        auto in = boat::blob_view{std::as_bytes(std::span(unhex))};
        auto geom = get<cartesian::variant>(in);
        BOOST_CHECK_EQUAL(wkt, boost::geometry::to_wkt(geom));
    }
}

BOOST_AUTO_TEST_CASE(geometry_fibonacci_monotonic)
{
    auto lim = 50;
    for (auto fib : geographic_fibonacci_levels | std::views::take(18))
        for (auto p : geographic_random() | std::views::take(lim)) {
            auto prev = 0.;
            auto indices = std::unordered_set<size_t>{};
            for (auto [i, j] : fib.nearests(p) | std::views::take(lim) |
                                   std::views::enumerate) {
                auto d = distance(p, fib[j]);
                BOOST_TEST(prev <= d,
                           wkt(p) << ", " << i << "/" << fib.num_points);
                prev = d;
                indices.insert(j);
            }
            auto expect = std::min<size_t>(lim, fib.num_points);
            BOOST_CHECK_EQUAL(indices.size(), expect);
        }
}

BOOST_AUTO_TEST_CASE(geometry_fibonacci_vs_rtree)
{
    namespace bgi = boost::geometry::index;
    for (auto fib : geographic_fibonacci_levels | std::views::take(8)) {
        auto rtree = bgi::rtree<geographic::point, bgi::rstar<4>>{};
        for (auto i : std::views::iota(0u, fib.num_points))
            rtree.insert(fib[i]);
        auto step = boat::numbers::earth::sqrt_area / std::sqrt(fib.num_points);
        for (auto p : geographic_random() | std::views::take(60)) {
            auto d = distance(p, fib[fib.nearest(p)]);
            BOOST_CHECK_LE(d, step);
            BOOST_CHECK_LE(d, distance(p, *rtree.qbegin(bgi::nearest(p, 1))));
        }
    }
}

BOOST_AUTO_TEST_CASE(geometry_raster)
{
    auto mbr = cartesian::box{{}, {width * 1., height * 1.}};
    auto ext = cartesian::box{};
    transform(
        geographic::box{{-180., -85.}, {180., 85.}}, ext, srs_forward(tf));
    for (auto p : geographic_random() | std::views::take(10)) {
        for (auto res : {1, 10, 100, 1000}) {
            auto px = cartesian::segment{};
            tf.forward(geographic::segment{p, add_meters(p, 0, res)}, px);
            auto mat = affine(width, height, px);
            auto in = *transform(mat_forward(mat))(mbr);
            if (!boost::geometry::covered_by(in, ext))
                continue;
            auto grid =
                geographic_interpolate(width, height, mat, srs, num_points);
            BOOST_CHECK(!grid.empty());
            auto& lls = grid.begin()->second;
            BOOST_CHECK_LE(lls.size(), num_points * 2);
            BOOST_CHECK_GE(lls.size(), num_points / 2);
            auto xys = cartesian::multi_point{};
            BOOST_CHECK(tf.forward(lls, xys));
            auto out = minmax(xys);
            BOOST_CHECK(boost::geometry::covered_by(out, in));
            auto iou = boost::geometry::area(out) / boost::geometry::area(in);
            BOOST_TEST(iou > .87, iou << "; " << res << "m/px; " << wkt(p));
        }
    }
}

BOOST_AUTO_TEST_CASE(geometry_slippy)
{
    auto p = geographic::point{139.7006793, 35.6590699};
    auto t = slippy::detail::snap(p, 18);
    BOOST_CHECK_EQUAL(t.x, 232'798);
    BOOST_CHECK_EQUAL(t.y, 103'246);
    BOOST_CHECK(covered_by(p, slippy::detail::envelope(t)));
    auto res = 300.;
    auto px = cartesian::segment{};
    tf.forward(geographic::segment{p, add_meters(p, 0, res)}, px);
    auto mat = affine(width, height, px);
    auto grid = geographic_interpolate(width, height, mat, srs, num_points);
    auto tiles = slippy::covers(grid, res);
    auto z = tiles.begin()->z;
    auto inv = transform(mat_forward(mat), srs_inverse(tf));
    auto a = slippy::detail::snap(*inv(geographic::point()), z);
    auto b = slippy::detail::snap(*inv(geographic::point(width, height)), z);
    for (int x = std::min<>(a.x, b.x); x <= std::max<>(a.x, b.x); ++x)
        for (int y = std::min<>(a.y, b.y); y <= std::max<>(a.y, b.y); ++y)
            BOOST_CHECK(tiles.contains({.z = z, .y = y, .x = x}));
}
