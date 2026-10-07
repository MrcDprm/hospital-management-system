#include "core/Access.h"

namespace Access {

bool allowed(Role role, Permission permission)
{
    switch (permission) {
    case Permission::ManageUsers:
    case Permission::ManageDoctors:
    case Permission::ViewReports:
        return role == Role::Admin;
    case Permission::ManagePatients:
    case Permission::ManageAppointments:
    case Permission::ViewAllSchedules:
        return role == Role::Admin || role == Role::Receptionist;
    case Permission::ViewPatients:
        return true;
    // Tıbbi bilgiler sadece doktorlara açık: sekreter ve sistem yöneticisi tanıları görmez (hasta mahremiyeti)
    case Permission::WriteExaminations:
    case Permission::ViewMedicalRecords:
        return role == Role::Doctor;
    }
    return false; // bilinmeyen izin: kapalı
}

} // namespace Access
