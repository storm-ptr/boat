// Andrew Naplavkov

#ifndef SELECT_SOURCE_DIALOG_H
#define SELECT_SOURCE_DIALOG_H

#include <QDialog>

class QComboBox;
class QLineEdit;

class select_source_dialog : public QDialog {
public:
    explicit select_source_dialog(QWidget* parent = nullptr);
    QString name() const;
    QString address() const;

private:
    QLineEdit* name_ = nullptr;
    QComboBox* address_ = nullptr;
};

#endif  // SELECT_SOURCE_DIALOG_H
