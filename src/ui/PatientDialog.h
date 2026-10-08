#pragma once

#include "core/Models.h"

#include <QDialog>

class Database;
class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;

// Hasta ekleme ve düzenleme formu. Doğrulama kayıtta (PatientRepository) yapılır; hata formun altında görünür.
class PatientDialog : public QDialog
{
    Q_OBJECT

public:
    PatientDialog(Database &db, const User &actor, const Patient &patient, QWidget *parent);
    qint64 savedId() const { return m_patient.id; }

private:
    void save();

    Database &m_db;
    const User &m_actor;
    Patient m_patient;
    QLineEdit *m_nationalId, *m_name, *m_phone, *m_email, *m_allergies, *m_chronic;
    QDateEdit *m_birth;
    QComboBox *m_gender, *m_blood;
    QLabel *m_error;
};
