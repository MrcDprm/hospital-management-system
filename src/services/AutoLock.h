#pragma once

#include <QObject>
#include <QTimer>

// Hareketsizlikte otomatik kilit: uygulamaya gelen her fare ve klavye olayı sayacı sıfırlar.
// Uygulamanın tamamını dinlemek için QApplication'a olay süzgeci (event filter) olarak takılır.
class AutoLock : public QObject
{
    Q_OBJECT

public:
    explicit AutoLock(QObject *parent = nullptr);

    void start(int minutes);
    void stop();

signals:
    void timedOut();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QTimer m_timer;
};
