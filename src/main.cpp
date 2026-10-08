#include "app/I18n.h"
#include "app/Settings.h"
#include "app/Theme.h"
#include "core/Passwords.h"
#include "data/Database.h"
#include "ui/MainWindow.h"
#include "ui/UiHelpers.h"

#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("HospitalManager");
    app.setOrganizationName("MrcDprm");
    app.setApplicationVersion(APP_VERSION);
    app.setWindowIcon(QIcon(":/icon.png"));

    // Veritabanı ve ayarlar program klasörüne değil kullanıcı klasörüne yazılır: %APPDATA%\MrcDprm\HospitalManager
    const QString dataFolder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataFolder);

    Settings settings(dataFolder);
    I18n::setLanguage(settings.language());
    Theme::apply(app, settings.darkTheme());

    if (!Passwords::init()) {
        Ui::message(nullptr, QMessageBox::Critical, I18n::t("crypto_failed"));
        return 1;
    }
    Database db;
    if (!db.open(dataFolder + "/hospital.db")) {
        // Ayrıntılı hata kullanıcıya gösterilmez; sadece ne olduğu ve dosyanın yeri söylenir
        Ui::message(nullptr, QMessageBox::Critical,
                    I18n::t("db_open_failed").replace("{0}", QDir::toNativeSeparators(dataFolder)));
        return 1;
    }

    MainWindow window(db, settings, dataFolder);
    window.show();
    return app.exec();
}
