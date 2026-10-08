#include "app/I18n.h"

#include "core/Rules.h"

#include <QHash>
#include <QLocale>

namespace {

QString g_language = "tr";

struct Text {
    const char *tr;
    const char *en;
};

const QHash<QString, Text> &texts()
{
    static const QHash<QString, Text> table = {
        // Genel
        {"app_name", {"Hastane Yönetimi", "Hospital Manager"}},
        {"yes", {"Evet", "Yes"}},
        {"no", {"Hayır", "No"}},
        {"ok", {"Tamam", "OK"}},
        {"save", {"Kaydet", "Save"}},
        {"cancel", {"Vazgeç", "Cancel"}},
        {"close", {"Kapat", "Close"}},
        {"edit", {"Düzenle", "Edit"}},
        {"delete", {"Sil", "Delete"}},
        {"choose", {"Seçiniz", "Choose"}},
        {"none", {"Yok", "None"}},
        {"name", {"Ad", "Name"}},
        {"active", {"Aktif", "Active"}},
        {"inactive", {"Pasif", "Inactive"}},
        {"theme", {"Tema", "Theme"}},
        {"language", {"English", "Türkçe"}},
        {"about", {"Hakkında", "About"}},
        {"version", {"Sürüm {0}", "Version {0}"}},
        {"data_folder", {"Veri klasörü", "Data folder"}},
        {"view_on_github", {"GitHub'da görüntüle", "View on GitHub"}},
        {"about_text",
         {"C++ ve Qt ile yazılmış hastane yönetim uygulaması: hasta kayıtları, doktor takvimleri, randevular, "
          "muayene ve reçeteler. Yönetici, sekreter ve doktor rolleriyle çalışır; veriler sadece bu bilgisayarda durur.",
          "A hospital management app written in C++ and Qt: patient records, doctor schedules, appointments, "
          "examinations and prescriptions. It works with admin, receptionist and doctor roles; data stays on this "
          "computer."}},
        {"credits",
         {"Tanı kodları: Dünya Sağlık Örgütü ICD-10. Örnek verideki kişiler hayalidir.",
          "Diagnosis codes: World Health Organization ICD-10. People in the sample data are fictional."}},
        {"db_open_failed", {"Veritabanı açılamadı. Klasöre yazma izni olduğundan emin ol: {0}",
                            "The database could not be opened. Make sure the folder is writable: {0}"}},
        {"crypto_failed", {"Şifreleme kitaplığı başlatılamadı.", "The cryptography library could not be started."}},
        {"unexpected_error", {"Beklenmeyen bir hata oluştu. İşlem kaydedilmedi.",
                              "An unexpected error occurred. Nothing was saved."}},
        {"csv_saved", {"CSV dosyası kaydedildi.", "The CSV file was saved."}},
        {"csv_failed", {"Dosya kaydedilemedi. Klasöre yazma izni olduğundan emin ol.",
                        "The file could not be saved. Make sure the folder is writable."}},
        {"export_csv", {"CSV olarak dışa aktar", "Export as CSV"}},
        {"pdf_failed", {"PDF kaydedilemedi. Klasöre yazma izni olduğundan emin ol.",
                        "The PDF could not be saved. Make sure the folder is writable."}},
        {"save_pdf", {"PDF olarak kaydet", "Save as PDF"}},

        // İlk kurulum ve giriş
        {"setup_text", {"Hoş geldin! Başlamak için yönetici hesabını oluştur ya da uygulamayı örnek veriyle dene.",
                        "Welcome! Create the administrator account to start, or try the app with sample data."}},
        {"create_admin", {"Yönetici hesabı oluştur", "Create administrator account"}},
        {"start_with_demo", {"Örnek veriyle başla", "Start with sample data"}},
        {"demo_note",
         {"8 bölüm, 20 doktor, 1.200 hasta ve son üç ayın randevuları yüklenir. Her rol için hazır bir demo hesap gelir.",
          "Loads 8 departments, 20 doctors, 1,200 patients and the last three months of appointments, with a ready demo "
          "account for each role."}},
        {"demo_failed", {"Örnek veri yüklenemedi.", "The sample data could not be loaded."}},
        {"username", {"Kullanıcı adı", "Username"}},
        {"password", {"Şifre", "Password"}},
        {"repeat_password", {"Şifre tekrar", "Repeat password"}},
        {"password_rules",
         {"Şifre en az 10 karakter olmalı; büyük harf, küçük harf ve rakam içermeli, kullanıcı adını "
          "içermemeli.",
          "The password must be at least 10 characters, contain uppercase and lowercase letters and a digit, "
          "and must not contain the username."}},
        {"passwords_differ", {"Şifreler aynı değil.", "The passwords do not match."}},
        {"show_password", {"Şifreyi göster", "Show password"}},
        {"hide_password", {"Şifreyi gizle", "Hide password"}},
        {"sign_in", {"Giriş yap", "Sign in"}},
        {"sign_out", {"Çıkış yap", "Sign out"}},
        {"demo_accounts", {"Demo hesapları", "Demo accounts"}},
        {"demo_fill", {"Bilgileri giriş formuna doldur", "Fill in the sign-in form"}},
        {"signed_out_idle", {"Uzun süre işlem yapılmadığı için oturum kapatıldı.",
                             "You were signed out after a period of inactivity."}},
        {"role", {"Rol", "Role"}},
        {"role_admin", {"Yönetici", "Administrator"}},
        {"role_receptionist", {"Sekreter", "Receptionist"}},
        {"role_doctor", {"Doktor", "Doctor"}},

        // Menü
        {"appointments", {"Randevular", "Appointments"}},
        {"my_appointments", {"Randevularım", "My appointments"}},
        {"calendar", {"Takvim", "Calendar"}},
        {"patients", {"Hastalar", "Patients"}},
        {"reports", {"Raporlar", "Reports"}},
        {"staff", {"Bölümler ve doktorlar", "Departments and doctors"}},
        {"users", {"Kullanıcılar", "Users"}},
        {"change_password", {"Şifre değiştir", "Change password"}},

        // Randevular
        {"today", {"Bugün", "Today"}},
        {"date", {"Tarih", "Date"}},
        {"time", {"Saat", "Time"}},
        {"note", {"Not", "Note"}},
        {"status", {"Durum", "Status"}},
        {"patient", {"Hasta", "Patient"}},
        {"doctor", {"Doktor", "Doctor"}},
        {"doctors", {"Doktorlar", "Doctors"}},
        {"department", {"Bölüm", "Department"}},
        {"departments", {"Bölümler", "Departments"}},
        {"all_doctors", {"Bütün doktorlar", "All doctors"}},
        {"all_departments", {"Bütün bölümler", "All departments"}},
        {"search_patient", {"Hasta adı ya da T.C. kimlik no ile ara", "Search by patient name or ID number"}},
        {"summary_total", {"Toplam randevu", "Appointments"}},
        {"summary_waiting", {"Bekleniyor", "Expected"}},
        {"summary_checked_in", {"Geldi, sırada", "Checked in"}},
        {"summary_examined", {"Muayene edildi", "Examined"}},
        {"summary_no_show", {"Gelmedi", "No-show"}},
        {"new_appointment", {"Randevu ver", "Book appointment"}},
        {"book", {"Randevuyu kaydet", "Book"}},
        {"check_in", {"Geldi", "Check in"}},
        {"no_show", {"Gelmedi", "No-show"}},
        {"cancel_appointment", {"Randevuyu iptal et", "Cancel appointment"}},
        {"examine", {"Muayene et", "Examine"}},
        {"confirm_no_show", {"Hasta gelmedi olarak işaretlensin mi?\n{0}", "Mark the patient as a no-show?\n{0}"}},
        {"confirm_cancel", {"Randevu iptal edilsin mi? Saat başka hastaya açılır.\n{0}",
                            "Cancel this appointment? The time becomes free for other patients.\n{0}"}},
        {"choose_patient", {"Listeden bir hasta seç ya da yeni hasta ekle.", "Choose a patient from the list or add a new one."}},
        {"choose_doctor", {"Bir doktor seç.", "Choose a doctor."}},
        {"choose_slot", {"Boş bir saat seç.", "Choose a free time."}},
        {"selected_slot", {"Seçilen saat: {0}", "Selected time: {0}"}},
        {"doctor_not_working", {"Doktor bu gün çalışmıyor.", "The doctor does not work on this day."}},
        {"no_free_slots", {"Bu günde boş saat kalmadı; başka bir gün seç.", "No free times left on this day; choose another day."}},
        {"appointment_booked", {"Randevu kaydedildi.", "The appointment was booked."}},
        {"this_week", {"Bu hafta", "This week"}},
        {"double_click_to_book", {"Randevu vermek için çift tıkla", "Double-click to book"}},
        {"free_slots_week", {"Bu hafta {0} boş saat", "{0} free slots this week"}},

        // Hastalar
        {"new_patient", {"Yeni hasta", "New patient"}},
        {"edit_patient", {"Hastayı düzenle", "Edit patient"}},
        {"search_patient_long", {"Ad, T.C. kimlik no ya da telefon ile ara", "Search by name, ID number or phone"}},
        {"patient_count", {"{0} hasta", "{0} patients"}},
        {"national_id", {"T.C. kimlik no", "National ID"}},
        {"full_name", {"Ad soyad", "Full name"}},
        {"birth_date", {"Doğum tarihi", "Date of birth"}},
        {"age", {"Yaş", "Age"}},
        {"gender", {"Cinsiyet", "Gender"}},
        {"phone", {"Telefon", "Phone"}},
        {"email", {"E-posta", "Email"}},
        {"blood_group", {"Kan grubu", "Blood group"}},
        {"allergies", {"Alerjiler", "Allergies"}},
        {"allergies_hint", {"ör. Penisilin, polen", "e.g. Penicillin, pollen"}},
        {"chronic", {"Kronik hastalıklar", "Chronic conditions"}},
        {"patient_history", {"Hasta geçmişi", "Patient history"}},
        {"examinations", {"Muayeneler", "Examinations"}},
        {"confirm_delete_patient", {"{0} silinsin mi? Bu işlem geri alınamaz.", "Delete {0}? This cannot be undone."}},
        {"patients_file", {"Hastalar", "Patients"}},

        // Muayene ve reçete
        {"examination", {"Muayene", "Examination"}},
        {"edit_examination", {"Muayeneyi düzenle", "Edit examination"}},
        {"complaint", {"Şikâyet", "Complaint"}},
        {"findings", {"Bulgular", "Findings"}},
        {"diagnosis", {"Tanı", "Diagnosis"}},
        {"diagnosis_hint", {"Tanı adı ya da ICD-10 kodu yaz, listeden seç", "Type a diagnosis or ICD-10 code and pick from the list"}},
        {"doctor_notes", {"Doktor notu", "Doctor's notes"}},
        {"prescription", {"Reçete", "Prescription"}},
        {"medicines", {"İlaçlar", "Medicines"}},
        {"medicine", {"İlaç", "Medicine"}},
        {"dose", {"Doz", "Dose"}},
        {"usage", {"Kullanım", "Usage"}},
        {"days", {"Gün", "Days"}},
        {"duration", {"Süre", "Duration"}},
        {"day_count", {"{0} gün", "{0} days"}},
        {"add_medicine", {"İlaç ekle", "Add medicine"}},
        {"remove_medicine", {"Seçili ilacı çıkar", "Remove selected"}},
        {"save_and_print", {"Kaydet ve reçeteyi PDF yap", "Save and export prescription"}},
        {"prescription_pdf", {"Reçete PDF", "Prescription PDF"}},
        {"prescription_title", {"Reçete ve Muayene Özeti", "Prescription and Visit Summary"}},
        {"prescription_file", {"Recete", "Prescription"}},
        {"no_medicines", {"Bu muayenede ilaç yazılmadı.", "No medicines were prescribed in this visit."}},
        {"number", {"No", "No."}},
        {"location", {"Yer", "Location"}},
        {"location_hint", {"ör. A Blok, 2. kat", "e.g. Block A, 2nd floor"}},
        {"signature_stamp", {"İmza / Kaşe", "Signature / Stamp"}},
        {"generated_by", {"Bu belge {0} ile oluşturuldu.", "This document was generated by {0}."}},

        // Bölümler ve doktorlar
        {"new_department", {"Yeni bölüm", "New department"}},
        {"edit_department", {"Bölümü düzenle", "Edit department"}},
        {"confirm_delete_department", {"{0} bölümü silinsin mi?", "Delete the {0} department?"}},
        {"add_department_first", {"Önce bir bölüm ekle.", "Add a department first."}},
        {"new_doctor", {"Yeni doktor", "New doctor"}},
        {"edit_doctor", {"Doktoru düzenle", "Edit doctor"}},
        {"title_label", {"Unvan", "Title"}},
        {"work_days", {"Çalışma günleri", "Working days"}},
        {"work_hours", {"Çalışma saatleri", "Working hours"}},
        {"slot_length", {"Randevu süresi", "Appointment length"}},
        {"minutes_count", {"{0} dk", "{0} min"}},
        {"active_doctor", {"Randevu alabilir (aktif)", "Accepts appointments (active)"}},
        {"inactive_hint", {"Pasif doktora yeni randevu verilemez; eski kayıtları silinmez.",
                           "Inactive doctors cannot get new appointments; their records are kept."}},

        // Kullanıcılar ve şifreler
        {"new_user", {"Yeni kullanıcı", "New user"}},
        {"edit_user", {"Kullanıcıyı düzenle", "Edit user"}},
        {"doctor_record", {"Doktor kaydı", "Doctor record"}},
        {"account_active", {"Hesap aktif (giriş yapabilir)", "Account active (can sign in)"}},
        {"first_password", {"İlk şifre", "First password"}},
        {"first_password_note", {"Kullanıcı ilk girişte bu şifreyi kendi şifresiyle değiştirir.",
                                 "The user replaces this password with their own at first sign-in."}},
        {"users_note", {"Kullanıcılar silinmez, pasif yapılır: geçmiş kayıtlar kimin yaptığını göstermeye devam eder.",
                        "Users are not deleted but deactivated, so past records still show who made them."}},
        {"password_pending", {"şifre değişikliği bekleniyor", "password change pending"}},
        {"reset_password", {"Şifreyi sıfırla", "Reset password"}},
        {"reset_note", {"{0} için geçici bir şifre belirle. Kullanıcı ilk girişte şifresini değiştirmek zorunda olacak.",
                        "Set a temporary password for {0}. They will have to change it at their next sign-in."}},
        {"password_reset_done", {"{0} için şifre sıfırlandı.", "The password for {0} was reset."}},
        {"must_change_password", {"Hesabının şifresi yönetici tarafından sıfırlandı. Devam etmek için yeni bir şifre belirle.",
                                  "Your password was reset by an administrator. Set a new password to continue."}},
        {"current_password", {"Mevcut şifre", "Current password"}},
        {"new_password", {"Yeni şifre", "New password"}},
        {"password_changed", {"Şifren değiştirildi.", "Your password was changed."}},

        // Raporlar
        {"period", {"Dönem", "Period"}},
        {"last_7_days", {"Son 7 gün", "Last 7 days"}},
        {"last_30_days", {"Son 30 gün", "Last 30 days"}},
        {"last_90_days", {"Son 90 gün", "Last 90 days"}},
        {"this_year", {"Bu yıl", "This year"}},
        {"by_department", {"Bölümlere göre", "By department"}},
        {"doctor_occupancy", {"Doktor doluluk oranları", "Doctor occupancy"}},
        {"capacity", {"Kapasite", "Capacity"}},
        {"occupancy", {"Doluluk", "Occupancy"}},
        {"no_show_rate", {"Gelmeme", "No-show rate"}},
        {"monthly_appointments", {"Aylara göre randevular", "Appointments by month"}},
        {"report_file", {"Doktor-doluluk-raporu", "Doctor-occupancy-report"}},

        // Hatalar (veri katmanından gelen anahtarlar, "err_" önekiyle)
        {"err_login_failed", {"Kullanıcı adı ya da şifre yanlış.", "Wrong username or password."}},
        {"err_account_locked", {"Çok fazla hatalı deneme. 30 saniye sonra tekrar dene.",
                                "Too many failed attempts. Try again in 30 seconds."}},
        {"err_account_inactive", {"Bu hesap pasif. Yöneticiyle görüş.", "This account is inactive. Contact an administrator."}},
        {"err_forbidden", {"Bu işlem için yetkin yok.", "You are not allowed to do this."}},
        {"err_not_found", {"Kayıt bulunamadı; başka biri silmiş olabilir.", "The record was not found; someone may have deleted it."}},
        {"err_save_failed", {"Kayıt yapılamadı. Değişiklikler geri alındı.", "The record could not be saved. Changes were rolled back."}},
        {"err_invalid_state", {"Randevunun durumu bu işleme uygun değil.", "The appointment's status does not allow this."}},
        {"err_invalid_username", {"Kullanıcı adı 3-30 karakter olmalı; harf, rakam, nokta, tire ve alt çizgi içerebilir.",
                                  "The username must be 3-30 characters of letters, digits, dot, dash or underscore."}},
        {"err_duplicate_username", {"Bu kullanıcı adı kullanılıyor.", "This username is taken."}},
        {"err_doctor_link_required", {"Doktor rolündeki kullanıcı bir doktor kaydına bağlanmalı.",
                                      "A doctor user must be linked to a doctor record."}},
        {"err_cannot_demote_self", {"Kendi yönetici yetkini kaldıramaz ya da hesabını pasif yapamazsın.",
                                    "You cannot remove your own admin role or deactivate yourself."}},
        {"err_wrong_current_password", {"Mevcut şifre yanlış.", "The current password is wrong."}},
        {"err_same_password", {"Yeni şifre eskisiyle aynı olamaz.", "The new password must differ from the old one."}},
        {"err_password_short", {"Şifre en az 10 karakter olmalı.", "The password must be at least 10 characters."}},
        {"err_password_long", {"Şifre en fazla 128 karakter olabilir.", "The password can be at most 128 characters."}},
        {"err_password_variety", {"Şifre büyük harf, küçük harf ve rakam içermeli.",
                                  "Use uppercase and lowercase letters and a digit."}},
        {"err_password_has_username", {"Şifre kullanıcı adını içeremez.", "The password must not contain the username."}},
        {"err_invalid_name", {"Ad soyad 3-100 karakter olmalı.", "The name must be 3-100 characters."}},
        {"err_invalid_national_id", {"T.C. kimlik numarası geçersiz.", "The national ID number is invalid."}},
        {"err_invalid_birth_date", {"Doğum tarihi geçersiz.", "The date of birth is invalid."}},
        {"err_invalid_phone", {"Telefon numarası geçersiz (ör. 0532 123 45 67).", "The phone number is invalid (e.g. 0532 123 45 67)."}},
        {"err_invalid_email", {"E-posta adresi geçersiz.", "The email address is invalid."}},
        {"err_text_too_long", {"Metinlerden biri çok uzun.", "One of the texts is too long."}},
        {"err_duplicate_national_id", {"Bu T.C. kimlik numarasıyla kayıtlı bir hasta var.",
                                       "A patient with this national ID already exists."}},
        {"err_has_history", {"Randevusu olan hasta silinemez; kayıtlar saklanmalı.",
                             "A patient with appointments cannot be deleted; records must be kept."}},
        {"err_invalid_department", {"Geçerli bir bölüm seç.", "Choose a valid department."}},
        {"err_duplicate_department", {"Bu adla bir bölüm var.", "A department with this name exists."}},
        {"err_department_has_doctors", {"Doktoru olan bölüm silinemez.", "A department with doctors cannot be deleted."}},
        {"err_invalid_work_days", {"En az bir çalışma günü seç.", "Choose at least one working day."}},
        {"err_invalid_hours", {"Mesai bitişi başlangıçtan sonra olmalı ve en az bir randevu sığmalı.",
                               "The end time must be after the start and fit at least one appointment."}},
        {"err_invalid_slot", {"Randevu süresi geçersiz.", "The appointment length is invalid."}},
        {"err_doctor_inactive", {"Doktor şu an randevu kabul etmiyor.", "The doctor is not accepting appointments."}},
        {"err_slot_in_past", {"Geçmiş bir saate randevu verilemez.", "You cannot book a time in the past."}},
        {"err_too_far_ahead", {"En fazla 90 gün sonrasına randevu verilebilir.", "Appointments can be booked up to 90 days ahead."}},
        {"err_not_working", {"Doktor bu saatte çalışmıyor.", "The doctor does not work at this time."}},
        {"err_lunch_break", {"Öğle arasına randevu verilemez.", "You cannot book during the lunch break."}},
        {"err_slot_taken", {"Bu saat az önce doldu. Başka bir saat seç.", "This time was just taken. Choose another time."}},
        {"err_patient_busy", {"Hastanın bu saatte başka bir randevusu var.", "The patient has another appointment at this time."}},
        {"err_not_today", {"\"Geldi\" sadece bugünkü randevular için işaretlenebilir.", "Only today's appointments can be checked in."}},
        {"err_not_started_yet", {"Randevu saati henüz gelmedi.", "The appointment has not started yet."}},
        {"err_already_started", {"Saati geçmiş randevu iptal edilemez; \"Gelmedi\" olarak işaretle.",
                                 "A past appointment cannot be cancelled; mark it as a no-show."}},
        {"err_complaint_required", {"Şikâyet yazılmalı.", "Enter the complaint."}},
        {"err_diagnosis_required", {"Tanı yazılmalı.", "Enter the diagnosis."}},
        {"err_too_many_medicines", {"Bir reçetede en fazla 20 ilaç olabilir.", "A prescription can have at most 20 medicines."}},
        {"err_invalid_prescription", {"Her ilaçta ad olmalı ve süre 1-365 gün arasında olmalı.",
                                      "Each medicine needs a name and a duration of 1-365 days."}},
    };
    return table;
}

QString lookup(const QString &key)
{
    const auto it = texts().constFind(key);
    if (it == texts().constEnd())
        return key;
    return QString::fromUtf8(g_language == "en" ? it->en : it->tr);
}

QString pick(const Text &text)
{
    return QString::fromUtf8(g_language == "en" ? text.en : text.tr);
}

} // namespace

