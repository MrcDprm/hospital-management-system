#include "app/Settings.h"

#include <QRegularExpression>
#include <QSettings>

Settings::Settings(const QString &dataFolder)
    : m_path(dataFolder + "/settings.ini")
{
}

QString Settings::language() const
{
    const QString value = QSettings(m_path, QSettings::IniFormat).value("language", "tr").toString();
    return value == "en" ? "en" : "tr";
}

void Settings::setLanguage(const QString &language)
{
    QSettings(m_path, QSettings::IniFormat).setValue("language", language == "en" ? "en" : "tr");
}

bool Settings::darkTheme() const
{
    return QSettings(m_path, QSettings::IniFormat).value("theme", "dark").toString() != "light";
}

void Settings::setDarkTheme(bool dark)
{
    QSettings(m_path, QSettings::IniFormat).setValue("theme", dark ? "dark" : "light");
}

QString Settings::lastUsername() const
{
    // Kullanıcı adı kuralına uymayan değer (dosya elle değiştirilmiş) yok sayılır
    static const QRegularExpression name("^[a-z0-9._-]{3,30}$");
    const QString value = QSettings(m_path, QSettings::IniFormat).value("last_username").toString();
    return name.match(value).hasMatch() ? value : QString();
}

void Settings::setLastUsername(const QString &username)
{
    QSettings(m_path, QSettings::IniFormat).setValue("last_username", username);
}

bool Settings::demoLoaded() const
{
    return QSettings(m_path, QSettings::IniFormat).value("demo_loaded", false).toBool();
}

void Settings::setDemoLoaded(bool loaded)
{
    QSettings(m_path, QSettings::IniFormat).setValue("demo_loaded", loaded);
}
