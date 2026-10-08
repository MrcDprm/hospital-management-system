#include "ui/UsersPage.h"

#include "data/StaffRepository.h"
#include "data/UserRepository.h"
#include "ui/UiHelpers.h"
#include "ui/UserDialogs.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QHash>
#include <QVBoxLayout>

UsersPage::UsersPage(Database &db, const User &user, QWidget *parent)
    : QWidget(parent), m_db(db), m_user(user)
{
    auto *add = Ui::accentButton("+ " + I18n::t("new_user"), this);
    auto *top = new QHBoxLayout;
    top->addWidget(Ui::title(I18n::t("users"), this));
    top->addStretch();
    top->addWidget(add);

    m_table = Ui::makeTable({I18n::t("username"), I18n::t("full_name"), I18n::t("role"), I18n::t("doctor_record"),
                             I18n::t("status")},
                            this);
    m_edit = new QPushButton(I18n::t("edit"), this);
    m_reset = new QPushButton(I18n::t("reset_password"), this);
    auto *note = new QLabel(I18n::t("users_note"), this);
    note->setObjectName("muted");
    note->setWordWrap(true);
    auto *actions = new QHBoxLayout;
    actions->addWidget(m_edit);
    actions->addWidget(m_reset);
    actions->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(12);
    layout->addLayout(top);
    layout->addWidget(m_table, 1);
    layout->addLayout(actions);
    layout->addWidget(note);

    connect(add, &QPushButton::clicked, this, [this] { edit(User{}); });
    connect(m_edit, &QPushButton::clicked, this, [this] { edit(selected()); });
    connect(m_reset, &QPushButton::clicked, this, &UsersPage::resetPassword);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this] { edit(selected()); });
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this] {
        const bool any = selected().id != 0;
        m_edit->setEnabled(any);
        m_reset->setEnabled(any && selected().id != m_user.id); // kendi şifresini "şifre değiştir"den değiştirir
    });
}

void UsersPage::refresh()
{
    QHash<qint64, QString> doctors;
    for (const Doctor &d : StaffRepository(m_db).doctors())
        doctors.insert(d.id, d.displayName());
    m_users = UserRepository(m_db).all(m_user);
    m_table->setRowCount(0);
    for (const User &u : m_users) {
        QString status = I18n::t(u.active ? "active" : "inactive");
        if (u.mustChangePassword)
            status += " · " + I18n::t("password_pending");
        Ui::addRow(m_table, {u.username, u.fullName, I18n::role(u.role), doctors.value(u.doctorId, "—"), status}, u.id,
                   u.active ? QColor() : Theme::muted());
    }
    m_table->resizeColumnsToContents();
    m_edit->setEnabled(false);
    m_reset->setEnabled(false);
}

User UsersPage::selected() const
{
    const qint64 id = Ui::selectedId(m_table);
    for (const User &u : m_users)
        if (u.id == id)
            return u;
    return {};
}

void UsersPage::edit(const User &user)
{
    if (UserDialog(m_db, m_user, user, this).exec() == QDialog::Accepted)
        refresh();
}

void UsersPage::resetPassword()
{
    const User user = selected();
    if (!user.id)
        return;
    PasswordDialog dialog(m_db, user, PasswordDialog::Mode::Reset, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const Result result = UserRepository(m_db).resetPassword(m_user, user.id, dialog.password());
    QApplication::restoreOverrideCursor();
    if (Ui::showResult(this, result)) {
        Ui::inform(this, I18n::t("password_reset_done").replace("{0}", user.fullName));
        refresh();
    }
}
