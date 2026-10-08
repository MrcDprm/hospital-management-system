#pragma once

#include "core/Models.h"

#include <QWidget>

class Database;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

// Hasta listesi: arama, ekleme, düzenleme, silme, randevu verme ve hasta geçmişi.
class PatientsPage : public QWidget
{
    Q_OBJECT

public:
    PatientsPage(Database &db, const User &user, QWidget *parent);

public slots:
    void refresh();

private:
    void fill();
    void updateButtons();
    Patient selected() const;
    void edit(const Patient &patient);
    void remove();
    void exportCsv();

    Database &m_db;
    const User &m_user;
    QList<Patient> m_patients;
    QLineEdit *m_search;
    QLabel *m_count;
    QTableWidget *m_table;
    QPushButton *m_edit, *m_delete, *m_book, *m_history;
};
