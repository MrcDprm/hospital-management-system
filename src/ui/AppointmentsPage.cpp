#include "ui/AppointmentsPage.h"

#include "core/Access.h"
#include "data/AppointmentRepository.h"
#include "data/PatientRepository.h"
#include "data/ReportRepository.h"
#include "data/StaffRepository.h"
#include "ui/AppointmentActions.h"
#include "ui/BookingDialog.h"
#include "ui/UiHelpers.h"

#include <QComboBox>
#include <QDateEdit>
#include <QFrame>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QVBoxLayout>

using AppointmentActions::Action;

AppointmentsPage::AppointmentsPage(Database &db, const User &user, QWidget *parent)
    : QWidget(parent), m_db(db), m_user(user)
{
    const bool allSchedules = Access::allowed(user.role, Permission::ViewAllSchedules);
    auto *previous = new QPushButton("◀", this);
    auto *next = new QPushButton("▶", this);
    auto *today = new QPushButton(I18n::t("today"), this);
    for (QPushButton *button : {previous, next})
        button->setFixedWidth(40);
    m_date = new QDateEdit(QDate::currentDate(), this);
    m_date->setCalendarPopup(true);
    m_date->setDisplayFormat("dd.MM.yyyy  dddd");
    m_date->setMinimumWidth(200);
    m_doctor = new QComboBox(this);
    m_doctor->setMinimumWidth(240);
    m_doctor->setVisible(allSchedules); // doktor sadece kendi listesini görür; seçim gerekmez
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(I18n::t("search_patient"));
    m_search->setClearButtonEnabled(true);

    auto *top = new QHBoxLayout;
    top->addWidget(Ui::title(I18n::t(allSchedules ? "appointments" : "my_appointments"), this));
    top->addStretch();
    if (Access::allowed(user.role, Permission::ManageAppointments)) {
        auto *bookButton = Ui::accentButton("+ " + I18n::t("new_appointment"), this);
        connect(bookButton, &QPushButton::clicked, this, &AppointmentsPage::book);
        top->addWidget(bookButton);
    }

    auto *filters = new QHBoxLayout;
    filters->addWidget(previous);
    filters->addWidget(m_date);
    filters->addWidget(next);
    filters->addWidget(today);
    filters->addSpacing(12);
    filters->addWidget(m_doctor);
    filters->addWidget(m_search, 1);

    auto *cards = new QHBoxLayout;
    cards->addWidget(summaryCard("summary_total", m_total));
    cards->addWidget(summaryCard("summary_waiting", m_waiting));
    cards->addWidget(summaryCard("summary_checked_in", m_checkedIn));
    cards->addWidget(summaryCard("summary_examined", m_examined));
    cards->addWidget(summaryCard("summary_no_show", m_noShow));

    m_table = Ui::makeTable({I18n::t("time"), I18n::t("patient"), I18n::t("national_id"), I18n::t("doctor"),
                             I18n::t("department"), I18n::t("status"), I18n::t("note")},
                            this);
    // Doktor kendi listesinde doktor ve bölüm sütunlarına ihtiyaç duymaz
    m_table->setColumnHidden(3, !allSchedules);
    m_table->setColumnHidden(4, !allSchedules);

    // İşlem düğmeleri; sadece seçili randevuda yapılabilecek olanlar etkin
    auto *actions = new QHBoxLayout;
    const char *keys[] = {"check_in", "no_show", "cancel_appointment", "examine", "patient_history"};
    for (int i = 0; i < 5; ++i) {
        const auto action = static_cast<Action>(i);
        auto *button = action == Action::Examine ? Ui::accentButton(I18n::t(keys[i]), this)
                                                 : new QPushButton(I18n::t(keys[i]), this);
        // Rolüne hiç uymayan düğme gösterilmez (sekreterde "Muayene et" yok)
        const bool relevant = action == Action::History
                              || (action == Action::Examine ? Access::allowed(user.role, Permission::WriteExaminations)
                                  : action == Action::NoShow
                                      ? true
                                      : Access::allowed(user.role, Permission::ManageAppointments));
        button->setVisible(relevant);
        connect(button, &QPushButton::clicked, this, [this, i] { act(i); });
        m_actionButtons << button;
        actions->addWidget(button);
    }
    actions->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(12);
    layout->addLayout(top);
    layout->addLayout(filters);
    layout->addLayout(cards);
    layout->addWidget(m_table, 1);
    layout->addLayout(actions);

    connect(previous, &QPushButton::clicked, this, [this] { m_date->setDate(m_date->date().addDays(-1)); });
    connect(next, &QPushButton::clicked, this, [this] { m_date->setDate(m_date->date().addDays(1)); });
    connect(today, &QPushButton::clicked, this, [this] { m_date->setDate(QDate::currentDate()); });
    connect(m_date, &QDateEdit::dateChanged, this, &AppointmentsPage::refresh);
    connect(m_doctor, &QComboBox::currentIndexChanged, this, &AppointmentsPage::refresh);
    connect(m_search, &QLineEdit::textChanged, this, &AppointmentsPage::fill);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &AppointmentsPage::updateButtons);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this] {
        // Çift tık: doktor için muayene, diğerleri için hasta geçmişi
        const Appointment a = selected();
        act(static_cast<int>(AppointmentActions::can(Action::Examine, m_user, a) ? Action::Examine : Action::History));
    });
}

