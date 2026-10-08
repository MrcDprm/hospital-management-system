#pragma once

#include "core/Models.h"
#include "data/Database.h"

#include <optional>

// Hasta kayıtları. Herkes listeyi görebilir; ekleme ve düzenleme yönetici ve sekreterde.
class PatientRepository
{
public:
    explicit PatientRepository(Database &db) : m_db(db) {}

    QList<Patient> all() const;
    std::optional<Patient> find(qint64 id) const;
    std::optional<Patient> findByNationalId(const QString &nationalId) const;
    Result add(const User &actor, Patient patient, const QDate &today = QDate::currentDate());
    Result update(const User &actor, Patient patient, const QDate &today = QDate::currentDate());
    Result remove(const User &actor, qint64 id); // randevusu olan hasta silinmez

private:
    Result save(const User &actor, Patient patient, const QDate &today, bool insert);

    Database &m_db;
};
