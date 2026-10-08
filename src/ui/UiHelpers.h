#pragma once

#include "app/I18n.h"
#include "app/Theme.h"
#include "core/Models.h"
#include "data/Database.h"

#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>

// Sayfaların ortak küçük yardımcıları: tablo kurulumu, satır ekleme, düğmeler, soru ve uyarı kutuları.
namespace Ui {

inline QTableWidget *makeTable(const QStringList &headers, QWidget *parent)
{
    auto *table = new QTableWidget(0, headers.size(), parent);
    table->setHorizontalHeaderLabels(headers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers); // tablo sadece gösterir; düzenleme formdan
    table->verticalHeader()->hide();
    table->verticalHeader()->setDefaultSectionSize(34);
    table->setShowGrid(false);
    table->setAlternatingRowColors(true);
    table->setWordWrap(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    return table;
}

// Satıra hücre ekler; ilk hücreye kaydın kimliği gizli veri olarak konur (seçilen satırın hangi kayıt olduğu)
inline void setRow(QTableWidget *table, int row, const QStringList &cells, qint64 id, const QColor &color = QColor())
{
    for (int column = 0; column < cells.size(); ++column) {
        auto *item = new QTableWidgetItem(cells[column]);
        if (column == 0)
            item->setData(Qt::UserRole, id);
        if (color.isValid())
            item->setForeground(color);
        table->setItem(row, column, item);
    }
}

inline int addRow(QTableWidget *table, const QStringList &cells, qint64 id, const QColor &color = QColor())
{
    const int row = table->rowCount();
    table->insertRow(row);
    setRow(table, row, cells, id, color);
    return row;
}

inline qint64 selectedId(const QTableWidget *table)
{
    const QList<QTableWidgetItem *> items = table->selectedItems();
    if (items.isEmpty())
        return 0;
    return table->item(items.first()->row(), 0)->data(Qt::UserRole).toLongLong();
}

inline QPushButton *accentButton(const QString &text, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setProperty("accent", true); // stil sayfasındaki QPushButton[accent="true"] kuralı
    return button;
}

inline QLabel *errorLabel(QWidget *parent)
{
    auto *label = new QLabel(parent);
    label->setWordWrap(true);
    label->setTextFormat(Qt::PlainText);
    label->setStyleSheet("color: " + Theme::danger().name());
    return label;
}

inline QLabel *title(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName("title");
    return label;
}

// Qt'nin hazır düğmeleri çeviri dosyası olmadan İngilizce görünür; düğmeler kendi metinlerimizle kurulur
inline bool ask(QWidget *parent, const QString &text, bool defaultYes = false)
{
    QMessageBox box(QMessageBox::Question, I18n::t("app_name"), text, QMessageBox::NoButton, parent);
    box.setTextFormat(Qt::PlainText); // hasta adı gibi kullanıcı verisi HTML olarak yorumlanmasın
    QPushButton *yes = box.addButton(I18n::t("yes"), QMessageBox::YesRole);
    QPushButton *no = box.addButton(I18n::t("no"), QMessageBox::NoRole);
    box.setDefaultButton(defaultYes ? yes : no);
    box.exec();
    return box.clickedButton() == yes;
}

inline void message(QWidget *parent, QMessageBox::Icon icon, const QString &text)
{
    QMessageBox box(icon, I18n::t("app_name"), text, QMessageBox::NoButton, parent);
    box.setTextFormat(Qt::PlainText);
    box.addButton(I18n::t("ok"), QMessageBox::AcceptRole);
    box.exec();
}

inline void inform(QWidget *parent, const QString &text)
{
    message(parent, QMessageBox::Information, text);
}

inline void warn(QWidget *parent, const QString &text)
{
    message(parent, QMessageBox::Warning, text);
}

inline bool showResult(QWidget *parent, const Result &result)
{
    if (!result.ok())
        warn(parent, I18n::error(result.error));
    return result.ok();
}

// Randevu durumunun rengi: alındı mavi, geldi sarı, muayene edildi yeşil, gelmedi kırmızı, iptal soluk
inline QColor statusColor(AppointmentStatus status)
{
    switch (status) {
    case AppointmentStatus::Booked: return QColor(Theme::isDark() ? "#6cb6ff" : "#005fb8"); // yeşil "muayene"den ayrılsın
    case AppointmentStatus::CheckedIn: return Theme::warning();
    case AppointmentStatus::Examined: return Theme::success();
    case AppointmentStatus::NoShow: return Theme::danger();
    case AppointmentStatus::Cancelled: return Theme::muted();
    }
    return QColor();
}

// T.C. kimlik numarası listede maskelenir (12*******46); tam numara sadece kayıt formunda görünür
inline QString maskId(const QString &id)
{
    return id.size() == 11 ? id.left(2) + QString(7, QChar('*')) + id.right(2) : id;
}

} // namespace Ui
