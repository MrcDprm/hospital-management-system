#include "ui/BookingDialog.h"

#include "core/Schedule.h"
#include "data/AppointmentRepository.h"
#include "data/PatientRepository.h"
#include "data/StaffRepository.h"
#include "ui/PatientDialog.h"
#include "ui/UiHelpers.h"

#include <QComboBox>
#include <QCompleter>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

BookingDialog::BookingDialog(Database &db, const User &actor, QWidget *parent, qint64 patientId, qint64 doctorId,
                             const QDateTime &start)
    : QDialog(parent), m_db(db), m_actor(actor), m_preselect(start)
{
    setWindowTitle(I18n::t("new_appointment"));

    // Hasta: yazdıkça süzülen liste (ad ya da T.C. kimlik numarasının bir kısmı)
    m_patient = new QComboBox(this);
    m_patient->setEditable(true);
    m_patient->setInsertPolicy(QComboBox::NoInsert);
    m_patient->completer()->setFilterMode(Qt::MatchContains);
    m_patient->completer()->setCompletionMode(QCompleter::PopupCompletion);
    auto *newPatient = new QPushButton("+ " + I18n::t("new_patient"), this);
    auto *patientRow = new QHBoxLayout;
    patientRow->addWidget(m_patient, 1);
    patientRow->addWidget(newPatient);

    m_department = new QComboBox(this);
    m_department->addItem(I18n::t("all_departments"), 0);
    for (const Department &d : StaffRepository(db).departments())
        m_department->addItem(d.name, d.id);
    m_doctor = new QComboBox(this);
    m_date = new QDateEdit(start.isValid() ? start.date() : QDate::currentDate(), this);
    m_date->setCalendarPopup(true);
    m_date->setDisplayFormat("dd.MM.yyyy  dddd");
    m_date->setMinimumDate(QDate::currentDate());
    m_date->setMaximumDate(QDate::currentDate().addDays(Schedule::MAX_DAYS_AHEAD));

    m_slots = new QListWidget(this);
    m_slots->setObjectName("slots"); // boş saatler çerçeveli düğme, dolu ve geçmiş saatler soluk
    m_slots->setViewMode(QListView::IconMode);
    m_slots->setResizeMode(QListView::Adjust);
    m_slots->setMovement(QListView::Static);
    m_slots->setGridSize(QSize(76, 36));
    m_slots->setUniformItemSizes(true);
    m_slots->setMinimumHeight(190);
    m_summary = new QLabel(this);
    m_summary->setObjectName("muted");
    m_note = new QLineEdit(this);
    m_note->setMaxLength(500);

    auto *form = new QFormLayout;
    form->addRow(I18n::t("patient"), patientRow);
    form->addRow(I18n::t("department"), m_department);
    form->addRow(I18n::t("doctor"), m_doctor);
    form->addRow(I18n::t("date"), m_date);
    form->addRow(I18n::t("time"), m_slots);
    form->addRow(QString(), m_summary);
    form->addRow(I18n::t("note"), m_note);
    m_error = Ui::errorLabel(this);
    auto *buttons = new QDialogButtonBox(this);
    buttons->addButton(Ui::accentButton(I18n::t("book"), this), QDialogButtonBox::AcceptRole);
    buttons->addButton(I18n::t("cancel"), QDialogButtonBox::RejectRole);
    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_error);
    layout->addWidget(buttons);
    setMinimumWidth(620);

    connect(newPatient, &QPushButton::clicked, this, &BookingDialog::addPatient);
    connect(m_department, &QComboBox::currentIndexChanged, this, &BookingDialog::loadDoctors);
    connect(m_doctor, &QComboBox::currentIndexChanged, this, &BookingDialog::loadSlots);
    connect(m_date, &QDateEdit::dateChanged, this, &BookingDialog::loadSlots);
    connect(m_slots, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *item) {
        m_summary->setText(item ? I18n::t("selected_slot").replace("{0}", item->text()) : QString());
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &BookingDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    loadPatients(patientId);
    // Takvimden gelindiyse doktorun bölümü ve doktor seçili gelir
    if (doctorId)
        if (const auto doctor = StaffRepository(db).doctor(doctorId))
            m_department->setCurrentIndex(m_department->findData(doctor->departmentId));
    loadDoctors();
    if (doctorId)
        m_doctor->setCurrentIndex(m_doctor->findData(doctorId));
}

