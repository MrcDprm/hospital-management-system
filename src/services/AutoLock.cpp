#include "services/AutoLock.h"

#include <QEvent>

AutoLock::AutoLock(QObject *parent)
    : QObject(parent)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &AutoLock::timedOut);
}

void AutoLock::start(int minutes)
{
    m_timer.start(minutes * 60 * 1000);
}

void AutoLock::stop()
{
    m_timer.stop();
}

bool AutoLock::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::KeyPress:
    case QEvent::Wheel:
        if (m_timer.isActive())
            m_timer.start(); // aynı süreyle baştan başlar
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event); // olay engellenmez, sadece izlenir
}
