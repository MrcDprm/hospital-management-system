#pragma once

#include <QList>
#include <QStringList>

// Listeleri Excel'de açılabilen CSV dosyasına yazar.
namespace CsvExport {

QChar separatorFor(const QString &language); // Türkçe Excel ";" bekler, İngilizce ","
QString escape(const QString &cell, QChar separator);
bool write(const QString &path, const QStringList &header, const QList<QStringList> &rows, QChar separator);

} // namespace CsvExport