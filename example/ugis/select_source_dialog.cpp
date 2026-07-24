// Andrew Naplavkov

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFont>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <boat/address.hpp>
#include <boat/sql/commands.hpp>
#include "formats.h"
#include "select_source_dialog.h"

namespace {

struct preset_address {
    QStringList gdal;
    QStringList slippy;
    QStringList sql;
};

preset_address make_preset_address()
{
    auto all = QStringList{
        boat::config::mysql_address.data(),
        boat::config::mysql_gdal_address.data(),
        boat::config::postgres_address.data(),
        boat::config::postgres_gdal_address.data(),
        "http://ugis@basemaps.cartocdn.com/light_all/{z}/{x}/{y}.png",
        "http://ugis@mt.google.com/vt/lyrs=s&z={z}&x={x}&y={y}?zmax=19",
        "https://ugis@tile.openstreetmap.org/{z}/{x}/{y}.png",
        "sqlite:///C:/home/gis_data/sqlite/california_roads.sqlite",
        "wms:https://wms.gebco.net/mapserv?request=GetCapabilities&service=WMS",
        "/vsicurl/https://download.osgeo.org/gdal/data/gtiff/small_world.tif",
    };
    for (auto adr : boat::config::odbc_address())
        all.push_back(adr.data());
    if (auto adr = boat::config::mssql_gdal_address(); !adr.empty())
        all.push_back(adr.data());
    auto ret = preset_address{};
    for (auto& adr : all) {
        auto std_adr = adr.toStdString();
        if (std_adr.starts_with("http://") || std_adr.starts_with("https://"))
            ret.slippy.push_back(adr);
        else if (boat::sql::supported_url(std_adr))
            ret.sql.push_back(adr);
        else
            ret.gdal.push_back(adr);
    }
    ret.gdal.sort();
    ret.slippy.sort();
    ret.sql.sort();
    return ret;
}

void add_group(QComboBox* box, QString const& header, QStringList const& items)
{
    if (items.isEmpty())
        return;
    box->insertSeparator(box->count());
    box->addItem(header);
    auto sep =
        qobject_cast<QStandardItemModel*>(box->model())->item(box->count() - 1);
    auto font = box->font();
    font.setBold(true);
    sep->setFont(font);
    sep->setFlags(Qt::NoItemFlags);
    for (auto& item : items)
        box->addItem(item);
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
    auto preset = make_preset_address();
    add_group(address_, "gdal", preset.gdal);
    add_group(address_, "slippy", preset.slippy);
    add_group(address_, "sql", preset.sql);
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
    auto path = QFileDialog::getOpenFileName(
        this, {}, {}, open_filter(), nullptr, QFileDialog::DontUseNativeDialog);
    if (!path.isEmpty())
        address_->setEditText(path);
}

void select_source_dialog::update_ok()
{
    ok_->setEnabled(!name_->text().trimmed().isEmpty() &&
                    !address_->currentText().trimmed().isEmpty());
}
