cmake_minimum_required(VERSION 3.5)

project(Wachdienst-Manager LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow")

find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(Qt6 COMPONENTS Sql REQUIRED)
find_package(Qt6 COMPONENTS Network REQUIRED)

add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x060001)

set(PROJECT_SOURCES
    resources.qrc
    src/main.cpp
    src/startupwindow.h
    src/startupwindow.cpp
    src/startupwindow.ui
    src/aboutdialog.h
    src/aboutdialog.cpp
    src/aboutdialog.ui
    src/settingsdialog.h
    src/settingsdialog.cpp
    src/settingsdialog.ui
    src/personneldatabasedialog.h
    src/personneldatabasedialog.cpp
    src/personneldatabasedialog.ui
    src/personneleditordialog.h
    src/personneleditordialog.cpp
    src/personneleditordialog.ui
    src/newreportdialog.h
    src/newreportdialog.cpp
    src/newreportdialog.ui
    src/reportwindow.h
    src/reportwindow.cpp
    src/reportwindow.ui
    src/updatereportpersonentrydialog.h
    src/updatereportpersonentrydialog.cpp
    src/updatereportpersonentrydialog.ui
    src/auxil.h
    src/auxil.cpp
    src/databasecreator.h
    src/databasecreator.cpp
    src/databasecache.h
    src/databasecache.cpp
    src/settingscache.h
    src/settingscache.cpp
    src/qualificationchecker.h
    src/qualificationchecker.cpp
    src/pdfexporter.h
    src/pdfexporter.cpp
    src/person.h
    src/person.cpp
    src/boatdrive.h
    src/boatdrive.cpp
    src/report.h
    src/report.cpp
    src/boatlog.h
    src/boatlog.cpp
    src/singleinstancesynchronizer.h
    src/singleinstancesynchronizer.cpp
    src/version.h
)

if (WIN32)
    set(PROJECT_SOURCES
        application-icon.rc
        ${PROJECT_SOURCES})
endif()

qt_add_executable(Wachdienst-Manager WIN32 ${PROJECT_SOURCES})

target_link_libraries(Wachdienst-Manager PRIVATE Qt6::Widgets Qt6::Sql Qt6::Network)
