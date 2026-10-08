#include "core/Access.h"
#include "core/Passwords.h"
#include "core/Rules.h"
#include "core/Schedule.h"

#include <QTest>

class TestCore : public QObject
{
    Q_OBJECT

    // Pazartesi-cuma 09:00-12:30 ve 13:00-14:00 arası 30 dakikalık randevular
    Doctor doctor()
    {
        Doctor d;
        d.id = 1;
        d.fullName = "Ayşe Kaya";
        d.departmentId = 1;
        d.workDays = 0b0011111;
        d.startTime = QTime(9, 0);
        d.endTime = QTime(14, 0);
        d.slotMinutes = 30;
        return d;
    }

    const QDate m_monday{2026, 10, 12};

private slots:
    void initTestCase() { QVERIFY(Passwords::init()); }

    void nationalIdAndContacts()
    {
        QVERIFY(Rules::isValidNationalId("10000000146"));
        QVERIFY(!Rules::isValidNationalId("10000000147")); // sağlama basamağı yanlış
        QVERIFY(!Rules::isValidNationalId("01234567890")); // 0 ile başlayamaz
        QCOMPARE(Rules::normalizePhone("0 (532) 123 45 67"), QString("5321234567"));
        QVERIFY(Rules::isValidEmail("ayse@ornek.com"));
        QVERIFY(!Rules::isValidEmail("ayse@"));
    }

    void patientRules()
    {
        Patient p{0, "10000000146", "Ali Veli", QDate(1990, 1, 1), Gender::Male, "", "", BloodGroup::APos, "", ""};
        QVERIFY(Rules::patientProblems(p, m_monday).isEmpty());
        p.birthDate = m_monday.addDays(1);
        QCOMPARE(Rules::patientProblems(p, m_monday).first(), QString("invalid_birth_date"));
        p.birthDate = QDate(1990, 1, 1);
        p.phone = "123";
        QCOMPARE(Rules::patientProblems(p, m_monday).first(), QString("invalid_phone"));
    }

    void doctorRules()
    {
        Doctor d = doctor();
        QVERIFY(Rules::doctorProblems(d).isEmpty());
        d.workDays = 0;
        QCOMPARE(Rules::doctorProblems(d).first(), QString("invalid_work_days"));
        d = doctor();
        d.endTime = QTime(9, 10); // tek slot bile sığmıyor
        QCOMPARE(Rules::doctorProblems(d).first(), QString("invalid_hours"));
        d = doctor();
        d.slotMinutes = 3;
        QCOMPARE(Rules::doctorProblems(d).first(), QString("invalid_slot"));
    }

    void slotsSkipLunchAndBooked()
    {
        const Doctor d = doctor();
        Appointment booked;
        booked.id = 7;
        booked.doctorId = 1;
        booked.start = QDateTime(m_monday, QTime(10, 0));
        booked.end = booked.start.addSecs(1800);
        Appointment cancelled = booked;
        cancelled.id = 8;
        cancelled.start = QDateTime(m_monday, QTime(10, 30));
        cancelled.end = cancelled.start.addSecs(1800);
        cancelled.status = AppointmentStatus::Cancelled;

        const QList<Schedule::Slot> times =
            Schedule::daySlots(d, m_monday, {booked, cancelled}, QDateTime(m_monday, QTime(9, 15)));
        QCOMPARE(times.size(), 10); // 09:00 … 13:30
        QCOMPARE(times[0].state, Schedule::SlotState::Past);   // 09:00 geçti
        QCOMPARE(times[2].state, Schedule::SlotState::Booked); // 10:00
        QCOMPARE(times[2].appointmentId, 7);
        QCOMPARE(times[3].state, Schedule::SlotState::Free);   // iptal edilen 10:30 boş
        QCOMPARE(times[6].state, Schedule::SlotState::Lunch);  // 12:00
        QCOMPARE(times[7].state, Schedule::SlotState::Lunch);  // 12:30
        QCOMPARE(times[8].state, Schedule::SlotState::Free);   // 13:00
        QVERIFY(Schedule::daySlots(d, m_monday.addDays(5), {}, QDateTime(m_monday, QTime(0, 0))).isEmpty()); // cumartesi
    }

