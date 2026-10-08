#include "services/DemoData.h"

#include "core/Passwords.h"
#include "core/Rules.h"
#include "core/Schedule.h"
#include "data/SqlUtil.h"
#include "data/UserRepository.h"

#include <QRandomGenerator>
#include <QSet>

namespace {

constexpr int PEDIATRICS = 2; // departments() sırasındaki çocuk ve kadın doğum bölümleri
constexpr int GYNECOLOGY = 3;
constexpr int PATIENT_COUNT = 1200; // hasta başına birkaç ziyaret düşsün (gerçekçi geçmiş)

struct DepartmentSeed {
    const char *name, *location;
    QStringList diagnoses; // bu bölümde sık konan ICD-10 tanıları (şikâyet tanıdan seçilir)
};

const QList<DepartmentSeed> &departments()
{
    static const QList<DepartmentSeed> list = {
        {"Dahiliye", "A Blok, Zemin kat", {"I10", "E11.9", "E78.5", "K29.7", "D50.9", "E55.9", "J06.9"}},
        {"Kardiyoloji", "A Blok, 1. kat", {"I10", "I20.9", "I48", "R07.4", "I25.1", "I50.9"}},
        {"Çocuk Sağlığı ve Hastalıkları", "B Blok, Zemin kat", {"J06.9", "J03.9", "H66.9", "A09", "Z00.1", "R50.9"}},
        {"Kadın Hastalıkları ve Doğum", "B Blok, 1. kat", {"Z34.9", "N94.6", "N39.0", "Z00.0"}},
        {"Ortopedi ve Travmatoloji", "C Blok, Zemin kat", {"M54.5", "M17.9", "S93.4", "M25.5", "M54.2"}},
        {"Göz Hastalıkları", "C Blok, 1. kat", {"H52.1", "H10.9", "Z01.0"}},
        {"Kulak Burun Boğaz", "C Blok, 2. kat", {"J01.9", "J02.9", "J30.4", "H66.9", "R42"}},
        {"Dermatoloji", "A Blok, 2. kat", {"L70.0", "L20.9", "L50.9", "L40.9", "L30.9"}},
    };
    return list;
}

// Tanıya uygun reçete önerileri (etken madde, doz, kullanım, gün)
QList<PrescriptionItem> prescriptionFor(const QString &code)
{
    static const QHash<QString, QList<PrescriptionItem>> map = {
        {"I10", {{"Amlodipin", "5 mg", "Günde 1 kez, sabah", 30}}},
        {"E11.9", {{"Metformin", "1000 mg", "Günde 2 kez, tok karnına", 30}}},
        {"E78.5", {{"Atorvastatin", "20 mg", "Günde 1 kez, akşam", 30}}},
        {"K29.7", {{"Pantoprazol", "40 mg", "Günde 1 kez, sabah aç karnına", 14}}},
        {"D50.9", {{"Ferröz sülfat", "80 mg", "Günde 1 kez, aç karnına", 30}}},
        {"E55.9", {{"Kolekalsiferol (D vitamini)", "50.000 IU", "Haftada 1 kez", 56}}},
        {"J06.9", {{"Parasetamol", "500 mg", "Günde 3 kez, tok karnına", 5}}},
        {"J03.9", {{"Amoksisilin + Klavulanik asit", "1000 mg", "Günde 2 kez, tok karnına", 10}, {"Parasetamol", "500 mg", "Ateş olduğunda", 5}}},
        {"H66.9", {{"Amoksisilin", "500 mg", "Günde 3 kez, tok karnına", 7}}},
        {"I48", {{"Metoprolol", "50 mg", "Günde 1 kez, sabah", 30}}},
        {"I20.9", {{"Asetilsalisilik asit", "100 mg", "Günde 1 kez, öğle", 30}}},
        {"M54.5", {{"Naproksen", "550 mg", "Günde 2 kez, tok karnına", 7}, {"Tiyokolşikosid", "4 mg", "Günde 2 kez", 5}}},
        {"M17.9", {{"Diklofenak", "75 mg", "Günde 2 kez, tok karnına", 10}}},
        {"S93.4", {{"İbuprofen", "400 mg", "Günde 3 kez, tok karnına", 5}}},
        {"J01.9", {{"Amoksisilin + Klavulanik asit", "1000 mg", "Günde 2 kez, tok karnına", 10}}},
        {"J30.4", {{"Desloratadin", "5 mg", "Günde 1 kez", 30}, {"Flutikazon", "50 mcg", "Günde 1 kez, her burun deliğine 2 puf", 30}}},
        {"L70.0", {{"Klaritromisin", "500 mg", "Günde 2 kez", 14}}},
        {"L50.9", {{"Setirizin", "10 mg", "Günde 1 kez, akşam", 14}}},
        {"N39.0", {{"Siprofloksasin", "500 mg", "Günde 2 kez", 5}}},
        {"N94.6", {{"Naproksen", "550 mg", "Ağrı olduğunda, günde en fazla 2 kez", 5}}},
    };
    return map.value(code);
}

// Tanıyla uyumlu şikâyet (örnek veride şikâyet ve tanı birbirini tutsun)
QString complaintFor(const QString &code)
{
    static const QHash<QString, QString> complaints = {
        {"I10", "Baş ağrısı ve tansiyon yüksekliği"}, {"E11.9", "Çok su içme, sık idrara çıkma"},
        {"E78.5", "Kontrol tahlilleri"}, {"K29.7", "Mide ağrısı ve yanma"}, {"D50.9", "Halsizlik, çabuk yorulma"},
        {"E55.9", "Kas ve kemik ağrıları"}, {"J06.9", "Boğaz ağrısı, burun akıntısı"}, {"I20.9", "Eforla gelen göğüs ağrısı"},
        {"I48", "Çarpıntı"}, {"R07.4", "Göğüs ağrısı"}, {"I25.1", "Kontrol muayenesi"}, {"I50.9", "Nefes darlığı, ayaklarda şişlik"},
        {"J03.9", "Boğaz ağrısı ve ateş"}, {"H66.9", "Kulak ağrısı"}, {"A09", "İshal ve karın ağrısı"},
        {"Z00.1", "Rutin kontrol"}, {"R50.9", "Ateş"}, {"Z34.9", "Gebelik kontrolü"}, {"N94.6", "Adet sancısı"},
        {"N39.0", "İdrarda yanma"}, {"Z00.0", "Yıllık kontrol"}, {"M54.5", "Bel ağrısı"}, {"M17.9", "Diz ağrısı"},
        {"S93.4", "Ayak bileği burkulması"}, {"M25.5", "Omuz ağrısı"}, {"M54.2", "Boyun ağrısı"},
        {"H52.1", "Uzağı bulanık görme"}, {"H10.9", "Gözde kızarıklık ve çapaklanma"}, {"Z01.0", "Gözlük kontrolü"},
        {"J01.9", "Burun tıkanıklığı, yüzde ağrı"}, {"J02.9", "Boğaz ağrısı"}, {"J30.4", "Hapşırma ve burun akıntısı"},
        {"R42", "Baş dönmesi"}, {"L70.0", "Sivilce"}, {"L20.9", "Ciltte kaşıntı ve kuruluk"}, {"L50.9", "Kaşıntılı kabarıklıklar"},
        {"L40.9", "Ciltte pullanma"}, {"L30.9", "Kızarıklık ve döküntü"},
    };
    return complaints.value(code, "Kontrol muayenesi");
}

QString diagnosisName(const QString &code)
{
    static const QHash<QString, QString> names = {
        {"I10", "Esansiyel hipertansiyon"}, {"E11.9", "Tip 2 diyabet, komplikasyonsuz"}, {"E78.5", "Hiperlipidemi"},
        {"K29.7", "Gastrit"}, {"D50.9", "Demir eksikliği anemisi"}, {"E55.9", "D vitamini eksikliği"},
        {"J06.9", "Akut üst solunum yolu enfeksiyonu"}, {"I20.9", "Angina pektoris"}, {"I48", "Atriyal fibrilasyon"},
        {"R07.4", "Göğüs ağrısı"}, {"I25.1", "Aterosklerotik kalp hastalığı"}, {"I50.9", "Kalp yetmezliği"},
        {"J03.9", "Akut tonsillit"}, {"H66.9", "Orta kulak iltihabı (otitis media)"},
        {"A09", "Enfeksiyöz gastroenterit ve kolit"}, {"Z00.1", "Rutin çocuk sağlığı kontrolü"}, {"R50.9", "Ateş"},
        {"Z34.9", "Gebelik takibi"}, {"N94.6", "Dismenore"}, {"N39.0", "İdrar yolu enfeksiyonu"},
        {"Z00.0", "Genel sağlık muayenesi"}, {"M54.5", "Bel ağrısı"}, {"M17.9", "Diz osteoartriti"},
        {"S93.4", "Ayak bileği burkulması"}, {"M25.5", "Eklem ağrısı"}, {"M54.2", "Boyun ağrısı"},
        {"H52.1", "Miyopi"}, {"H10.9", "Konjonktivit"}, {"Z01.0", "Göz muayenesi"}, {"J01.9", "Akut sinüzit"},
        {"J02.9", "Akut farenjit"}, {"J30.4", "Alerjik rinit"}, {"R42", "Baş dönmesi"}, {"L70.0", "Akne"},
        {"L20.9", "Atopik dermatit"}, {"L50.9", "Ürtiker"}, {"L40.9", "Sedef hastalığı (psoriazis)"},
        {"L30.9", "Dermatit (egzama)"},
    };
    return names.value(code, code);
}

const char *const FIRST_F[] = {"Ayşe", "Fatma", "Zeynep", "Elif", "Merve", "Selin", "Ece", "Deniz", "Gizem", "Buse",
                               "Esra", "Hatice", "Emine", "Derya", "Nazlı", "İrem", "Ceren", "Duygu", "Sibel", "Aslı"};
const char *const FIRST_M[] = {"Mehmet", "Mustafa", "Ahmet", "Ali", "Hüseyin", "Can", "Emre", "Burak", "Mert", "Kerem",
                               "Hakan", "Serkan", "Oğuzhan", "Tolga", "Barış", "Murat", "Onur", "Yusuf", "Cem", "Eren"};
const char *const LAST[] = {"Yılmaz", "Kaya", "Demir", "Şahin", "Çelik", "Yıldız", "Yıldırım", "Öztürk", "Aydın",
                            "Özdemir", "Arslan", "Doğan", "Kılıç", "Aslan", "Çetin", "Kara", "Koç", "Kurt", "Özkan",
                            "Şimşek", "Polat", "Erdem", "Aksoy", "Tekin", "Güneş", "Karaca", "Yavuz", "Bulut"};

class Generator
{
public:
    explicit Generator(quint32 seed) : m_rng(seed) {}
    int between(int low, int high) { return low + int(m_rng.bounded(high - low + 1)); }
    bool chance(int percent) { return int(m_rng.bounded(100)) < percent; }
    template <typename T, size_t N> const T &pick(const T (&items)[N]) { return items[m_rng.bounded(int(N))]; }
    QString pick(const QStringList &items) { return items[int(m_rng.bounded(items.size()))]; }

private:
    QRandomGenerator m_rng;
};

QString nationalId(Generator &g)
{
    int d[11];
    d[0] = g.between(1, 9);
    for (int i = 1; i < 9; ++i)
        d[i] = g.between(0, 9);
    d[9] = (((d[0] + d[2] + d[4] + d[6] + d[8]) * 7 - (d[1] + d[3] + d[5] + d[7])) % 10 + 10) % 10;
    int sum = 0;
    for (int i = 0; i < 10; ++i)
        sum += d[i];
    d[10] = sum % 10;
    QString id;
    for (int digit : d)
        id += QChar('0' + digit);
    return id;
}

bool exec(QSqlQuery &q, const QString &sql, const QVariantList &values)
{
    return Sql::run(q, sql, values);
}

} // namespace

