cmake_minimum_required(VERSION 3.21)
project(HospitalManager VERSION 0.1.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Sql Widgets Test)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)
qt_standard_project_setup()

# İş kuralları ve veri katmanı: arayüzden bağımsız, testler de bunu kullanır
qt_add_library(hospital_core STATIC
    src/core/Models.h
    src/core/Rules.cpp
    src/core/Schedule.cpp
    src/core/Access.cpp
    src/core/Passwords.cpp
    src/data/Database.cpp
    src/data/SqlUtil.h
    src/data/UserRepository.cpp
    src/data/StaffRepository.cpp
    src/data/PatientRepository.cpp
    src/data/AppointmentRepository.cpp
    src/data/ExaminationRepository.cpp
    src/data/ReportRepository.cpp
    src/services/DemoData.cpp
)
target_include_directories(hospital_core PUBLIC src)
target_link_libraries(hospital_core PUBLIC Qt6::Core Qt6::Sql PRIVATE PkgConfig::SODIUM)

# Sürüm numarası ve ikon .exe dosyasına buradan gömülür (Özellikler > Ayrıntılar)
configure_file(resources/app.rc.in ${CMAKE_BINARY_DIR}/app.rc @ONLY)

qt_add_executable(HospitalManager WIN32
    src/main.cpp
    src/app/I18n.cpp
    src/app/Settings.cpp
    src/app/Theme.cpp
    src/services/AutoLock.cpp
    src/services/Catalog.cpp
    src/services/CsvExport.cpp
    src/services/PrescriptionPrinter.cpp
    src/ui/UiHelpers.h
    src/ui/PasswordField.cpp
    src/ui/SetupPage.cpp
    src/ui/LoginPage.cpp
    src/ui/PatientDialog.cpp
    src/ui/BookingDialog.cpp
    src/ui/ExaminationDialog.cpp
    src/ui/PatientHistoryDialog.cpp
    src/ui/AppointmentActions.cpp
    src/ui/AppointmentsPage.cpp
    src/ui/CalendarPage.cpp
    src/ui/PatientsPage.cpp
    src/ui/StaffDialogs.cpp
    src/ui/StaffPage.cpp
    src/ui/UserDialogs.cpp
    src/ui/UsersPage.cpp
    src/ui/ReportsPage.cpp
    src/ui/AboutDialog.cpp
    src/ui/MainWindow.cpp
    ${CMAKE_BINARY_DIR}/app.rc
)
qt_add_resources(HospitalManager "app_resources" PREFIX "/" BASE resources
    FILES resources/icon.png resources/icd10.tsv)
target_link_libraries(HospitalManager PRIVATE hospital_core Qt6::Widgets)
target_compile_definitions(HospitalManager PRIVATE APP_VERSION="${PROJECT_VERSION}")

enable_testing()
qt_add_executable(test_core tests/test_core.cpp)
target_link_libraries(test_core PRIVATE hospital_core Qt6::Test)
add_test(NAME test_core COMMAND test_core)
qt_add_executable(test_data tests/test_data.cpp)
target_link_libraries(test_data PRIVATE hospital_core Qt6::Test)
add_test(NAME test_data COMMAND test_data)
