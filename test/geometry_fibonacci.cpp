// Andrew Naplavkov

#include <boat/geometry/distribution.hpp>
#include <boost/test/unit_test.hpp>

using namespace boat::geometry;
using namespace boat::numbers;
using namespace boost::geometry;

namespace {

std::generator<geographic_fibonacci> fibonacci()
{
    for (auto z : std::views::iota(0))
        co_yield {static_cast<size_t>(std::pow(4, z))};
}

}  // namespace

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
        auto step = earth::sqrt_area / std::sqrt(fib.num_points);
        for (auto p : random() | std::views::take(60)) {
            auto d = distance(p, fib[fib.nearest(p)]);
            BOOST_CHECK_LE(d, step);
            BOOST_CHECK_LE(d, distance(p, *rtree.qbegin(index::nearest(p, 1))));
        }
    }
}
