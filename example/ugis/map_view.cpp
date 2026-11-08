// Andrew Naplavkov

#include <QMainWindow>
#include <QMouseEvent>
#include <QStatusBar>
#include <QWheelEvent>
#include <boat/gui/caches/lru.hpp>
#include <boat/gui/qt.hpp>
#include <boat/gui/utility.hpp>
#include <boost/gil.hpp>
#include "map_view.h"
#include "tree.h"

namespace geo = boat::geometry;
using boat::gui::affine;
using point = geo::geographic::point;

namespace {

constexpr auto longitude_max = 180.;
constexpr auto latitude_max = 85.;
constexpr auto resolution_min = 0.5972;
constexpr auto resolution_max = 19'567.88;

auto unproject(viewport const& vp, QPointF cursor)
{
    return [=, tf = geo::transformation(geo::ortho(vp.view.center))](auto res) {
        auto mat = affine(vp.width, vp.height, {vp.view.center, res}, tf);
        return geo::transform(geo::mat_forward(mat), geo::srs_inverse(tf))(
            point(cursor.x(), cursor.y()));
    };
}

}  // namespace

map_view::map_view(QWidget* parent)
    : QWidget(parent)
    , cache_(std::make_shared<boat::gui::caches::lru>(1024))
    , tasks_{1}
{
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

map_view::~map_view() = default;

void map_view::leaveEvent(QEvent*)
{
    if (auto mw = qobject_cast<QMainWindow*>(window()))
        mw->statusBar()->clearMessage();
}

void map_view::mouseMoveEvent(QMouseEvent* event)
{
    auto pos = event->pos();
    auto guard = qScopeGuard([&] { update_status(pos); });
    if (!panning_pos_)
        return;
    auto delta = pos - *std::exchange(panning_pos_, pos);
    auto tf = geo::transformation(geo::ortho(map_.center));
    auto fwd = geo::transform(geo::srs_forward(tf));
    auto xy = fwd(map_.center);
    if (!xy)
        return;
    auto mat = affine(width(), height(), map_, fwd);
    auto cursor =
        geo::transform(geo::mat_forward(mat))(point(pos.x(), pos.y()));
    auto pole_y = boat::numbers::earth::mean_radius *
                  std::cos(map_.center.y() * boat::numbers::degree);
    auto flip =
        cursor && std::copysign(1., map_.center.y()) * cursor->y() > pole_y;
    auto dx = (flip ? delta.x() : -delta.x()) * map_.resolution;
    auto dy = delta.y() * map_.resolution;
    auto ll =
        geo::transform(geo::srs_inverse(tf))(point(xy->x() + dx, xy->y() + dy));
    if (!ll)
        return;
    map_ = bounded({*ll, map_.resolution}).value_or(map_);
    schedule_paint();
}

void map_view::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
        return;
    panning_pos_ = event->pos();
    setCursor(Qt::ClosedHandCursor);
}

void map_view::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
        return;
    panning_pos_.reset();
    setCursor(tasks_.busy() ? Qt::BusyCursor : Qt::ArrowCursor);
}

void map_view::paintEvent(QPaintEvent*)
{
    auto art = QPainter{this};
    art.fillRect(rect(), Qt::white);
    if (img_.isNull())
        return;
    if (boost::geometry::equals(img_view_.center, map_.center) &&
        img_view_.resolution == map_.resolution && img_.size() == size())
        return art.drawImage(0, 0, img_);
    auto img_crs = geo::ortho(img_view_.center);
    auto img_mat = affine(img_.width(), img_.height(), img_view_, img_crs);
    auto map_crs = geo::ortho(map_.center);
    auto map_mat = affine(width(), height(), map_, map_crs);
    auto bits = reinterpret_cast<boost::gil::rgba8_pixel_t const*>(img_.bits());
    auto gil = boost::gil::interleaved_view(
        img_.width(), img_.height(), bits, img_.bytesPerLine());
    boat::gui::draw_image(
        std::execution::par, gil, img_mat, img_crs, art, map_mat, map_crs);
}

void map_view::schedule_paint()
{
    img_timer_.start(350, this);
    if (!map_timer_.isActive())
        map_timer_.start(75, this);
}

void map_view::set_layers(std::vector<leaf> layers)
{
    layers_ = std::move(layers);
    redraw();
}

void map_view::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == map_timer_.timerId()) {
        map_timer_.stop();
        update();
    }
    else if (event->timerId() == img_timer_.timerId()) {
        img_timer_.stop();
        redraw();
    }
    else
        QWidget::timerEvent(event);
}

QString map_view::coordinate_text(QPointF cursor) const
{
    if (auto ll = unproject(view(), cursor)(map_.resolution))
        return QString::asprintf("lon: %.6f, lat: %.6f", ll->x(), ll->y());
    return {};
}

void map_view::update_status(QPointF cursor)
{
    if (auto mw = qobject_cast<QMainWindow*>(window()))
        mw->statusBar()->showMessage(coordinate_text(cursor));
}

viewport map_view::view() const
{
    return {map_, width(), height()};
}

void map_view::watch_task(QFuture<void> fut)
{
    if (!panning_pos_)
        setCursor(Qt::BusyCursor);
    fut.then(this, [this] {
        if (!panning_pos_)
            setCursor(tasks_.busy() ? Qt::BusyCursor : Qt::ArrowCursor);
    });
}

void map_view::set_view(boat::gui::view const& v)
{
    auto valid = std::abs(v.center.x()) <= longitude_max &&
                 std::abs(v.center.y()) <= latitude_max &&
                 v.resolution >= resolution_min &&
                 v.resolution <= resolution_max;
    map_ = valid ? v : boat::gui::view{};
    redraw();
}

std::optional<boat::gui::view> map_view::bounded(boat::gui::view v)
{
    for (auto x : {v.center.x(), v.center.y(), v.resolution})
        if (!std::isfinite(x))
            return {};
    v.center = geo::wrap(
        {v.center.x(), std::clamp(v.center.y(), -latitude_max, latitude_max)});
    v.resolution = std::clamp(v.resolution, resolution_min, resolution_max);
    return v;
}

void map_view::wheelEvent(QWheelEvent* event)
{
    auto pos = event->position();
    auto factor = std::pow(2., event->angleDelta().y() / 8. / 60.);
    auto next = bounded({map_.center, map_.resolution / factor}).value_or(map_);
    auto ll = unproject(view(), pos);
    auto a = ll(map_.resolution), b = ll(next.resolution);
    if (a && b)
        next.center = {
            map_.center.x() + a->x() - b->x(),
            map_.center.y() + a->y() - b->y()};
    map_ = bounded(next).value_or(map_);
    schedule_paint();
    update_status(pos);
}
