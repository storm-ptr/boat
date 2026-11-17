// Andrew Naplavkov

#include <QMainWindow>
#include <QMouseEvent>
#include <QStatusBar>
#include <QWheelEvent>
#include <boat/gui/caches/lru.hpp>
#include <boat/gui/qt.hpp>
#include "map_view.h"
#include "tree.h"

namespace geo = boat::geometry;
using point = geo::geographic::point;

namespace {

constexpr auto lon_max = 180.;
constexpr auto lat_max = 85.;
constexpr auto res_min = 0.5972;
constexpr auto res_max = 19'567.88;

auto unproject(viewport const& vp, QPointF cursor)
{
    return [=, tf = geo::transformation(geo::ortho(vp.view.center))](auto res) {
        auto mat = geo::affine(vp.extent, vp.view.center, res, tf);
        return geo::transform(geo::forward(mat), geo::backward(tf))(
            point(cursor.x(), cursor.y()));
    };
}

}  // namespace

map_view::map_view(QWidget* parent)
    : QWidget(parent)
    , cache_(std::make_shared<boat::gui::caches::lru>(2'000))
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
    auto const& [mid, res] = map_;
    auto pos = event->pos();
    auto guard = qScopeGuard([&] { update_status(pos); });
    if (!panning_pos_)
        return;
    auto delta = pos - *std::exchange(panning_pos_, pos);
    auto tf = geo::transformation(geo::ortho(mid));
    auto fwd = geo::transform(geo::forward(tf));
    auto xy = fwd(mid);
    if (!xy)
        return;
    auto mat = geo::affine({width(), height()}, mid, res, fwd);
    auto cursor = geo::transform(geo::forward(mat))(point(pos.x(), pos.y()));
    auto pole_y = boat::numbers::earth::mean_radius *
                  std::cos(mid.y() * boat::numbers::degree);
    auto flip = cursor && std::copysign(1., mid.y()) * cursor->y() > pole_y;
    auto dx = (flip ? delta.x() : -delta.x()) * res;
    auto dy = delta.y() * res;
    auto ll =
        geo::transform(geo::backward(tf))(point(xy->x() + dx, xy->y() + dy));
    if (!ll)
        return;
    map_ = bounded({*ll, res}).value_or(map_);
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
    auto const& [mid, res] = map_;
    auto const& [img_mid, img_res] = img_view_;
    auto art = QPainter{this};
    art.fillRect(rect(), Qt::white);
    if (img_.isNull())
        return;
    if (boost::geometry::equals(img_mid, mid) && img_res == res &&
        img_.size() == size())
        return art.drawImage(0, 0, img_);
    auto img_crs = geo::ortho(img_mid);
    auto img_mat =
        geo::affine({img_.width(), img_.height()}, img_mid, img_res, img_crs);
    auto map_crs = geo::ortho(mid);
    auto map_mat = geo::affine({width(), height()}, mid, res, map_crs);
    auto bits = img_.constBits();
    auto gil = boost::gil::interleaved_view(
        img_.width(),
        img_.height(),
        boost::gil::make_step_iterator(
            boost::gil::planar_pixel_iterator<uchar const*, boost::gil::rgba_t>{
                bits, bits + 1, bits + 2, bits + 3},
            4),
        img_.bytesPerLine());
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
    return {map_, {width(), height()}};
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
    auto valid = std::abs(v.center.x()) <= lon_max &&
                 std::abs(v.center.y()) <= lat_max && v.resolution >= res_min &&
                 v.resolution <= res_max;
    map_ = valid ? v : boat::gui::view{};
    redraw();
}

std::optional<boat::gui::view> map_view::bounded(boat::gui::view v)
{
    for (auto x : {v.center.x(), v.center.y(), v.resolution})
        if (!std::isfinite(x))
            return {};
    v.center =
        geo::wrap({v.center.x(), std::clamp(v.center.y(), -lat_max, lat_max)});
    v.resolution = std::clamp(v.resolution, res_min, res_max);
    return v;
}

void map_view::wheelEvent(QWheelEvent* event)
{
    auto const& [mid, res] = map_;
    auto pos = event->position();
    auto factor = std::pow(2., event->angleDelta().y() / 8. / 60.);
    auto next = bounded({mid, res / factor}).value_or(map_);
    auto ll = unproject(view(), pos);
    auto a = ll(res), b = ll(next.resolution);
    if (a && b)
        next.center = {mid.x() + a->x() - b->x(), mid.y() + a->y() - b->y()};
    map_ = bounded(next).value_or(map_);
    schedule_paint();
    update_status(pos);
}
