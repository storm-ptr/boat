// Andrew Naplavkov

#ifndef BOAT_GUI_WX_HPP
#define BOAT_GUI_WX_HPP

#include <wx/graphics.h>
#include <wx/mstream.h>
#include <wx/wx.h>
#include <boat/gui/detail/geometry.hpp>
#include <boat/gui/detail/gil.hpp>

namespace boat::gui {

wxGraphicsPath& add(geometry::curve auto&& in, wxGraphicsPath& out)
{
    out.MoveToPoint(in.at(0).x(), in.at(0).y());
    for (size_t i = 1; i < in.size(); ++i)
        out.AddLineToPoint(in[i].x(), in[i].y());
    return out;
}

inline auto draw_geometry(wxGraphicsContext& out)
{
    return overloaded{
        [&](geometry::point auto&& v) { out.DrawEllipse(v.x(), v.y(), 1, 1); },
        [&](geometry::linestring auto&& v) {
            auto path = out.CreatePath();
            out.StrokePath(add(v, path));
        },
        [&](geometry::polygon auto&& v) {
            auto path = out.CreatePath();
            add(v.outer(), path).CloseSubpath();
            for (auto& item : v.inners())
                add(item, path).CloseSubpath();
            out.DrawPath(path);
        },
        [](this auto&& self, geometry::multi auto&& v) -> void {
            std::ranges::for_each(v, self);
        },
        [](this auto&& self, geometry::dynamic auto&& v) -> void {
            std::visit(self, v);
        }};
}

void draw_image(  //
    execution_policy auto policy,
    boost::gil::rgba8c_view_t in,
    geometry::matrix const& in_affine,
    geometry::srs_params auto&& in_crs,
    wxGraphicsContext& out,
    geometry::matrix const& out_affine,
    geometry::srs_params auto&& out_crs)
{
    auto [fwd, inv] = bidirectional(in_affine, in_crs, out_affine, out_crs);
    auto intersect = [&](geometry::box auto&& v) {
        wxDouble w, h;
        out.GetSize(&w, &h);
        w = std::ceil(w), h = std::ceil(h);
        auto a = v.min_corner(), b = v.max_corner();
        auto x = std::floor(a.x()), y = std::floor(a.y());
        return wxRect2DDouble{0., 0., w, h}.CreateIntersection(
            {x, y, std::ceil(b.x()) - x, std::ceil(b.y()) - y});
    };
    auto mbr = fwd(multi_point(in.width(), in.height()))
                   .transform(geometry::minmax)
                   .transform(intersect);
    if (!mbr || mbr->IsEmpty())
        return;
    auto img = wxImage{mbr->GetSize()};
    img.InitAlpha();
    auto ys = std::views::iota(0, img.GetHeight());
    auto pixel = get_pixel(in);
    std::for_each(policy, ys.begin(), ys.end(), [&](int y) {
        auto d = y * img.GetWidth();
        auto rgb = reinterpret_cast<wxImage::RGBValue*>(img.GetData()) + d;
        auto alpha = img.GetAlpha() + d;
        for (int x{}; x < img.GetWidth(); ++x)
            if (auto px =
                    inv(geometry::geographic::point(x + mbr->m_x, y + mbr->m_y))
                        .and_then(pixel)) {
                rgb[x] =
                    wxImage::RGBValue(get_color(*px, boost::gil::red_t()),
                                      get_color(*px, boost::gil::green_t()),
                                      get_color(*px, boost::gil::blue_t()));
                alpha[x] = get_color(*px, boost::gil::alpha_t());
            }
            else
                alpha[x] = wxIMAGE_ALPHA_TRANSPARENT;
    });
    out.DrawBitmap(  //
        out.CreateBitmapFromImage(img),
        mbr->m_x,
        mbr->m_y,
        mbr->m_width,
        mbr->m_height);
}

}  // namespace boat::gui

#endif  // BOAT_GUI_WX_HPP
