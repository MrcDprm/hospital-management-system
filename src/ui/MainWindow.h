#pragma once

#include "core/Models.h"

#include <QMainWindow>

class AutoLock;
class Database;
class Settings;
class QListWidget;
class QPushButton;
class QStackedWidget;

// Ana pencere. Üç hâl: ilk kurulum (hiç kullanıcı yok), giriş ve çalışma alanı.
// Çalışma alanında soldaki menü role göre kurulur; hareketsiz kalınca oturum kendiliğinden kapanır.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Database &db, Settings &settings, const QString &dataFolder);

private:
    void showAuth(const QString &message = QString());
    void showWorkspace();
    QWidget *buildWorkspace();
    QWidget *authBar();
    void replaceScreen(QWidget *screen);
    void createAdmin(const QString &username, const QString &fullName, const QString &password);
    void loadDemo();
    void login(const QString &username, const QString &password);
    void signOut(const QString &message = QString());
    void showPage(int index);
    void toggleTheme();
    void toggleLanguage();
    void rebuild();

    Database &m_db;
    Settings &m_settings;
    QString m_dataFolder;
    User m_user;
    bool m_signedIn = false;
    AutoLock *m_autoLock;
    QWidget *m_screen = nullptr;
    QListWidget *m_navigation = nullptr;
    QStackedWidget *m_pages = nullptr;
};
