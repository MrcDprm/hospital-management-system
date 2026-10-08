#include "services/CsvExport.h"

#include <QSaveFile>

namespace CsvExport {

QChar separatorFor(const QString &language)
{
    return language == "en" ? ',' : ';';
}

QString escape(const QString &cell, QChar separator)
{
    QString value = cell;
    // "CSV injection": =, +, -, @ ile başlayan hücreyi Excel formül sanıp çalıştırabilir.
    // Başına ' konunca metin olarak gösterilir.
    if (!value.isEmpty() && QString("=+-@\t\r").contains(value.front()))
        value.prepend('\'');
    if (value.contains(separator) || value.contains('"') || value.contains('\n') || value.contains('\r')) {
        value.replace("\"", "\"\"");
        value = '"' + value + '"';
    }
    return value;
}

bool write(const QString &path, const QStringList &header, const QList<QStringList> &rows, QChar separator)
{
    QSaveFile file(path); // önce geçici dosyaya yazar; yarıda kalırsa eski dosya bozulmaz
    if (!file.open(QIODevice::WriteOnly))
        return false;
    QByteArray data = "\xEF\xBB\xBF"; // UTF-8 BOM: Excel Türkçe karakterleri doğru göstersin
    auto line = [&](const QStringList &cells) {
        QStringList escaped;
        for (const QString &cell : cells)
            escaped << escape(cell, separator);
        data += escaped.join(separator).toUtf8() + "\r\n";
    };
    line(header);
    for (const QStringList &row : rows)
        line(row);
    return file.write(data) == data.size() && file.commit();
}

} // namespace CsvExport