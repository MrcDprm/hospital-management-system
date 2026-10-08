#include "ui/MainWindow.h"

#include "app/Settings.h"
#include "core/Access.h"
#include "data/UserRepository.h"
#include "services/AutoLock.h"
#include "services/DemoData.h"
#include "ui/AboutDialog.h"
#include "ui/AppointmentsPage.h"
#include "ui/CalendarPage.h"
#include "ui/LoginPage.h"
#include "ui/PatientsPage.h"
#include "ui/ReportsPage.h"
#include "ui/SetupPage.h"
#include "ui/StaffPage.h"
#include "ui/UiHelpers.h"
#include "ui/UserDialogs.h"
#include "ui/UsersPage.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

constexpr int IDLE_MINUTES = 10; // ortak bilgisayarda açık kalan oturum kapanır

} // namespace

MainWindow::MainWindow(Database &db, Settings &settings, const QString &dataFolder)
    : m_db(db), m_settings(settings), m_dataFolder(dataFolder), m_autoLock(new AutoLock(this))
{
    qApp->installEventFilter(m_autoLock);
    connect(m_autoLock, &AutoLock::timedOut, this, [this] {
        // Açık pencereler (form, menü) kapatılır; kaydedilmemiş değişiklik kaydedilmez
        while (QWidget *modal = QApplication::activeModalWidget())
            modal->close();
        signOut(I18n::t("signed_out_idle"));
    });
    setWindowTitle(I18n::t("app_name"));
    resize(1240, 760);
    showAuth();
}

void MainWindow::replaceScreen(QWidget *screen)
{
    m_screen = screen;
    setCentralWidget(screen); // eski ekranı Qt siler (deleteLater: o ekranın düğmesinden çağrılsa da güvenli)
}

