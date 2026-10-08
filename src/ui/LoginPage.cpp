#include "ui/LoginPage.h"

#include "services/DemoData.h"
#include "ui/PasswordField.h"
#include "ui/UiHelpers.h"

#include <QApplication>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QVBoxLayout>

LoginPage::LoginPage(const QString &lastUsername, bool showDemo, QWidget *parent)
    : QWidget(parent)
{
    auto *icon = new QLabel(this);
    icon->setPixmap(QPixmap(":/icon.png").scaled(84, 84, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    icon->setAlignment(Qt::AlignCenter);
    auto *title = Ui::title(I18n::t("app_name"), this);
    title->setAlignment(Qt::AlignCenter);

    auto *card = new QFrame(this);
    card->setObjectName("panel");
    card->setFixedWidth(400);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(22, 18, 22, 18);
    auto *heading = new QLabel(I18n::t("sign_in"), card);
    heading->setObjectName("heading");
    m_username = new QLineEdit(lastUsername, card);
    m_username->setMaxLength(30);
    m_username->setPlaceholderText(I18n::t("username"));
    m_password = new PasswordField(card);
    m_password->setPlaceholderText(I18n::t("password"));
    m_error = Ui::errorLabel(card);
    m_login = Ui::accentButton(I18n::t("sign_in"), card);
    cardLayout->addWidget(heading);
    cardLayout->addWidget(m_username);
    cardLayout->addWidget(m_password);
    cardLayout->addWidget(m_error);
    cardLayout->addWidget(m_login);

    auto *column = new QVBoxLayout;
    column->addStretch();
    column->addWidget(icon);
    column->addWidget(title);
    column->addSpacing(14);
    column->addWidget(card);

    if (showDemo) {
        // Demo hesapları sadece örnek veri yüklendiyse görünür; gerçek kurulumda böyle hesap yoktur
        auto *demo = new QFrame(this);
        demo->setObjectName("panel");
        demo->setFixedWidth(400);
        auto *demoLayout = new QVBoxLayout(demo);
        auto *demoTitle = new QLabel(I18n::t("demo_accounts"), demo);
        demoTitle->setStyleSheet("font-weight: 600;");
        demoLayout->addWidget(demoTitle);
        for (const DemoData::Account &account : DemoData::accounts()) {
            auto *row = new QPushButton(QString("%1  ·  %2").arg(I18n::t(account.roleKey.toLatin1().constData()),
                                                                  account.username), demo);
            row->setFlat(true);
            row->setToolTip(I18n::t("demo_fill"));
            connect(row, &QPushButton::clicked, this, [this, account] {
                m_username->setText(account.username);
                m_password->setText(account.password);
                m_login->setFocus();
            });
            demoLayout->addWidget(row);
        }
        column->addSpacing(8);
        column->addWidget(demo);
    }
    column->addStretch();
    auto *layout = new QHBoxLayout(this);
    layout->addStretch();
    layout->addLayout(column);
    layout->addStretch();

    connect(m_login, &QPushButton::clicked, this, &LoginPage::submit);
    connect(m_password, &QLineEdit::returnPressed, this, &LoginPage::submit);
    connect(m_username, &QLineEdit::returnPressed, m_password, qOverload<>(&QWidget::setFocus));
}

void LoginPage::submit()
{
    if (m_username->text().trimmed().isEmpty() || m_password->text().isEmpty())
        return;
    m_login->setEnabled(false);
    QApplication::setOverrideCursor(Qt::WaitCursor); // şifre doğrulama (Argon2id) bilerek biraz sürer
    QApplication::processEvents();
    emit loginRequested(m_username->text(), m_password->text());
    QApplication::restoreOverrideCursor();
    m_login->setEnabled(true);
}

void LoginPage::failed(const QString &message)
{
    m_error->setText(message);
    m_password->clear();
    m_password->setFocus();
}

void LoginPage::reset()
{
    m_password->clear(); // çıkışta ve girişten sonra şifre kutuda kalmaz
    m_password->setRevealed(false);
    m_error->clear();
    (m_username->text().isEmpty() ? static_cast<QWidget *>(m_username) : m_password)->setFocus();
}
