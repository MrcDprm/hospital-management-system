#pragma once

#include "core/Models.h"
#include "core/Schedule.h"

#include <QString>

// Arayüz metinleri (Türkçe / İngilizce). Metinler anahtarla istenir: I18n::t("patients").
// {0}, {1}… yer tutucuları çağıran tarafta doldurulur.
namespace I18n {

void setLanguage(const QString &language); // "tr" ya da "en"
QString language();

QString t(const char *key);
QString error(const QString &key); // veri katmanından gelen hata anahtarları ("slot_taken")

QString role(Role value);
QString gender(Gender value);
QString bloodGroup(BloodGroup value);
QString status(AppointmentStatus value);
QString slotState(Schedule::SlotState value);
QString weekday(int day); // 1 = pazartesi … 7 = pazar
QString date(const QDate &value);
QString dateTime(const QDateTime &value);
QString phone(const QString &digits); // "5321234567" → "0532 123 45 67"
QString age(const QDate &birthDate, const QDate &today = QDate::currentDate());

} // namespace I18n
