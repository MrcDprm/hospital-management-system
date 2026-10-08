#pragma once

#include "core/Access.h"
#include "data/Database.h"

#include <QDateTime>
#include <QSqlQuery>
#include <QVariant>

// Depoların ortak küçük yardımcıları: tarih biçimleri, enum okuma, sorgu çalıştırma, yetki kontrolü.
namespace Sql {

inline QString date(const QDate &d) { return d.toString(Qt::ISODate); }
inline QString dateTime(const QDateTime &d) { return d.toString("yyyy-MM-ddTHH:mm:ss"); } // yerel saat, saniyeye kadar
inline QString time(const QTime &t) { return t.toString("HH:mm"); }
inline QDate toDate(const QVariant &v) { return QDate::fromString(v.toString(), Qt::ISODate); }
inline QDateTime toDateTime(const QVariant &v) { return QDateTime::fromString(v.toString(), "yyyy-MM-ddTHH:mm:ss"); }
inline QTime toTime(const QVariant &v) { return QTime::fromString(v.toString(), "HH:mm"); }

// Veritabanından okunan sayı bilinmeyen bir enum değeriyse varsayılana döner (dosya elle bozulmuş olabilir)
template <typename Enum>
Enum toEnum(const QVariant &value, Enum last, Enum fallback)
{
    const int number = value.toInt();
    return number >= 0 && number <= static_cast<int>(last) ? static_cast<Enum>(number) : fallback;
}

// Parametreli sorgu: değerler "?" yerlerine sırayla bağlanır; metin birleştirme yapılmaz (SQL injection'a kapalı)
inline bool run(QSqlQuery &query, const QString &sql, const QVariantList &values = {})
{
    query.prepare(sql);
    for (const QVariant &value : values)
        query.addBindValue(value);
    return query.exec();
}

inline bool can(const User &actor, Permission permission)
{
    return actor.active && Access::allowed(actor.role, permission);
}

} // namespace Sql
