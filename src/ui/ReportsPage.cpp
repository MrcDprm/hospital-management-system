#include "ui/ReportsPage.h"

#include "data/ReportRepository.h"
#include "services/CsvExport.h"
#include "ui/UiHelpers.h"

#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLocale>
#include <QPainter>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace {

QString percent(int part, int whole)
{
    return whole > 0 ? QString("%1%").arg(qRound(100.0 * part / whole)) : "—";
}

// Dönem seçenekleri: son 7 gün, son 30 gün, son 90 gün, bu yıl
std::pair<QDate, QDate> range(int index)
{
    const QDate today = QDate::currentDate();
    switch (index) {
    case 0: return {today.addDays(-6), today.addDays(1)};
    case 1: return {today.addDays(-29), today.addDays(1)};
    case 2: return {today.addDays(-89), today.addDays(1)};
    default: return {QDate(today.year(), 1, 1), QDate(today.year() + 1, 1, 1)};
    }
}

} // namespace

MonthlyChart::MonthlyChart(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(220);
}

void MonthlyChart::setValues(const QList<int> &values)
{
    m_values = values;
    update();
}

void MonthlyChart::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QLocale locale(I18n::language() == "en" ? QLocale::English : QLocale::Turkish);
    const int labelHeight = fontMetrics().height() + 6;
    const QRectF area = QRectF(rect()).adjusted(8, labelHeight, -8, -labelHeight);
    const int maximum = m_values.isEmpty() ? 0 : *std::max_element(m_values.cbegin(), m_values.cend());
    const double slot = area.width() / 12.0;
    const double barWidth = slot * 0.6;
    const QDate today = QDate::currentDate();

    painter.setPen(Theme::muted());
    painter.drawLine(area.bottomLeft(), area.bottomRight());
    for (int month = 0; month < 12; ++month) {
        const double x = area.left() + slot * month;
        const int value = month < m_values.size() ? m_values[month] : 0;
        const double height = maximum > 0 ? area.height() * value / maximum : 0;
        const QRectF bar(x + (slot - barWidth) / 2, area.bottom() - height, barWidth, height);
        painter.setPen(Qt::NoPen);
        // İçinde bulunulan ay vurgulu, diğerleri soluk
        QColor color = Theme::accent();
        if (month + 1 != today.month())
            color.setAlpha(150);
        painter.setBrush(color);
        painter.drawRoundedRect(bar, 3, 3);
        painter.setPen(palette().color(QPalette::WindowText));
        painter.drawText(QRectF(x, area.bottom() + 3, slot, labelHeight), Qt::AlignHCenter | Qt::AlignTop,
                         locale.monthName(month + 1, QLocale::ShortFormat));
        if (value > 0)
            painter.drawText(QRectF(x, bar.top() - labelHeight, slot, labelHeight), Qt::AlignCenter,
                             locale.toString(value));
    }
}

