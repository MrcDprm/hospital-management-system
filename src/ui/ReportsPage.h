#pragma once

#include "core/Models.h"

#include <QWidget>

class Database;
class QComboBox;
class QSpinBox;
class QTableWidget;

// 12 aylık randevu sayısı çubuk grafiği; QPainter ile çizilir.
class MonthlyChart : public QWidget
{
    Q_OBJECT

public:
    explicit MonthlyChart(QWidget *parent);
    void setValues(const QList<int> &values);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<int> m_values;
};

// Raporlar (yönetici): dönem seçimi, bölüm bazında randevu ve gelmeme oranı, doktor doluluk oranı, aylık grafik.
// Sadece sayılar; tanı ya da muayene içeriği yok.
class ReportsPage : public QWidget
{
    Q_OBJECT

public:
    ReportsPage(Database &db, const User &user, QWidget *parent);

public slots:
    void refresh();

private:
    void exportCsv();

    Database &m_db;
    QComboBox *m_period;
    QSpinBox *m_year;
    QTableWidget *m_departments, *m_doctors;
    MonthlyChart *m_chart;
};