    void bookingChecks()
    {
        const Doctor d = doctor();
        const QDateTime now(m_monday, QTime(8, 0));
        QCOMPARE(Schedule::check(d, QDateTime(m_monday, QTime(9, 30)), {}, {}, now), QString());
        QCOMPARE(Schedule::check(d, QDateTime(m_monday, QTime(9, 40)), {}, {}, now), QString("not_working")); // slot değil
        QCOMPARE(Schedule::check(d, QDateTime(m_monday, QTime(12, 0)), {}, {}, now), QString("lunch_break"));
        QCOMPARE(Schedule::check(d, QDateTime(m_monday.addDays(-1), QTime(10, 0)), {}, {}, now), QString("slot_in_past"));
        QCOMPARE(Schedule::check(d, QDateTime(m_monday.addDays(5), QTime(10, 0)), {}, {}, now), QString("not_working"));
        QCOMPARE(Schedule::check(d, QDateTime(m_monday.addDays(120), QTime(10, 0)), {}, {}, now), QString("too_far_ahead"));

        Appointment taken{1, 5, 1, QDateTime(m_monday, QTime(9, 30)), QDateTime(m_monday, QTime(10, 0)),
                          AppointmentStatus::Booked, ""};
        QCOMPARE(Schedule::check(d, taken.start, {taken}, {}, now), QString("slot_taken"));
        // Hasta aynı saatte başka doktorda
        Appointment elsewhere{2, 9, 2, QDateTime(m_monday, QTime(10, 15)), QDateTime(m_monday, QTime(10, 45)),
                              AppointmentStatus::Booked, ""};
        QCOMPARE(Schedule::check(d, QDateTime(m_monday, QTime(10, 0)), {}, {elsewhere}, now), QString("patient_busy"));
        Doctor inactive = d;
        inactive.active = false;
        QCOMPARE(Schedule::check(inactive, QDateTime(m_monday, QTime(9, 30)), {}, {}, now), QString("doctor_inactive"));
    }

    void permissions()
    {
        QVERIFY(Access::allowed(Role::Admin, Permission::ManageUsers));
        QVERIFY(!Access::allowed(Role::Receptionist, Permission::ManageUsers));
        QVERIFY(Access::allowed(Role::Receptionist, Permission::ManageAppointments));
        QVERIFY(!Access::allowed(Role::Doctor, Permission::ManageAppointments));
        // Tıbbi kayıtlar sadece doktorda: yönetici de göremez
        QVERIFY(Access::allowed(Role::Doctor, Permission::ViewMedicalRecords));
        QVERIFY(!Access::allowed(Role::Admin, Permission::ViewMedicalRecords));
        QVERIFY(!Access::allowed(Role::Receptionist, Permission::WriteExaminations));
    }

    void passwordRulesAndHash()
    {
        QCOMPARE(Passwords::problems("Kisa1", "ayse").first(), QString("password_short"));
        QCOMPARE(Passwords::problems("sadecekucuk1", "ayse").first(), QString("password_variety"));
        QCOMPARE(Passwords::problems("Ayse-Parola-2026", "ayse").first(), QString("password_has_username"));
        QVERIFY(Passwords::problems("Guclu-Parola-2026", "ayse").isEmpty());
        const auto hash = Passwords::hash("Guclu-Parola-2026");
        QVERIFY(hash && hash->startsWith("$argon2id$"));
        QVERIFY(!hash->contains("Guclu")); // düz metin saklanmaz
        QVERIFY(Passwords::verify("Guclu-Parola-2026", *hash));
        QVERIFY(!Passwords::verify("guclu-parola-2026", *hash));
        QVERIFY(!Passwords::verify("Guclu-Parola-2026", "bozuk"));
        QVERIFY(*Passwords::hash("Guclu-Parola-2026") != *hash); // her hash'in kendi tuzu var
    }
};

QTEST_GUILESS_MAIN(TestCore)
#include "test_core.moc"
