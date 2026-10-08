#pragma once

#include "core/Models.h"
#include "data/Database.h"

#include <optional>

// Kullanıcı hesapları ve giriş. Şifreler Argon2id hash'i olarak saklanır.
// Art arda 5 yanlış girişte hesap 30 saniye kilitlenir (kalıcı: program kapanıp açılsa da sürer).
class UserRepository
{
public:
    static constexpr int MAX_FAILED = 5;
    static constexpr int LOCK_SECONDS = 30;

    explicit UserRepository(Database &db) : m_db(db) {}

    int count() const;
    QList<User> all(const User &actor) const;
    std::optional<User> find(qint64 id) const;

    // İlk kurulum: hiç kullanıcı yokken tek seferlik yönetici hesabı
    Result createFirstAdmin(User user, const QString &password);
    // Giriş: kullanıcı yok ve şifre yanlış aynı hatayı verir ("login_failed"); kim olduğu tahmin edilemesin
    Result authenticate(const QString &username, const QString &password, const QDateTime &now = QDateTime::currentDateTime());

    Result add(const User &actor, User user, const QString &password);
    Result update(const User &actor, User user);
    Result resetPassword(const User &actor, qint64 id, const QString &password); // ilk girişte değiştirilmek üzere
    Result changeOwnPassword(qint64 id, const QString &current, const QString &next);

private:
    Result insert(User user, const QString &password);
    QString hashFor(qint64 id) const;

    Database &m_db;
};
