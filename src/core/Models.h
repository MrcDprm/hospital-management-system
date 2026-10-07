#pragma once

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QTime>

// Uygulamanın temel kayıtları. Kimlikler veritabanının kendi sayılarıdır ama arayüzde gösterilmez.

enum class Role { Admin, Receptionist, Doctor };
enum class Gender { Female, Male, Other };
enum class BloodGroup { Unknown, APos, ANeg, BPos, BNeg, ABPos, ABNeg, ZeroPos, ZeroNeg };
enum class AppointmentStatus { Booked, CheckedIn, Examined, NoShow, Cancelled };

struct User {
    qint64 id = 0;
    QString username;
    QString fullName;
    Role role = Role::Receptionist;
    qint64 doctorId = 0; // doktor rolündeki kullanıcının doktor kaydı
    bool active = true;
    bool mustChangePassword = false; // yönetici sıfırladıysa ilk girişte değiştirilir
};

struct Department {
    qint64 id = 0;
    QString name;
    QString location; // ör. "B Blok, 2. kat"
};

struct Doctor {
    qint64 id = 0;
    QString title; // Dr., Uzm. Dr., Doç. Dr., Prof. Dr.
    QString fullName;
    qint64 departmentId = 0;
    QString phone;
    QString email;
    int workDays = 0b0011111; // bit 0 = pazartesi … bit 6 = pazar
    QTime startTime = QTime(9, 0);
    QTime endTime = QTime(17, 0);
    int slotMinutes = 15;
    bool active = true;

    QString displayName() const { return title.isEmpty() ? fullName : title + " " + fullName; }
};

struct Patient {
    qint64 id = 0;
    QString nationalId;
    QString fullName;
    QDate birthDate;
    Gender gender = Gender::Other;
    QString phone;
    QString email;
    BloodGroup bloodGroup = BloodGroup::Unknown;
    QString allergies;
    QString chronicConditions;
};

struct Appointment {
    qint64 id = 0;
    qint64 patientId = 0;
    qint64 doctorId = 0;
    QDateTime start;
    QDateTime end;
    AppointmentStatus status = AppointmentStatus::Booked;
    QString note;
};

struct PrescriptionItem {
    QString medicine;
    QString dose;  // ör. "500 mg"
    QString usage; // ör. "Günde 2 kez, tok karnına"
    int days = 0;
};

struct Examination {
    qint64 id = 0;
    qint64 appointmentId = 0;
    qint64 patientId = 0;
    qint64 doctorId = 0;
    QDateTime date;
    QString complaint;
    QString findings;
    QString diagnosisCode; // ICD-10
    QString diagnosisName;
    QString notes;
    QList<PrescriptionItem> prescription;
};
