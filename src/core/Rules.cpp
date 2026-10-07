#include "core/Rules.h"

#include <QRegularExpression>

namespace Rules {

bool isValidNationalId(const QString &id)
{
    // T.C. kimlik numarası: 11 hane, ilk hane 0 değil, son iki hane sağlama basamağı
    static const QRegularExpression digits("^[1-9]\\d{10}$");
    if (!digits.match(id).hasMatch())
        return false;
    int d[11];
    for (int i = 0; i < 11; ++i)
        d[i] = id.at(i).digitValue();
    const int odd = d[0] + d[2] + d[4] + d[6] + d[8];
    const int even = d[1] + d[3] + d[5] + d[7];
    const int tenth = ((odd * 7 - even) % 10 + 10) % 10;
    int sum = 0;
    for (int i = 0; i < 10; ++i)
        sum += d[i];
    return d[9] == tenth && d[10] == sum % 10;
}

bool isValidEmail(const QString &email)
{
    static const QRegularExpression format("^[^@\\s]+@[^@\\s]+\\.[^@\\s]{2,}$");
    return email.size() <= MAX_NAME && format.match(email).hasMatch();
}

QString normalizePhone(const QString &text)
{
    // Boşluk, tire ve parantezler atılır; +90 ya da baştaki 0 kaldırılır: "0 (532) 123 45 67" → "5321234567"
    QString digits;
    for (const QChar c : text)
        if (c.isDigit())
            digits += c;
    if (digits.startsWith("90") && digits.size() == 12)
        digits.remove(0, 2);
    if (digits.startsWith('0') && digits.size() == 11)
        digits.remove(0, 1);
    return digits;
}

bool isValidPhone(const QString &phone)
{
    static const QRegularExpression format("^[2-5]\\d{9}$");
    return format.match(phone).hasMatch();
}

int fullYears(const QDate &from, const QDate &to)
{
    int years = to.year() - from.year();
    if (to.month() < from.month() || (to.month() == from.month() && to.day() < from.day()))
        --years;
    return years;
}

QStringList patientProblems(const Patient &p, const QDate &today)
{
    QStringList problems;
    if (p.fullName.trimmed().size() < 3 || p.fullName.size() > MAX_NAME)
        problems << "invalid_name";
    if (!isValidNationalId(p.nationalId))
        problems << "invalid_national_id";
    if (!p.birthDate.isValid() || p.birthDate > today || fullYears(p.birthDate, today) > 120)
        problems << "invalid_birth_date";
    if (!p.phone.isEmpty() && !isValidPhone(p.phone))
        problems << "invalid_phone";
    if (!p.email.isEmpty() && !isValidEmail(p.email))
        problems << "invalid_email";
    if (p.allergies.size() > MAX_TEXT || p.chronicConditions.size() > MAX_TEXT)
        problems << "text_too_long";
    return problems;
}

QStringList doctorProblems(const Doctor &d)
{
    QStringList problems;
    if (d.fullName.trimmed().size() < 3 || d.fullName.size() > MAX_NAME || d.title.size() > 30)
        problems << "invalid_name";
    if (d.departmentId <= 0)
        problems << "invalid_department";
    if (!d.phone.isEmpty() && !isValidPhone(d.phone))
        problems << "invalid_phone";
    if (!d.email.isEmpty() && !isValidEmail(d.email))
        problems << "invalid_email";
    if ((d.workDays & 0x7F) == 0 || (d.workDays & ~0x7F) != 0)
        problems << "invalid_work_days";
    if (!d.startTime.isValid() || !d.endTime.isValid() || d.startTime.addSecs(d.slotMinutes * 60) > d.endTime)
        problems << "invalid_hours";
    if (d.slotMinutes < MIN_SLOT || d.slotMinutes > MAX_SLOT)
        problems << "invalid_slot";
    return problems;
}

QStringList departmentProblems(const Department &d)
{
    QStringList problems;
    if (d.name.trimmed().size() < 2 || d.name.size() > MAX_NAME)
        problems << "invalid_name";
    if (d.location.size() > MAX_NAME)
        problems << "text_too_long";
    return problems;
}

QStringList examinationProblems(const Examination &e)
{
    QStringList problems;
    if (e.complaint.trimmed().isEmpty())
        problems << "complaint_required";
    if (e.diagnosisName.trimmed().isEmpty())
        problems << "diagnosis_required";
    if (e.complaint.size() > MAX_TEXT || e.findings.size() > MAX_TEXT || e.notes.size() > MAX_TEXT
        || e.diagnosisName.size() > 200 || e.diagnosisCode.size() > 10)
        problems << "text_too_long";
    if (e.prescription.size() > 20)
        problems << "too_many_medicines";
    for (const PrescriptionItem &item : e.prescription)
        if (item.medicine.trimmed().isEmpty() || item.medicine.size() > 120 || item.dose.size() > 60
            || item.usage.size() > 120 || item.days < 1 || item.days > 365) {
            problems << "invalid_prescription";
            break;
        }
    return problems;
}

} // namespace Rules