namespace I18n {

void setLanguage(const QString &language)
{
    g_language = language == "en" ? "en" : "tr";
    QLocale::setDefault(QLocale(g_language == "en" ? QLocale::English : QLocale::Turkish,
                                g_language == "en" ? QLocale::UnitedStates : QLocale::Turkey));
}

QString language()
{
    return g_language;
}

QString t(const char *key)
{
    return lookup(QString::fromLatin1(key));
}

QString error(const QString &key)
{
    const QString text = lookup("err_" + key);
    return text.startsWith("err_") ? lookup("unexpected_error") : text;
}

QString role(Role value)
{
    static const Text names[] = {{"Yönetici", "Administrator"}, {"Sekreter", "Receptionist"}, {"Doktor", "Doctor"}};
    return pick(names[static_cast<int>(value)]);
}

QString gender(Gender value)
{
    static const Text names[] = {{"Kadın", "Female"}, {"Erkek", "Male"}, {"Belirtilmemiş", "Not specified"}};
    return pick(names[static_cast<int>(value)]);
}

QString bloodGroup(BloodGroup value)
{
    static const Text names[] = {{"Bilinmiyor", "Unknown"}, {"A Rh+", "A+"},   {"A Rh−", "A−"},
                                 {"B Rh+", "B+"},           {"B Rh−", "B−"},   {"AB Rh+", "AB+"},
                                 {"AB Rh−", "AB−"},         {"0 Rh+", "O+"},   {"0 Rh−", "O−"}};
    return pick(names[static_cast<int>(value)]);
}

QString status(AppointmentStatus value)
{
    static const Text names[] = {{"Randevulu", "Booked"},
                                 {"Geldi", "Checked in"},
                                 {"Muayene edildi", "Examined"},
                                 {"Gelmedi", "No-show"},
                                 {"İptal", "Cancelled"}};
    return pick(names[static_cast<int>(value)]);
}

QString slotState(Schedule::SlotState value)
{
    static const Text names[] = {{"Boş", "Free"}, {"Dolu", "Booked"}, {"Geçti", "Past"}, {"Öğle arası", "Lunch break"}};
    return pick(names[static_cast<int>(value)]);
}

QString weekday(int day)
{
    static const Text names[] = {{"Pzt", "Mon"}, {"Sal", "Tue"}, {"Çar", "Wed"}, {"Per", "Thu"},
                                 {"Cum", "Fri"}, {"Cmt", "Sat"}, {"Paz", "Sun"}};
    return day >= 1 && day <= 7 ? pick(names[day - 1]) : QString();
}

QString date(const QDate &value)
{
    if (!value.isValid())
        return "—";
    return value.toString(g_language == "en" ? "MMM d, yyyy" : "dd.MM.yyyy");
}

QString dateTime(const QDateTime &value)
{
    if (!value.isValid())
        return "—";
    return value.toString(g_language == "en" ? "MMM d, yyyy  HH:mm" : "dd.MM.yyyy  HH:mm");
}

QString phone(const QString &digits)
{
    // Kayıtta numara sadece rakam olarak tutulur; ekranda okunması kolay biçimde gösterilir
    if (digits.size() != 10)
        return digits;
    return QString("0%1 %2 %3 %4").arg(digits.left(3), digits.mid(3, 3), digits.mid(6, 2), digits.mid(8, 2));
}

QString age(const QDate &birthDate, const QDate &today)
{
    const int years = Rules::fullYears(birthDate, today);
    return g_language == "en" ? QString("%1 years old").arg(years) : QString("%1 yaş").arg(years);
}

} // namespace I18n
