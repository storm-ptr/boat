// Andrew Naplavkov

#define BOOST_TEST_MODULE boat_gui_qt

#include <boat/gui/qt.hpp>
#include <boost/test/included/unit_test.hpp>
#include <format>
#include "../context.hpp"
#include "../queries.hpp"

BOOST_AUTO_TEST_CASE(qt_draw)
{
    auto pixels_sum = 0., transparent_sum = 0.;
    auto qs = queries() | std::ranges::to<std::vector>();
    for (auto [i, ctx] : std::views::enumerate(contexts())) {
        auto img =
            QImage{ctx.extent.x(), ctx.extent.y(), QImage::Format_ARGB32};
        img.fill(Qt::transparent);
        auto art = QPainter{&img};
        art.setCompositionMode(QPainter::CompositionMode_Darken);
        art.setRenderHint(QPainter::Antialiasing);
        art.setPen({Qt::darkCyan, 5});
        art.setBrush(Qt::cyan);
        auto drw = boat::gui::draw_variant(
            std::execution::par, art, ctx.affine, ctx.crs);
        for (auto& q : qs) {
            q.footprint = ctx.footprint;
            for (auto var : fetch(q))
                std::visit(drw, var);
        }
        auto path = std::format("drop.{:02}.png", i);
        img.save(path.data());
        auto rgb = reinterpret_cast<QRgb*>(img.bits());
        auto num_pixels = 1LL * ctx.extent.x() * ctx.extent.y();
        pixels_sum += num_pixels;
        transparent_sum += std::count_if(
            rgb, rgb + num_pixels, [](auto c) { return !qAlpha(c); });
    }
    BOOST_TEST(
        transparent_sum / pixels_sum == transparent_ratio,
        boost::test_tools::tolerance(transparent_tolerance));
}
