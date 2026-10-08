#include "services/PrescriptionPrinter.h"

#include "app/I18n.h"

#include <QAbstractTextDocumentLayout>
#include <QFileInfo>
#include <QPageSize>
#include <QPdfWriter>
#include <QTextDocument>

namespace {

const QString ACCENT = "#0f6e6e";
const QString LIGHT = "#e8f3f2";
const QString LINE = "#c3dbd8";
const QString MUTED = "#5f6b7a";

// Kayıtlardaki metinler HTML'e kaçışlanarak girer: hasta adında "<b>" olsa bile biçim olarak çalışmaz
QString escaped(const QString &text)
{
    return text.toHtmlEscaped();
}

QString t(const char *key)
{
    return escaped(I18n::t(key));
}

QString orDash(const QString &text)
{
    return text.trimmed().isEmpty() ? "—" : text;
}

QString row(const QString &label, const QString &value)
{
    return QString("<tr><td width='38%' style='color:%1'>%2</td><td><b>%3</b></td></tr>")
        .arg(MUTED, escaped(label), escaped(orDash(value)));
}

QString box(const QString &title, const QString &rows)
{
    return QString("<table width='100%' cellspacing='0' cellpadding='4' style='border:1px solid %1'>"
                   "<tr><td colspan='2' bgcolor='%2' style='color:%3'><b>%4</b></td></tr>%5</table>")
        .arg(LINE, LIGHT, ACCENT, escaped(title), rows);
}

QString columns(const QString &left, const QString &right)
{
    return QString("<table width='100%' cellspacing='0' cellpadding='0'><tr><td width='49%' valign='top'>%1</td>"
                   "<td width='2%'></td><td width='49%' valign='top'>%2</td></tr></table><p></p>")
        .arg(left, right);
}

// Başlıklı serbest metin bölümü (şikâyet, bulgular); satır sonları korunur
QString section(const QString &title, const QString &text)
{
    return QString("<p style='color:%1;font-size:10.5pt;margin-bottom:2px'><b>%2</b></p>"
                   "<p style='margin-top:0'>%3</p>")
        .arg(ACCENT, escaped(title), escaped(orDash(text)).replace("\n", "<br>"));
}

} // namespace

