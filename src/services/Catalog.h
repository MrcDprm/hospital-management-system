#pragma once

#include <QList>
#include <QStringList>

// Muayene ekranındaki seçenek listeleri: sık kullanılan ICD-10 tanıları ve ilaç adları.
// Listeden seçmek zorunlu değil; doktor kendisi de yazabilir.
namespace Catalog {

struct Diagnosis {
    QString code;
    QString nameTr;
    QString nameEn;
};

const QList<Diagnosis> &diagnoses(); // gömülü icd10.tsv
const QStringList &medicines();      // etken madde adları
const QStringList &usages();         // "Günde 2 kez, tok karnına" gibi kalıplar (seçili dile göre)

} // namespace Catalog
