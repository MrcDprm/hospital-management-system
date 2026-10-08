#include "data/StaffRepository.h"

#include "core/Rules.h"
#include "data/SqlUtil.h"

namespace {

const QString DOCTOR_COLUMNS = "id, title, full_name, department_id, phone, email, work_days, start_time, end_time, "
                               "slot_minutes, active";

Doctor doctorFrom(const QSqlQuery &q)
{
    Doctor d;
    d.id = q.value(0).toLongLong();
    d.title = q.value(1).toString();
    d.fullName = q.value(2).toString();
    d.departmentId = q.value(3).toLongLong();
    d.phone = q.value(4).toString();
    d.email = q.value(5).toString();
    d.workDays = q.value(6).toInt() & 0x7F; // bilinmeyen bitler atılır
    d.startTime = Sql::toTime(q.value(7));
    d.endTime = Sql::toTime(q.value(8));
    d.slotMinutes = q.value(9).toInt();
    d.active = q.value(10).toBool();
    return d;
}

} // namespace

QList<Department> StaffRepository::departments() const
{
    QList<Department> list;
    QSqlQuery q(m_db.connection());
    if (Sql::run(q, "SELECT id, name, location FROM departments ORDER BY name"))
        while (q.next())
            list << Department{q.value(0).toLongLong(), q.value(1).toString(), q.value(2).toString()};
    return list;
}

Result StaffRepository::addDepartment(const User &actor, Department d)
{
    if (!Sql::can(actor, Permission::ManageDoctors))
        return Result::failure("forbidden");
    d.name = d.name.trimmed();
    d.location = d.location.trimmed();
    if (const QStringList problems = Rules::departmentProblems(d); !problems.isEmpty())
        return Result::failure(problems.first());
    QSqlQuery q(m_db.connection());
    if (!Sql::run(q, "INSERT INTO departments (name, location) VALUES (?, ?)", {d.name, text(d.location)}))
        return Result::failure("duplicate_department");
    return {QString(), q.lastInsertId().toLongLong()};
}

Result StaffRepository::updateDepartment(const User &actor, Department d)
{
    if (!Sql::can(actor, Permission::ManageDoctors))
        return Result::failure("forbidden");
    d.name = d.name.trimmed();
    d.location = d.location.trimmed();
    if (const QStringList problems = Rules::departmentProblems(d); !problems.isEmpty())
        return Result::failure(problems.first());
    QSqlQuery q(m_db.connection());
    if (!Sql::run(q, "UPDATE departments SET name = ?, location = ? WHERE id = ?", {d.name, text(d.location), d.id}))
        return Result::failure("duplicate_department");
    if (q.numRowsAffected() == 0)
        return Result::failure("not_found");
    return {QString(), d.id};
}

Result StaffRepository::removeDepartment(const User &actor, qint64 id)
{
    if (!Sql::can(actor, Permission::ManageDoctors))
        return Result::failure("forbidden");
    QSqlQuery used(m_db.connection());
    if (!Sql::run(used, "SELECT COUNT(*) FROM doctors WHERE department_id = ?", {id}) || !used.next()
        || used.value(0).toInt() > 0)
        return Result::failure("department_has_doctors");
    QSqlQuery q(m_db.connection());
    if (!Sql::run(q, "DELETE FROM departments WHERE id = ?", {id}) || q.numRowsAffected() == 0)
        return Result::failure("not_found");
    return {QString(), id};
}

QList<Doctor> StaffRepository::doctors(qint64 departmentId, bool activeOnly) const
{
    QList<Doctor> list;
    QSqlQuery q(m_db.connection());
    // Süzgeçler parametreyle: 0 / false "hepsi" demek
    if (Sql::run(q, "SELECT " + DOCTOR_COLUMNS + " FROM doctors WHERE (? = 0 OR department_id = ?) "
                    "AND (? = 0 OR active = 1) ORDER BY full_name",
                 {departmentId, departmentId, activeOnly ? 1 : 0}))
        while (q.next())
            list << doctorFrom(q);
    return list;
}

std::optional<Doctor> StaffRepository::doctor(qint64 id) const
{
    QSqlQuery q(m_db.connection());
    if (Sql::run(q, "SELECT " + DOCTOR_COLUMNS + " FROM doctors WHERE id = ?", {id}) && q.next())
        return doctorFrom(q);
    return std::nullopt;
}

Result StaffRepository::addDoctor(const User &actor, Doctor d)
{
    return saveDoctor(actor, d, true);
}

Result StaffRepository::updateDoctor(const User &actor, Doctor d)
{
    return saveDoctor(actor, d, false);
}

Result StaffRepository::saveDoctor(const User &actor, Doctor d, bool insert)
{
    if (!Sql::can(actor, Permission::ManageDoctors))
        return Result::failure("forbidden");
    d.fullName = d.fullName.trimmed();
    d.title = d.title.trimmed();
    d.email = d.email.trimmed().toLower();
    d.phone = Rules::normalizePhone(d.phone);
    if (const QStringList problems = Rules::doctorProblems(d); !problems.isEmpty())
        return Result::failure(problems.first());
    const QVariantList values = {text(d.title), d.fullName, d.departmentId, text(d.phone), text(d.email), d.workDays,
                                 Sql::time(d.startTime), Sql::time(d.endTime), d.slotMinutes, d.active};
    QSqlQuery q(m_db.connection());
    if (insert) {
        if (!Sql::run(q, "INSERT INTO doctors (title, full_name, department_id, phone, email, work_days, start_time, "
                         "end_time, slot_minutes, active) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", values))
            return Result::failure("invalid_department"); // tek yabancı anahtar bölüm
        return {QString(), q.lastInsertId().toLongLong()};
    }
    if (!Sql::run(q, "UPDATE doctors SET title = ?, full_name = ?, department_id = ?, phone = ?, email = ?, "
                     "work_days = ?, start_time = ?, end_time = ?, slot_minutes = ?, active = ? WHERE id = ?",
                  values + QVariantList{d.id}))
        return Result::failure("invalid_department");
    if (q.numRowsAffected() == 0)
        return Result::failure("not_found");
    return {QString(), d.id};
}
