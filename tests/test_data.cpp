#include "core/Passwords.h"
#include "data/AppointmentRepository.h"
#include "data/Database.h"
#include "data/ExaminationRepository.h"
#include "data/PatientRepository.h"
#include "data/ReportRepository.h"
#include "data/StaffRepository.h"
#include "data/UserRepository.h"
#include "services/DemoData.h"

#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <memory>

// Her test kendi geçici veritabanıyla başlar; gerçek kullanıcı verisine dokunulmaz.
class TestData : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;
    std::unique_ptr<Database> m_db;
    int m_counter = 0;
    const QDate m_monday{2026, 10, 12};
    const QDateTime m_now{QDate(2026, 10, 12), QTime(8, 0)};
    User m_admin, m_reception, m_doctor, m_otherDoctor;
    qint64 m_doctorId = 0, m_otherDoctorId = 0, m_patient = 0;

    User makeUser(qint64 id, Role role, qint64 doctorId = 0)
    {
        User u;
        u.id = id;
        u.username = "u" + QString::number(id);
        u.fullName = "Kullanıcı " + QString::number(id);
        u.role = role;
        u.doctorId = doctorId;
        return u;
    }

    Appointment at(int hour, int minute, qint64 doctorId = 0, qint64 patientId = 0)
    {
        Appointment a;
        a.doctorId = doctorId ? doctorId : m_doctorId;
        a.patientId = patientId ? patientId : m_patient;
        a.start = QDateTime(m_monday, QTime(hour, minute));
        return a;
    }

