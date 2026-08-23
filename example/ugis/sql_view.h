// Andrew Naplavkov

#ifndef SQL_VIEW_H
#define SQL_VIEW_H

#include <QWidget>
#include <functional>
#include <string>
#include "task_group.h"

class QPlainTextEdit;
class QPushButton;

class sql_view : public QWidget {
public:
    explicit sql_view(std::string address, QWidget* parent = nullptr);
    void close_async();
    void set_close_handler(std::function<void()>);

private:
    void run();
    void watch_task(QFuture<void>);

    std::string address_;
    QPlainTextEdit* query_;
    QPlainTextEdit* output_;
    QPushButton* run_;
    QPushButton* cancel_;
    std::function<void()> close_handler_;
    bool closing_{};
    task_group tasks_{1};
};

#endif  // SQL_VIEW_H