ReportsPage::ReportsPage(Database &db, const User &, QWidget *parent)
    : QWidget(parent), m_db(db)
{
    m_period = new QComboBox(this);
    m_period->addItems({I18n::t("last_7_days"), I18n::t("last_30_days"), I18n::t("last_90_days"), I18n::t("this_year")});
    m_period->setCurrentIndex(1);
    auto *csv = new QPushButton(I18n::t("export_csv"), this);
    auto *top = new QHBoxLayout;
    top->addWidget(Ui::title(I18n::t("reports"), this));
    top->addStretch();
    top->addWidget(new QLabel(I18n::t("period"), this));
    top->addWidget(m_period);
    top->addWidget(csv);

    auto *splitter = new QSplitter(this);
    auto *left = new QWidget(splitter);
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 8, 0);
    auto *departments = new QLabel(I18n::t("by_department"), left);
    departments->setObjectName("heading");
    m_departments = Ui::makeTable({I18n::t("department"), I18n::t("appointments"), I18n::t("summary_examined"),
                                   I18n::t("no_show_rate")},
                                  left);
    leftLayout->addWidget(departments);
    leftLayout->addWidget(m_departments);
    auto *right = new QWidget(splitter);
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(8, 0, 0, 0);
    auto *doctors = new QLabel(I18n::t("doctor_occupancy"), right);
    doctors->setObjectName("heading");
    m_doctors = Ui::makeTable({I18n::t("doctor"), I18n::t("department"), I18n::t("appointments"), I18n::t("capacity"),
                               I18n::t("occupancy"), I18n::t("no_show_rate")},
                              right);
    m_doctors->setSortingEnabled(true);
    rightLayout->addWidget(doctors);
    rightLayout->addWidget(m_doctors);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    splitter->setSizes({500, 900});

    auto *chartTitle = new QHBoxLayout;
    auto *monthly = new QLabel(I18n::t("monthly_appointments"), this);
    monthly->setObjectName("heading");
    m_year = new QSpinBox(this);
    m_year->setRange(2000, QDate::currentDate().year() + 1);
    m_year->setValue(QDate::currentDate().year());
    chartTitle->addWidget(monthly);
    chartTitle->addStretch();
    chartTitle->addWidget(m_year);
    m_chart = new MonthlyChart(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(12);
    layout->addLayout(top);
    layout->addWidget(splitter, 1);
    layout->addLayout(chartTitle);
    layout->addWidget(m_chart);

    connect(m_period, &QComboBox::currentIndexChanged, this, &ReportsPage::refresh);
    connect(m_year, &QSpinBox::valueChanged, this, [this](int year) { m_chart->setValues(ReportRepository(m_db).monthly(year)); });
    connect(csv, &QPushButton::clicked, this, &ReportsPage::exportCsv);
}

void ReportsPage::refresh()
{
    const auto [from, to] = range(m_period->currentIndex());
    ReportRepository repo(m_db);
    m_departments->setRowCount(0);
    for (const DepartmentStat &s : repo.departments(from, to))
        Ui::addRow(m_departments, {s.name, QString::number(s.appointments), QString::number(s.examined),
                                   percent(s.noShow, s.appointments)},
                   0);
    m_departments->resizeColumnsToContents();

    m_doctors->setSortingEnabled(false);
    m_doctors->setRowCount(0);
    for (const DoctorStat &s : repo.doctors(from, to)) {
        const int row = Ui::addRow(m_doctors, {s.name, s.department, QString::number(s.booked), QString::number(s.capacity),
                                               percent(s.booked, s.capacity), percent(s.noShow, s.booked)},
                                   0);
        // Doluluk %85 üstü dikkat rengi (o doktora yeni randevu bulmak zorlaşır)
        if (s.capacity > 0 && s.booked * 100 >= s.capacity * 85)
            m_doctors->item(row, 4)->setForeground(Theme::warning());
    }
    m_doctors->setSortingEnabled(true);
    m_doctors->resizeColumnsToContents();
    m_chart->setValues(repo.monthly(m_year->value()));
}

void ReportsPage::exportCsv()
{
    const QString path = QFileDialog::getSaveFileName(
        this, I18n::t("export_csv"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + I18n::t("report_file") + ".csv",
        "CSV (*.csv)");
    if (path.isEmpty())
        return;
    QList<QStringList> rows;
    for (int row = 0; row < m_doctors->rowCount(); ++row) {
        QStringList cells;
        for (int column = 0; column < m_doctors->columnCount(); ++column)
            cells << m_doctors->item(row, column)->text();
        rows << cells;
    }
    QStringList header;
    for (int column = 0; column < m_doctors->columnCount(); ++column)
        header << m_doctors->horizontalHeaderItem(column)->text();
    if (CsvExport::write(path, header, rows, CsvExport::separatorFor(I18n::language())))
        Ui::inform(this, I18n::t("csv_saved"));
    else
        Ui::warn(this, I18n::t("csv_failed"));
}
