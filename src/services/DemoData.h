#pragma once

#include <QDateTime>
#include <QList>
#include <QString>

class Database;

// Uygulamayı denemek için örnek veri: 8 bölüm, 20 doktor, 1.200 hasta, son 3 ayın ve önümüzdeki 2 haftanın
// randevuları, muayeneleri ve reçeteleri; her rol için bir demo hesap. Sadece hiç kullanıcı yokken yüklenir.
namespace DemoData {

struct Account {
    QString username;
    QString password;
    QString roleKey; // metin anahtarı: "role_admin" gibi
};

const QList<Account> &accounts();
bool load(Database &db, const QDateTime &now = QDateTime::currentDateTime());

} // namespace DemoData
