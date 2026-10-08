#pragma once

#include "data/Database.h"

#include <QDate>
#include <QList>

// Raporlar için sayımlar. Sadece okur; tıbbi içerik (tanı, not) içermez, sadece sayılar.
struct DaySummary {
    int total = 0;     // iptal hariç bugünkü randevular
    int waiting = 0;   // alındı (henüz gelmedi)
    int checkedIn = 0; // geldi, muayene bekliyor
    int examined = 0;
    int noShow = 0;
};

struct DepartmentStat {
    QString name;
    int appointments = 0;
    int examined = 0;
    int noShow = 0;
};

struct DoctorStat {
    QString name;
    QString department;
    int booked = 0;
    int capacity = 0; // dönemdeki toplam slot sayısı
    int noShow = 0;
};

class ReportRepository
{
public:
    explicit ReportRepository(Database &db) : m_db(db) {}

    DaySummary day(const QDate &date, qint64 doctorId = 0) const;
    QList<DepartmentStat> departments(const QDate &from, const QDate &to) const;
    QList<DoctorStat> doctors(const QDate &from, const QDate &to) const;
    QList<int> monthly(int year) const; // 12 ay, iptal hariç randevu sayısı

private:
    Database &m_db;
};
