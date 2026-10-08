#include "data/PatientRepository.h"

#include "core/Rules.h"
#include "data/SqlUtil.h"

namespace {

const QString COLUMNS = "id, national_id, full_name, birth_date, gender, phone, email, blood_group, allergies, chronic";

Patient fromQuery(const QSqlQuery &q)
{
    Patient p;
    p.id = q.value(0).toLongLong();
    p.nationalId = q.value(1).toString();
    p.fullName = q.value(2).toString();
    p.birthDate = Sql::toDate(q.value(3));
    p.gender = Sql::toEnum(q.value(4), Gender::Other, Gender::Other);
    p.phone = q.value(5).toString();
    p.email = q.value(6).toString();
    p.bloodGroup = Sql::toEnum(q.value(7), BloodGroup::ZeroNeg, BloodGroup::Unknown);
    p.allergies = q.value(8).toString();
    p.chronicConditions = q.value(9).toString();
    return p;
}

} // namespace

QList<Patient> PatientRepository::all() const
{
    QList<Patient> list;
    QSqlQuery q(m_db.connection());
    if (Sql::run(q, "SELECT " + COLUMNS + " FROM patients ORDER BY full_name"))
        while (q.next())
            list << fromQuery(q);
    return list;
}

std::optional<Patient> PatientRepository::find(qint64 id) const
{
    QSqlQuery q(m_db.connection());
    if (Sql::run(q, "SELECT " + COLUMNS + " FROM patients WHERE id = ?", {id}) && q.next())
        return fromQuery(q);
    return std::nullopt;
}

std::optional<Patient> PatientRepository::findByNationalId(const QString &nationalId) const
{
    QSqlQuery q(m_db.connection());
    if (Sql::run(q, "SELECT " + COLUMNS + " FROM patients WHERE national_id = ?", {nationalId.trimmed()}) && q.next())
        return fromQuery(q);
    return std::nullopt;
}

Result PatientRepository::add(const User &actor, Patient patient, const QDate &today)
{
    return save(actor, patient, today, true);
}

Result PatientRepository::update(const User &actor, Patient patient, const QDate &today)
{
    return save(actor, patient, today, false);
}

Result PatientRepository::save(const User &actor, Patient p, const QDate &today, bool insert)
{
    if (!Sql::can(actor, Permission::ManagePatients))
        return Result::failure("forbidden");
    // Kayıttan önce temizlenir: fazla boşluklar, telefon biçimi, e-posta harfleri
    p.fullName = p.fullName.simplified();
    p.nationalId = p.nationalId.trimmed();
    p.phone = Rules::normalizePhone(p.phone);
    p.email = p.email.trimmed().toLower();
    p.allergies = p.allergies.trimmed();
    p.chronicConditions = p.chronicConditions.trimmed();
    if (const QStringList problems = Rules::patientProblems(p, today); !problems.isEmpty())
        return Result::failure(problems.first());
    const QVariantList values = {p.nationalId, p.fullName, Sql::date(p.birthDate), static_cast<int>(p.gender),
                                 text(p.phone), text(p.email), static_cast<int>(p.bloodGroup), text(p.allergies),
                                 text(p.chronicConditions)};
    QSqlQuery q(m_db.connection());
    if (insert) {
        if (!Sql::run(q, "INSERT INTO patients (national_id, full_name, birth_date, gender, phone, email, blood_group, "
                         "allergies, chronic) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)", values))
            return Result::failure("duplicate_national_id");
        return {QString(), q.lastInsertId().toLongLong()};
    }
    if (!Sql::run(q, "UPDATE patients SET national_id = ?, full_name = ?, birth_date = ?, gender = ?, phone = ?, "
                     "email = ?, blood_group = ?, allergies = ?, chronic = ? WHERE id = ?", values + QVariantList{p.id}))
        return Result::failure("duplicate_national_id");
    if (q.numRowsAffected() == 0)
        return Result::failure("not_found");
    return {QString(), p.id};
}

Result PatientRepository::remove(const User &actor, qint64 id)
{
    if (!Sql::can(actor, Permission::ManagePatients))
        return Result::failure("forbidden");
    QSqlQuery used(m_db.connection());
    if (!Sql::run(used, "SELECT COUNT(*) FROM appointments WHERE patient_id = ?", {id}) || !used.next()
        || used.value(0).toInt() > 0)
        return Result::failure("has_history"); // geçmişi olan kayıt silinmez (tıbbi kayıtlar korunur)
    QSqlQuery q(m_db.connection());
    if (!Sql::run(q, "DELETE FROM patients WHERE id = ?", {id}) || q.numRowsAffected() == 0)
        return Result::failure("not_found");
    return {QString(), id};
}
