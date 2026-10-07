#pragma once

#include "core/Models.h"

// Randevu saatleri: doktorun çalışma günü ve saatlerine, öğle arasına ve dolu randevulara göre
// bir günün saat dilimleri (slot) hesaplanır. Arayüz ve kayıt aynı hesabı kullanır.
namespace Schedule {

inline const QTime LUNCH_START(12, 0);
inline const QTime LUNCH_END(13, 0);
constexpr int MAX_DAYS_AHEAD = 90; // en fazla 3 ay sonrasına randevu

enum class SlotState { Free, Booked, Past, Lunch };

struct Slot {
    QDateTime start;
    QDateTime end;
    SlotState state = SlotState::Free;
    qint64 appointmentId = 0; // dolu slotun randevusu
};

bool worksOn(const Doctor &doctor, const QDate &date);
// O günün bütün slotları. İptal edilen randevu yeri boşaltır; diğer durumlar yeri tutar.
QList<Slot> daySlots(const Doctor &doctor, const QDate &date, const QList<Appointment> &appointments,
                     const QDateTime &now);
bool isActive(AppointmentStatus status); // yer kaplayan durumlar
bool overlaps(const QDateTime &aStart, const QDateTime &aEnd, const QDateTime &bStart, const QDateTime &bEnd);

// Yeni randevunun kurallara uyup uymadığı; hata anahtarı döner, boşsa uygun
QString check(const Doctor &doctor, const QDateTime &start, const QList<Appointment> &doctorAppointments,
              const QList<Appointment> &patientAppointments, const QDateTime &now);

} // namespace Schedule
