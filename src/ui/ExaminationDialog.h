#pragma once

#include "core/Models.h"

#include <QDialog>

class Database;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QTableWidget;

// Muayene ekranı (sadece doktor): şikâyet, bulgular, ICD-10 tanısı, notlar ve reçete.
// Kaydedilince randevu "muayene edildi" olur; istenirse reçete PDF olarak kaydedilir.
class ExaminationDialog : public QDialog
{
    Q_OBJECT

public:
    ExaminationDialog(Database &db, const User &actor, const Appointment &appointment, QWidget *parent);

private:
    QWidget *patientCard();
    void addMedicineRow(const PrescriptionItem &item = {});
    Examination collect() const;
    bool save();
    void saveAndPrint();

    Database &m_db;
    const User &m_actor;
    Appointment m_appointment;
    Patient m_patient;
    Examination m_exam;
    QPlainTextEdit *m_complaint, *m_findings, *m_notes;
    QLineEdit *m_code, *m_diagnosis;
    QTableWidget *m_medicines;
    QLabel *m_error;
};

// Reçeteyi PDF olarak kaydetme (muayene ekranı ve hasta geçmişi kullanır)
namespace Prescription {
void exportPdf(QWidget *parent, Database &db, const Examination &exam);
}
