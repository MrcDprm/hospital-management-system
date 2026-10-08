#include "data/AppointmentRepository.h"

#include "core/Schedule.h"
#include "data/SqlUtil.h"
#include "data/StaffRepository.h"

namespace {

const QString COLUMNS = "id, patient_id, doctor_id, start, end, status, note";
constexpr int MAX_NOTE = 500;

Appointment fromQuery(const QSqlQuery &q)
{
    Appointment a;
    a.id = q.value(0).toLongLong();
    a.patientId = q.value(1).toLongLong();
    a.doctorId = q.value(2).toLongLong();
    a.start = Sql::toDateTime(q.value(3));
    a.end = Sql::toDateTime(q.value(4));
    a.status = Sql::toEnum(q.value(5), AppointmentStatus::Cancelled, AppointmentStatus::Booked);
    a.note = q.value(6).toString();
    return a;
}

} // namespace

QList<Appointment> AppointmentRepository::query(const QString &where, const QVariantList &values) const
{
    QList<Appointment> list;
    QSqlQuery q(m_db.connection());
    if (Sql::run(q, "SELECT " + COLUMNS + " FROM appointments WHERE " + where + " ORDER BY start", values))
        while (q.next())
            list << fromQuery(q);
    return list;
}

QList<Appointment> AppointmentRepository::list(const User &actor, const QDate &from, const QDate &to,
                                                qint64 doctorId) const
{
    if (!Sql::can(actor, Permission::ViewAllSchedules)) {
        if (actor.role != Role::Doctor || !actor.active)
            return {};
        doctorId = actor.doctorId; // doktor başkasının listesini isteyemez
    }
    // Tarih metin olarak saklanıyor; ISO biçimi sıralanabildiği için metin karşılaştırması doğru çalışır
    return query("start >= ? AND start < ? AND (? = 0 OR doctor_id = ?)",
                 {Sql::date(from), Sql::date(to), doctorId, doctorId});
}

QList<Appointment> AppointmentRepository::forPatient(qint64 patientId) const
{
    return query("patient_id = ?", {patientId});
}

QList<Appointment> AppointmentRepository::forDoctorOn(qint64 doctorId, const QDate &date) const
{
    return query("doctor_id = ? AND start >= ? AND start < ?", {doctorId, Sql::date(date), Sql::date(date.addDays(1))});
}

std::optional<Appointment> AppointmentRepository::find(qint64 id) const
{
    const QList<Appointment> found = query("id = ?", {id});
    return found.isEmpty() ? std::nullopt : std::optional<Appointment>(found.first());
}

Result AppointmentRepository::book(const User &actor, Appointment a, const QDateTime &now)
{
    if (!Sql::can(actor, Permission::ManageAppointments))
        return Result::failure("forbidden");
    if (a.note.size() > MAX_NOTE)
        return Result::failure("text_too_long");
    const std::optional<Doctor> doctor = StaffRepository(m_db).doctor(a.doctorId);
    if (!doctor)
        return Result::failure("not_found");
    QSqlQuery patient(m_db.connection());
    if (!Sql::run(patient, "SELECT 1 FROM patients WHERE id = ?", {a.patientId}) || !patient.next())
        return Result::failure("not_found");

    // Kontrol ve kayıt aynı işlemde: arada başka bir kullanıcı aynı slotu alamaz
    Transaction transaction(m_db.connection());
    const QDate day = a.start.date();
    const QList<Appointment> patientDay =
        query("patient_id = ? AND start >= ? AND start < ?", {a.patientId, Sql::date(day), Sql::date(day.addDays(1))});
    if (const QString error = Schedule::check(*doctor, a.start, forDoctorOn(a.doctorId, day), patientDay, now);
        !error.isEmpty())
        return Result::failure(error);
    a.end = a.start.addSecs(doctor->slotMinutes * 60);
    QSqlQuery q(m_db.connection());
    if (!Sql::run(q, "INSERT INTO appointments (patient_id, doctor_id, start, end, status, note) VALUES (?, ?, ?, ?, ?, ?)",
                  {a.patientId, a.doctorId, Sql::dateTime(a.start), Sql::dateTime(a.end),
                   static_cast<int>(AppointmentStatus::Booked), text(a.note.trimmed())})
        || !transaction.commit())
        return Result::failure("save_failed");
    return {QString(), q.lastInsertId().toLongLong()};
}

bool AppointmentRepository::ownsOrManages(const User &actor, const Appointment &a) const
{
    return Sql::can(actor, Permission::ManageAppointments)
           || (actor.active && actor.role == Role::Doctor && actor.doctorId == a.doctorId);
}

Result AppointmentRepository::changeStatus(qint64 id, AppointmentStatus from, AppointmentStatus to)
{
    // Durum sadece beklenen durumdaysa değişir: aynı anda iki kişi işlem yaparsa ikincisi başarısız olur
    QSqlQuery q(m_db.connection());
    if (!Sql::run(q, "UPDATE appointments SET status = ? WHERE id = ? AND status = ?",
                  {static_cast<int>(to), id, static_cast<int>(from)})
        || q.numRowsAffected() != 1)
        return Result::failure("invalid_state");
    return {QString(), id};
}

Result AppointmentRepository::checkIn(const User &actor, qint64 id, const QDateTime &now)
{
    const std::optional<Appointment> a = find(id);
    if (!a)
        return Result::failure("not_found");
    if (!ownsOrManages(actor, *a))
        return Result::failure("forbidden");
    if (a->start.date() != now.date())
        return Result::failure("not_today"); // sadece randevu günü "geldi" işaretlenir
    return changeStatus(id, AppointmentStatus::Booked, AppointmentStatus::CheckedIn);
}

Result AppointmentRepository::markNoShow(const User &actor, qint64 id, const QDateTime &now)
{
    const std::optional<Appointment> a = find(id);
    if (!a)
        return Result::failure("not_found");
    if (!ownsOrManages(actor, *a))
        return Result::failure("forbidden");
    if (a->start > now)
        return Result::failure("not_started_yet"); // saati gelmeden "gelmedi" denemez
    return changeStatus(id, a->status == AppointmentStatus::CheckedIn ? AppointmentStatus::CheckedIn
                                                                       : AppointmentStatus::Booked,
                        AppointmentStatus::NoShow);
}

Result AppointmentRepository::cancel(const User &actor, qint64 id, const QDateTime &now)
{
    if (!Sql::can(actor, Permission::ManageAppointments))
        return Result::failure("forbidden");
    const std::optional<Appointment> a = find(id);
    if (!a)
        return Result::failure("not_found");
    if (a->start <= now)
        return Result::failure("already_started"); // geçmiş randevu iptal edilmez, "gelmedi" işaretlenir
    return changeStatus(id, AppointmentStatus::Booked, AppointmentStatus::Cancelled);
}
