#pragma once

#include <QLineEdit>

// Şifre kutusu: sağında göster/gizle düğmesi. İçerik varsayılan olarak noktalarla gizlidir.
class PasswordField : public QLineEdit
{
    Q_OBJECT

public:
    explicit PasswordField(QWidget *parent = nullptr);
    void setRevealed(bool revealed);

private:
    QAction *m_toggle;
};
