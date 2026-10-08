#include "data/ReportRepository.h"

#include "core/Schedule.h"
#include "data/SqlUtil.h"
#include "data/StaffRepository.h"

DaySummary ReportRepository::day(const QDate &date, qint64 doctorId) const
{
    DaySummary s;
    QSqlQuery q(m_db.connection());
    if (!Sql::run(q, "SELECT status, COUNT(*) FROM appointments WHERE start >= ? AND start < ? "
                     "AND (? = 0 OR doctor_id = ?) GROUP BY status",
                  {Sql::date(date), Sql::date(date.addDays(1)), doctorId, doctorId}))
        return s;
    while (q.next()) {
        const auto status = Sql::toEnum(q.value(0), AppointmentStatus::Cancelled, AppointmentStatus::Cancelled);
        const int count = q.value(1).toInt();
        switch (status) {
        case AppointmentStatus::Booked: s.waiting += count; break;
        case AppointmentStatus::CheckedIn: s.checkedIn += count; break;
        case AppointmentStatus::Examined: s.examined += count; break;
        case AppointmentStatus::NoShow: s.noShow += count; break;
        case AppointmentStatus::Cancelled: continue;
        }
        s.total += count;
    }
    return s;
}

QList<DepartmentStat> ReportRepository::departments(const QDate &from, const QDate &to) const
{
    QList<DepartmentStat> list;
    QSqlQuery q(m_db.connection());
    if (!Sql::run(q, "SELECT dep.name, "
                     "SUM(CASE WHEN a.status != ? THEN 1 ELSE 0 END), "
                     "SUM(CASE WHEN a.status = ? THEN 1 ELSE 0 END), "
                     "SUM(CASE WHEN a.status = ? THEN 1 ELSE 0 END) "
                     "FROM appointments a JOIN doctors d ON d.id = a.doctor_id JOIN departments dep ON dep.id = d.department_id "
                     "WHERE a.start >= ? AND a.start < ? GROUP BY dep.id ORDER BY 2 DESC",
                  {static_cast<int>(AppointmentStatus::Cancelled), static_cast<int>(AppointmentStatus::Examined),
                   static_cast<int>(AppointmentStatus::NoShow), Sql::date(from), Sql::date(to)}))
        return list;
    while (q.next())
        list << DepartmentStat{q.value(0).toString(), q.value(1).toInt(), q.value(2).toInt(), q.value(3).toInt()};
    return list;
}

QList<DoctorStat> ReportRepository::doctors(const QDate &from, const QDate &to) const
{
    QList<DoctorStat> list;
    StaffRepository staff(m_db);
    QHash<qint64, QString> departmentNames;
    for (const Department &d : staff.departments())
        departmentNames.insert(d.id, d.name);
    for (const Doctor &doctor : staff.doctors()) {
        DoctorStat stat;
        stat.name = doctor.displayName();
        stat.department = departmentNames.value(doctor.departmentId);
        // Kapasite: dönemdeki çalışma günlerinin öğle arası dışındaki slotları
        for (QDate d = from; d < to; d = d.addDays(1))
            for (const Schedule::Slot &slot : Schedule::daySlots(doctor, d, {}, QDateTime(from, QTime(0, 0))))
                stat.capacity += slot.state != Schedule::SlotState::Lunch;
        QSqlQuery q(m_db.connection());
        if (Sql::run(q, "SELECT SUM(CASE WHEN status != ? THEN 1 ELSE 0 END), SUM(CASE WHEN status = ? THEN 1 ELSE 0 END) "
                        "FROM appointments WHERE doctor_id = ? AND start >= ? AND start < ?",
                     {static_cast<int>(AppointmentStatus::Cancelled), static_cast<int>(AppointmentStatus::NoShow),
                      doctor.id, Sql::date(from), Sql::date(to)})
            && q.next()) {
            stat.booked = q.value(0).toInt();
            stat.noShow = q.value(1).toInt();
        }
        if (stat.capacity > 0 || stat.booked > 0)
            list << stat;
    }
    std::sort(list.begin(), list.end(), [](const DoctorStat &a, const DoctorStat &b) { return a.booked > b.booked; });
    return list;
}

QList<int> ReportRepository::monthly(int year) const
{
    QList<int> months(12, 0);
    QSqlQuery q(m_db.connection());
    if (Sql::run(q, "SELECT CAST(substr(start, 6, 2) AS INTEGER), COUNT(*) FROM appointments "
                    "WHERE substr(start, 1, 4) = ? AND status != ? GROUP BY 1",
                 {QString::number(year), static_cast<int>(AppointmentStatus::Cancelled)}))
        while (q.next()) {
            const int month = q.value(0).toInt();
            if (month >= 1 && month <= 12)
                months[month - 1] = q.value(1).toInt();
        }
    return months;
}
