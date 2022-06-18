cmake_minimum_required(VERSION 3.5)

project(Wachdienst-Manager LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(Qt6 COMPONENTS Sql REQUIRED)
find_package(Qt6 COMPONENTS Network REQUIRED)

add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x060001)

set(PROJECT_SOURCES
    resources.qrc
    src/main.cpp
    src/startupwindow.cpp
    src/startupwindow.ui
    src/aboutdialog.cpp
    src/aboutdialog.ui
    src/settingsdialog.cpp
    src/settingsdialog.ui
    src/personneldatabasedialog.cpp
    src/personneldatabasedialog.ui
    src/personneleditordialog.cpp
    src/personneleditordialog.ui
    src/newreportdialog.cpp
    src/newreportdialog.ui
    src/reportwindow.cpp
    src/reportwindow.ui
    src/updatereportpersonentrydialog.cpp
    src/updatereportpersonentrydialog.ui
    src/auxil.cpp
    src/databasecreator.cpp
    src/databasecache.cpp
    src/settingscache.cpp
    src/qualificationchecker.cpp
    src/pdfexporter.cpp
    src/person.cpp
    src/boatdrive.cpp
    src/report.cpp
    src/boatlog.cpp
)

if (WIN32)
    set(PROJECT_SOURCES
        application-icon.rc
        ${PROJECT_SOURCES})
endif()

qt_add_executable(Wachdienst-Manager WIN32 ${PROJECT_SOURCES})

target_link_libraries(Wachdienst-Manager PRIVATE Qt6::Widgets Qt6::Sql Qt6::Network)