void BookingDialog::loadPatients(qint64 select)
{
    m_patient->clear();
    for (const Patient &p : PatientRepository(m_db).all())
        m_patient->addItem(QString("%1  ·  %2  ·  %3").arg(p.fullName, Ui::maskId(p.nationalId),
                                                           QString::number(p.birthDate.year())),
                           p.id);
    m_patient->setCurrentIndex(select ? m_patient->findData(select) : -1);
    if (!select)
        m_patient->setEditText(QString());
}

void BookingDialog::loadDoctors()
{
    const qint64 previous = m_doctor->currentData().toLongLong();
    m_doctor->blockSignals(true);
    m_doctor->clear();
    for (const Doctor &d : StaffRepository(m_db).doctors(m_department->currentData().toLongLong(), true))
        m_doctor->addItem(d.displayName(), d.id);
    m_doctor->setCurrentIndex(qMax(0, m_doctor->findData(previous)));
    m_doctor->blockSignals(false);
    loadSlots();
}

void BookingDialog::loadSlots()
{
    m_slots->clear();
    m_summary->clear();
    const auto doctor = StaffRepository(m_db).doctor(m_doctor->currentData().toLongLong());
    if (!doctor) {
        m_summary->setText(I18n::t("choose_doctor"));
        return;
    }
    const QDate date = m_date->date();
    const QList<Schedule::Slot> times =
        Schedule::daySlots(*doctor, date, AppointmentRepository(m_db).forDoctorOn(doctor->id, date),
                           QDateTime::currentDateTime());
    int free = 0;
    for (const Schedule::Slot &slot : times) {
        auto *item = new QListWidgetItem(slot.start.toString("HH:mm"), m_slots);
        item->setData(Qt::UserRole, slot.start);
        item->setTextAlignment(Qt::AlignCenter);
        if (slot.state != Schedule::SlotState::Free) {
            // Seçilemeyen saatler soluk ve nedenini ipucunda söyler
            item->setFlags(Qt::NoItemFlags);
            item->setToolTip(I18n::slotState(slot.state));
        } else {
            ++free;
        }
        if (m_preselect.isValid() && slot.start == m_preselect && slot.state == Schedule::SlotState::Free)
            m_slots->setCurrentItem(item);
    }
    if (times.isEmpty())
        m_summary->setText(I18n::t("doctor_not_working"));
    else if (free == 0)
        m_summary->setText(I18n::t("no_free_slots"));
}

void BookingDialog::addPatient()
{
    PatientDialog dialog(m_db, m_actor, Patient{}, this);
    if (dialog.exec() == QDialog::Accepted)
        loadPatients(dialog.savedId());
}

void BookingDialog::save()
{
    // Yazılan metin listedeki bir hastayla birebir eşleşmeli (yarım yazılmış ad geçerli değil)
    const int index = m_patient->findText(m_patient->currentText());
    if (index < 0) {
        m_error->setText(I18n::t("choose_patient"));
        return;
    }
    QListWidgetItem *slot = m_slots->currentItem();
    if (!slot) {
        m_error->setText(I18n::t("choose_slot"));
        return;
    }
    Appointment a;
    a.patientId = m_patient->itemData(index).toLongLong();
    a.doctorId = m_doctor->currentData().toLongLong();
    a.start = slot->data(Qt::UserRole).toDateTime();
    a.note = m_note->text();
    const Result result = AppointmentRepository(m_db).book(m_actor, a);
    if (!result.ok()) {
        m_error->setText(I18n::error(result.error));
        loadSlots(); // bu arada başkası almış olabilir: liste yenilenir
        return;
    }
    accept();
}
