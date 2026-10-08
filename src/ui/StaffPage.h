#pragma once

#include "core/Models.h"

#include <QWidget>

class Database;
class QPushButton;
class QTableWidget;

// Bölümler ve doktorlar (sadece yönetici). Solda bölümler, sağda seçili bölümün doktorları.
class StaffPage : public QWidget
{
    Q_OBJECT

public:
    StaffPage(Database &db, const User &user, QWidget *parent);

public slots:
    void refresh();

private:
    void fillDoctors();
    void editDepartment(bool create);
    void removeDepartment();
    void editDoctor(bool create);

    Database &m_db;
    const User &m_user;
    QList<Department> m_departments;
    QList<Doctor> m_doctorList;
    QTableWidget *m_departmentTable, *m_doctorTable;
    QPushButton *m_editDepartment, *m_removeDepartment, *m_editDoctor;
};
