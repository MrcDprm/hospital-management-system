#include "data/Database.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace {

// Her tablo ayrı bir komut; QSqlQuery tek seferde tek komut çalıştırır
const char *const SCHEMA[] = {
    R"(CREATE TABLE IF NOT EXISTS departments (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL UNIQUE COLLATE NOCASE,
        location TEXT NOT NULL))",
    R"(CREATE TABLE IF NOT EXISTS doctors (
        id INTEGER PRIMARY KEY,
        title TEXT NOT NULL,
        full_name TEXT NOT NULL,
        department_id INTEGER NOT NULL REFERENCES departments(id),
        phone TEXT NOT NULL,
        email TEXT NOT NULL,
        work_days INTEGER NOT NULL,
        start_time TEXT NOT NULL,
        end_time TEXT NOT NULL,
        slot_minutes INTEGER NOT NULL CHECK (slot_minutes BETWEEN 5 AND 120),
        active INTEGER NOT NULL))",
    R"(CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY,
        username TEXT NOT NULL UNIQUE COLLATE NOCASE,
        full_name TEXT NOT NULL,
        role INTEGER NOT NULL,
        doctor_id INTEGER REFERENCES doctors(id),
        password_hash TEXT NOT NULL,
        active INTEGER NOT NULL,
        must_change INTEGER NOT NULL,
        failed_attempts INTEGER NOT NULL DEFAULT 0,
        locked_until TEXT))",
    R"(CREATE TABLE IF NOT EXISTS patients (
        id INTEGER PRIMARY KEY,
        national_id TEXT NOT NULL UNIQUE,
        full_name TEXT NOT NULL,
        birth_date TEXT NOT NULL,
        gender INTEGER NOT NULL,
        phone TEXT NOT NULL,
        email TEXT NOT NULL,
        blood_group INTEGER NOT NULL,
        allergies TEXT NOT NULL,
        chronic TEXT NOT NULL))",
    R"(CREATE TABLE IF NOT EXISTS appointments (
        id INTEGER PRIMARY KEY,
        patient_id INTEGER NOT NULL REFERENCES patients(id),
        doctor_id INTEGER NOT NULL REFERENCES doctors(id),
        start TEXT NOT NULL,
        end TEXT NOT NULL,
        status INTEGER NOT NULL,
        note TEXT NOT NULL,
        CHECK (end > start)))",
    R"(CREATE TABLE IF NOT EXISTS examinations (
        id INTEGER PRIMARY KEY,
        appointment_id INTEGER NOT NULL UNIQUE REFERENCES appointments(id),
        patient_id INTEGER NOT NULL REFERENCES patients(id),
        doctor_id INTEGER NOT NULL REFERENCES doctors(id),
        date TEXT NOT NULL,
        complaint TEXT NOT NULL,
        findings TEXT NOT NULL,
        diagnosis_code TEXT NOT NULL,
        diagnosis_name TEXT NOT NULL,
        notes TEXT NOT NULL))",
    R"(CREATE TABLE IF NOT EXISTS prescription_items (
        id INTEGER PRIMARY KEY,
        examination_id INTEGER NOT NULL REFERENCES examinations(id) ON DELETE CASCADE,
        position INTEGER NOT NULL,
        medicine TEXT NOT NULL,
        dose TEXT NOT NULL,
        usage TEXT NOT NULL,
        days INTEGER NOT NULL))",
    "CREATE INDEX IF NOT EXISTS appointments_doctor ON appointments(doctor_id, start)",
    "CREATE INDEX IF NOT EXISTS appointments_patient ON appointments(patient_id, start)",
    "CREATE INDEX IF NOT EXISTS examinations_patient ON examinations(patient_id, date)",
};

} // namespace

Database::Database()
    : m_name(QUuid::createUuid().toString()) // her Database nesnesinin kendi bağlantı adı olur
{
}

Database::~Database()
{
    QSqlDatabase::database(m_name, false).close();
    QSqlDatabase::removeDatabase(m_name);
}

bool Database::open(const QString &path)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_name);
    db.setDatabaseName(path);
    if (!db.open()) {
        m_lastError = db.lastError().text();
        return false;
    }
    QSqlQuery pragma(db);
    pragma.exec("PRAGMA foreign_keys = ON"); // SQLite'ta yabancı anahtar kontrolü varsayılan olarak kapalıdır
    return createSchema();
}

QSqlDatabase Database::connection() const
{
    return QSqlDatabase::database(m_name, false);
}

bool Database::createSchema()
{
    QSqlQuery query(connection());
    for (const char *statement : SCHEMA) {
        if (!query.exec(statement)) {
            m_lastError = query.lastError().text();
            return false;
        }
    }
    return true;
}
