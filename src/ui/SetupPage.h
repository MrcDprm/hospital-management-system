#pragma once

#include <QWidget>

class PasswordField;
class QLabel;
class QLineEdit;
class QPushButton;

// İlk açılış (hiç kullanıcı yokken): yönetici hesabı oluşturma ya da örnek veriyle başlama.
class SetupPage : public QWidget
{
    Q_OBJECT

public:
    explicit SetupPage(QWidget *parent);
    void showError(const QString &text);

signals:
    void createAdmin(const QString &username, const QString &fullName, const QString &password);
    void loadDemo();

private:
    void validate();

    QLineEdit *m_username, *m_fullName;
    PasswordField *m_password, *m_repeat;
    QLabel *m_rules, *m_error;
    QPushButton *m_create;
};
