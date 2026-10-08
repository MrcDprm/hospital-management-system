#include "ui/StaffDialogs.h"

#include "data/StaffRepository.h"
#include "ui/UiHelpers.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTimeEdit>

namespace {

QDialogButtonBox *saveButtons(QDialog *dialog)
{
    auto *buttons = new QDialogButtonBox(dialog);
    buttons->addButton(Ui::accentButton(I18n::t("save"), dialog), QDialogButtonBox::AcceptRole);
    buttons->addButton(I18n::t("cancel"), QDialogButtonBox::RejectRole);
    QObject::connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    return buttons;
}

QTimeEdit *timeBox(const QTime &time, QWidget *parent)
{
    auto *box = new QTimeEdit(time, parent);
    box->setDisplayFormat("HH:mm");
    box->setTimeRange(QTime(6, 0), QTime(23, 0));
    return box;
}

} // namespace

DoctorDialog::DoctorDialog(Database &db, const User &actor, const Doctor &doctor, QWidget *parent)
    : QDialog(parent), m_db(db), m_actor(actor), m_doctor(doctor)
{
    setWindowTitle(I18n::t(doctor.id ? "edit_doctor" : "new_doctor"));
    m_title = new QComboBox(this);
    m_title->setEditable(true);
    m_title->addItems({"Dr.", "Uzm. Dr.", "Op. Dr.", "Doç. Dr.", "Prof. Dr."});
    m_title->setCurrentText(doctor.title.isEmpty() ? "Uzm. Dr." : doctor.title);
    m_title->lineEdit()->setMaxLength(20);
    m_name = new QLineEdit(doctor.fullName, this);
    m_name->setMaxLength(100);
    m_department = new QComboBox(this);
    for (const Department &d : StaffRepository(db).departments())
        m_department->addItem(d.name, d.id);
    m_department->setCurrentIndex(qMax(0, m_department->findData(doctor.departmentId)));
    m_phone = new QLineEdit(I18n::phone(doctor.phone), this);
    m_phone->setMaxLength(20);
    m_email = new QLineEdit(doctor.email, this);
    m_email->setMaxLength(100);

    // Çalışma günleri: bit 0 = pazartesi … bit 6 = pazar
    auto *daysRow = new QHBoxLayout;
    for (int day = 0; day < 7; ++day) {
        auto *box = new QCheckBox(I18n::weekday(day + 1), this);
        box->setChecked(doctor.workDays & (1 << day));
        m_days << box;
        daysRow->addWidget(box);
    }
    m_start = timeBox(doctor.startTime, this);
    m_end = timeBox(doctor.endTime, this);
    auto *hoursRow = new QHBoxLayout;
    hoursRow->addWidget(m_start);
    hoursRow->addWidget(new QLabel("–", this));
    hoursRow->addWidget(m_end);
    hoursRow->addStretch();
    m_slot = new QComboBox(this);
    for (int minutes : {10, 15, 20, 30, 45, 60})
        m_slot->addItem(I18n::t("minutes_count").replace("{0}", QString::number(minutes)), minutes);
    m_slot->setCurrentIndex(qMax(0, m_slot->findData(doctor.slotMinutes)));
    m_active = new QCheckBox(I18n::t("active_doctor"), this);
    m_active->setChecked(doctor.active);
    m_active->setToolTip(I18n::t("inactive_hint"));

    auto *form = new QFormLayout(this);
    form->addRow(I18n::t("title_label"), m_title);
    form->addRow(I18n::t("full_name"), m_name);
    form->addRow(I18n::t("department"), m_department);
    form->addRow(I18n::t("phone"), m_phone);
    form->addRow(I18n::t("email"), m_email);
    form->addRow(I18n::t("work_days"), daysRow);
    form->addRow(I18n::t("work_hours"), hoursRow);
    form->addRow(I18n::t("slot_length"), m_slot);
    form->addRow(QString(), m_active);
    m_error = Ui::errorLabel(this);
    form->addRow(m_error);
    auto *buttons = saveButtons(this);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &DoctorDialog::save);
    setMinimumWidth(540);
}

void DoctorDialog::save()
{
    Doctor d = m_doctor;
    d.title = m_title->currentText().trimmed();
    d.fullName = m_name->text();
    d.departmentId = m_department->currentData().toLongLong();
    d.phone = m_phone->text();
    d.email = m_email->text();
    d.workDays = 0;
    for (int day = 0; day < m_days.size(); ++day)
        if (m_days[day]->isChecked())
            d.workDays |= 1 << day;
    d.startTime = m_start->time();
    d.endTime = m_end->time();
    d.slotMinutes = m_slot->currentData().toInt();
    d.active = m_active->isChecked();
    StaffRepository repo(m_db);
    const Result result = d.id ? repo.updateDoctor(m_actor, d) : repo.addDoctor(m_actor, d);
    if (!result.ok()) {
        m_error->setText(I18n::error(result.error));
        return;
    }
    accept();
}

DepartmentDialog::DepartmentDialog(Database &db, const User &actor, const Department &department, QWidget *parent)
    : QDialog(parent), m_db(db), m_actor(actor), m_department(department)
{
    setWindowTitle(I18n::t(department.id ? "edit_department" : "new_department"));
    m_name = new QLineEdit(department.name, this);
    m_name->setMaxLength(100);
    m_location = new QLineEdit(department.location, this);
    m_location->setMaxLength(100);
    m_location->setPlaceholderText(I18n::t("location_hint"));
    auto *form = new QFormLayout(this);
    form->addRow(I18n::t("name"), m_name);
    form->addRow(I18n::t("location"), m_location);
    m_error = Ui::errorLabel(this);
    form->addRow(m_error);
    auto *buttons = saveButtons(this);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &DepartmentDialog::save);
    setMinimumWidth(400);
}

void DepartmentDialog::save()
{
    Department d = m_department;
    d.name = m_name->text();
    d.location = m_location->text();
    StaffRepository repo(m_db);
    const Result result = d.id ? repo.updateDepartment(m_actor, d) : repo.addDepartment(m_actor, d);
    if (!result.ok()) {
        m_error->setText(I18n::error(result.error));
        return;
    }
    accept();
}
