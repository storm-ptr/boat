// Andrew Naplavkov

#include <boat/geometry/map.hpp>
#include <boat/geometry/transform.hpp>
#include <boat/geometry/wkb.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/container.hpp>
#include <boost/test/unit_test.hpp>
#include <random>
#include "utility.hpp"

using namespace boat::geometry;
using namespace boost::geometry;
using namespace std::literals;

namespace {

constexpr auto covered = boat::overloaded{
    [](single auto const& lhs, box auto const& rhs) {
        return covered_by(lhs, rhs);
    },
    [](this auto&& self, multi auto const& lhs, box auto const& rhs) -> bool {
        return std::ranges::all_of(lhs, [&](auto& g) { return self(g, rhs); });
    },
    [](this auto&& self, dynamic auto const& lhs, box auto const& rhs) -> bool {
        return std::visit([&](auto& g) { return self(g, rhs); }, lhs);
    }};

template <ogc99 Geom1, same_tag<Geom1> Geom2>
void check(std::string const& wkt)
{
    auto pj = srs::projection<srs::static_epsg<3857>>{};
    auto ll = from_wkt<Geom1>(wkt);
    auto xy = Geom2{};
    BOOST_CHECK(transform(ll, xy, forwarder(pj)));
    BOOST_CHECK_NE(wkt, to_wkt(xy));
    auto wkb = boat::blob{} << xy;
    boat::blob_view{wkb} >> xy;
    BOOST_CHECK(transform(xy, ll, inverter(pj)));
    BOOST_CHECK_EQUAL(wkt, to_wkt(ll));
    auto mbr = envelope(xy);
    BOOST_CHECK(covered(xy, mbr));
    BOOST_CHECK(!covered(cartesian::point{}, mbr));
}

std::generator<geographic_fibonacci> fibonacci()
{
    for (auto z : std::views::iota(0))
        co_yield {static_cast<size_t>(std::pow(4, z))};
}

std::generator<geographic::point> random()
{
    namespace bm = boost::math::double_constants;
    auto gen = std::mt19937{std::random_device()()};
    auto dist = std::uniform_real_distribution<double>{0, 1};
    while (true) {
        auto azimuthal = bm::two_pi * dist(gen);
        auto polar = std::acos(1 - 2 * dist(gen));
        co_yield {azimuthal * bm::radian - 180, polar * bm::radian - 90};
    }
}

}  // namespace

BOOST_AUTO_TEST_CASE(geometry_base)
{
    auto geoms = boost::fusion::make_map<geographic::point,
                                         geographic::linestring,
                                         geographic::polygon,
                                         geographic::multi_point,
                                         geographic::multi_linestring,
                                         geographic::multi_polygon,
                                         geographic::geometry_collection>(
        // clang-format off
"POINT(30 10)"s,
"LINESTRING(30 10,10 30,40 40)"s,
"POLYGON((35 10,45 45,15 40,10 20,35 10),(20 30,35 35,30 20,20 30))"s,
"MULTIPOINT((10 40),(40 30),(20 20),(30 10))"s,
"MULTILINESTRING((10 10,20 20,10 40),(40 40,30 30,40 20,30 10))"s,
"MULTIPOLYGON(((40 40,20 45,45 30,40 40)),((20 35,10 30,10 10,30 5,45 20,20 35),(30 20,20 15,20 25,30 20)))"s,
"GEOMETRYCOLLECTION(POINT(40 10),LINESTRING(10 10,20 20,10 40),POLYGON((40 40,20 45,45 30,40 40)))"s
        // clang-format on
    );
    boost::fusion::for_each(geoms, [&]<class P>(P const& pair) {
        using geom1_t = P::first_type;
        using geom2_t = std::variant_alternative_t<variant_index_v<geom1_t>,
                                                   cartesian::variant>;
        auto const& wkt = pair.second;
        check<geom1_t, geom2_t>(wkt);
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
        BOOST_CHECK_EQUAL(wkt, to_wkt(geom));
    }
}

BOOST_AUTO_TEST_CASE(geometry_fibonacci_monotonic)
{
    auto lim = 50;
    for (auto fib : fibonacci() | std::views::take(18))
        for (auto p : random() | std::views::take(lim)) {
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
    for (auto fib : fibonacci() | std::views::take(8)) {
        auto rtree = index::rtree<geographic::point, index::rstar<4>>{};
        for (auto i : std::views::iota(0u, fib.num_points))
            rtree.insert(fib[i]);
        auto step = root_geoid_area / std::sqrt(fib.num_points);
        for (auto p : random() | std::views::take(60)) {
            auto d = distance(p, fib[fib.nearest(p)]);
            BOOST_CHECK_LE(d, step);
            BOOST_CHECK_LE(d, distance(p, *rtree.qbegin(index::nearest(p, 1))));
        }
    }
}

BOOST_AUTO_TEST_CASE(geometry_map)
{
    auto num_points = 100;
    auto cs = srs::epsg{3857};
    auto pj = srs::projection<>{cs};
    auto globe = cartesian::box{};
    BOOST_CHECK(pj.forward(geographic::point(-180, -85), globe.min_corner()));
    BOOST_CHECK(pj.forward(geographic::point(180, 85), globe.max_corner()));
    constexpr auto pred = [](auto const& ll) { return std::fabs(ll.y()) < 85; };
    for (auto ll : random() | std::views::filter(pred) | std::views::take(10)) {
        for (auto res : {1, 10, 100, 1000}) {
            auto mbr = forward(cs, ll, res, 1920, 1080);
            BOOST_CHECK(mbr);
            auto in = cartesian::box{};
            BOOST_CHECK(intersection(*mbr, globe, in));
            auto grid = inverse(cs, in, num_points);
            auto& lls = grid.begin()->second;
            BOOST_CHECK_LE(lls.size(), num_points);
            BOOST_CHECK_GE(lls.size() + 1, num_points / 2);
            auto xys = cartesian::multi_point{};
            BOOST_CHECK(pj.forward(lls, xys));
            auto out = envelope(xys);
            BOOST_CHECK(within(out, in));
            auto iou = area(out) / area(in);
            BOOST_TEST(iou > .9, iou << "; " << res << "m/px; " << wkt(ll));
        }
    }
}
