#include "services/Catalog.h"

#include "app/I18n.h"

#include <QFile>

namespace Catalog {

const QList<Diagnosis> &diagnoses()
{
    static const QList<Diagnosis> list = [] {
        QList<Diagnosis> result;
        QFile file(":/icd10.tsv");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
            while (!file.atEnd()) {
                const QStringList parts = QString::fromUtf8(file.readLine()).trimmed().split('\t');
                if (parts.size() == 3)
                    result << Diagnosis{parts[0], parts[1], parts[2]};
            }
        return result;
    }();
    return list;
}

const QStringList &medicines()
{
    // Etken madde (jenerik) adları; ticari ilaç adı kullanılmaz
    static const QStringList list = {
        "Amlodipin",      "Amoksisilin",      "Amoksisilin + Klavulanik asit", "Asetilsalisilik asit",
        "Atorvastatin",   "Azitromisin",      "Budesonid",          "Desloratadin",       "Diklofenak",
        "Esomeprazol",    "Ferröz sülfat",    "Flutikazon",         "Folik asit",         "Gliklazid",
        "Hidroklorotiyazid", "İbuprofen",     "Kolekalsiferol (D vitamini)", "Klaritromisin", "Levotiroksin",
        "Losartan",       "Metformin",        "Metoprolol",         "Montelukast",        "Naproksen",
        "Omeprazol",      "Pantoprazol",      "Parasetamol",        "Ramipril",           "Rosuvastatin",
        "Salbutamol",     "Sertralin",        "Setirizin",          "Siprofloksasin",     "Tiyokolşikosid",
    };
    return list;
}

const QStringList &usages()
{
    static const QStringList tr = {"Günde 1 kez, sabah", "Günde 1 kez, akşam", "Günde 2 kez, tok karnına",
                                   "Günde 3 kez, tok karnına", "Günde 2 kez, aç karnına", "Ağrı olduğunda, günde en fazla 3 kez",
                                   "Günde 2 kez, 2 puf", "Yatmadan önce"};
    static const QStringList en = {"Once a day, morning", "Once a day, evening", "Twice a day, after meals",
                                   "3 times a day, after meals", "Twice a day, before meals", "When in pain, up to 3 times a day",
                                   "Twice a day, 2 puffs", "At bedtime"};
    return I18n::language() == "en" ? en : tr;
}

} // namespace Catalog
