#pragma once

#include <QWidget>

class PasswordField;
class QLabel;
class QLineEdit;
class QPushButton;

// Giriş ekranı. Demo verisi yüklüyse demo hesapları gösterilir.
class LoginPage : public QWidget
{
    Q_OBJECT

public:
    LoginPage(const QString &lastUsername, bool showDemo, QWidget *parent);
    void failed(const QString &message);
    void reset();

signals:
    void loginRequested(const QString &username, const QString &password);

private:
    void submit();

    QLineEdit *m_username;
    PasswordField *m_password;
    QLabel *m_error;
    QPushButton *m_login;
};
