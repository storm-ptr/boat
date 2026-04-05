// Andrew Naplavkov

#define BOOST_TEST_MODULE boat_gui_qt

#include <boat/gui/qt.hpp>
#include <boost/test/included/unit_test.hpp>
#include "../context.hpp"
#include "../providers.hpp"

BOOST_AUTO_TEST_CASE(qt_draw)
{
    auto pixels_sum = 0., transparent_sum = 0.;
    auto pvds = providers() | std::ranges::to<std::vector>();
    for (auto [i, ctx] : std::views::enumerate(contexts())) {
        auto img =
            QImage{ctx.width, ctx.height, QImage::Format_ARGB32_Premultiplied};
        img.fill(Qt::transparent);
        auto art = QPainter{&img};
        art.setCompositionMode(QPainter::CompositionMode_Darken);
        art.setRenderHint(QPainter::Antialiasing);
        art.setPen(QPen(Qt::darkCyan, 5));
        art.setBrush(Qt::cyan);
        for (auto& pvd : pvds) {
            pvd.grid = ctx.grid;
            for (auto feat : pvd.features())
                boat::gui::qt::draw(art, feat, ctx.affine, ctx.system);
        }
        auto path = boat::concat(std::setfill('0'), std::setw(2), i, ".png");
        img.save(path.data());
        auto rgb = reinterpret_cast<QRgb*>(img.bits());
        auto num_pixels = ctx.width * ctx.height;
        pixels_sum += num_pixels;
        transparent_sum += std::count_if(
            rgb, rgb + num_pixels, [](auto c) { return !qAlpha(c); });
    }
    BOOST_TEST(transparent_sum / pixels_sum == transparent_ratio,
               boost::test_tools::tolerance(transparent_tolerance));
}
