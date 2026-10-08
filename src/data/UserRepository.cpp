#include "data/UserRepository.h"

#include "core/Passwords.h"
#include "data/SqlUtil.h"

#include <QRegularExpression>

namespace {

const QString COLUMNS = "id, username, full_name, role, doctor_id, active, must_change";

User fromQuery(const QSqlQuery &q)
{
    User u;
    u.id = q.value(0).toLongLong();
    u.username = q.value(1).toString();
    u.fullName = q.value(2).toString();
    u.role = Sql::toEnum(q.value(3), Role::Doctor, Role::Receptionist);
    u.doctorId = q.value(4).toLongLong();
    u.active = q.value(5).toBool();
    u.mustChangePassword = q.value(6).toBool();
    return u;
}

QString checkUser(const User &u)
{
    static const QRegularExpression name("^[a-z0-9._-]{3,30}$");
    if (!name.match(u.username).hasMatch())
        return "invalid_username";
    if (u.fullName.trimmed().size() < 3 || u.fullName.size() > 100)
        return "invalid_name";
    if (u.role == Role::Doctor && u.doctorId <= 0)
        return "doctor_link_required"; // doktor hesabı bir doktor kaydına bağlı olmalı
    return QString();
}

} // namespace

int UserRepository::count() const
{
    QSqlQuery q(m_db.connection());
    return Sql::run(q, "SELECT COUNT(*) FROM users") && q.next() ? q.value(0).toInt() : 0;
}

QList<User> UserRepository::all(const User &actor) const
{
    QList<User> users;
    if (!Sql::can(actor, Permission::ManageUsers))
        return users;
    QSqlQuery q(m_db.connection());
    if (Sql::run(q, "SELECT " + COLUMNS + " FROM users ORDER BY role, full_name"))
        while (q.next())
            users << fromQuery(q);
    return users;
}

std::optional<User> UserRepository::find(qint64 id) const
{
    QSqlQuery q(m_db.connection());
    if (Sql::run(q, "SELECT " + COLUMNS + " FROM users WHERE id = ?", {id}) && q.next())
        return fromQuery(q);
    return std::nullopt;
}

QString UserRepository::hashFor(qint64 id) const
{
    QSqlQuery q(m_db.connection());
    return Sql::run(q, "SELECT password_hash FROM users WHERE id = ?", {id}) && q.next() ? q.value(0).toString()
                                                                                       : QString();
}

Result UserRepository::insert(User user, const QString &password)
{
    user.username = user.username.trimmed().toLower();
    user.fullName = user.fullName.trimmed();
    if (const QString error = checkUser(user); !error.isEmpty())
        return Result::failure(error);
    if (const QStringList problems = Passwords::problems(password, user.username); !problems.isEmpty())
        return Result::failure(problems.first());
    const std::optional<QString> hash = Passwords::hash(password);
    if (!hash)
        return Result::failure("save_failed");
    QSqlQuery q(m_db.connection());
    if (!Sql::run(q,
                  "INSERT INTO users (username, full_name, role, doctor_id, password_hash, active, must_change) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?)",
                  {user.username, user.fullName, static_cast<int>(user.role),
                   user.role == Role::Doctor ? QVariant(user.doctorId) : QVariant(), *hash, user.active,
                   user.mustChangePassword}))
        return Result::failure("duplicate_username");
    return {QString(), q.lastInsertId().toLongLong()};
}

Result UserRepository::createFirstAdmin(User user, const QString &password)
{
    // Sadece hiç kullanıcı yokken: sonradan kimse bu yolla yönetici olamaz
    Transaction transaction(m_db.connection());
    if (count() > 0)
        return Result::failure("forbidden");
    user.role = Role::Admin;
    user.active = true;
    user.mustChangePassword = false;
    const Result result = insert(user, password);
    if (!result.ok() || !transaction.commit())
        return result.ok() ? Result::failure("save_failed") : result;
    return result;
}

