#include "ui/StaffPage.h"

#include "data/StaffRepository.h"
#include "ui/StaffDialogs.h"
#include "ui/UiHelpers.h"

#include <QHBoxLayout>
#include <QSplitter>
#include <QVBoxLayout>

namespace {

QString workDays(int mask)
{
    QStringList days;
    for (int day = 0; day < 7; ++day)
        if (mask & (1 << day))
            days << I18n::weekday(day + 1);
    return days.join(" ");
}

} // namespace

StaffPage::StaffPage(Database &db, const User &user, QWidget *parent)
    : QWidget(parent), m_db(db), m_user(user)
{
    auto *splitter = new QSplitter(this);

    auto *left = new QWidget(splitter);
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 8, 0);
    auto *departments = new QLabel(I18n::t("departments"), left);
    departments->setObjectName("heading");
    m_departmentTable = Ui::makeTable({I18n::t("name"), I18n::t("location"), I18n::t("doctors")}, left);
    auto *addDepartment = new QPushButton("+ " + I18n::t("new_department"), left);
    m_editDepartment = new QPushButton(I18n::t("edit"), left);
    m_removeDepartment = new QPushButton(I18n::t("delete"), left);
    auto *leftButtons = new QHBoxLayout;
    for (QPushButton *button : {addDepartment, m_editDepartment, m_removeDepartment})
        leftButtons->addWidget(button);
    leftButtons->addStretch();
    leftLayout->addWidget(departments);
    leftLayout->addWidget(m_departmentTable, 1);
    leftLayout->addLayout(leftButtons);

    auto *right = new QWidget(splitter);
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(8, 0, 0, 0);
    auto *doctors = new QLabel(I18n::t("doctors"), right);
    doctors->setObjectName("heading");
    m_doctorTable = Ui::makeTable({I18n::t("doctor"), I18n::t("work_days"), I18n::t("work_hours"),
                                   I18n::t("slot_length"), I18n::t("phone"), I18n::t("status")},
                                  right);
    auto *addDoctor = Ui::accentButton("+ " + I18n::t("new_doctor"), right);
    m_editDoctor = new QPushButton(I18n::t("edit"), right);
    auto *rightButtons = new QHBoxLayout;
    rightButtons->addWidget(addDoctor);
    rightButtons->addWidget(m_editDoctor);
    rightButtons->addStretch();
    rightLayout->addWidget(doctors);
    rightLayout->addWidget(m_doctorTable, 1);
    rightLayout->addLayout(rightButtons);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(12);
    layout->addWidget(Ui::title(I18n::t("staff"), this));
    layout->addWidget(splitter, 1);

    connect(m_departmentTable, &QTableWidget::itemSelectionChanged, this, &StaffPage::fillDoctors);
    connect(m_departmentTable, &QTableWidget::cellDoubleClicked, this, [this] { editDepartment(false); });
    connect(m_doctorTable, &QTableWidget::itemSelectionChanged, this,
            [this] { m_editDoctor->setEnabled(Ui::selectedId(m_doctorTable) != 0); });
    connect(m_doctorTable, &QTableWidget::cellDoubleClicked, this, [this] { editDoctor(false); });
    connect(addDepartment, &QPushButton::clicked, this, [this] { editDepartment(true); });
    connect(m_editDepartment, &QPushButton::clicked, this, [this] { editDepartment(false); });
    connect(m_removeDepartment, &QPushButton::clicked, this, &StaffPage::removeDepartment);
    connect(addDoctor, &QPushButton::clicked, this, [this] { editDoctor(true); });
    connect(m_editDoctor, &QPushButton::clicked, this, [this] { editDoctor(false); });
}

void StaffPage::refresh()
{
    const qint64 keep = Ui::selectedId(m_departmentTable);
    StaffRepository repo(m_db);
    m_departments = repo.departments();
    m_doctorList = repo.doctors();
    m_departmentTable->blockSignals(true);
    m_departmentTable->setRowCount(0);
    for (const Department &d : m_departments) {
        const auto count = std::count_if(m_doctorList.cbegin(), m_doctorList.cend(),
                                         [&](const Doctor &doctor) { return doctor.departmentId == d.id; });
        const int row = Ui::addRow(m_departmentTable, {d.name, d.location, QString::number(count)}, d.id);
        if (d.id == keep)
            m_departmentTable->selectRow(row);
    }
    m_departmentTable->resizeColumnsToContents();
    if (m_departmentTable->selectedItems().isEmpty() && m_departmentTable->rowCount() > 0)
        m_departmentTable->selectRow(0);
    m_departmentTable->blockSignals(false);
    fillDoctors();
}

void StaffPage::fillDoctors()
{
    const qint64 department = Ui::selectedId(m_departmentTable);
    m_editDepartment->setEnabled(department != 0);
    m_removeDepartment->setEnabled(department != 0);
    m_doctorTable->setRowCount(0);
    for (const Doctor &d : m_doctorList) {
        if (d.departmentId != department)
            continue;
        Ui::addRow(m_doctorTable,
                   {d.displayName(), workDays(d.workDays),
                    d.startTime.toString("HH:mm") + "–" + d.endTime.toString("HH:mm"),
                    I18n::t("minutes_count").replace("{0}", QString::number(d.slotMinutes)), I18n::phone(d.phone),
                    I18n::t(d.active ? "active" : "inactive")},
                   d.id, d.active ? QColor() : Theme::muted());
    }
    m_doctorTable->resizeColumnsToContents();
    m_editDoctor->setEnabled(false);
}

void StaffPage::editDepartment(bool create)
{
    Department department;
    if (!create) {
        const qint64 id = Ui::selectedId(m_departmentTable);
        for (const Department &d : m_departments)
            if (d.id == id)
                department = d;
        if (!department.id)
            return;
    }
    if (DepartmentDialog(m_db, m_user, department, this).exec() == QDialog::Accepted)
        refresh();
}

void StaffPage::removeDepartment()
{
    const qint64 id = Ui::selectedId(m_departmentTable);
    QString name;
    for (const Department &d : m_departments)
        if (d.id == id)
            name = d.name;
    if (!id || !Ui::ask(this, I18n::t("confirm_delete_department").replace("{0}", name)))
        return;
    if (Ui::showResult(this, StaffRepository(m_db).removeDepartment(m_user, id)))
        refresh();
}

void StaffPage::editDoctor(bool create)
{
    if (m_departments.isEmpty()) {
        Ui::inform(this, I18n::t("add_department_first"));
        return;
    }
    Doctor doctor;
    doctor.departmentId = Ui::selectedId(m_departmentTable);
    if (!create) {
        const qint64 id = Ui::selectedId(m_doctorTable);
        for (const Doctor &d : m_doctorList)
            if (d.id == id)
                doctor = d;
        if (!doctor.id)
            return;
    }
    if (DoctorDialog(m_db, m_user, doctor, this).exec() == QDialog::Accepted)
        refresh();
}
