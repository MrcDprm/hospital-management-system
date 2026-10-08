#include "ui/SetupPage.h"

#include "core/Passwords.h"
#include "ui/PasswordField.h"
#include "ui/UiHelpers.h"

#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

SetupPage::SetupPage(QWidget *parent)
    : QWidget(parent)
{
    auto *icon = new QLabel(this);
    icon->setPixmap(QPixmap(":/icon.png").scaled(84, 84, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    icon->setAlignment(Qt::AlignCenter);
    auto *title = Ui::title(I18n::t("app_name"), this);
    title->setAlignment(Qt::AlignCenter);
    auto *subtitle = new QLabel(I18n::t("setup_text"), this);
    subtitle->setObjectName("muted");
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setWordWrap(true);

    auto *card = new QFrame(this);
    card->setObjectName("panel");
    card->setFixedWidth(480);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(22, 18, 22, 18);
    auto *heading = new QLabel(I18n::t("create_admin"), card);
    heading->setObjectName("heading");
    m_username = new QLineEdit(card);
    m_username->setMaxLength(30);
    m_username->setValidator(new QRegularExpressionValidator(QRegularExpression("[a-zA-Z0-9._-]{0,30}"), m_username));
    m_fullName = new QLineEdit(card);
    m_fullName->setMaxLength(100);
    m_password = new PasswordField(card);
    m_repeat = new PasswordField(card);
    m_rules = new QLabel(I18n::t("password_rules"), card);
    m_rules->setObjectName("muted");
    m_rules->setWordWrap(true);
    auto *form = new QFormLayout;
    form->addRow(I18n::t("username"), m_username);
    form->addRow(I18n::t("full_name"), m_fullName);
    form->addRow(I18n::t("password"), m_password);
    form->addRow(I18n::t("repeat_password"), m_repeat);
    form->addRow(QString(), m_rules);
    m_error = Ui::errorLabel(card);
    m_create = Ui::accentButton(I18n::t("create_admin"), card);
    cardLayout->addWidget(heading);
    cardLayout->addLayout(form);
    cardLayout->addWidget(m_error);
    cardLayout->addWidget(m_create);

    auto *demo = new QPushButton(I18n::t("start_with_demo"), this);
    auto *demoNote = new QLabel(I18n::t("demo_note"), this);
    demoNote->setObjectName("muted");
    demoNote->setAlignment(Qt::AlignCenter);
    demoNote->setWordWrap(true);
    demoNote->setFixedWidth(480);

    auto *column = new QVBoxLayout;
    column->addStretch();
    column->addWidget(icon);
    column->addWidget(title);
    column->addWidget(subtitle);
    column->addSpacing(14);
    column->addWidget(card);
    column->addSpacing(8);
    column->addWidget(demo);
    column->addWidget(demoNote);
    column->addStretch();
    auto *layout = new QHBoxLayout(this);
    layout->addStretch();
    layout->addLayout(column);
    layout->addStretch();

    for (QLineEdit *field : std::initializer_list<QLineEdit *>{m_username, m_fullName, m_password, m_repeat})
        connect(field, &QLineEdit::textChanged, this, &SetupPage::validate);
    connect(m_create, &QPushButton::clicked, this, [this] {
        emit createAdmin(m_username->text(), m_fullName->text(), m_password->text());
    });
    connect(demo, &QPushButton::clicked, this, &SetupPage::loadDemo);
    validate();
}

void SetupPage::validate()
{
    // Kurallar yazarken gösterilir; asıl kontrol kayıtta (UserRepository) yapılır
    const QStringList problems = Passwords::problems(m_password->text(), m_username->text().toLower());
    QString error;
    if (!m_password->text().isEmpty() && !problems.isEmpty())
        error = I18n::error(problems.first());
    else if (!m_repeat->text().isEmpty() && m_password->text() != m_repeat->text())
        error = I18n::t("passwords_differ");
    m_error->setText(error);
    m_create->setEnabled(m_username->text().size() >= 3 && m_fullName->text().trimmed().size() >= 3
                         && problems.isEmpty() && m_password->text() == m_repeat->text());
}

void SetupPage::showError(const QString &text)
{
    m_error->setText(text);
}