private slots:
    void initTestCase() { QVERIFY(Passwords::init()); }

    void init()
    {
        m_db = std::make_unique<Database>();
        QVERIFY(m_db->open(m_dir.filePath(QString("t%1.db").arg(++m_counter))));
        m_admin = makeUser(1, Role::Admin);
        m_reception = makeUser(2, Role::Receptionist);
        StaffRepository staff(*m_db);
        const qint64 dep = staff.addDepartment(m_admin, {0, "Kardiyoloji", "A Blok"}).id;
        Doctor d;
        d.fullName = "Ayşe Kaya";
        d.title = "Uzm. Dr.";
        d.departmentId = dep;
        d.slotMinutes = 30;
        m_doctorId = staff.addDoctor(m_admin, d).id;
        d.fullName = "Mehmet Demir";
        m_otherDoctorId = staff.addDoctor(m_admin, d).id;
        m_doctor = makeUser(3, Role::Doctor, m_doctorId);
        m_otherDoctor = makeUser(4, Role::Doctor, m_otherDoctorId);
        m_patient = PatientRepository(*m_db)
                        .add(m_reception, {0, "10000000146", "Ali Veli", QDate(1990, 1, 1), Gender::Male, "05321234567",
                                           "", BloodGroup::APos, "Penisilin", ""}, m_monday)
                        .id;
        QVERIFY(m_doctorId && m_otherDoctorId && m_patient);
    }

    void cleanup() { m_db.reset(); }

    void firstAdminAndLogin()
    {
        UserRepository users(*m_db);
        QCOMPARE(users.count(), 0);
        User admin;
        admin.username = "Yonetici";
        admin.fullName = "Sistem Yöneticisi";
        QCOMPARE(users.createFirstAdmin(admin, "zayif").error, QString("password_short"));
        const Result created = users.createFirstAdmin(admin, "Guclu-Parola-2026");
        QVERIFY(created.ok());
        QCOMPARE(users.find(created.id)->username, QString("yonetici")); // küçük harfe çevrildi
        QCOMPARE(users.createFirstAdmin(admin, "Guclu-Parola-2026").error, QString("forbidden")); // ikinci kez olmaz

        QCOMPARE(users.authenticate("YONETICI", "Guclu-Parola-2026", m_now).id, created.id);
        QCOMPARE(users.authenticate("yonetici", "yanlis", m_now).error, QString("login_failed"));
        QCOMPARE(users.authenticate("olmayan", "Guclu-Parola-2026", m_now).error, QString("login_failed")); // aynı hata

        QSqlQuery q(m_db->connection());
        q.exec("SELECT password_hash FROM users");
        QVERIFY(q.next() && q.value(0).toString().startsWith("$argon2id$"));
    }

    void lockoutAfterFiveFailures()
    {
        UserRepository users(*m_db);
        User admin;
        admin.username = "admin";
        admin.fullName = "Yönetici";
        QVERIFY(users.createFirstAdmin(admin, "Guclu-Parola-2026").ok());
        for (int i = 0; i < 4; ++i)
            QCOMPARE(users.authenticate("admin", "yanlis", m_now).error, QString("login_failed"));
        QCOMPARE(users.authenticate("admin", "yanlis", m_now).error, QString("account_locked"));
        // Kilitliyken doğru şifre de girmez; 30 saniye sonra girer
        QCOMPARE(users.authenticate("admin", "Guclu-Parola-2026", m_now.addSecs(10)).error, QString("account_locked"));
        QVERIFY(users.authenticate("admin", "Guclu-Parola-2026", m_now.addSecs(31)).ok());
    }

    void userManagementNeedsAdmin()
    {
        UserRepository users(*m_db);
        User nurse;
        nurse.username = "sekreter";
        nurse.fullName = "Zeynep Sekreter";
        nurse.role = Role::Receptionist;
        QCOMPARE(users.add(m_reception, nurse, "Guclu-Parola-2026").error, QString("forbidden"));
        const Result added = users.add(m_admin, nurse, "Gecici-Parola-2026");
        QVERIFY(added.ok());
        QVERIFY(users.find(added.id)->mustChangePassword); // geçici şifre
        QCOMPARE(users.changeOwnPassword(added.id, "yanlis", "Yeni-Parola-2026").error, QString("wrong_current_password"));
        QVERIFY(users.changeOwnPassword(added.id, "Gecici-Parola-2026", "Yeni-Parola-2026").ok());
        QVERIFY(!users.find(added.id)->mustChangePassword);

        User doctor;
        doctor.username = "dr.ayse";
        doctor.fullName = "Ayşe Kaya";
        doctor.role = Role::Doctor;
        QCOMPARE(users.add(m_admin, doctor, "Guclu-Parola-2026").error, QString("doctor_link_required"));
        QCOMPARE(users.all(m_reception).size(), 0); // sekreter kullanıcı listesini göremez
    }

    void bookingAndConflicts()
    {
        AppointmentRepository appointments(*m_db);
        QCOMPARE(appointments.book(m_doctor, at(9, 0), m_now).error, QString("forbidden")); // doktor randevu vermez
        const Result first = appointments.book(m_reception, at(9, 0), m_now);
        QVERIFY(first.ok());
        QCOMPARE(appointments.find(first.id)->end, QDateTime(m_monday, QTime(9, 30))); // slot süresi kadar
        QCOMPARE(appointments.book(m_reception, at(9, 0), m_now).error, QString("slot_taken"));
        QCOMPARE(appointments.book(m_reception, at(9, 0, m_otherDoctorId), m_now).error, QString("patient_busy"));
        QCOMPARE(appointments.book(m_reception, at(12, 0), m_now).error, QString("lunch_break"));
        QCOMPARE(appointments.book(m_reception, at(9, 10), m_now).error, QString("not_working"));

        // İptal edilen slot yeniden verilebilir
        QVERIFY(appointments.cancel(m_reception, first.id, m_now).ok());
        QVERIFY(appointments.book(m_reception, at(9, 0), m_now).ok());
    }

    void statusTransitions()
    {
        AppointmentRepository appointments(*m_db);
        const qint64 id = appointments.book(m_reception, at(10, 0), m_now).id;
        QCOMPARE(appointments.markNoShow(m_reception, id, m_now).error, QString("not_started_yet"));
        QCOMPARE(appointments.checkIn(m_reception, id, m_now.addDays(-1)).error, QString("not_today"));
        QCOMPARE(appointments.checkIn(m_otherDoctor, id, m_now).error, QString("forbidden")); // başka doktorun hastası
        QVERIFY(appointments.checkIn(m_doctor, id, m_now).ok());
        QCOMPARE(appointments.checkIn(m_reception, id, m_now).error, QString("invalid_state")); // zaten geldi
        QCOMPARE(appointments.cancel(m_reception, id, m_now.addSecs(3 * 3600)).error, QString("already_started"));

        // Doktor kendi listesini görür, başkasınınkini isteyemez
        QCOMPARE(appointments.list(m_otherDoctor, m_monday, m_monday.addDays(1), m_doctorId).size(), 0);
        QCOMPARE(appointments.list(m_doctor, m_monday, m_monday.addDays(1)).size(), 1);
        QCOMPARE(appointments.list(m_reception, m_monday, m_monday.addDays(1)).size(), 1);
    }

    void examinationsArePrivate()
    {
        AppointmentRepository appointments(*m_db);
        ExaminationRepository exams(*m_db);
        const qint64 id = appointments.book(m_reception, at(10, 0), m_now).id;
        Examination e;
        e.appointmentId = id;
        e.complaint = "Göğüs ağrısı";
        e.diagnosisCode = "i10";
        e.diagnosisName = "Esansiyel hipertansiyon";
        e.prescription = {{"Amlodipin", "5 mg", "Günde 1 kez", 30}, {"", "", "", 0}};
        QCOMPARE(exams.save(m_reception, e, m_now).error, QString("forbidden"));
        QCOMPARE(exams.save(m_otherDoctor, e, m_now).error, QString("forbidden"));
        QVERIFY(exams.save(m_doctor, e, m_now).ok());
        QCOMPARE(appointments.find(id)->status, AppointmentStatus::Examined);

        const auto saved = exams.forAppointment(m_doctor, id);
        QVERIFY(saved);
        QCOMPARE(saved->diagnosisCode, QString("I10"));
        QCOMPARE(saved->prescription.size(), 1); // boş satır atlandı
        QVERIFY(!exams.forAppointment(m_admin, id));          // yönetici tıbbi kaydı göremez
        QVERIFY(exams.forPatient(m_reception, m_patient).isEmpty());
        QCOMPARE(exams.forPatient(m_otherDoctor, m_patient).size(), 1); // başka doktor geçmişi görebilir

        // Düzenleme: reçete baştan yazılır
        e.prescription = {{"Ramipril", "5 mg", "Sabah", 30}, {"Aspirin", "100 mg", "Öğle", 30}};
        QVERIFY(exams.save(m_doctor, e, m_now).ok());
        QCOMPARE(exams.forAppointment(m_doctor, id)->prescription.size(), 2);
        e.diagnosisName.clear();
        QCOMPARE(exams.save(m_doctor, e, m_now).error, QString("diagnosis_required"));
    }

    void patientsAndInjection()
    {
        PatientRepository patients(*m_db);
        Patient p{0, "10000000146", "Başka Biri", QDate(1980, 5, 5), Gender::Female, "", "", BloodGroup::Unknown, "", ""};
        QCOMPARE(patients.add(m_reception, p, m_monday).error, QString("duplicate_national_id"));
        QCOMPARE(patients.add(m_doctor, p, m_monday).error, QString("forbidden"));
        p.nationalId = "10000000214";
        p.fullName = "Robert'); DROP TABLE patients;--";
        const Result added = patients.add(m_reception, p, m_monday);
        QVERIFY(added.ok());
        QCOMPARE(patients.find(added.id)->fullName, p.fullName); // metin olduğu gibi saklandı, tablo duruyor
        QCOMPARE(patients.all().size(), 2);
        AppointmentRepository(*m_db).book(m_reception, at(9, 0), m_now);
        QCOMPARE(patients.remove(m_reception, m_patient).error, QString("has_history"));
        QVERIFY(patients.remove(m_reception, added.id).ok());
    }

    void staffAndReports()
    {
        StaffRepository staff(*m_db);
        QCOMPARE(staff.addDepartment(m_reception, {0, "Göz", ""}).error, QString("forbidden"));
        QCOMPARE(staff.addDepartment(m_admin, {0, "kardiyoloji", ""}).error, QString("duplicate_department"));
        QCOMPARE(staff.removeDepartment(m_admin, staff.departments().first().id).error, QString("department_has_doctors"));

        AppointmentRepository appointments(*m_db);
        const qint64 a = appointments.book(m_reception, at(9, 0), m_now).id;
        appointments.book(m_reception, at(10, 0), m_now);
        QVERIFY(appointments.checkIn(m_reception, a, m_now).ok());
        const DaySummary day = ReportRepository(*m_db).day(m_monday);
        QCOMPARE(day.total, 2);
        QCOMPARE(day.checkedIn, 1);
        QCOMPARE(day.waiting, 1);
        const QList<DoctorStat> doctors = ReportRepository(*m_db).doctors(m_monday, m_monday.addDays(1));
        QCOMPARE(doctors.first().booked, 2);
        QCOMPARE(doctors.first().capacity, 14); // 09:00-17:00, 30 dk, öğle arası hariç
    }

    void demoDataIsConsistent()
    {
        // init() bir bölüm ve iki doktor ekledi; örnek veri boş veritabanı ister
        m_db.reset();
        m_db = std::make_unique<Database>();
        QVERIFY(m_db->open(m_dir.filePath("demo.db")));
        QVERIFY(DemoData::load(*m_db, m_now));
        QVERIFY(!DemoData::load(*m_db, m_now)); // ikinci kez yüklenmez

        UserRepository users(*m_db);
        QCOMPARE(users.count(), 4);
        for (const DemoData::Account &account : DemoData::accounts())
            QVERIFY2(users.authenticate(account.username, account.password, m_now).ok(), qPrintable(account.username));

        StaffRepository staff(*m_db);
        QCOMPARE(staff.departments().size(), 8);
        QCOMPARE(staff.doctors().size(), 20);
        QCOMPARE(PatientRepository(*m_db).all().size(), 1200);

        // Aynı doktorda çakışan aktif randevu yok; geçmişte "alındı" kalmamış
        QSqlQuery q(m_db->connection());
        QVERIFY(q.exec("SELECT COUNT(*) FROM appointments a JOIN appointments b ON a.doctor_id = b.doctor_id "
                       "AND a.id < b.id AND a.start < b.end AND b.start < a.end AND a.status != 4 AND b.status != 4"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 0);
        QVERIFY(q.exec("SELECT COUNT(*) FROM appointments WHERE status = 0 AND end <= '2026-10-12T08:00:00'"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 0);
        QVERIFY(q.exec("SELECT COUNT(*) FROM appointments a LEFT JOIN examinations e ON e.appointment_id = a.id "
                       "WHERE a.status = 2 AND e.id IS NULL"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 0); // muayene edilenin muayene kaydı var
        const DaySummary today = ReportRepository(*m_db).day(m_monday.addDays(-3));
        QVERIFY(today.examined > 0 && today.noShow >= 0);
    }
};

QTEST_GUILESS_MAIN(TestData)
#include "test_data.moc"
