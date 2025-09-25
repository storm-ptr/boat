// Andrew Naplavkov

#ifndef BOAT_GUI_WX_HPP
#define BOAT_GUI_WX_HPP

#include <wx/graphics.h>
#include <wx/mstream.h>
#include <wx/wx.h>
#include <boat/geometry/transform.hpp>
#include <boat/geometry/wkb.hpp>
#include <boat/gui/feature.hpp>
#include <boost/geometry/srs/epsg.hpp>
#include <execution>

namespace boat::gui::wx {
namespace detail {

wxGraphicsPath& add(geometry::curve auto const& g, wxGraphicsPath& path)
{
    path.MoveToPoint(g.at(0).x(), g.at(0).y());
    for (size_t i = 1; i < g.size(); ++i)
        path.AddLineToPoint(g[i].x(), g[i].y());
    return path;
}

void draw(geometry::ogc99 auto const& geom, wxGraphicsContext& gc)
{
    overloaded{
        [&](geometry::point auto& g) { gc.DrawEllipse(g.x(), g.y(), 1, 1); },
        [&](geometry::linestring auto& g) {
            auto path = gc.CreatePath();
            gc.StrokePath(add(g, path));
        },
        [&](geometry::polygon auto& g) {
            auto path = gc.CreatePath();
            add(g.outer(), path).CloseSubpath();
            for (auto& item : g.inners())
                add(item, path).CloseSubpath();
            gc.DrawPath(path);
        },
        [](this auto&& self, geometry::multi auto& g) -> void {
            std::ranges::for_each(g, self);
        },
        [](this auto&& self, geometry::dynamic auto& g) -> void {
            std::visit(self, g);
        }}(geom);
}

}  // namespace detail

void draw(feature const& feat,
          geometry::srs auto const& srs,
          geometry::box auto const& mbr,
          wxGraphicsContext& gc)
{
    wxDouble w, h;
    gc.GetSize(&w, &h);
    w = std::ceil(w), h = std::ceil(h);
    auto tf = boost::geometry::srs::transformation<>(
        boost::geometry::srs::epsg{feat.epsg}, srs);
    auto fwd = geometry::transformer(geometry::forwarder(tf),
                                     geometry::forwarder(mbr, w, h));
    auto shape = geometry::geographic::variant{};
    blob_view{feat.shape} >> shape;
    if (feat.raster.empty()) {
        if (auto pj = fwd(shape))
            detail::draw(*pj, gc);
        return;
    }
    auto mbr1 = geometry::envelope(shape);
    auto mbr2 = fwd(mbr1).transform([=](auto&& g) {
        auto a = g.min_corner(), b = g.max_corner();
        auto x = std::floor(a.x()), y = std::floor(a.y());
        return wxRect2DDouble{0., 0., w, h}.CreateIntersection(
            {x, y, std::ceil(b.x()) - x, std::ceil(b.y()) - y});
    });
    auto img1 = wxImage{};
    auto is = wxMemoryInputStream{feat.raster.data(), feat.raster.size()};
    if (!mbr2 || mbr2->IsEmpty() || !img1.LoadFile(is))
        return;
    auto img2 = wxImage{mbr2->GetSize()};
    img2.InitAlpha();
    auto inv = geometry::transformer(
        geometry::inverter(mbr, w, h),
        geometry::inverter(tf),
        geometry::forwarder(mbr1, img1.GetWidth(), img1.GetHeight()));
    auto rgb1 = reinterpret_cast<wxImage::RGBValue*>(img1.GetData());
    auto ys = std::views::iota(0, img2.GetHeight());
    std::for_each(std::execution::par, ys.begin(), ys.end(), [&](int y) {
        int d = y * img2.GetWidth(), dx = mbr2->m_x, dy = mbr2->m_y;
        auto rgb2 = reinterpret_cast<wxImage::RGBValue*>(img2.GetData()) + d;
        auto alpha2 = img2.GetAlpha() + d;
        for (int x{}; x < img2.GetWidth(); ++x) {
            auto p =
                inv(geometry::geographic::point(x + dx, y + dy))
                    .transform([](auto&& p) { return wxPoint(p.x(), p.y()); });
            if (p && wxRect{img1.GetSize()}.Contains(*p))
                rgb2[x] = rgb1[p->y * img1.GetWidth() + p->x];
            else
                alpha2[x] = wxIMAGE_ALPHA_TRANSPARENT;
        }
    });
    gc.DrawBitmap(gc.CreateBitmapFromImage(img2), *mbr2);
}

}  // namespace boat::gui::wx

#endif  // BOAT_GUI_WX_HPP
