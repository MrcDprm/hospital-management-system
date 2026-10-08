#include "data/ExaminationRepository.h"

#include "core/Rules.h"
#include "data/AppointmentRepository.h"
#include "data/SqlUtil.h"

namespace {

const QString COLUMNS = "id, appointment_id, patient_id, doctor_id, date, complaint, findings, diagnosis_code, "
                        "diagnosis_name, notes";

} // namespace

QList<Examination> ExaminationRepository::query(const QString &where, const QVariantList &values) const
{
    QList<Examination> list;
    QSqlQuery q(m_db.connection());
    if (!Sql::run(q, "SELECT " + COLUMNS + " FROM examinations WHERE " + where + " ORDER BY date DESC", values))
        return list;
    while (q.next()) {
        Examination e;
        e.id = q.value(0).toLongLong();
        e.appointmentId = q.value(1).toLongLong();
        e.patientId = q.value(2).toLongLong();
        e.doctorId = q.value(3).toLongLong();
        e.date = Sql::toDateTime(q.value(4));
        e.complaint = q.value(5).toString();
        e.findings = q.value(6).toString();
        e.diagnosisCode = q.value(7).toString();
        e.diagnosisName = q.value(8).toString();
        e.notes = q.value(9).toString();
        list << e;
    }
    // Reçete kalemleri ayrı tablodan, sırasıyla
    for (Examination &e : list) {
        QSqlQuery items(m_db.connection());
        if (Sql::run(items, "SELECT medicine, dose, usage, days FROM prescription_items WHERE examination_id = ? "
                            "ORDER BY position", {e.id}))
            while (items.next())
                e.prescription << PrescriptionItem{items.value(0).toString(), items.value(1).toString(),
                                                   items.value(2).toString(), items.value(3).toInt()};
    }
    return list;
}

std::optional<Examination> ExaminationRepository::forAppointment(const User &actor, qint64 appointmentId) const
{
    if (!Sql::can(actor, Permission::ViewMedicalRecords))
        return std::nullopt;
    const QList<Examination> found = query("appointment_id = ?", {appointmentId});
    return found.isEmpty() ? std::nullopt : std::optional<Examination>(found.first());
}

QList<Examination> ExaminationRepository::forPatient(const User &actor, qint64 patientId) const
{
    if (!Sql::can(actor, Permission::ViewMedicalRecords))
        return {};
    return query("patient_id = ?", {patientId});
}

Result ExaminationRepository::save(const User &actor, Examination e, const QDateTime &now)
{
    if (!Sql::can(actor, Permission::WriteExaminations))
        return Result::failure("forbidden");
    const std::optional<Appointment> a = AppointmentRepository(m_db).find(e.appointmentId);
    if (!a)
        return Result::failure("not_found");
    if (a->doctorId != actor.doctorId)
        return Result::failure("forbidden"); // başka doktorun hastasına muayene yazılamaz
    if (a->status == AppointmentStatus::Cancelled || a->status == AppointmentStatus::NoShow)
        return Result::failure("invalid_state");
    if (a->start.date() > now.date())
        return Result::failure("not_started_yet");

    e.complaint = e.complaint.trimmed();
    e.findings = e.findings.trimmed();
    e.diagnosisCode = e.diagnosisCode.trimmed().toUpper();
    e.diagnosisName = e.diagnosisName.trimmed();
    e.notes = e.notes.trimmed();
    QList<PrescriptionItem> items;
    for (PrescriptionItem item : e.prescription) {
        item.medicine = item.medicine.trimmed();
        if (item.medicine.isEmpty() && item.dose.trimmed().isEmpty() && item.usage.trimmed().isEmpty())
            continue; // tamamen boş satır atlanır
        item.dose = item.dose.trimmed();
        item.usage = item.usage.trimmed();
        items << item;
    }
    e.prescription = items;
    if (const QStringList problems = Rules::examinationProblems(e); !problems.isEmpty())
        return Result::failure(problems.first());

    Transaction transaction(m_db.connection());
    const std::optional<Examination> existing = forAppointment(actor, e.appointmentId);
    QSqlQuery q(m_db.connection());
    const QVariantList fields = {text(e.complaint), text(e.findings), text(e.diagnosisCode), e.diagnosisName,
                                 text(e.notes)};
    qint64 id = 0;
    if (existing) {
        id = existing->id;
        if (!Sql::run(q, "UPDATE examinations SET complaint = ?, findings = ?, diagnosis_code = ?, diagnosis_name = ?, "
                         "notes = ? WHERE id = ?", fields + QVariantList{id}))
            return Result::failure("save_failed");
    } else {
        if (!Sql::run(q, "INSERT INTO examinations (appointment_id, patient_id, doctor_id, date, complaint, findings, "
                         "diagnosis_code, diagnosis_name, notes) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
                      QVariantList{a->id, a->patientId, a->doctorId, Sql::dateTime(now)} + fields))
            return Result::failure("save_failed");
        id = q.lastInsertId().toLongLong();
    }
    // Reçete baştan yazılır: silinen, eklenen ve sırası değişen kalemler için en basit doğru yol
    QSqlQuery clear(m_db.connection());
    if (!Sql::run(clear, "DELETE FROM prescription_items WHERE examination_id = ?", {id}))
        return Result::failure("save_failed");
    for (int i = 0; i < e.prescription.size(); ++i) {
        const PrescriptionItem &item = e.prescription[i];
        QSqlQuery insert(m_db.connection());
        if (!Sql::run(insert, "INSERT INTO prescription_items (examination_id, position, medicine, dose, usage, days) "
                              "VALUES (?, ?, ?, ?, ?, ?)", {id, i, item.medicine, text(item.dose), text(item.usage), item.days}))
            return Result::failure("save_failed");
    }
    QSqlQuery status(m_db.connection());
    if (!Sql::run(status, "UPDATE appointments SET status = ? WHERE id = ?",
                  {static_cast<int>(AppointmentStatus::Examined), a->id})
        || !transaction.commit())
        return Result::failure("save_failed");
    return {QString(), id};
}
