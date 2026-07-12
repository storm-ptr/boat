// Andrew Naplavkov

#ifndef SELECT_SOURCE_DIALOG_H
#define SELECT_SOURCE_DIALOG_H

#include <QDialog>

class QComboBox;
class QLineEdit;
class QPushButton;

class select_source_dialog : public QDialog {
public:
    explicit select_source_dialog(QWidget* parent = nullptr);
    QString name() const;
    QString address() const;

private:
    void browse_file();
    void update_ok();

    QLineEdit* name_ = nullptr;
    QComboBox* address_ = nullptr;
    QPushButton* ok_ = nullptr;
};

#endif  // SELECT_SOURCE_DIALOG_H
