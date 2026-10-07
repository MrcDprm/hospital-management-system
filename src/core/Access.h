#pragma once

#include "core/Models.h"

// Rol bazlı yetkiler. Arayüz düğmeleri buna göre gizlenir ama asıl kontrol veri katmanındadır:
// arayüzde bir düğme unutulsa bile yetkisiz işlem veritabanına ulaşamaz.
enum class Permission {
    ManageUsers,        // kullanıcı ekleme, rol, şifre sıfırlama (yönetici)
    ManageDoctors,      // bölüm ve doktor tanımları (yönetici)
    ManagePatients,     // hasta kaydı ve düzenleme (yönetici, sekreter)
    ViewPatients,       // hasta listesi ve iletişim bilgileri (herkes)
    ManageAppointments, // randevu verme, iptal, geldi/gelmedi (yönetici, sekreter)
    ViewAllSchedules,   // bütün doktorların takvimi (yönetici, sekreter)
    WriteExaminations,  // muayene ve reçete yazma (sadece doktor, sadece kendi hastası)
    ViewMedicalRecords, // tanılar, muayene notları, reçeteler (sadece doktor)
    ViewReports,        // istatistikler (yönetici)
};

namespace Access {

bool allowed(Role role, Permission permission);

} // namespace Access
