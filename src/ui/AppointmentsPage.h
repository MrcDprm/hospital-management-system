#pragma once

#include "core/Models.h"

#include <QHash>
#include <QWidget>

class Database;
class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

// Günlük randevu listesi (açılışta bugün): özet kartları, liste ve randevu işlemleri.
// Doktor sadece kendi randevularını görür; sekreter ve yönetici bütün doktorları.
class AppointmentsPage : public QWidget
{
    Q_OBJECT

public:
    AppointmentsPage(Database &db, const User &user, QWidget *parent);

public slots:
    void refresh();

private:
    QWidget *summaryCard(const char *key, QLabel *&value);
    void fill();
    void updateButtons();
    Appointment selected() const;
    void act(int action);
    void book();

    Database &m_db;
    const User &m_user;
    QList<Appointment> m_appointments;
    QHash<qint64, Patient> m_patients;
    QHash<qint64, Doctor> m_doctors;
    QHash<qint64, QString> m_departments;
    QDateEdit *m_date;
    QComboBox *m_doctor;
    QLineEdit *m_search;
    QLabel *m_total, *m_waiting, *m_checkedIn, *m_examined, *m_noShow;
    QTableWidget *m_table;
    QList<QPushButton *> m_actionButtons; // AppointmentActions::Action sırasıyla
};
