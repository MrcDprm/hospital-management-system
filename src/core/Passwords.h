#pragma once

#include <QStringList>
#include <optional>

// Kullanıcı şifreleri: kurallar ve Argon2id hash'i (libsodium). Şifre düz metin olarak hiçbir yerde saklanmaz.
namespace Passwords {

constexpr int MIN_LENGTH = 10;
constexpr int MAX_LENGTH = 128;

bool init();
QStringList problems(const QString &password, const QString &username); // hata anahtarları
std::optional<QString> hash(const QString &password);
bool verify(const QString &password, const QString &storedHash);

} // namespace Passwords
