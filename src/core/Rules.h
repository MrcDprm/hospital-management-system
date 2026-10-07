#pragma once

#include "core/Models.h"

#include <QStringList>

// Girdi doğrulama. Hatalar metin değil anahtar olarak döner ("invalid_national_id");
// ekranda gösterilecek metni arayüz, seçili dile göre seçer.
namespace Rules {

constexpr int MAX_NAME = 100;
constexpr int MAX_TEXT = 2000;
constexpr int MIN_SLOT = 5;
constexpr int MAX_SLOT = 120;

bool isValidNationalId(const QString &id);
bool isValidEmail(const QString &email);
QString normalizePhone(const QString &text);
bool isValidPhone(const QString &phone);
int fullYears(const QDate &from, const QDate &to);

QStringList patientProblems(const Patient &patient, const QDate &today);
QStringList doctorProblems(const Doctor &doctor);
QStringList departmentProblems(const Department &department);
QStringList examinationProblems(const Examination &exam);

} // namespace Rules
