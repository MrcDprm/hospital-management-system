#include "ui/UserDialogs.h"

#include "core/Passwords.h"
#include "data/StaffRepository.h"
#include "data/UserRepository.h"
#include "ui/PasswordField.h"
#include "ui/UiHelpers.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QRegularExpressionValidator>

PasswordDialog::PasswordDialog(Database &db, const User &user, Mode mode, QWidget *parent)
    : QDialog(parent), m_db(db), m_user(user), m_mode(mode)
{
    setWindowTitle(I18n::t(mode == Mode::Reset ? "reset_password" : "change_password"));
    auto *form = new QFormLayout(this);
    if (mode == Mode::ForcedChange) {
        auto *note = new QLabel(I18n::t("must_change_password"), this);
        note->setWordWrap(true);
        form->addRow(note);
    }
    if (mode == Mode::Reset) {
        auto *note = new QLabel(I18n::t("reset_note").replace("{0}", user.fullName), this);
        note->setTextFormat(Qt::PlainText);
        note->setWordWrap(true);
        form->addRow(note);
    } else {
        m_current = new PasswordField(this);
        form->addRow(I18n::t("current_password"), m_current);
    }
    m_password = new PasswordField(this);
    m_repeat = new PasswordField(this);
    form->addRow(I18n::t("new_password"), m_password);
    form->addRow(I18n::t("repeat_password"), m_repeat);
    auto *rules = new QLabel(I18n::t("password_rules"), this);
    rules->setObjectName("muted");
    rules->setWordWrap(true);
    form->addRow(rules);
    m_error = Ui::errorLabel(this);
    form->addRow(m_error);
    auto *buttons = new QDialogButtonBox(this);
    m_save = Ui::accentButton(I18n::t("save"), this);
    buttons->addButton(m_save, QDialogButtonBox::AcceptRole);
    // Zorunlu değişiklikte vazgeçmek oturumu kapatır (pencereyi açan taraf karar verir)
    buttons->addButton(I18n::t(mode == Mode::ForcedChange ? "sign_out" : "cancel"), QDialogButtonBox::RejectRole);
    form->addRow(buttons);
    for (QLineEdit *field : {static_cast<QLineEdit *>(m_password), static_cast<QLineEdit *>(m_repeat)})
        connect(field, &QLineEdit::textChanged, this, &PasswordDialog::validate);
    connect(buttons, &QDialogButtonBox::accepted, this, &PasswordDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    setMinimumWidth(440);
    validate();
}

QString PasswordDialog::password() const
{
    return m_password->text();
}

void PasswordDialog::validate()
{
    const QStringList problems = Passwords::problems(m_password->text(), m_user.username);
    QString error;
    if (!m_password->text().isEmpty() && !problems.isEmpty())
        error = I18n::error(problems.first());
    else if (!m_repeat->text().isEmpty() && m_password->text() != m_repeat->text())
        error = I18n::t("passwords_differ");
    m_error->setText(error);
    m_save->setEnabled(problems.isEmpty() && m_password->text() == m_repeat->text());
}

void PasswordDialog::save()
{
    if (m_mode == Mode::Reset) {
        accept(); // sıfırlamayı sayfa yapar (yetki kontrolü veri katmanında)
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const Result result = UserRepository(m_db).changeOwnPassword(m_user.id, m_current->text(), m_password->text());
    QApplication::restoreOverrideCursor();
    if (!result.ok()) {
        m_error->setText(I18n::error(result.error));
        return;
    }
    accept();
}

UserDialog::UserDialog(Database &db, const User &actor, const User &user, QWidget *parent)
    : QDialog(parent), m_db(db), m_actor(actor), m_user(user)
{
    setWindowTitle(I18n::t(user.id ? "edit_user" : "new_user"));
    m_username = new QLineEdit(user.username, this);
    m_username->setMaxLength(30);
    m_username->setValidator(new QRegularExpressionValidator(QRegularExpression("[a-zA-Z0-9._-]{0,30}"), m_username));
    m_username->setReadOnly(user.id != 0); // kullanıcı adı sonradan değişmez
    m_name = new QLineEdit(user.fullName, this);
    m_name->setMaxLength(100);
    m_role = new QComboBox(this);
    for (Role role : {Role::Admin, Role::Receptionist, Role::Doctor})
        m_role->addItem(I18n::role(role), static_cast<int>(role));
    m_role->setCurrentIndex(m_role->findData(static_cast<int>(user.role)));
    m_doctor = new QComboBox(this);
    m_doctor->addItem(I18n::t("choose"), 0);
    for (const Doctor &d : StaffRepository(db).doctors())
        m_doctor->addItem(d.displayName(), d.id);
    m_doctor->setCurrentIndex(qMax(0, m_doctor->findData(user.doctorId)));
    m_active = new QCheckBox(I18n::t("account_active"), this);
    m_active->setChecked(user.active);

    auto *form = new QFormLayout(this);
    form->addRow(I18n::t("username"), m_username);
    form->addRow(I18n::t("full_name"), m_name);
    form->addRow(I18n::t("role"), m_role);
    form->addRow(I18n::t("doctor_record"), m_doctor);
    if (!user.id) {
        m_password = new PasswordField(this);
        form->addRow(I18n::t("first_password"), m_password);
        auto *rules = new QLabel(I18n::t("first_password_note"), this);
        rules->setObjectName("muted");
        rules->setWordWrap(true);
        form->addRow(QString(), rules);
    }
    form->addRow(QString(), m_active);
    m_error = Ui::errorLabel(this);
    form->addRow(m_error);
    auto *buttons = new QDialogButtonBox(this);
    buttons->addButton(Ui::accentButton(I18n::t("save"), this), QDialogButtonBox::AcceptRole);
    buttons->addButton(I18n::t("cancel"), QDialogButtonBox::RejectRole);
    form->addRow(buttons);
    connect(m_role, &QComboBox::currentIndexChanged, this, &UserDialog::roleChanged);
    connect(buttons, &QDialogButtonBox::accepted, this, &UserDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    setMinimumWidth(460);
    roleChanged();
}

void UserDialog::roleChanged()
{
    // Doktor kaydı sadece doktor rolünde anlamlı (doktor kendi takvimini ve hastalarını görür)
    m_doctor->setEnabled(static_cast<Role>(m_role->currentData().toInt()) == Role::Doctor);
}

void UserDialog::save()
{
    User u = m_user;
    u.username = m_username->text();
    u.fullName = m_name->text();
    u.role = static_cast<Role>(m_role->currentData().toInt());
    u.doctorId = u.role == Role::Doctor ? m_doctor->currentData().toLongLong() : 0;
    u.active = m_active->isChecked();
    UserRepository repo(m_db);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const Result result = u.id ? repo.update(m_actor, u) : repo.add(m_actor, u, m_password->text());
    QApplication::restoreOverrideCursor();
    if (!result.ok()) {
        m_error->setText(I18n::error(result.error));
        return;
    }
    accept();
}
