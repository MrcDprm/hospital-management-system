#pragma once

#include "core/Models.h"

#include <QDialog>

class Database;
class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QListWidget;

// Randevu verme: hasta, bölüm, doktor ve tarih seçilir; doktorun o günkü saatleri düğme olarak listelenir.
// Dolu, geçmiş ve öğle arası saatler seçilemez. Takvimden açılınca doktor ve saat hazır gelir.
class BookingDialog : public QDialog
{
    Q_OBJECT

public:
    BookingDialog(Database &db, const User &actor, QWidget *parent, qint64 patientId = 0, qint64 doctorId = 0,
                  const QDateTime &start = QDateTime());

private:
    void loadPatients(qint64 select);
    void loadDoctors();
    void loadSlots();
    void addPatient();
    void save();

    Database &m_db;
    const User &m_actor;
    QDateTime m_preselect;
    QComboBox *m_patient, *m_department, *m_doctor;
    QDateEdit *m_date;
    QListWidget *m_slots;
    QLineEdit *m_note;
    QLabel *m_summary, *m_error;
};
