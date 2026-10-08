#include "ui/ExaminationDialog.h"

#include "data/ExaminationRepository.h"
#include "data/PatientRepository.h"
#include "data/StaffRepository.h"
#include "services/Catalog.h"
#include "services/PrescriptionPrinter.h"
#include "ui/PatientHistoryDialog.h"
#include "ui/UiHelpers.h"

#include <QComboBox>
#include <QCompleter>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>

namespace {

constexpr int MAX_MEDICINES = 20;
enum Column { MedicineColumn, DoseColumn, UsageColumn, DaysColumn };

QPlainTextEdit *textBox(const QString &text, int height, QWidget *parent)
{
    auto *box = new QPlainTextEdit(text, parent);
    box->setFixedHeight(height);
    box->setTabChangesFocus(true); // Tab bir sonraki kutuya geçer, metne sekme eklemez
    return box;
}

} // namespace

ExaminationDialog::ExaminationDialog(Database &db, const User &actor, const Appointment &appointment, QWidget *parent)
    : QDialog(parent), m_db(db), m_actor(actor), m_appointment(appointment)
{
    m_patient = PatientRepository(db).find(appointment.patientId).value_or(Patient{});
    m_exam = ExaminationRepository(db).forAppointment(actor, appointment.id).value_or(Examination{});
    setWindowTitle(I18n::t(m_exam.id ? "edit_examination" : "examination") + " · " + m_patient.fullName);

    m_complaint = textBox(m_exam.complaint, 64, this);
    m_findings = textBox(m_exam.findings, 64, this);
    m_notes = textBox(m_exam.notes, 52, this);

    // Tanı: koddan ya da addan arama; listeden seçilince kod ve ad birlikte dolar. Listede olmayan da yazılabilir.
    m_code = new QLineEdit(m_exam.diagnosisCode, this);
    m_code->setMaxLength(10);
    m_code->setFixedWidth(90);
    m_code->setPlaceholderText("J06.9");
    m_diagnosis = new QLineEdit(m_exam.diagnosisName, this);
    m_diagnosis->setMaxLength(200);
    m_diagnosis->setPlaceholderText(I18n::t("diagnosis_hint"));
    QStringList options;
    for (const Catalog::Diagnosis &d : Catalog::diagnoses())
        options << d.code + "  " + (I18n::language() == "en" ? d.nameEn : d.nameTr);
    auto *completer = new QCompleter(options, m_diagnosis);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_diagnosis->setCompleter(completer);
    connect(completer, qOverload<const QString &>(&QCompleter::activated), this, [this](const QString &text) {
        const int split = text.indexOf("  ");
        m_code->setText(text.left(split));
        // Tamamlayıcı kendi metnini kutuya yazdıktan sonra adı ayırıyoruz
        QMetaObject::invokeMethod(m_diagnosis, [this, name = text.mid(split + 2)] { m_diagnosis->setText(name); },
                                  Qt::QueuedConnection);
    });
    auto *diagnosisRow = new QHBoxLayout;
    diagnosisRow->addWidget(m_code);
    diagnosisRow->addWidget(m_diagnosis, 1);

    // Reçete tablosu: her hücrede düzenlenebilir kutu
    m_medicines = new QTableWidget(0, 4, this);
    m_medicines->setHorizontalHeaderLabels({I18n::t("medicine"), I18n::t("dose"), I18n::t("usage"), I18n::t("days")});
    m_medicines->verticalHeader()->hide();
    m_medicines->verticalHeader()->setDefaultSectionSize(34);
    m_medicines->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_medicines->setSelectionMode(QAbstractItemView::SingleSelection);
    m_medicines->horizontalHeader()->setSectionResizeMode(MedicineColumn, QHeaderView::Stretch);
    m_medicines->horizontalHeader()->setSectionResizeMode(UsageColumn, QHeaderView::Stretch);
    m_medicines->setColumnWidth(DoseColumn, 110);
    m_medicines->setColumnWidth(DaysColumn, 80);
    m_medicines->setMinimumHeight(170);
    for (const PrescriptionItem &item : m_exam.prescription)
        addMedicineRow(item);
    if (m_exam.prescription.isEmpty())
        addMedicineRow(); // boş bir satırla başlar; doldurulmazsa kayıtta atlanır
    auto *addMedicine = new QPushButton("+ " + I18n::t("add_medicine"), this);
    auto *removeMedicine = new QPushButton(I18n::t("remove_medicine"), this);
    auto *medicineButtons = new QHBoxLayout;
    medicineButtons->addWidget(addMedicine);
    medicineButtons->addWidget(removeMedicine);
    medicineButtons->addStretch();

    auto *form = new QFormLayout;
    form->addRow(I18n::t("complaint") + " *", m_complaint);
    form->addRow(I18n::t("findings"), m_findings);
    form->addRow(I18n::t("diagnosis") + " *", diagnosisRow);
    form->addRow(I18n::t("doctor_notes"), m_notes);

    auto *heading = new QLabel(I18n::t("prescription"), this);
    heading->setObjectName("heading");
    m_error = Ui::errorLabel(this);
    auto *buttons = new QDialogButtonBox(this);
    auto *print = Ui::accentButton(I18n::t("save_and_print"), this);
    buttons->addButton(print, QDialogButtonBox::ActionRole);
    buttons->addButton(I18n::t("save"), QDialogButtonBox::AcceptRole);
    buttons->addButton(I18n::t("cancel"), QDialogButtonBox::RejectRole);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(patientCard());
    layout->addLayout(form);
    layout->addWidget(heading);
    layout->addWidget(m_medicines, 1);
    layout->addLayout(medicineButtons);
    layout->addWidget(m_error);
    layout->addWidget(buttons);
    resize(820, 780);

    connect(addMedicine, &QPushButton::clicked, this, [this] {
        if (m_medicines->rowCount() < MAX_MEDICINES)
            addMedicineRow();
    });
    connect(removeMedicine, &QPushButton::clicked, this, [this] {
        const int row = m_medicines->currentRow();
        if (row >= 0)
            m_medicines->removeRow(row);
    });
    connect(print, &QPushButton::clicked, this, &ExaminationDialog::saveAndPrint);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        if (save())
            accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QWidget *ExaminationDialog::patientCard()
{
    // Muayene öncesi doktorun görmesi gerekenler: yaş, kan grubu, alerji ve kronik hastalıklar
    auto *card = new QFrame(this);
    card->setObjectName("panel");
    auto *grid = new QGridLayout(card);
    grid->setContentsMargins(14, 10, 14, 10);
    auto *name = new QLabel(m_patient.fullName, card);
    name->setObjectName("heading");
    name->setTextFormat(Qt::PlainText);
    auto *details = new QLabel(QString("%1 · %2 · %3 %4")
                                   .arg(I18n::age(m_patient.birthDate), I18n::gender(m_patient.gender),
                                        I18n::t("blood_group"), I18n::bloodGroup(m_patient.bloodGroup)),
                               card);
    details->setObjectName("muted");
    auto *allergies = new QLabel(I18n::t("allergies") + ": "
                                     + (m_patient.allergies.isEmpty() ? I18n::t("none") : m_patient.allergies),
                                 card);
    allergies->setTextFormat(Qt::PlainText);
    allergies->setWordWrap(true);
    if (!m_patient.allergies.isEmpty())
        allergies->setStyleSheet("color: " + Theme::danger().name() + "; font-weight: 600;");
    auto *chronic = new QLabel(I18n::t("chronic") + ": "
                                   + (m_patient.chronicConditions.isEmpty() ? I18n::t("none") : m_patient.chronicConditions),
                               card);
    chronic->setTextFormat(Qt::PlainText);
    chronic->setWordWrap(true);
    auto *history = new QPushButton(I18n::t("patient_history"), card);
    connect(history, &QPushButton::clicked, this, [this] {
        PatientHistoryDialog(m_db, m_actor, m_patient, this).exec();
    });
    grid->addWidget(name, 0, 0);
    grid->addWidget(history, 0, 1, Qt::AlignRight);
    grid->addWidget(details, 1, 0, 1, 2);
    grid->addWidget(allergies, 2, 0, 1, 2);
    grid->addWidget(chronic, 3, 0, 1, 2);
    return card;
}

void ExaminationDialog::addMedicineRow(const PrescriptionItem &item)
{
    const int row = m_medicines->rowCount();
    m_medicines->insertRow(row);
    auto *medicine = new QLineEdit(item.medicine, m_medicines);
    medicine->setMaxLength(120);
    auto *completer = new QCompleter(Catalog::medicines(), medicine);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    medicine->setCompleter(completer);
    auto *dose = new QLineEdit(item.dose, m_medicines);
    dose->setMaxLength(60);
    dose->setPlaceholderText("500 mg");
    auto *usage = new QComboBox(m_medicines);
    usage->setEditable(true);
    usage->addItems(Catalog::usages());
    usage->setCurrentIndex(-1); // boş satır boş kalsın (yoksa ilk kalıp seçili gelir)
    usage->setEditText(item.usage);
    usage->lineEdit()->setMaxLength(120);
    auto *days = new QSpinBox(m_medicines);
    days->setRange(1, 365);
    days->setValue(item.days > 0 ? item.days : 7);
    m_medicines->setCellWidget(row, MedicineColumn, medicine);
    m_medicines->setCellWidget(row, DoseColumn, dose);
    m_medicines->setCellWidget(row, UsageColumn, usage);
    m_medicines->setCellWidget(row, DaysColumn, days);
    if (item.medicine.isEmpty())
        medicine->setFocus();
}

Examination ExaminationDialog::collect() const
{
    Examination e = m_exam;
    e.appointmentId = m_appointment.id;
    e.patientId = m_appointment.patientId;
    e.doctorId = m_appointment.doctorId;
    e.complaint = m_complaint->toPlainText();
    e.findings = m_findings->toPlainText();
    e.diagnosisCode = m_code->text();
    e.diagnosisName = m_diagnosis->text();
    e.notes = m_notes->toPlainText();
    e.prescription.clear();
    for (int row = 0; row < m_medicines->rowCount(); ++row) {
        PrescriptionItem item;
        item.medicine = static_cast<QLineEdit *>(m_medicines->cellWidget(row, MedicineColumn))->text();
        item.dose = static_cast<QLineEdit *>(m_medicines->cellWidget(row, DoseColumn))->text();
        item.usage = static_cast<QComboBox *>(m_medicines->cellWidget(row, UsageColumn))->currentText();
        item.days = static_cast<QSpinBox *>(m_medicines->cellWidget(row, DaysColumn))->value();
        e.prescription << item;
    }
    return e;
}

bool ExaminationDialog::save()
{
    const Result result = ExaminationRepository(m_db).save(m_actor, collect());
    if (!result.ok()) {
        m_error->setText(I18n::error(result.error));
        return false;
    }
    m_exam = ExaminationRepository(m_db).forAppointment(m_actor, m_appointment.id).value_or(m_exam);
    return true;
}

void ExaminationDialog::saveAndPrint()
{
    if (!save())
        return;
    Prescription::exportPdf(this, m_db, m_exam);
    accept();
}

namespace Prescription {

void exportPdf(QWidget *parent, Database &db, const Examination &exam)
{
    const auto patient = PatientRepository(db).find(exam.patientId);
    const auto doctor = StaffRepository(db).doctor(exam.doctorId);
    if (!patient || !doctor)
        return;
    Department department;
    for (const Department &d : StaffRepository(db).departments())
        if (d.id == doctor->departmentId)
            department = d;

    // Dosya adı: Recete-Ayse-Yilmaz-2026-10-07.pdf (adın sadece harf ve rakamları)
    QString name = patient->fullName.normalized(QString::NormalizationForm_KD);
    name.remove(QRegularExpression("[^A-Za-z0-9 ]")).replace(' ', '-');
    const QString suggested = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/"
                              + I18n::t("prescription_file") + "-" + name + "-" + exam.date.date().toString(Qt::ISODate)
                              + ".pdf";
    const QString path = QFileDialog::getSaveFileName(parent, I18n::t("save_pdf"), suggested, "PDF (*.pdf)");
    if (path.isEmpty())
        return;
    if (!PrescriptionPrinter::savePdf(PrescriptionPrinter::html(exam, *patient, *doctor, department), path)) {
        Ui::warn(parent, I18n::t("pdf_failed"));
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

} // namespace Prescription