namespace PrescriptionPrinter {

QString html(const Examination &exam, const Patient &patient, const Doctor &doctor, const Department &department)
{
    const QString number = QString("%1").arg(exam.id, 6, 10, QChar('0'));
    QString page = "<html><body style='font-family:\"Segoe UI\";font-size:9.5pt;color:#1a1a1a'>";

    // Başlık bandı: solda belge ve hastane adı, sağda numara ve tarih
    page += QString("<table width='100%' cellspacing='0' cellpadding='14' bgcolor='%1'><tr>"
                    "<td style='color:white'><span style='font-size:20pt;font-weight:bold'>&#8478; %2</span><br>%3</td>"
                    "<td align='right' style='color:white'>%4 <b>%5</b><br>%6 <b>%7</b></td></tr></table><p></p>")
                .arg(ACCENT, t("prescription_title"), t("app_name"), t("number"), number, t("date"),
                     escaped(I18n::dateTime(exam.date)));

    const QString patientRows = row(I18n::t("full_name"), patient.fullName)
                                + row(I18n::t("national_id"), patient.nationalId)
                                + row(I18n::t("birth_date"), I18n::date(patient.birthDate) + " ("
                                                                 + I18n::age(patient.birthDate, exam.date.date()) + ")")
                                + row(I18n::t("gender"), I18n::gender(patient.gender))
                                + row(I18n::t("blood_group"), I18n::bloodGroup(patient.bloodGroup))
                                + row(I18n::t("allergies"), patient.allergies);
    const QString doctorRows = row(I18n::t("doctor"), doctor.displayName())
                               + row(I18n::t("department"), department.name)
                               + row(I18n::t("location"), department.location)
                               + row(I18n::t("phone"), I18n::phone(doctor.phone))
                               + row(I18n::t("email"), doctor.email);
    page += columns(box(I18n::t("patient"), patientRows), box(I18n::t("doctor"), doctorRows));

    // Tanı kutusu
    page += QString("<table width='100%' cellspacing='0' cellpadding='8' style='border:1px solid %1'><tr>"
                    "<td bgcolor='%2' width='22%' style='color:%3'><b>%4</b></td>"
                    "<td><span style='font-size:12pt;font-weight:bold'>%5</span>&nbsp;&nbsp;%6</td></tr></table><p></p>")
                .arg(LINE, LIGHT, ACCENT, t("diagnosis"), escaped(exam.diagnosisCode), escaped(exam.diagnosisName));

    page += section(I18n::t("complaint"), exam.complaint);
    page += section(I18n::t("findings"), exam.findings);

    // İlaç tablosu
    QString items;
    int index = 0;
    for (const PrescriptionItem &item : exam.prescription) {
        const QString background = (index % 2) ? QString(" bgcolor='%1'").arg(LIGHT) : QString();
        items += QString("<tr%1><td align='center'>%2</td><td><b>%3</b></td><td>%4</td><td>%5</td>"
                         "<td align='center'>%6</td></tr>")
                     .arg(background)
                     .arg(++index)
                     .arg(escaped(item.medicine), escaped(orDash(item.dose)), escaped(orDash(item.usage)),
                          item.days > 0 ? I18n::t("day_count").replace("{0}", QString::number(item.days)) : "—");
    }
    if (items.isEmpty())
        items = QString("<tr><td colspan='5' align='center' style='color:%1'>%2</td></tr>").arg(MUTED, t("no_medicines"));
    page += QString("<p style='color:%1;font-size:10.5pt;margin-bottom:4px'><b>%2</b></p>"
                    "<table width='100%' cellspacing='0' cellpadding='6' border='1' style='border-collapse:collapse;"
                    "border-color:%3'><tr bgcolor='%1' style='color:white'><td width='6%' align='center'><b>#</b></td>"
                    "<td width='32%'><b>%4</b></td><td width='16%'><b>%5</b></td><td><b>%6</b></td>"
                    "<td width='12%' align='center'><b>%7</b></td></tr>%8</table><p></p>")
                .arg(ACCENT, t("medicines"), LINE, t("medicine"), t("dose"), t("usage"), t("duration"), items);

    if (!exam.notes.trimmed().isEmpty())
        page += section(I18n::t("doctor_notes"), exam.notes);

    // İmza ve kaşe alanı sağda
    page += QString("<p></p><table width='100%' cellspacing='0' cellpadding='10'><tr><td width='55%'></td>"
                    "<td style='border:1px solid %1' height='90' valign='top'><b>%2</b><br>%3<br>"
                    "<span style='color:%4'>%5</span></td></tr></table>")
                .arg(LINE, t("signature_stamp"), escaped(doctor.displayName()), MUTED, escaped(department.name));

    page += QString("<p align='center' style='color:%1;font-size:8pt'>%2</p>")
                .arg(MUTED, escaped(I18n::t("generated_by").replace("{0}", I18n::t("app_name"))));
    return page + "</body></html>";
}

bool savePdf(const QString &html, const QString &path)
{
    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(14, 12, 14, 12), QPageLayout::Millimeter);
    writer.setResolution(300);
    writer.setTitle(QFileInfo(path).completeBaseName());

    QTextDocument document;
    // Yazı boyutları PDF'in çözünürlüğüne göre hesaplansın; yoksa 300 dpi sayfada her şey minicik kalır
    document.documentLayout()->setPaintDevice(&writer);
    document.setHtml(html);
    document.setPageSize(writer.pageLayout().paintRectPixels(writer.resolution()).size());
    document.print(&writer);
    return QFileInfo(path).size() > 0; // klasöre yazılamadıysa QPdfWriter sessizce başarısız olur
}

} // namespace PrescriptionPrinter
