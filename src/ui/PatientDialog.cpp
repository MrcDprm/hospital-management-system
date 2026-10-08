#include "ui/PatientDialog.h"

#include "data/PatientRepository.h"
#include "ui/UiHelpers.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QRegularExpressionValidator>

PatientDialog::PatientDialog(Database &db, const User &actor, const Patient &patient, QWidget *parent)
    : QDialog(parent), m_db(db), m_actor(actor), m_patient(patient)
{
    setWindowTitle(I18n::t(patient.id ? "edit_patient" : "new_patient"));
    m_nationalId = new QLineEdit(patient.nationalId, this);
    // Kutuya sadece rakam yazılabilir; asıl kontrol (sağlama basamakları) kayıtta yapılır
    m_nationalId->setValidator(new QRegularExpressionValidator(QRegularExpression("\\d{0,11}"), m_nationalId));
    m_name = new QLineEdit(patient.fullName, this);
    m_name->setMaxLength(100);
    m_birth = new QDateEdit(patient.birthDate.isValid() ? patient.birthDate : QDate::currentDate().addYears(-30), this);
    m_birth->setCalendarPopup(true);
    m_birth->setDisplayFormat("dd.MM.yyyy");
    m_birth->setMaximumDate(QDate::currentDate());
    m_birth->setMinimumDate(QDate::currentDate().addYears(-120));
    m_gender = new QComboBox(this);
    for (int i = 0; i <= static_cast<int>(Gender::Other); ++i)
        m_gender->addItem(I18n::gender(static_cast<Gender>(i)));
    m_gender->setCurrentIndex(static_cast<int>(patient.gender));
    m_phone = new QLineEdit(I18n::phone(patient.phone), this);
    m_phone->setMaxLength(20);
    m_phone->setPlaceholderText("0532 123 45 67");
    m_email = new QLineEdit(patient.email, this);
    m_email->setMaxLength(100);
    m_blood = new QComboBox(this);
    for (int i = 0; i <= static_cast<int>(BloodGroup::ZeroNeg); ++i)
        m_blood->addItem(I18n::bloodGroup(static_cast<BloodGroup>(i)));
    m_blood->setCurrentIndex(static_cast<int>(patient.bloodGroup));
    m_allergies = new QLineEdit(patient.allergies, this);
    m_allergies->setMaxLength(500);
    m_allergies->setPlaceholderText(I18n::t("allergies_hint"));
    m_chronic = new QLineEdit(patient.chronicConditions, this);
    m_chronic->setMaxLength(500);

    auto *form = new QFormLayout(this);
    form->addRow(I18n::t("national_id"), m_nationalId);
    form->addRow(I18n::t("full_name"), m_name);
    form->addRow(I18n::t("birth_date"), m_birth);
    form->addRow(I18n::t("gender"), m_gender);
    form->addRow(I18n::t("phone"), m_phone);
    form->addRow(I18n::t("email"), m_email);
    form->addRow(I18n::t("blood_group"), m_blood);
    form->addRow(I18n::t("allergies"), m_allergies);
    form->addRow(I18n::t("chronic"), m_chronic);
    m_error = Ui::errorLabel(this);
    form->addRow(m_error);
    auto *buttons = new QDialogButtonBox(this);
    buttons->addButton(Ui::accentButton(I18n::t("save"), this), QDialogButtonBox::AcceptRole);
    buttons->addButton(I18n::t("cancel"), QDialogButtonBox::RejectRole);
    form->addRow(buttons);
    setMinimumWidth(460);
    connect(buttons, &QDialogButtonBox::accepted, this, &PatientDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void PatientDialog::save()
{
    Patient p = m_patient;
    p.nationalId = m_nationalId->text();
    p.fullName = m_name->text();
    p.birthDate = m_birth->date();
    p.gender = static_cast<Gender>(m_gender->currentIndex());
    p.phone = m_phone->text();
    p.email = m_email->text();
    p.bloodGroup = static_cast<BloodGroup>(m_blood->currentIndex());
    p.allergies = m_allergies->text();
    p.chronicConditions = m_chronic->text();
    PatientRepository repo(m_db);
    const Result result = p.id ? repo.update(m_actor, p) : repo.add(m_actor, p);
    if (!result.ok()) {
        m_error->setText(I18n::error(result.error));
        return;
    }
    m_patient.id = result.id;
    accept();
}
