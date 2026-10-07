#include "core/Schedule.h"

namespace Schedule {

bool worksOn(const Doctor &doctor, const QDate &date)
{
    return date.isValid() && (doctor.workDays & (1 << (date.dayOfWeek() - 1))) != 0; // dayOfWeek: 1 = pazartesi
}

bool isActive(AppointmentStatus status)
{
    return status != AppointmentStatus::Cancelled;
}

bool overlaps(const QDateTime &aStart, const QDateTime &aEnd, const QDateTime &bStart, const QDateTime &bEnd)
{
    return aStart < bEnd && bStart < aEnd; // yarı açık aralıklar: 10:00-10:15 ile 10:15-10:30 çakışmaz
}

QList<Slot> daySlots(const Doctor &doctor, const QDate &date, const QList<Appointment> &appointments,
                     const QDateTime &now)
{
    QList<Slot> times;
    if (!doctor.active || !worksOn(doctor, date) || doctor.slotMinutes < 1)
        return times;
    for (QTime t = doctor.startTime; t.isValid() && t.addSecs(doctor.slotMinutes * 60) <= doctor.endTime;) {
        Slot slot;
        slot.start = QDateTime(date, t);
        slot.end = slot.start.addSecs(doctor.slotMinutes * 60);
        if (t < LUNCH_END && slot.end.time() > LUNCH_START)
            slot.state = SlotState::Lunch;
        for (const Appointment &a : appointments)
            if (a.doctorId == doctor.id && isActive(a.status) && overlaps(slot.start, slot.end, a.start, a.end)) {
                slot.state = SlotState::Booked;
                slot.appointmentId = a.id;
            }
        if (slot.state == SlotState::Free && slot.start < now)
            slot.state = SlotState::Past;
        times << slot;
        const QTime next = t.addSecs(doctor.slotMinutes * 60);
        if (next <= t) // gece yarısını geçti
            break;
        t = next;
    }
    return times;
}

QString check(const Doctor &doctor, const QDateTime &start, const QList<Appointment> &doctorAppointments,
              const QList<Appointment> &patientAppointments, const QDateTime &now)
{
    if (!doctor.active)
        return "doctor_inactive";
    if (!start.isValid() || start < now)
        return "slot_in_past";
    if (start.date() > now.date().addDays(MAX_DAYS_AHEAD))
        return "too_far_ahead";
    // Saat doktorun o günkü slotlarından biri olmalı (09:07 gibi rastgele saat olmaz)
    const QList<Slot> times = daySlots(doctor, start.date(), doctorAppointments, now);
    const auto it = std::find_if(times.cbegin(), times.cend(), [&](const Slot &s) { return s.start == start; });
    if (it == times.cend())
        return "not_working";
    if (it->state == SlotState::Lunch)
        return "lunch_break";
    if (it->state == SlotState::Booked)
        return "slot_taken";
    // Hasta aynı saatte başka bir doktorda olamaz
    for (const Appointment &a : patientAppointments)
        if (isActive(a.status) && overlaps(it->start, it->end, a.start, a.end))
            return "patient_busy";
    return QString();
}

} // namespace Schedule
