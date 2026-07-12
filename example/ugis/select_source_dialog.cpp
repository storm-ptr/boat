// Andrew Naplavkov

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <boat/config.hpp>
#include "formats.h"
#include "select_source_dialog.h"

namespace {

QStringList preset_address()
{
    auto ret = QStringList{
        boat::config::mysql_address.data(),
        boat::config::mysql_gdal_address.data(),
        boat::config::postgresql_address.data(),
        boat::config::postgresql_gdal_address.data(),
        "http://ugis@basemaps.cartocdn.com/light_all/{z}/{x}/{y}.png",
        "http://ugis@mt.google.com/vt/lyrs=s&z={z}&x={x}&y={y}?zmax=19",
        "https://ugis@tile.openstreetmap.org/{z}/{x}/{y}.png",
        "sqlite:///C:/home/gis_data/sqlite/Mexico.sqlite",
        "wms:https://wms.gebco.net/mapserv?request=GetCapabilities&service=WMS",
        "/vsicurl/https://download.osgeo.org/gdal/data/gtiff/small_world.tif",
    };
    for (auto adr : boat::config::odbc_address())
        ret.push_back(adr.data());
    if (auto adr = boat::config::mssql_gdal_address(); !adr.empty())
        ret.push_back(adr.data());
    ret.sort();
    return ret;
}

}  // namespace

select_source_dialog::select_source_dialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("select source");
    resize(640, 0);
    name_ = new QLineEdit{this};
    address_ = new QComboBox{this};
    address_->setEditable(true);
    address_->setInsertPolicy(QComboBox::NoInsert);
    address_->addItems(preset_address());
    address_->setCurrentIndex(-1);
    address_->clearEditText();
    address_->setSizeAdjustPolicy(
        QComboBox::AdjustToMinimumContentsLengthWithIcon);
    address_->setMinimumContentsLength(60);
    auto browse = new QPushButton{"...", this};
    connect(browse, &QPushButton::clicked, this, [this] { browse_file(); });
    auto address_row = new QHBoxLayout{};
    address_row->addWidget(address_, 1);
    address_row->addWidget(browse);
    auto form = new QFormLayout{};
    form->addRow("name", name_);
    form->addRow("address", address_row);
    auto buttons = new QDialogButtonBox{
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this};
    ok_ = buttons->button(QDialogButtonBox::Ok);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(name_, &QLineEdit::textChanged, this, [this] { update_ok(); });
    connect(address_->lineEdit(), &QLineEdit::textChanged, this, [this] {
        update_ok();
    });
    auto layout = new QVBoxLayout{this};
    layout->addLayout(form);
    layout->addWidget(buttons);
    update_ok();
}

QString select_source_dialog::name() const
{
    return name_->text();
}

QString select_source_dialog::address() const
{
    return address_->currentText();
}

void select_source_dialog::browse_file()
{
    auto path = QFileDialog::getOpenFileName(this, {}, {}, open_filter());
    if (!path.isEmpty())
        address_->setEditText(path);
}

void select_source_dialog::update_ok()
{
    ok_->setEnabled(!name_->text().trimmed().isEmpty() &&
                    !address_->currentText().trimmed().isEmpty());
}