QWidget *AppointmentsPage::summaryCard(const char *key, QLabel *&value)
{
    auto *card = new QFrame(this);
    card->setObjectName("card");
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 10, 14, 10);
    auto *label = new QLabel(I18n::t(key), card);
    label->setObjectName("cardLabel");
    value = new QLabel("0", card);
    value->setObjectName("cardValue");
    layout->addWidget(label);
    layout->addWidget(value);
    return card;
}

void AppointmentsPage::refresh()
{
    // Sözlükler (hasta, doktor, bölüm adları) her açılışta yeniden okunur: başka sayfadaki değişiklik görünür
    m_patients.clear();
    for (const Patient &p : PatientRepository(m_db).all())
        m_patients.insert(p.id, p);
    m_departments.clear();
    for (const Department &d : StaffRepository(m_db).departments())
        m_departments.insert(d.id, d.name);
    m_doctors.clear();
    const qint64 chosen = m_doctor->currentData().toLongLong();
    m_doctor->blockSignals(true);
    m_doctor->clear();
    m_doctor->addItem(I18n::t("all_doctors"), 0);
    for (const Doctor &d : StaffRepository(m_db).doctors()) {
        m_doctors.insert(d.id, d);
        if (d.active)
            m_doctor->addItem(d.displayName(), d.id);
    }
    m_doctor->setCurrentIndex(qMax(0, m_doctor->findData(chosen)));
    m_doctor->blockSignals(false);

    const QDate date = m_date->date();
    const qint64 doctorId = m_user.role == Role::Doctor ? m_user.doctorId : m_doctor->currentData().toLongLong();
    m_appointments = AppointmentRepository(m_db).list(m_user, date, date.addDays(1), doctorId);

    const DaySummary s = ReportRepository(m_db).day(date, doctorId);
    m_total->setText(QString::number(s.total));
    m_waiting->setText(QString::number(s.waiting));
    m_checkedIn->setText(QString::number(s.checkedIn));
    m_examined->setText(QString::number(s.examined));
    m_noShow->setText(QString::number(s.noShow));
    fill();
}

void AppointmentsPage::fill()
{
    const qint64 keep = Ui::selectedId(m_table);
    const QString search = m_search->text().trimmed();
    m_table->setRowCount(0);
    for (const Appointment &a : m_appointments) {
        const Patient patient = m_patients.value(a.patientId);
        if (!search.isEmpty() && !patient.fullName.contains(search, Qt::CaseInsensitive)
            && !patient.nationalId.startsWith(search))
            continue;
        const Doctor doctor = m_doctors.value(a.doctorId);
        const int row = Ui::addRow(m_table,
                                   {a.start.toString("HH:mm"), patient.fullName, Ui::maskId(patient.nationalId),
                                    doctor.displayName(), m_departments.value(doctor.departmentId),
                                    I18n::status(a.status), a.note},
                                   a.id);
        m_table->item(row, 5)->setForeground(Ui::statusColor(a.status));
        if (a.status == AppointmentStatus::Cancelled)
            for (int column = 0; column < m_table->columnCount(); ++column)
                m_table->item(row, column)->setForeground(Theme::muted());
        if (a.id == keep)
            m_table->selectRow(row);
    }
    m_table->resizeColumnsToContents();
    updateButtons();
}

Appointment AppointmentsPage::selected() const
{
    const qint64 id = Ui::selectedId(m_table);
    for (const Appointment &a : m_appointments)
        if (a.id == id)
            return a;
    return {};
}

void AppointmentsPage::updateButtons()
{
    const Appointment a = selected();
    for (int i = 0; i < m_actionButtons.size(); ++i)
        m_actionButtons[i]->setEnabled(AppointmentActions::can(static_cast<Action>(i), m_user, a));
}

void AppointmentsPage::act(int action)
{
    const Appointment a = selected();
    if (!AppointmentActions::can(static_cast<Action>(action), m_user, a))
        return;
    if (AppointmentActions::run(static_cast<Action>(action), this, m_db, m_user, a))
        refresh();
}

void AppointmentsPage::book()
{
    BookingDialog dialog(m_db, m_user, this, 0, m_doctor->currentData().toLongLong());
    if (dialog.exec() == QDialog::Accepted)
        refresh();
}
