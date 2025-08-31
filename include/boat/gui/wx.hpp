// Andrew Naplavkov

#ifndef BOAT_GUI_WX_HPP
#define BOAT_GUI_WX_HPP

#include <wx/graphics.h>
#include <wx/mstream.h>
#include <wx/wx.h>
#include <boat/geometry/transform.hpp>
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
    auto fwd = geometry::transform(geometry::forward(tf),
                                   geometry::forward(mbr, w, h));
    if (feat.raster.empty()) {
        if (auto pj = fwd(feat.shape))
            detail::draw(*pj, gc);
        return;
    }
    auto in_mbr = geometry::envelope(feat.shape);
    auto out_mbr = fwd(in_mbr).transform([=](auto&& g) {
        auto a = g.min_corner(), b = g.max_corner();
        auto x = std::floor(a.x()), y = std::floor(a.y());
        return wxRect2DDouble{0., 0., w, h}.CreateIntersection(
            {x, y, std::ceil(b.x()) - x, std::ceil(b.y()) - y});
    });
    if (!out_mbr || out_mbr->IsEmpty())
        return;
    auto in = wxImage{};
    auto in_mem = wxMemoryInputStream{feat.raster.data(), feat.raster.size()};
    if (!in.LoadFile(in_mem))
        return;
    auto out = wxImage{out_mbr->GetSize()};
    out.InitAlpha();
    auto inv = geometry::transform(
        geometry::inverse(mbr, w, h),
        geometry::inverse(tf),
        geometry::forward(in_mbr, in.GetWidth(), in.GetHeight()));
    auto in_rgb = reinterpret_cast<wxImage::RGBValue*>(in.GetData());
    auto ys = std::views::iota(0, out.GetHeight());
    std::for_each(std::execution::par, ys.begin(), ys.end(), [&](int y) {
        int d = y * out.GetWidth(), dx = out_mbr->m_x, dy = out_mbr->m_y;
        auto out_alpha = out.GetAlpha() + d;
        auto out_rgb = reinterpret_cast<wxImage::RGBValue*>(out.GetData()) + d;
        for (int x{}; x < out.GetWidth(); ++x) {
            auto p =
                inv(geometry::geographic::point(x + dx, y + dy))
                    .transform([](auto&& p) { return wxPoint(p.x(), p.y()); });
            if (!p || !wxRect{in.GetSize()}.Contains(*p))
                out_alpha[x] = wxIMAGE_ALPHA_TRANSPARENT;
            else
                out_rgb[x] = in_rgb[p->y * in.GetWidth() + p->x];
        }
    });
    gc.DrawBitmap(gc.CreateBitmapFromImage(out), *out_mbr);
}

}  // namespace boat::gui::wx

#endif  // BOAT_GUI_WX_HPP
