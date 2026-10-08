#include "core/Passwords.h"

#include <sodium.h>

namespace Passwords {

bool init()
{
    return sodium_init() >= 0;
}

QStringList problems(const QString &password, const QString &username)
{
    QStringList result;
    if (password.size() < MIN_LENGTH)
        result << "password_short";
    if (password.size() > MAX_LENGTH)
        result << "password_long";
    const auto has = [&](auto test) { return std::any_of(password.begin(), password.end(), test); };
    if (!has([](QChar c) { return c.isUpper(); }) || !has([](QChar c) { return c.isLower(); })
        || !has([](QChar c) { return c.isDigit(); }))
        result << "password_variety";
    if (!username.isEmpty() && password.contains(username, Qt::CaseInsensitive))
        result << "password_has_username";
    return result;
}

std::optional<QString> hash(const QString &password)
{
    QByteArray utf8 = password.toUtf8();
    char out[crypto_pwhash_STRBYTES];
    // Çıktı algoritmayı, ayarları ve rastgele tuzu kendi içinde taşır: "$argon2id$v=19$m=...,t=...,p=1$tuz$özet"
    const bool ok = crypto_pwhash_str(out, utf8.constData(), static_cast<unsigned long long>(utf8.size()),
                                      crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE) == 0;
    sodium_memzero(utf8.data(), static_cast<size_t>(utf8.size()));
    if (!ok)
        return std::nullopt;
    return QString::fromLatin1(out);
}

bool verify(const QString &password, const QString &storedHash)
{
    QByteArray utf8 = password.toUtf8();
    const QByteArray stored = storedHash.toLatin1();
    // Karşılaştırma sabit sürede yapılır; bozuk hash'te de güvenle false döner
    const bool ok = !stored.isEmpty()
                    && crypto_pwhash_str_verify(stored.constData(), utf8.constData(),
                                                static_cast<unsigned long long>(utf8.size())) == 0;
    sodium_memzero(utf8.data(), static_cast<size_t>(utf8.size()));
    return ok;
}

} // namespace Passwords
