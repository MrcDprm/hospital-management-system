#pragma once

#include "core/Models.h"

#include <QHash>
#include <QWidget>

class Database;
class QComboBox;
class QLabel;
class QTableWidget;

// Haftalık takvim: seçili doktorun bir haftası, satırlar saat dilimleri, sütunlar günler.
// Boş saate çift tıklayınca randevu verilir; dolu saatte sağ tık menüsünden işlem yapılır.
class CalendarPage : public QWidget
{
    Q_OBJECT

public:
    CalendarPage(Database &db, const User &user, QWidget *parent);

public slots:
    void refresh();

private:
    void openCell(int row, int column);
    void showMenu(const QPoint &position);

    Database &m_db;
    const User &m_user;
    QDate m_monday;
    QHash<qint64, Appointment> m_appointments;
    QComboBox *m_doctor;
    QLabel *m_week, *m_hint;
    QTableWidget *m_grid;
};
