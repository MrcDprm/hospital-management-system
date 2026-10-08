#pragma once

#include "core/Models.h"
#include "data/Database.h"

#include <optional>

// Randevular. Verme ve iptal yönetici/sekreterde; doktor sadece kendi randevularını görür.
// Çakışma kontrolü ve kayıt aynı veritabanı işleminde: aynı slota iki randevu yazılamaz.
class AppointmentRepository
{
public:
    explicit AppointmentRepository(Database &db) : m_db(db) {}

    // [from, to) aralığı; doctorId 0 ise hepsi. Yetkisi olmayan doktor için sadece kendi randevuları döner.
    QList<Appointment> list(const User &actor, const QDate &from, const QDate &to, qint64 doctorId = 0) const;
    QList<Appointment> forPatient(qint64 patientId) const;
    QList<Appointment> forDoctorOn(qint64 doctorId, const QDate &date) const;
    std::optional<Appointment> find(qint64 id) const;

    Result book(const User &actor, Appointment appointment, const QDateTime &now = QDateTime::currentDateTime());
    Result checkIn(const User &actor, qint64 id, const QDateTime &now = QDateTime::currentDateTime());
    Result markNoShow(const User &actor, qint64 id, const QDateTime &now = QDateTime::currentDateTime());
    Result cancel(const User &actor, qint64 id, const QDateTime &now = QDateTime::currentDateTime());

private:
    QList<Appointment> query(const QString &where, const QVariantList &values) const;
    Result changeStatus(qint64 id, AppointmentStatus from, AppointmentStatus to);
    bool ownsOrManages(const User &actor, const Appointment &appointment) const;

    Database &m_db;
};
