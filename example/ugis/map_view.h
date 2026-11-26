// Andrew Naplavkov

#ifndef MAP_VIEW_H
#define MAP_VIEW_H

#include <QBasicTimer>
#include <QWidget>
#include <boat/gui/caches/cache.hpp>
#include "task_group.h"
#include "viewport.h"

struct feature;
struct leaf;

class map_view : public QWidget {
public:
    explicit map_view(QWidget* parent = nullptr);
    ~map_view() override;
    QWidget* panel() const;
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
    static constexpr auto query_limit = 512;
    static constexpr auto feature_limit = 10;

    static std::optional<boat::gui::view> bounded(boat::gui::view);
    void clear_identify();
    QString coordinate_text(QPointF cursor) const;
    void draw_highlight(QPainter&) const;
    void identify(QPointF cursor, viewport);
    void init_panel();
    void redraw();
    void schedule_paint();
    void show_hits(std::vector<feature>);
    void update_cursor();
    void update_status(QPointF cursor);
    void watch_task(QFuture<void>);

    std::shared_ptr<boat::gui::caches::cache> cache_;
    boat::gui::caches::cache_stats last_cache_report_;

    QImage img_;
    boat::gui::view img_view_;
    QBasicTimer img_timer_;

    std::vector<leaf> layers_;

    boat::gui::view map_;
    QBasicTimer map_timer_;

    std::optional<QPoint> panning_pos_;
    struct selection;
    selection* panel_{};
    task_group query_tasks_{1};
    task_group render_tasks_{1};
};

#endif  // MAP_VIEW_H
