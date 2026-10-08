#pragma once

#include "core/Models.h"

#include <QDialog>

class Database;

// Hastanın geçmişi: bütün randevuları ve (sadece doktor için) muayene ve reçeteleri.
class PatientHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    PatientHistoryDialog(Database &db, const User &actor, const Patient &patient, QWidget *parent);
};
