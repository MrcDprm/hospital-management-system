#pragma once

#include "core/Models.h"

#include <QWidget>

class Database;
class QPushButton;
class QTableWidget;

// Kullanıcı hesapları (sadece yönetici): ekleme, düzenleme, şifre sıfırlama.
class UsersPage : public QWidget
{
    Q_OBJECT

public:
    UsersPage(Database &db, const User &user, QWidget *parent);

public slots:
    void refresh();

private:
    User selected() const;
    void edit(const User &user);
    void resetPassword();

    Database &m_db;
    const User &m_user;
    QList<User> m_users;
    QTableWidget *m_table;
    QPushButton *m_edit, *m_reset;
};
