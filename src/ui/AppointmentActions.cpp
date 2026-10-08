#include "ui/AppointmentActions.h"

#include "core/Access.h"
#include "data/AppointmentRepository.h"
#include "data/PatientRepository.h"
#include "ui/ExaminationDialog.h"
#include "ui/PatientHistoryDialog.h"
#include "ui/UiHelpers.h"

#include <QMenu>

namespace AppointmentActions {

bool can(Action action, const User &user, const Appointment &a, const QDateTime &now)
{
    if (a.id == 0)
        return false;
    const bool manages = Access::allowed(user.role, Permission::ManageAppointments);
    const bool owns = user.role == Role::Doctor && user.doctorId == a.doctorId;
    const bool booked = a.status == AppointmentStatus::Booked;
    switch (action) {
    case Action::CheckIn:
        return manages && booked && a.start.date() == now.date();
    case Action::NoShow:
        return (manages || owns) && booked && a.start <= now;
    case Action::Cancel:
        return manages && booked && a.start > now;
    case Action::Examine:
        // Muayene edilmiş randevu tekrar açılabilir (kaydı düzeltmek için)
        return owns && Access::allowed(user.role, Permission::WriteExaminations) && a.start.date() <= now.date()
               && (booked || a.status == AppointmentStatus::CheckedIn || a.status == AppointmentStatus::Examined);
    case Action::History:
        return true;
    }
    return false;
}

bool run(Action action, QWidget *parent, Database &db, const User &user, const Appointment &a)
{
    AppointmentRepository repo(db);
    const Patient patient = PatientRepository(db).find(a.patientId).value_or(Patient{});
    const QString who = QString("%1 · %2").arg(patient.fullName, I18n::dateTime(a.start));
    switch (action) {
    case Action::CheckIn:
        return Ui::showResult(parent, repo.checkIn(user, a.id));
    case Action::NoShow:
        if (!Ui::ask(parent, I18n::t("confirm_no_show").replace("{0}", who)))
            return false;
        return Ui::showResult(parent, repo.markNoShow(user, a.id));
    case Action::Cancel:
        if (!Ui::ask(parent, I18n::t("confirm_cancel").replace("{0}", who)))
            return false;
        return Ui::showResult(parent, repo.cancel(user, a.id));
    case Action::Examine:
        return ExaminationDialog(db, user, a, parent).exec() == QDialog::Accepted;
    case Action::History:
        PatientHistoryDialog(db, user, patient, parent).exec();
        return false;
    }
    return false;
}

void fillMenu(QMenu *menu, QWidget *parent, Database &db, const User &user, const Appointment &a,
              const std::function<void()> &changed)
{
    const struct {
        Action action;
        const char *key;
    } items[] = {{Action::Examine, "examine"},
                 {Action::CheckIn, "check_in"},
                 {Action::NoShow, "no_show"},
                 {Action::Cancel, "cancel_appointment"},
                 {Action::History, "patient_history"}};
    for (const auto &item : items) {
        if (!can(item.action, user, a))
            continue; // yapılamayan işlem menüde hiç görünmez
        QAction *entry = menu->addAction(I18n::t(item.key));
        QObject::connect(entry, &QAction::triggered, parent, [=, &db, &user] {
            if (run(item.action, parent, db, user, a))
                changed();
        });
    }
}

} // namespace AppointmentActions
