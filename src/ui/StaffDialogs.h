#pragma once

#include "core/Models.h"

#include <QDialog>

class Database;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QTimeEdit;

// Doktor formu: unvan, ad, bölüm, iletişim, çalışma günleri ve saatleri, randevu süresi.
class DoctorDialog : public QDialog
{
    Q_OBJECT

public:
    DoctorDialog(Database &db, const User &actor, const Doctor &doctor, QWidget *parent);

private:
    void save();

    Database &m_db;
    const User &m_actor;
    Doctor m_doctor;
    QComboBox *m_title, *m_department, *m_slot;
    QLineEdit *m_name, *m_phone, *m_email;
    QList<QCheckBox *> m_days;
    QTimeEdit *m_start, *m_end;
    QCheckBox *m_active;
    QLabel *m_error;
};

// Bölüm formu: ad ve yer (blok, kat).
class DepartmentDialog : public QDialog
{
    Q_OBJECT

public:
    DepartmentDialog(Database &db, const User &actor, const Department &department, QWidget *parent);

private:
    void save();

    Database &m_db;
    const User &m_actor;
    Department m_department;
    QLineEdit *m_name, *m_location;
    QLabel *m_error;
};
