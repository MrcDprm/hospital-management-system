# Hospital Manager

**English** | [Türkçe](README.tr.md)

A desktop management app for a clinic or small hospital, written in C++ and Qt: patients, doctors and departments, conflict-free appointments, examinations, prescriptions and reports, with role-based sign-in. Data is stored locally in SQLite.

> 🚧 Work in progress. This README is the project plan and will be completed at v1.0.0.

## Plan

### MVP
- **Sign-in with roles:** administrator, receptionist and doctor. Passwords hashed with **Argon2id** (libsodium), strong password rules, growing wait after wrong attempts. Permissions are checked in the data layer, not only in the UI. The first start creates the administrator account.
- **Patients:** Turkish ID number with check digits, contact details, date of birth, gender, blood group, allergies and chronic conditions. Search and CSV export.
- **Departments and doctors:** departments (cardiology, paediatrics…), doctors with title, department, working days and hours, and appointment length.
- **Appointments:** pick a department and doctor, see only free slots; double bookings are impossible. Statuses: booked, checked in, examined, no-show, cancelled.
- **Today screen:** today's appointments by doctor with quick actions (check in, start examination, no-show).
- **Examinations:** complaint, findings, diagnosis (ICD-10 code and name) and notes, written by the doctor.
- **Desktop app:** dark and light theme, Turkish and English, icon, version, About window, data in the user's folder, Windows installer.
- **Tests:** slot calculation, overlap rules, ID validation, permissions and password rules, with Qt Test.

### Extras
- **Prescriptions:** medicines with dose and usage, printed as an A4 PDF.
- **Weekly calendar:** each doctor's week with booked and free slots; click to book.
- **Patient history:** all examinations, diagnoses and prescriptions of a patient on one timeline.
- **Reports:** appointments per department, no-show rate, doctor utilisation, with charts and CSV export.
- **Sample data:** offered once on first start (departments, doctors, patients and a few months of appointments).

### Future Plans
- SMS or e-mail appointment reminders.
- Lab results and file attachments.
- Billing and insurance.
- Multiple branches.

## Tech Stack
- C++, Qt (Widgets, Sql, Test)
- SQLite, libsodium (Argon2id)
- CMake, MinGW-w64 (MSYS2 UCRT64)
- windeployqt, Inno Setup
