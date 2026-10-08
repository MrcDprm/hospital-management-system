#pragma once

#include <QDialog>

// Hakkında: uygulama adı, sürüm, kısa açıklama, veri klasörü ve GitHub linki.
class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    AboutDialog(const QString &dataFolder, QWidget *parent);
};
