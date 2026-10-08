#include "ui/CalendarPage.h"

#include "core/Access.h"
#include "core/Schedule.h"
#include "data/AppointmentRepository.h"
#include "data/PatientRepository.h"
#include "data/StaffRepository.h"
#include "ui/AppointmentActions.h"
#include "ui/BookingDialog.h"
#include "ui/UiHelpers.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QMenu>
#include <QVBoxLayout>

namespace {

constexpr int StartRole = Qt::UserRole + 1; // hücrenin saat diliminin başlangıcı
constexpr int StateRole = Qt::UserRole + 2;
constexpr int NotWorking = -1;              // doktorun çalışmadığı gün (SlotState dışında)

QColor tint(const QColor &color, int alpha)
{
    QColor c = color;
    c.setAlpha(alpha);
    return c;
}

} // namespace

CalendarPage::CalendarPage(Database &db, const User &user, QWidget *parent)
    : QWidget(parent), m_db(db), m_user(user)
{
    const QDate today = QDate::currentDate();
    m_monday = today.addDays(1 - today.dayOfWeek());

    m_doctor = new QComboBox(this);
    m_doctor->setMinimumWidth(280);
    m_doctor->setVisible(Access::allowed(user.role, Permission::ViewAllSchedules));
    auto *previous = new QPushButton("◀", this);
    auto *next = new QPushButton("▶", this);
    auto *thisWeek = new QPushButton(I18n::t("this_week"), this);
    for (QPushButton *button : {previous, next})
        button->setFixedWidth(40);
    m_week = new QLabel(this);
    m_week->setObjectName("heading");
    m_week->setMinimumWidth(260);
    m_week->setAlignment(Qt::AlignCenter);
    m_hint = new QLabel(this);
    m_hint->setObjectName("muted");

    auto *top = new QHBoxLayout;
    top->addWidget(Ui::title(I18n::t("calendar"), this));
    top->addStretch();
    top->addWidget(m_doctor);
    auto *navigation = new QHBoxLayout;
    navigation->addWidget(previous);
    navigation->addWidget(m_week);
    navigation->addWidget(next);
    navigation->addWidget(thisWeek);
    navigation->addStretch();
    navigation->addWidget(m_hint);

    m_grid = new QTableWidget(0, 7, this);
    m_grid->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_grid->setSelectionMode(QAbstractItemView::SingleSelection);
    m_grid->setContextMenuPolicy(Qt::CustomContextMenu);
    m_grid->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_grid->verticalHeader()->setDefaultSectionSize(30);
    m_grid->setWordWrap(false);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(12);
    layout->addLayout(top);
    layout->addLayout(navigation);
    layout->addWidget(m_grid, 1);

    connect(previous, &QPushButton::clicked, this, [this] { m_monday = m_monday.addDays(-7); refresh(); });
    connect(next, &QPushButton::clicked, this, [this] { m_monday = m_monday.addDays(7); refresh(); });
    connect(thisWeek, &QPushButton::clicked, this, [this] {
        m_monday = QDate::currentDate().addDays(1 - QDate::currentDate().dayOfWeek());
        refresh();
    });
    connect(m_doctor, &QComboBox::currentIndexChanged, this, &CalendarPage::refresh);
    connect(m_grid, &QTableWidget::cellDoubleClicked, this, &CalendarPage::openCell);
    connect(m_grid, &QTableWidget::customContextMenuRequested, this, &CalendarPage::showMenu);
}

