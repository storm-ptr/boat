// Andrew Naplavkov

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

class QPlainTextEdit;

class main_window : public QMainWindow {
public:
    main_window();
    ~main_window();

private:
    QPlainTextEdit* log_;
};

#endif  // MAIN_WINDOW_H
