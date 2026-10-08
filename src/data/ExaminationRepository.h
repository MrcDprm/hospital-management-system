#pragma once

#include "core/Models.h"
#include "data/Database.h"

#include <optional>

// Muayene ve reçeteler (tıbbi kayıt). Sadece doktor okur ve yazar; doktor sadece kendi randevusuna muayene yazar.
// Muayene kaydedilince randevu "muayene edildi" olur; muayene ve reçete aynı işlemde yazılır.
class ExaminationRepository
{
public:
    explicit ExaminationRepository(Database &db) : m_db(db) {}

    std::optional<Examination> forAppointment(const User &actor, qint64 appointmentId) const;
    QList<Examination> forPatient(const User &actor, qint64 patientId) const; // en yenisi önce
    Result save(const User &actor, Examination exam, const QDateTime &now = QDateTime::currentDateTime());

private:
    QList<Examination> query(const QString &where, const QVariantList &values) const;

    Database &m_db;
};