void CalendarPage::refresh()
{
    StaffRepository staff(m_db);
    // Doktor listesi her açılışta yenilenir (yeni eklenen doktor görünsün); doktor rolünde sadece kendisi
    const qint64 chosen = m_doctor->currentData().toLongLong();
    m_doctor->blockSignals(true);
    m_doctor->clear();
    for (const Doctor &d : staff.doctors(0, true))
        if (m_user.role != Role::Doctor || d.id == m_user.doctorId)
            m_doctor->addItem(d.displayName(), d.id);
    m_doctor->setCurrentIndex(qMax(0, m_doctor->findData(chosen)));
    m_doctor->blockSignals(false);
    const QDate sunday = m_monday.addDays(6);
    m_week->setText(I18n::date(m_monday) + " – " + I18n::date(sunday));
    m_grid->clear();
    m_grid->setRowCount(0);
    m_appointments.clear();

    const auto doctor = staff.doctor(m_doctor->currentData().toLongLong());
    if (!doctor) {
        m_hint->setText(I18n::t("choose_doctor"));
        return;
    }
    QHash<qint64, QString> patients;
    for (const Patient &p : PatientRepository(m_db).all())
        patients.insert(p.id, p.fullName);
    const QList<Appointment> week = AppointmentRepository(m_db).list(m_user, m_monday, m_monday.addDays(7), doctor->id);

    // Satırlar: doktorun mesai başından sonuna saat dilimleri (öğle arası dahil, ayrı renkte)
    QList<QTime> times;
    for (QTime t = doctor->startTime; t.isValid() && t < doctor->endTime; t = t.addSecs(doctor->slotMinutes * 60)) {
        times << t;
        if (t.addSecs(doctor->slotMinutes * 60) < t)
            break; // gece yarısını geçerse dur
    }
    m_grid->setRowCount(times.size());
    QStringList rowLabels;
    for (const QTime &t : times)
        rowLabels << t.toString("HH:mm");
    m_grid->setVerticalHeaderLabels(rowLabels);
    QStringList headers;
    for (int day = 0; day < 7; ++day)
        headers << I18n::weekday(day + 1) + "\n" + m_monday.addDays(day).toString("dd.MM");
    m_grid->setHorizontalHeaderLabels(headers);

    const QDateTime now = QDateTime::currentDateTime();
    const bool canBook = Access::allowed(m_user.role, Permission::ManageAppointments);
    int free = 0;
    for (int day = 0; day < 7; ++day) {
        const QDate date = m_monday.addDays(day);
        QList<Appointment> ofDay;
        for (const Appointment &a : week)
            if (a.start.date() == date) {
                ofDay << a;
                m_appointments.insert(a.id, a);
            }
        const QList<Schedule::Slot> slotList = Schedule::daySlots(*doctor, date, ofDay, now);
        if (slotList.isEmpty()) {
            for (int row = 0; row < times.size(); ++row) {
                auto *item = new QTableWidgetItem;
                item->setData(StateRole, NotWorking);
                item->setBackground(tint(Theme::muted(), 30));
                item->setFlags(Qt::NoItemFlags);
                m_grid->setItem(row, day, item);
            }
            continue;
        }
        for (const Schedule::Slot &slot : slotList) {
            const int row = times.indexOf(slot.start.time());
            if (row < 0)
                continue;
            auto *item = new QTableWidgetItem;
            item->setData(StartRole, slot.start);
            item->setData(StateRole, static_cast<int>(slot.state));
            item->setData(Qt::UserRole, slot.appointmentId);
            switch (slot.state) {
            case Schedule::SlotState::Booked: {
                const Appointment a = m_appointments.value(slot.appointmentId);
                item->setText(patients.value(a.patientId));
                item->setToolTip(QString("%1\n%2 · %3").arg(patients.value(a.patientId), slot.start.toString("HH:mm"),
                                                            I18n::status(a.status)));
                item->setBackground(tint(Ui::statusColor(a.status), 70));
                break;
            }
            case Schedule::SlotState::Lunch:
                if (slot.start.time() == Schedule::LUNCH_START)
                    item->setText(I18n::slotState(slot.state)); // yazı sadece ilk satırda
                item->setForeground(Theme::muted());
                item->setBackground(tint(Theme::muted(), 30));
                item->setFlags(Qt::NoItemFlags);
                break;
            case Schedule::SlotState::Past:
                item->setBackground(tint(Theme::muted(), 18));
                item->setFlags(Qt::NoItemFlags);
                break;
            case Schedule::SlotState::Free:
                ++free;
                item->setToolTip(canBook ? I18n::t("double_click_to_book") : I18n::slotState(slot.state));
                break;
            }
            m_grid->setItem(row, day, item);
        }
    }
    m_hint->setText(I18n::t("free_slots_week").replace("{0}", QString::number(free)));
}

void CalendarPage::openCell(int row, int column)
{
    QTableWidgetItem *item = m_grid->item(row, column);
    if (!item)
        return;
    const auto state = static_cast<Schedule::SlotState>(item->data(StateRole).toInt());
    if (state == Schedule::SlotState::Free && Access::allowed(m_user.role, Permission::ManageAppointments)) {
        BookingDialog dialog(m_db, m_user, this, 0, m_doctor->currentData().toLongLong(),
                             item->data(StartRole).toDateTime());
        if (dialog.exec() == QDialog::Accepted)
            refresh();
    } else if (state == Schedule::SlotState::Booked) {
        // Dolu saatte: doktor için muayene, diğerleri için hasta geçmişi
        const Appointment a = m_appointments.value(item->data(Qt::UserRole).toLongLong());
        using AppointmentActions::Action;
        const Action action = AppointmentActions::can(Action::Examine, m_user, a) ? Action::Examine : Action::History;
        if (AppointmentActions::run(action, this, m_db, m_user, a))
            refresh();
    }
}

void CalendarPage::showMenu(const QPoint &position)
{
    QTableWidgetItem *item = m_grid->itemAt(position);
    if (!item || static_cast<Schedule::SlotState>(item->data(StateRole).toInt()) != Schedule::SlotState::Booked)
        return;
    const Appointment a = m_appointments.value(item->data(Qt::UserRole).toLongLong());
    QMenu menu(this);
    AppointmentActions::fillMenu(&menu, this, m_db, m_user, a, [this] {
        QMetaObject::invokeMethod(this, &CalendarPage::refresh, Qt::QueuedConnection); // menü kapandıktan sonra
    });
    if (!menu.isEmpty())
        menu.exec(m_grid->viewport()->mapToGlobal(position));
}
