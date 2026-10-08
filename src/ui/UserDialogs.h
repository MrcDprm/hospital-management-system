#pragma once

#include "core/Models.h"

#include <QDialog>

class Database;
class PasswordField;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;

// Şifre penceresi. Kendi şifresini değiştirirken mevcut şifre de sorulur;
// yönetici sıfırlarken sadece yeni şifre girilir (kullanıcı ilk girişte değiştirir).
class PasswordDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Mode { ChangeOwn, ForcedChange, Reset };
    PasswordDialog(Database &db, const User &user, Mode mode, QWidget *parent);
    QString password() const;

private:
    void validate();
    void save();

    Database &m_db;
    const User &m_user;
    Mode m_mode;
    PasswordField *m_current = nullptr;
    PasswordField *m_password, *m_repeat;
    QLabel *m_error;
    QPushButton *m_save;
};

// Kullanıcı formu (yönetici): kullanıcı adı, ad, rol, doktor bağlantısı, aktiflik; yeni kullanıcıda ilk şifre.
class UserDialog : public QDialog
{
    Q_OBJECT

public:
    UserDialog(Database &db, const User &actor, const User &user, QWidget *parent);

private:
    void roleChanged();
    void save();

    Database &m_db;
    const User &m_actor;
    User m_user;
    QLineEdit *m_username, *m_name;
    QComboBox *m_role, *m_doctor;
    QCheckBox *m_active;
    PasswordField *m_password = nullptr;
    QLabel *m_error;
};