namespace DemoData {

const QList<Account> &accounts()
{
    static const QList<Account> list = {
        {"yonetici", "Demo-Yonetici-2026", "role_admin"},
        {"sekreter", "Demo-Sekreter-2026", "role_receptionist"},
        {"dr.ayse", "Demo-Doktor-2026", "role_doctor"},
        {"dr.mehmet", "Demo-Doktor-2026", "role_doctor"},
    };
    return list;
}

bool load(Database &db, const QDateTime &now)
{
    if (UserRepository(db).count() > 0)
        return false;
    Generator g(2026);
    QSqlDatabase c = db.connection();
    Transaction transaction(c); // hepsi ya da hiçbiri
    QSqlQuery q(c);

    // Bölümler ve doktorlar
    const char *const titles[] = {"Uzm. Dr.", "Uzm. Dr.", "Uzm. Dr.", "Doç. Dr.", "Prof. Dr.", "Op. Dr."};
    QList<Doctor> doctors;
    QHash<qint64, int> departmentOf; // doktor → bölüm sırası
    QSet<QString> usedNames;
    for (int i = 0; i < departments().size(); ++i) {
        const DepartmentSeed &dep = departments()[i];
        if (!exec(q, "INSERT INTO departments (name, location) VALUES (?, ?)", {dep.name, dep.location}))
            return false;
        const qint64 depId = q.lastInsertId().toLongLong();
        const int count = i < 4 ? 3 : 2;
        for (int k = 0; k < count; ++k) {
            Doctor d;
            QString name;
            do {
                name = QString::fromUtf8(g.chance(50) ? g.pick(FIRST_F) : g.pick(FIRST_M)) + " " + QString::fromUtf8(g.pick(LAST));
            } while (usedNames.contains(name));
            usedNames.insert(name);
            // İlk iki doktor demo hesaplarına bağlanır: Dr. Ayşe ve Dr. Mehmet
            if (doctors.isEmpty())
                name = "Ayşe Yılmaz";
            else if (doctors.size() == 3)
                name = "Mehmet Kaya";
            d.fullName = name;
            d.title = g.pick(titles);
            d.departmentId = depId;
            d.phone = QString("212%1").arg(g.between(1000000, 9999999));
            d.email = QString();
            d.workDays = g.chance(80) ? 0b0011111 : 0b0011011; // bazıları çarşamba çalışmaz
            d.startTime = QTime(g.chance(70) ? 9 : 8, 0);
            d.endTime = QTime(g.chance(70) ? 17 : 16, 0);
            d.slotMinutes = i == 5 ? 10 : (g.chance(60) ? 15 : 20); // göz polikliniği kısa randevu
            if (!exec(q, "INSERT INTO doctors (title, full_name, department_id, phone, email, work_days, start_time, "
                         "end_time, slot_minutes, active) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 1)",
                      {d.title, d.fullName, depId, d.phone, "", d.workDays, Sql::time(d.startTime),
                       Sql::time(d.endTime), d.slotMinutes}))
                return false;
            d.id = q.lastInsertId().toLongLong();
            doctors << d;
            departmentOf.insert(d.id, i);
        }
    }

    // Demo hesaplar (şifreler Argon2id ile)
    const QList<Account> &acc = accounts();
    const struct { int account; Role role; qint64 doctorId; QString fullName; } users[] = {
        {0, Role::Admin, 0, "Demo Yönetici"},
        {1, Role::Receptionist, 0, "Demo Sekreter"},
        {2, Role::Doctor, doctors[0].id, doctors[0].displayName()},
        {3, Role::Doctor, doctors[3].id, doctors[3].displayName()},
    };
    for (const auto &u : users) {
        const std::optional<QString> hash = Passwords::hash(acc[u.account].password);
        if (!hash || !exec(q, "INSERT INTO users (username, full_name, role, doctor_id, password_hash, active, must_change) "
                              "VALUES (?, ?, ?, ?, ?, 1, 0)",
                           {acc[u.account].username, u.fullName, static_cast<int>(u.role),
                            u.doctorId ? QVariant(u.doctorId) : QVariant(), *hash}))
            return false;
    }

    // Hastalar
    const char *const allergies[] = {"Penisilin", "Aspirin", "Polen", "Fıstık", "Lateks", "Sülfonamid"};
    const char *const chronic[] = {"Hipertansiyon", "Tip 2 diyabet", "Astım", "Hipotiroidi", "Migren"};
    struct SeedPatient {
        qint64 id;
        int age;
        bool female;
    };
    QList<SeedPatient> patients;
    QSet<QString> ids;
    for (int i = 0; i < PATIENT_COUNT; ++i) {
        const bool female = g.chance(52);
        QString id;
        do {
            id = nationalId(g);
        } while (ids.contains(id));
        ids.insert(id);
        const QString name = QString::fromUtf8(female ? g.pick(FIRST_F) : g.pick(FIRST_M)) + " " + QString::fromUtf8(g.pick(LAST));
        const QDate birth = now.date().addDays(-g.between(365, 85 * 365));
        if (!exec(q, "INSERT INTO patients (national_id, full_name, birth_date, gender, phone, email, blood_group, "
                     "allergies, chronic) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
                  {id, name, Sql::date(birth), static_cast<int>(female ? Gender::Female : Gender::Male),
                   QString("5%1%2").arg(g.between(30, 55)).arg(g.between(1000000, 9999999)),
                   g.chance(40) ? QString("hasta%1@example.com").arg(i + 1) : QString(""), g.between(1, 8),
                   g.chance(15) ? QString::fromUtf8(g.pick(allergies)) : QString(""),
                   g.chance(20) ? QString::fromUtf8(g.pick(chronic)) : QString("")}))
            return false;
        patients << SeedPatient{q.lastInsertId().toLongLong(), Rules::fullYears(birth, now.date()), female};
    }

    // Randevular: son 90 gün ve önümüzdeki 14 gün; geçmişte doluluk ~%45, gelecekte giderek azalır
    QSet<QString> patientBusy; // hasta + saat: aynı hasta aynı saatte iki yerde olmasın
    for (const Doctor &d : doctors) {
        const DepartmentSeed &dep = departments()[departmentOf.value(d.id)];
        for (QDate day = now.date().addDays(-90); day <= now.date().addDays(14); day = day.addDays(1)) {
            const int daysAhead = static_cast<int>(now.date().daysTo(day));
            const int fill = daysAhead <= 0 ? 45 : std::max(8, 50 - daysAhead * 4);
            for (const Schedule::Slot &slot : Schedule::daySlots(d, day, {}, QDateTime(day, QTime(0, 0)))) {
                if (slot.state == Schedule::SlotState::Lunch || !g.chance(fill))
                    continue;
                // Bölüme uygun hasta: çocuk polikliniğine 16 yaş altı, kadın doğuma yetişkin kadın, diğerlerine yetişkin
                SeedPatient seed{};
                for (int attempt = 0; attempt < 50; ++attempt) {
                    const SeedPatient &candidate = patients[g.between(0, patients.size() - 1)];
                    const int index = departmentOf.value(d.id);
                    const bool fits = index == PEDIATRICS ? candidate.age < 16
                                    : index == GYNECOLOGY ? candidate.female && candidate.age >= 16
                                                          : candidate.age >= 16;
                    if (fits) {
                        seed = candidate;
                        break;
                    }
                }
                if (!seed.id)
                    continue;
                const qint64 patient = seed.id;
                const QString key = QString::number(patient) + Sql::dateTime(slot.start);
                if (patientBusy.contains(key))
                    continue;
                patientBusy.insert(key);
                AppointmentStatus status = AppointmentStatus::Booked;
                if (slot.end <= now) {
                    const int roll = g.between(0, 99);
                    status = roll < 82 ? AppointmentStatus::Examined : roll < 92 ? AppointmentStatus::NoShow
                                                                                 : AppointmentStatus::Cancelled;
                } else if (slot.start <= now.addSecs(1800) && day == now.date()) {
                    status = g.chance(60) ? AppointmentStatus::CheckedIn : AppointmentStatus::Booked; // bekleme salonunda
                } else if (daysAhead > 0 && g.chance(5)) {
                    status = AppointmentStatus::Cancelled;
                }
                if (!exec(q, "INSERT INTO appointments (patient_id, doctor_id, start, end, status, note) VALUES (?, ?, ?, ?, ?, '')",
                          {patient, d.id, Sql::dateTime(slot.start), Sql::dateTime(slot.end), static_cast<int>(status)}))
                    return false;
                if (status != AppointmentStatus::Examined)
                    continue;
                const qint64 appointment = q.lastInsertId().toLongLong();
                QString code = g.pick(dep.diagnoses);
                if (code == "Z34.9" && seed.age > 42)
                    code = "Z00.0"; // gebelik takibi yaşa uygun olsun
                if (!exec(q, "INSERT INTO examinations (appointment_id, patient_id, doctor_id, date, complaint, findings, "
                             "diagnosis_code, diagnosis_name, notes) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
                          {appointment, patient, d.id, Sql::dateTime(slot.start.addSecs(300)), complaintFor(code),
                           g.chance(50) ? QString("Genel durum iyi, bilinç açık.") : QString(""), code,
                           diagnosisName(code), g.chance(30) ? QString("Kontrol önerildi.") : QString("")}))
                    return false;
                const qint64 exam = q.lastInsertId().toLongLong();
                const QList<PrescriptionItem> items = g.chance(75) ? prescriptionFor(code) : QList<PrescriptionItem>();
                for (int i = 0; i < items.size(); ++i)
                    if (!exec(q, "INSERT INTO prescription_items (examination_id, position, medicine, dose, usage, days) "
                                 "VALUES (?, ?, ?, ?, ?, ?)",
                              {exam, i, items[i].medicine, items[i].dose, items[i].usage, items[i].days}))
                        return false;
            }
        }
    }
    return transaction.commit();
}

} // namespace DemoData