QWidget *MainWindow::authBar()
{
    // Giriş ekranlarında sağ üstte tema ve dil düğmeleri
    auto *bar = new QWidget;
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(12, 8, 12, 0);
    layout->addStretch();
    auto *theme = new QPushButton((Theme::isDark() ? "☀  " : "☾  ") + I18n::t("theme"), bar);
    auto *language = new QPushButton("🌐  " + I18n::t("language"), bar);
    auto *about = new QPushButton("ⓘ  " + I18n::t("about"), bar);
    for (QPushButton *button : {theme, language, about}) {
        button->setFlat(true);
        layout->addWidget(button);
    }
    connect(theme, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    connect(language, &QPushButton::clicked, this, &MainWindow::toggleLanguage);
    connect(about, &QPushButton::clicked, this, [this] { AboutDialog(m_dataFolder, this).exec(); });
    return bar;
}

void MainWindow::showAuth(const QString &message)
{
    auto *screen = new QWidget;
    auto *layout = new QVBoxLayout(screen);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(authBar());
    if (UserRepository(m_db).count() == 0) {
        auto *setup = new SetupPage(screen);
        connect(setup, &SetupPage::createAdmin, this, &MainWindow::createAdmin);
        connect(setup, &SetupPage::loadDemo, this, &MainWindow::loadDemo);
        layout->addWidget(setup, 1);
    } else {
        auto *loginPage = new LoginPage(m_settings.lastUsername(), m_settings.demoLoaded(), screen);
        connect(loginPage, &LoginPage::loginRequested, this, &MainWindow::login);
        layout->addWidget(loginPage, 1);
        loginPage->reset();
        if (!message.isEmpty())
            loginPage->failed(message); // ör. hareketsizlik nedeniyle çıkış
    }
    m_navigation = nullptr;
    m_pages = nullptr;
    replaceScreen(screen);
}

void MainWindow::createAdmin(const QString &username, const QString &fullName, const QString &password)
{
    User admin;
    admin.username = username;
    admin.fullName = fullName;
    admin.role = Role::Admin;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const Result result = UserRepository(m_db).createFirstAdmin(admin, password);
    QApplication::restoreOverrideCursor();
    if (!result.ok()) {
        if (auto *setup = m_screen->findChild<SetupPage *>())
            setup->showError(I18n::error(result.error));
        return;
    }
    // Hesabı oluşturan kişi doğrudan içeri alınır
    m_user = UserRepository(m_db).find(result.id).value_or(User{});
    m_settings.setLastUsername(m_user.username);
    showWorkspace();
}

void MainWindow::loadDemo()
{
    QApplication::setOverrideCursor(Qt::WaitCursor); // 4 hesabın şifre hash'i ve binlerce kayıt birkaç saniye sürer
    const bool loaded = DemoData::load(m_db);
    QApplication::restoreOverrideCursor();
    if (!loaded) {
        Ui::warn(this, I18n::t("demo_failed"));
        return;
    }
    m_settings.setDemoLoaded(true);
    showAuth();
}

void MainWindow::login(const QString &username, const QString &password)
{
    const Result result = UserRepository(m_db).authenticate(username, password);
    auto *loginPage = m_screen->findChild<LoginPage *>();
    if (!result.ok()) {
        if (loginPage)
            loginPage->failed(I18n::error(result.error));
        return;
    }
    m_user = UserRepository(m_db).find(result.id).value_or(User{});
    m_settings.setLastUsername(m_user.username);
    if (m_user.mustChangePassword) {
        // Yönetici şifreyi sıfırladıysa kullanıcı önce kendi şifresini belirler
        PasswordDialog dialog(m_db, m_user, PasswordDialog::Mode::ForcedChange, this);
        if (dialog.exec() != QDialog::Accepted) {
            m_user = User{};
            if (loginPage)
                loginPage->reset();
            return;
        }
        m_user.mustChangePassword = false;
    }
    showWorkspace();
}

void MainWindow::showWorkspace()
{
    m_signedIn = true;
    replaceScreen(buildWorkspace());
    m_autoLock->start(IDLE_MINUTES);
}

QWidget *MainWindow::buildWorkspace()
{
    auto *screen = new QWidget;
    auto *layout = new QHBoxLayout(screen);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *sidebar = new QWidget(screen);
    sidebar->setFixedWidth(240);
    auto *side = new QVBoxLayout(sidebar);
    side->setContentsMargins(10, 16, 10, 12);
    auto *name = new QLabel(m_user.fullName, sidebar);
    name->setTextFormat(Qt::PlainText);
    name->setStyleSheet("font-weight: 600; font-size: 11pt;");
    name->setWordWrap(true);
    auto *role = new QLabel(I18n::role(m_user.role), sidebar);
    role->setObjectName("muted");
    side->addWidget(name);
    side->addWidget(role);
    side->addSpacing(10);

    m_navigation = new QListWidget(sidebar);
    m_navigation->setObjectName("navigation");
    m_navigation->setFocusPolicy(Qt::NoFocus);
    m_navigation->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_navigation->setTextElideMode(Qt::ElideRight);
    m_pages = new QStackedWidget(screen);

    // Menü role göre: her sayfa sadece yetkisi olan role eklenir
    const Role r = m_user.role;
    auto add = [this](const QString &text, QWidget *page) {
        m_navigation->addItem(text);
        m_pages->addWidget(page);
    };
    add("📅  " + I18n::t(Access::allowed(r, Permission::ViewAllSchedules) ? "appointments" : "my_appointments"),
        new AppointmentsPage(m_db, m_user, m_pages));
    add("🗓  " + I18n::t("calendar"), new CalendarPage(m_db, m_user, m_pages));
    add("👤  " + I18n::t("patients"), new PatientsPage(m_db, m_user, m_pages));
    if (Access::allowed(r, Permission::ViewReports))
        add("📊  " + I18n::t("reports"), new ReportsPage(m_db, m_user, m_pages));
    if (Access::allowed(r, Permission::ManageDoctors))
        add("🩺  " + I18n::t("staff"), new StaffPage(m_db, m_user, m_pages));
    if (Access::allowed(r, Permission::ManageUsers))
        add("🔑  " + I18n::t("users"), new UsersPage(m_db, m_user, m_pages));
    side->addWidget(m_navigation, 1);

    auto *password = new QPushButton("✱  " + I18n::t("change_password"), sidebar);
    auto *theme = new QPushButton((Theme::isDark() ? "☀  " : "☾  ") + I18n::t("theme"), sidebar);
    auto *language = new QPushButton("🌐  " + I18n::t("language"), sidebar);
    auto *about = new QPushButton("ⓘ  " + I18n::t("about"), sidebar);
    auto *logout = new QPushButton("⎋  " + I18n::t("sign_out"), sidebar);
    for (QPushButton *button : {password, theme, language, about, logout}) {
        button->setFlat(true);
        button->setStyleSheet("text-align: left; padding: 8px 12px;");
        side->addWidget(button);
    }
    connect(password, &QPushButton::clicked, this, [this] {
        if (PasswordDialog(m_db, m_user, PasswordDialog::Mode::ChangeOwn, this).exec() == QDialog::Accepted)
            Ui::inform(this, I18n::t("password_changed"));
    });
    connect(theme, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    connect(language, &QPushButton::clicked, this, &MainWindow::toggleLanguage);
    connect(about, &QPushButton::clicked, this, [this] { AboutDialog(m_dataFolder, this).exec(); });
    connect(logout, &QPushButton::clicked, this, [this] { signOut(); });
    connect(m_navigation, &QListWidget::currentRowChanged, this, &MainWindow::showPage);

    layout->addWidget(sidebar);
    layout->addWidget(m_pages, 1);
    m_navigation->setCurrentRow(0);
    return screen;
}

void MainWindow::showPage(int index)
{
    if (index < 0 || !m_pages)
        return;
    m_pages->setCurrentIndex(index);
    // Her sayfa açıldığında veriyi yeniden okur: başka sayfada yapılan değişiklik hemen görünür
    QMetaObject::invokeMethod(m_pages->currentWidget(), "refresh");
}

void MainWindow::signOut(const QString &message)
{
    m_autoLock->stop();
    m_signedIn = false;
    m_user = User{};
    showAuth(message);
}

void MainWindow::rebuild()
{
    // Metinler kurulurken dile göre seçildiği için ekran baştan kurulur; açık sayfa korunur
    if (!m_signedIn) {
        showAuth();
        return;
    }
    const int page = m_navigation ? m_navigation->currentRow() : 0;
    replaceScreen(buildWorkspace());
    m_navigation->setCurrentRow(qMax(0, page));
}

void MainWindow::toggleTheme()
{
    const bool dark = !Theme::isDark();
    m_settings.setDarkTheme(dark);
    Theme::apply(*qApp, dark);
    rebuild(); // renkli hücreler ve grafik yeni renklerle çizilsin
}

void MainWindow::toggleLanguage()
{
    const QString language = I18n::language() == "tr" ? "en" : "tr";
    m_settings.setLanguage(language);
    I18n::setLanguage(language);
    setWindowTitle(I18n::t("app_name"));
    rebuild();
}
