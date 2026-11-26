// Andrew Naplavkov

#include <QClipboard>
#include <QContextMenuEvent>
#include <QDebug>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <array>
#include <boat/gdal/dataset.hpp>
#include <boat/geometry/raster.hpp>
#include "formats.h"
#include "map_view.h"
#include "tree.h"

namespace geo = boat::geometry;

namespace {

void save_png(
    QString const& path,
    QPixmap const& image,
    geo::matrix const& mat,
    geo::srs::proj4 const& crs)
{
    boat::check(image.save(path), "save png");
    auto ds = boat::gdal::open(path.toUtf8().constData());
    auto px = 1. / image.devicePixelRatio();
    auto a = std::array{
        mat.a[0][2],
        mat.a[0][0] * px,
        mat.a[0][1] * px,
        mat.a[1][2],
        mat.a[1][0] * px,
        mat.a[1][1] * px};
    boat::gdal::check(GDALSetGeoTransform(ds.get(), a.data()));
    auto srs = boat::gdal::make_srs(0, {}, crs.str());
    boat::gdal::check(GDALSetSpatialRef(ds.get(), srs.get()));
    boat::gdal::check(GDALClose(ds.release()));
}

}  // namespace

void map_view::contextMenuEvent(QContextMenuEvent* event)
{
    auto menu = QMenu{this};
    auto pos = event->pos();
    auto text = coordinate_text(pos);
    if (!text.isEmpty() &&
        std::ranges::any_of(layers_, [](auto& l) { return !l.layer.raster; }))
        menu.addAction("identify features", this, [this, pos, vp = view()] {
            identify(pos, vp);
        });
    menu.addAction("save map as PNG", this, [this] {
        auto path = QFileDialog::getSaveFileName(this, {}, {}, "PNG (*.png)");
        if (path.isEmpty())
            return;
        try {
            auto crs = geo::ortho(map_.center);
            auto mat = geo::affine(
                {width(), height()}, map_.center, map_.resolution, crs);
            save_png(
                ensure_extension(std::move(path), ".png"), grab(), mat, crs);
        }
        catch (std::exception const& e) {
            QMessageBox::warning(this, {}, QString::fromUtf8(e.what()));
        }
    });
    if (!text.isEmpty())
        menu.addAction("copy coordinates", this, [text] {
            QGuiApplication::clipboard()->setText(text);
        });
    menu.addAction("cache statistics in log", this, [this] {
        auto now = cache_->stats();
        auto summary = [](size_t requests, size_t misses) {
            auto rate =
                requests ? QString::asprintf("%.1f%%", 100. * misses / requests)
                         : "n/a";
            return QString{"%1/%2 (%3)"}.arg(misses).arg(requests).arg(rate);
        };
        qInfo().noquote().nospace()
            << "cache misses/requests | total: "
            << summary(now.requests, now.misses) << " | since last: "
            << summary(
                   now.requests - last_cache_report_.requests,
                   now.misses - last_cache_report_.misses);
        last_cache_report_ = now;
    });
    menu.exec(event->globalPos());
}
