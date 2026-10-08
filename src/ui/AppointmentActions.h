#pragma once

#include "core/Models.h"

#include <QDateTime>

#include <functional>

class Database;
class QMenu;
class QWidget;

// Bir randevu üzerinde yapılabilecek işlemler. Randevu listesi ve takvim aynı kuralları kullanır:
// düğme ya da menü maddesi sadece işlem gerçekten yapılabilecekse etkin olur (asıl kontrol yine veri katmanında).
namespace AppointmentActions {

enum class Action { CheckIn, NoShow, Cancel, Examine, History };

bool can(Action action, const User &user, const Appointment &a, const QDateTime &now = QDateTime::currentDateTime());
// İşlemi yapar (onay sorar, hata gösterir); kayıt değiştiyse true
bool run(Action action, QWidget *parent, Database &db, const User &user, const Appointment &a);
// Takvimdeki sağ tık menüsü
void fillMenu(QMenu *menu, QWidget *parent, Database &db, const User &user, const Appointment &a,
              const std::function<void()> &changed);

} // namespace AppointmentActions
