// Andrew Naplavkov

#ifndef MAP_VIEW_H
#define MAP_VIEW_H

#include <QBasicTimer>
#include <QImage>
#include <QPoint>
#include <QWidget>
#include <boat/geometry/raster.hpp>
#include <boat/gui/caches/lru.hpp>
#include <boat/gui/provider.hpp>
#include <memory>
#include <optional>
#include "task_group.h"
#include "tree.h"

class QContextMenuEvent;

struct viewport {
    boat::gui::view view;
    int width;
    int height;
};

class map_view : public QWidget {
public:
    explicit map_view(QWidget* parent = nullptr);
    void locate(leaf);
    void set_layers(std::vector<leaf>);
    void set_view(boat::gui::view const&);
    viewport view() const;

protected:
    void contextMenuEvent(QContextMenuEvent*) override;
    void leaveEvent(QEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override { schedule_paint(); }
    void timerEvent(QTimerEvent*) override;
    void wheelEvent(QWheelEvent*) override;

private:
    static std::optional<boat::gui::view> bounded(boat::gui::view);
    void redraw();
    void schedule_paint();
    void update_status(QPointF cursor);
    void watch_task(QFuture<void>);

    std::shared_ptr<boat::gui::caches::lru> cache_;

    QImage img_;
    boat::gui::view img_view_;
    QBasicTimer img_timer_;

    std::vector<leaf> layers_;

    boat::gui::view map_;
    QBasicTimer map_timer_;

    std::optional<QPoint> panning_pos_;
    task_group tasks_;
};

#endif  // MAP_VIEW_H
