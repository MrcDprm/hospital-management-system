#include "ui/PatientsPage.h"

#include "core/Access.h"
#include "core/Rules.h"
#include "data/PatientRepository.h"
#include "services/CsvExport.h"
#include "ui/BookingDialog.h"
#include "ui/PatientDialog.h"
#include "ui/PatientHistoryDialog.h"
#include "ui/UiHelpers.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QStandardPaths>
#include <QVBoxLayout>

PatientsPage::PatientsPage(Database &db, const User &user, QWidget *parent)
    : QWidget(parent), m_db(db), m_user(user)
{
    const bool manages = Access::allowed(user.role, Permission::ManagePatients);
    const bool books = Access::allowed(user.role, Permission::ManageAppointments);

    auto *top = new QHBoxLayout;
    top->addWidget(Ui::title(I18n::t("patients"), this));
    top->addStretch();
    if (manages) {
        auto *add = Ui::accentButton("+ " + I18n::t("new_patient"), this);
        connect(add, &QPushButton::clicked, this, [this] { edit(Patient{}); });
        top->addWidget(add);
    }

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(I18n::t("search_patient_long"));
    m_search->setClearButtonEnabled(true);
    m_count = new QLabel(this);
    m_count->setObjectName("muted");
    auto *filters = new QHBoxLayout;
    filters->addWidget(m_search, 1);
    filters->addWidget(m_count);

    m_table = Ui::makeTable({I18n::t("full_name"), I18n::t("national_id"), I18n::t("age"), I18n::t("gender"),
                             I18n::t("phone"), I18n::t("blood_group"), I18n::t("allergies")},
                            this);
    m_table->setSortingEnabled(true);

    m_edit = new QPushButton(I18n::t("edit"), this);
    m_delete = new QPushButton(I18n::t("delete"), this);
    m_book = new QPushButton(I18n::t("new_appointment"), this);
    m_history = new QPushButton(I18n::t("patient_history"), this);
    auto *csv = new QPushButton(I18n::t("export_csv"), this);
    m_edit->setVisible(manages);
    m_delete->setVisible(manages);
    m_book->setVisible(books);
    csv->setVisible(manages);
    auto *actions = new QHBoxLayout;
    for (QPushButton *button : {m_edit, m_delete, m_book, m_history})
        actions->addWidget(button);
    actions->addStretch();
    actions->addWidget(csv);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(12);
    layout->addLayout(top);
    layout->addLayout(filters);
    layout->addWidget(m_table, 1);
    layout->addLayout(actions);

    connect(m_search, &QLineEdit::textChanged, this, &PatientsPage::fill);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &PatientsPage::updateButtons);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this, manages] {
        if (manages)
            edit(selected());
        else
            PatientHistoryDialog(m_db, m_user, selected(), this).exec();
    });
    connect(m_edit, &QPushButton::clicked, this, [this] { edit(selected()); });
    connect(m_delete, &QPushButton::clicked, this, &PatientsPage::remove);
    connect(m_book, &QPushButton::clicked, this, [this] {
        if (BookingDialog(m_db, m_user, this, selected().id).exec() == QDialog::Accepted)
            Ui::inform(this, I18n::t("appointment_booked"));
    });
    connect(m_history, &QPushButton::clicked, this, [this] { PatientHistoryDialog(m_db, m_user, selected(), this).exec(); });
    connect(csv, &QPushButton::clicked, this, &PatientsPage::exportCsv);
}

void PatientsPage::refresh()
{
    m_patients = PatientRepository(m_db).all();
    fill();
}

void PatientsPage::fill()
{
    const qint64 keep = Ui::selectedId(m_table);
    const QString search = m_search->text().trimmed();
    const QString digits = Rules::normalizePhone(search);
    m_table->setSortingEnabled(false); // doldururken sıralama satırları kaydırmasın
    m_table->setRowCount(0);
    for (const Patient &p : m_patients) {
        // Ad, T.C. kimlik numarasının başı ya da telefonun bir kısmıyla aranır
        if (!search.isEmpty() && !p.fullName.contains(search, Qt::CaseInsensitive) && !p.nationalId.startsWith(search)
            && !(digits.size() >= 3 && p.phone.contains(digits)))
            continue;
        const int row = Ui::addRow(m_table,
                                   {p.fullName, Ui::maskId(p.nationalId), QString::number(Rules::fullYears(p.birthDate, QDate::currentDate())),
                                    I18n::gender(p.gender), I18n::phone(p.phone), I18n::bloodGroup(p.bloodGroup), p.allergies},
                                   p.id);
        // Yaş sütunu sayı olarak sıralansın ("9" "10"dan büyük görünmesin)
        m_table->item(row, 2)->setData(Qt::DisplayRole, Rules::fullYears(p.birthDate, QDate::currentDate()));
        if (!p.allergies.isEmpty())
            m_table->item(row, 6)->setForeground(Theme::danger());
        if (p.id == keep)
            m_table->selectRow(row);
    }
    m_table->setSortingEnabled(true);
    m_table->resizeColumnsToContents();
    m_count->setText(I18n::t("patient_count").replace("{0}", QString::number(m_table->rowCount())));
    updateButtons();
}

Patient PatientsPage::selected() const
{
    const qint64 id = Ui::selectedId(m_table);
    for (const Patient &p : m_patients)
        if (p.id == id)
            return p;
    return {};
}

void PatientsPage::updateButtons()
{
    const bool any = selected().id != 0;
    for (QPushButton *button : {m_edit, m_delete, m_book, m_history})
        button->setEnabled(any);
}

void PatientsPage::edit(const Patient &patient)
{
    PatientDialog dialog(m_db, m_user, patient, this);
    if (dialog.exec() == QDialog::Accepted)
        refresh();
}

void PatientsPage::remove()
{
    const Patient p = selected();
    if (!p.id || !Ui::ask(this, I18n::t("confirm_delete_patient").replace("{0}", p.fullName)))
        return;
    if (Ui::showResult(this, PatientRepository(m_db).remove(m_user, p.id)))
        refresh();
}

void PatientsPage::exportCsv()
{
    const QString path = QFileDialog::getSaveFileName(
        this, I18n::t("export_csv"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + I18n::t("patients_file") + ".csv",
        "CSV (*.csv)");
    if (path.isEmpty())
        return;
    // Dışa aktarımda da tıbbi bilgi yok: kimlik, iletişim ve kan grubu
    QList<QStringList> rows;
    for (const Patient &p : m_patients)
        rows << QStringList{p.fullName, p.nationalId, p.birthDate.toString(Qt::ISODate), I18n::gender(p.gender),
                            I18n::phone(p.phone), p.email, I18n::bloodGroup(p.bloodGroup)};
    const QStringList header = {I18n::t("full_name"), I18n::t("national_id"), I18n::t("birth_date"), I18n::t("gender"),
                                I18n::t("phone"), I18n::t("email"), I18n::t("blood_group")};
    if (CsvExport::write(path, header, rows, CsvExport::separatorFor(I18n::language())))
        Ui::inform(this, I18n::t("csv_saved"));
    else
        Ui::warn(this, I18n::t("csv_failed"));
}
