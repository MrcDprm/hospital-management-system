#pragma once

#include <QString>

// Kullanıcı ayarları (dil, tema, son kullanıcı adı) kullanıcı klasöründeki settings.ini dosyasında tutulur.
// Dosyadan okunan değerlere güvenilmez: bilinmeyen değer yerine varsayılan kullanılır.
class Settings
{
public:
    explicit Settings(const QString &dataFolder);

    QString language() const;
    void setLanguage(const QString &language);
    bool darkTheme() const;
    void setDarkTheme(bool dark);
    QString lastUsername() const; // giriş ekranında hatırlanır (şifre asla)
    void setLastUsername(const QString &username);
    bool demoLoaded() const;      // demo hesapları giriş ekranında gösterilsin mi
    void setDemoLoaded(bool loaded);

private:
    QString m_path;
};
