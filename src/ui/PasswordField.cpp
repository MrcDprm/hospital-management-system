#include "ui/PasswordField.h"

#include "app/I18n.h"

#include <QAction>
#include <QPainter>

PasswordField::PasswordField(QWidget *parent)
    : QLineEdit(parent)
{
    setEchoMode(QLineEdit::Password);
    // Windows'un "metin tahmini" ve geri alma geçmişi şifreyi tutmasın
    setAttribute(Qt::WA_InputMethodEnabled, false);
    m_toggle = addAction(QIcon(), QLineEdit::TrailingPosition);
    m_toggle->setToolTip(I18n::t("show_password"));
    connect(m_toggle, &QAction::triggered, this, [this] { setRevealed(echoMode() == QLineEdit::Password); });
    setRevealed(false);
}

void PasswordField::setRevealed(bool revealed)
{
    setEchoMode(revealed ? QLineEdit::Normal : QLineEdit::Password);
    m_toggle->setToolTip(I18n::t(revealed ? "hide_password" : "show_password"));
    // Simge harf olarak çizilir: göz açık / kapalı
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(palette().color(QPalette::Text), 2.4));
    painter.drawEllipse(QRectF(4, 10, 24, 12));
    painter.setBrush(palette().color(QPalette::Text));
    painter.drawEllipse(QPointF(16, 16), 3.5, 3.5);
    if (!revealed)
        painter.drawLine(QPointF(5, 27), QPointF(27, 5));
    painter.end();
    m_toggle->setIcon(QIcon(pixmap));
}
