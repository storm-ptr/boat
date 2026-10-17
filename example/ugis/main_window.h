// Andrew Naplavkov

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

class QPlainTextEdit;

class main_window : public QMainWindow {
public:
    explicit main_window(QString const& scene_path = {});
    ~main_window();

private:
    QPlainTextEdit* log_;
};

#endif  // MAIN_WINDOW_H
