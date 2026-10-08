#pragma once

#include "core/Models.h"

// Muayene sonrası reçete ve muayene özeti (A4, PDF). Hasta, doktor, tanı ve ilaç listesi.
namespace PrescriptionPrinter {

QString html(const Examination &exam, const Patient &patient, const Doctor &doctor, const Department &department);
bool savePdf(const QString &html, const QString &path);

} // namespace PrescriptionPrinter
