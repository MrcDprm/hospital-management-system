#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVariant>

// Boş QString "null" sayılır ve SQL'e NULL olarak gider; NOT NULL sütunlar için boş metne çevrilir
inline QVariant text(const QString &value)
{
    return value.isNull() ? QString("") : value;
}

// İşlem sonucu: hata yoksa error boş kalır. Hata metin değil anahtar ("duplicate_national_id");
// arayüz seçili dile göre mesaja çevirir.
struct Result {
    QString error;
    qint64 id = 0;
    bool ok() const { return error.isEmpty(); }
    static Result failure(const QString &key) { return {key, 0}; }
};

// Veritabanı işlemi (transaction): commit edilmeden çıkılırsa (hata, erken return) değişiklikler geri alınır.
// C++'ta kaynak nesnenin ömrüne bağlanır (RAII): yıkıcı her durumda çalışır.
class Transaction
{
public:
    explicit Transaction(QSqlDatabase db) : m_db(db) { m_db.transaction(); }
    ~Transaction()
    {
        if (!m_committed)
            m_db.rollback();
    }
    Transaction(const Transaction &) = delete;
    Transaction &operator=(const Transaction &) = delete;
    bool commit() { return m_committed = m_db.commit(); }

private:
    QSqlDatabase m_db;
    bool m_committed = false;
};

// SQLite bağlantısı ve tablolar. Veriler kullanıcı klasöründeki tek bir dosyada tutulur.
class Database
{
public:
    Database();
    ~Database();
    Database(const Database &) = delete;            // bağlantı kopyalanamaz
    Database &operator=(const Database &) = delete;

    bool open(const QString &path);
    QString lastError() const { return m_lastError; }
    QSqlDatabase connection() const;

private:
    bool createSchema();

    QString m_name;
    QString m_lastError;
};