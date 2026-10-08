#include "ui/PatientHistoryDialog.h"

#include "core/Access.h"
#include "data/AppointmentRepository.h"
#include "data/ExaminationRepository.h"
#include "data/StaffRepository.h"
#include "ui/ExaminationDialog.h"
#include "ui/UiHelpers.h"

#include <QDialogButtonBox>
#include <QHash>
#include <QSplitter>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace {

// Muayenenin okunur özeti (sağdaki panel); kullanıcı verisi kaçışlanır
QString summary(const Examination &e, const QString &doctor)
{
    auto section = [](const char *key, const QString &text) {
        return text.isEmpty() ? QString()
                              : QString("<p><b>%1</b><br>%2</p>")
                                    .arg(I18n::t(key).toHtmlEscaped(), text.toHtmlEscaped().replace("\n", "<br>"));
    };
    QString html = QString("<h3>%1 %2</h3><p>%3 · %4</p>")
                       .arg(e.diagnosisCode.toHtmlEscaped(), e.diagnosisName.toHtmlEscaped(),
                            I18n::dateTime(e.date), doctor.toHtmlEscaped());
    html += section("complaint", e.complaint) + section("findings", e.findings) + section("doctor_notes", e.notes);
    if (!e.prescription.isEmpty()) {
        html += "<p><b>" + I18n::t("prescription").toHtmlEscaped() + "</b></p><ul>";
        for (const PrescriptionItem &item : e.prescription)
            html += QString("<li>%1 %2 · %3 · %4</li>")
                        .arg(item.medicine.toHtmlEscaped(), item.dose.toHtmlEscaped(), item.usage.toHtmlEscaped(),
                             I18n::t("day_count").replace("{0}", QString::number(item.days)));
        html += "</ul>";
    }
    return html;
}

} // namespace

PatientHistoryDialog::PatientHistoryDialog(Database &db, const User &actor, const Patient &patient, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(I18n::t("patient_history") + " · " + patient.fullName);
    QHash<qint64, QString> doctors;
    for (const Doctor &d : StaffRepository(db).doctors())
        doctors.insert(d.id, d.displayName());

    auto *tabs = new QTabWidget(this);

    // Randevular: herkes görebilir (tarih, doktor, durum); tanı içermez
    auto *appointments = Ui::makeTable({I18n::t("date"), I18n::t("doctor"), I18n::t("status"), I18n::t("note")}, tabs);
    QList<Appointment> list = AppointmentRepository(db).forPatient(patient.id);
    std::sort(list.begin(), list.end(), [](const Appointment &a, const Appointment &b) { return a.start > b.start; });
    for (const Appointment &a : list)
        Ui::addRow(appointments, {I18n::dateTime(a.start), doctors.value(a.doctorId), I18n::status(a.status), a.note},
                   a.id, Ui::statusColor(a.status));
    appointments->resizeColumnsToContents();
    tabs->addTab(appointments, I18n::t("appointments") + QString(" (%1)").arg(list.size()));

    // Muayeneler: sadece doktor (tıbbi kayıt)
    if (Access::allowed(actor.role, Permission::ViewMedicalRecords)) {
        const QList<Examination> exams = ExaminationRepository(db).forPatient(actor, patient.id);
        auto *splitter = new QSplitter(tabs);
        auto *table = Ui::makeTable({I18n::t("date"), I18n::t("diagnosis"), I18n::t("doctor")}, splitter);
        for (const Examination &e : exams)
            Ui::addRow(table, {I18n::date(e.date.date()), e.diagnosisCode + " " + e.diagnosisName, doctors.value(e.doctorId)},
                       e.id);
        table->resizeColumnsToContents();
        auto *right = new QWidget(splitter);
        auto *rightLayout = new QVBoxLayout(right);
        rightLayout->setContentsMargins(0, 0, 0, 0);
        auto *detail = new QTextBrowser(right);
        detail->setOpenLinks(false);
        auto *pdf = new QPushButton(I18n::t("prescription_pdf"), right);
        pdf->setEnabled(false);
        rightLayout->addWidget(detail, 1);
        rightLayout->addWidget(pdf, 0, Qt::AlignRight);
        splitter->setStretchFactor(0, 3);
        splitter->setStretchFactor(1, 2);
        connect(table, &QTableWidget::itemSelectionChanged, this, [=] {
            const qint64 id = Ui::selectedId(table);
            for (const Examination &e : exams)
                if (e.id == id)
                    detail->setHtml(summary(e, doctors.value(e.doctorId)));
            pdf->setEnabled(id != 0);
        });
        connect(pdf, &QPushButton::clicked, this, [=, &db, this] {
            const qint64 id = Ui::selectedId(table);
            for (const Examination &e : exams)
                if (e.id == id)
                    Prescription::exportPdf(this, db, e);
        });
        tabs->addTab(splitter, I18n::t("examinations") + QString(" (%1)").arg(exams.size()));
        if (!exams.isEmpty())
            tabs->setCurrentIndex(1);
    }

    auto *buttons = new QDialogButtonBox(this);
    buttons->addButton(I18n::t("close"), QDialogButtonBox::RejectRole);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tabs, 1);
    layout->addWidget(buttons);
    resize(900, 560);
}