Result UserRepository::authenticate(const QString &username, const QString &password, const QDateTime &now)
{
    QSqlQuery q(m_db.connection());
    if (!Sql::run(q, "SELECT id, password_hash, active, failed_attempts, locked_until FROM users WHERE username = ?",
                  {username.trimmed().toLower()})
        || !q.next()) {
        // Kullanıcı yoksa da bir hash doğrulanır: yanıt süresinden kullanıcının varlığı anlaşılmasın
        static const QString dummy = Passwords::hash("Timing-Dummy-2026").value_or(QString());
        Passwords::verify(password, dummy);
        return Result::failure("login_failed");
    }
    const qint64 id = q.value(0).toLongLong();
    const QString hash = q.value(1).toString();
    const bool active = q.value(2).toBool();
    const int failed = q.value(3).toInt();
    const QDateTime lockedUntil = Sql::toDateTime(q.value(4));
    if (lockedUntil.isValid() && lockedUntil > now)
        return Result::failure("account_locked");

    QSqlQuery update(m_db.connection());
    if (!Passwords::verify(password, hash)) {
        const int attempts = failed + 1;
        // 5. yanlışta 30 saniye kilit; sayaç sıfırlanır ki sonraki 5 deneme için yeniden başlasın
        const QVariant lock = attempts >= MAX_FAILED ? QVariant(Sql::dateTime(now.addSecs(LOCK_SECONDS))) : QVariant();
        Sql::run(update, "UPDATE users SET failed_attempts = ?, locked_until = ? WHERE id = ?",
                 {attempts >= MAX_FAILED ? 0 : attempts, lock, id});
        return Result::failure(attempts >= MAX_FAILED ? "account_locked" : "login_failed");
    }
    if (!active) // şifre doğruysa söylenir: pasif hesabın sahibi neden giremediğini bilsin
        return Result::failure("account_inactive");
    Sql::run(update, "UPDATE users SET failed_attempts = 0, locked_until = NULL WHERE id = ?", {id});
    return {QString(), id};
}

Result UserRepository::add(const User &actor, User user, const QString &password)
{
    if (!Sql::can(actor, Permission::ManageUsers))
        return Result::failure("forbidden");
    user.mustChangePassword = true; // yöneticinin verdiği geçici şifre ilk girişte değiştirilir
    return insert(user, password);
}

Result UserRepository::update(const User &actor, User user)
{
    if (!Sql::can(actor, Permission::ManageUsers))
        return Result::failure("forbidden");
    user.fullName = user.fullName.trimmed();
    user.username = user.username.trimmed().toLower();
    if (const QString error = checkUser(user); !error.isEmpty())
        return Result::failure(error);
    // Yönetici kendini pasif yapamaz ve kendi yetkisini düşüremez: sistem yöneticisiz kalmasın
    if (user.id == actor.id && (!user.active || user.role != Role::Admin))
        return Result::failure("cannot_demote_self");
    QSqlQuery q(m_db.connection());
    if (!Sql::run(q, "UPDATE users SET username = ?, full_name = ?, role = ?, doctor_id = ?, active = ? WHERE id = ?",
                  {user.username, user.fullName, static_cast<int>(user.role),
                   user.role == Role::Doctor ? QVariant(user.doctorId) : QVariant(), user.active, user.id}))
        return Result::failure("duplicate_username");
    if (q.numRowsAffected() == 0)
        return Result::failure("not_found");
    return {QString(), user.id};
}

Result UserRepository::resetPassword(const User &actor, qint64 id, const QString &password)
{
    if (!Sql::can(actor, Permission::ManageUsers))
        return Result::failure("forbidden");
    const std::optional<User> user = find(id);
    if (!user)
        return Result::failure("not_found");
    if (const QStringList problems = Passwords::problems(password, user->username); !problems.isEmpty())
        return Result::failure(problems.first());
    const std::optional<QString> hash = Passwords::hash(password);
    QSqlQuery q(m_db.connection());
    if (!hash || !Sql::run(q, "UPDATE users SET password_hash = ?, must_change = 1, failed_attempts = 0, "
                              "locked_until = NULL WHERE id = ?", {*hash, id}))
        return Result::failure("save_failed");
    return {QString(), id};
}

Result UserRepository::changeOwnPassword(qint64 id, const QString &current, const QString &next)
{
    const std::optional<User> user = find(id);
    if (!user)
        return Result::failure("not_found");
    // Mevcut şifre yeniden istenir: açık bırakılmış oturumda başkası şifreyi değiştiremesin
    if (!Passwords::verify(current, hashFor(id)))
        return Result::failure("wrong_current_password");
    if (current == next)
        return Result::failure("same_password");
    if (const QStringList problems = Passwords::problems(next, user->username); !problems.isEmpty())
        return Result::failure(problems.first());
    const std::optional<QString> hash = Passwords::hash(next);
    QSqlQuery q(m_db.connection());
    if (!hash || !Sql::run(q, "UPDATE users SET password_hash = ?, must_change = 0 WHERE id = ?", {*hash, id}))
        return Result::failure("save_failed");
    return {QString(), id};
}
