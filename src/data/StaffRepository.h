#pragma once

#include "core/Models.h"
#include "data/Database.h"

#include <optional>

// Bölümler ve doktorlar. Okuma herkese açık (randevu verirken lazım), değişiklik sadece yöneticiye.
class StaffRepository
{
public:
    explicit StaffRepository(Database &db) : m_db(db) {}

    QList<Department> departments() const;
    Result addDepartment(const User &actor, Department department);
    Result updateDepartment(const User &actor, Department department);
    Result removeDepartment(const User &actor, qint64 id); // doktoru olan bölüm silinmez

    QList<Doctor> doctors(qint64 departmentId = 0, bool activeOnly = false) const;
    std::optional<Doctor> doctor(qint64 id) const;
    Result addDoctor(const User &actor, Doctor doctor);
    Result updateDoctor(const User &actor, Doctor doctor);

private:
    Result saveDoctor(const User &actor, Doctor doctor, bool insert);

    Database &m_db;
};
