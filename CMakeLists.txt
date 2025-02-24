#[[
#########################################################################################
##
##  This file is part of Wachdienst-Manager, a program to manage DLRG watch duty reports.
##  Copyright (C) 2021–2025 M. Frohne
##
##  Wachdienst-Manager is free software: you can redistribute it and/or modify
##  it under the terms of the GNU Affero General Public License as published
##  by the Free Software Foundation, either version 3 of the License,
##  or (at your option) any later version.
##
##  Wachdienst-Manager is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
##  See the GNU Affero General Public License for more details.
##
##  You should have received a copy of the GNU Affero General Public License
##  along with Wachdienst-Manager. If not, see <https://www.gnu.org/licenses/>.
##
#########################################################################################
#]]

cmake_minimum_required(VERSION 3.18)

if(NOT DEFINED CACHE{CMAKE_BUILD_TYPE})
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

#Tweak CMake defaults for compiler flags

if(NOT MSVC)
    set(DEFAULT_FLAGS "")
    set(DEFAULT_FLAGS_DEBUG "-g -Og")
    set(DEFAULT_FLAGS_MINSIZEREL "-Os -DNDEBUG")
    set(DEFAULT_FLAGS_RELEASE "-O2 -DNDEBUG")
    set(DEFAULT_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
else()
    #Those MSVC flags are probably not tested
    set(DEFAULT_FLAGS_DEBUG "/MDd /Zi /Ob0 /Od /RTC1")
    set(DEFAULT_FLAGS_MINSIZEREL "/MD /O1 /Ob1 /DNDEBUG")
    set(DEFAULT_FLAGS_RELEASE "/MD /O2 /Ob2 /DNDEBUG")
    set(DEFAULT_FLAGS_RELWITHDEBINFO "/MD /Zi /O2 /Ob1 /DNDEBUG")
endif()

if(NOT DEFINED CACHE{CMAKE_CXX_FLAGS})
    if(NOT MSVC)
        set(CMAKE_CXX_FLAGS "${DEFAULT_FLAGS}" CACHE STRING "Flags used by the CXX compiler during all build types." FORCE)
    endif()
endif()
if(NOT DEFINED CACHE{CMAKE_CXX_FLAGS_DEBUG})
    set(CMAKE_CXX_FLAGS_DEBUG "${DEFAULT_FLAGS_DEBUG}" CACHE STRING "Flags used by the CXX compiler during DEBUG builds." FORCE)
endif()
if(NOT DEFINED CACHE{CMAKE_CXX_FLAGS_MINSIZEREL})
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${DEFAULT_FLAGS_MINSIZEREL}" CACHE STRING "Flags used by the CXX compiler during MINSIZEREL builds." FORCE)
endif()
if(NOT DEFINED CACHE{CMAKE_CXX_FLAGS_RELEASE})
    set(CMAKE_CXX_FLAGS_RELEASE "${DEFAULT_FLAGS_RELEASE}" CACHE STRING "Flags used by the CXX compiler during RELEASE builds." FORCE)
endif()
if(NOT DEFINED CACHE{CMAKE_CXX_FLAGS_RELWITHDEBINFO})
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${DEFAULT_FLAGS_RELWITHDEBINFO}" CACHE STRING "Flags used by the CXX compiler during RELWITHDEBINFO builds." FORCE)
endif()

#

project(Wachdienst-Manager LANGUAGES CXX)

include(CheckPIESupported)
check_pie_supported()

include(CheckIPOSupported)
check_ipo_supported()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#

if(NOT MSVC)
    if(MINGW)
        string(CONCAT TMP_CXX_FLAGS "-Wall;-Wextra;-Wshadow;-pedantic-errors;"
                                    "-pipe;-march=x86-64;-mtune=generic;-Wp,-D_FORTIFY_SOURCE=3;"
                                    "-fstack-clash-protection;-fcf-protection=full")
    else()
        string(CONCAT TMP_CXX_FLAGS "-Wall;-Wextra;-Wshadow;-pedantic-errors;"
                                    "-pipe;-march=x86-64;-mtune=generic;-Wp,-D_FORTIFY_SOURCE=3;-Wp,-D_GLIBCXX_ASSERTIONS;"
                                    "-fno-plt;-fstack-protector-strong;-fstack-clash-protection;-fcf-protection=full")
    endif()
    set(WDMGR_CXX_FLAGS "${TMP_CXX_FLAGS}" CACHE STRING "Additional compiler flags.")
    if(MINGW)
        set(WDMGR_LINK_FLAGS "-Wl,--dynamicbase;-Wl,--nxcompat;-Wl,--sort-common;-Wl,--as-needed"
            CACHE STRING "Additional linker flags.")
    else()
        set(WDMGR_LINK_FLAGS "-Wl,--sort-common;-Wl,--as-needed;-Wl,-z,defs;-Wl,-z,relro;-Wl,-z,now;-Wl,-z,pack-relative-relocs"
            CACHE STRING "Additional linker flags.")
    endif()
    set(WDMGR_LINK_FLAGS_DEBUG "" CACHE STRING "Additional linker flags to add for DEBUG builds.")
    set(WDMGR_LINK_FLAGS_RELEASE "-Wl,-O1" CACHE STRING "Additional linker flags to add for non-DEBUG builds.")
else()
    #Those MSVC flags are probably not tested
    string(CONCAT TMP_CXX_FLAGS "/W4;/permissive-;"
                                "/GS;/guard:cf;/guard:ehcont")
    set(WDMGR_CXX_FLAGS "${TMP_CXX_FLAGS}" CACHE STRING "Additional compiler flags.")
    set(WDMGR_LINK_FLAGS "/CETCOMPAT;/DYNAMICBASE;/OPT" CACHE STRING "Additional linker flags.")
    set(WDMGR_LINK_FLAGS_DEBUG "/DEBUG" CACHE STRING "Additional linker flags to add for DEBUG builds.")
    set(WDMGR_LINK_FLAGS_RELEASE "" CACHE STRING "Additional linker flags to add for non-DEBUG builds.")
endif()

if(NOT MSVC)
    set(WDMGR_ENABLE_GLIBCXX_DEBUG_MODE OFF CACHE BOOL "Add compiler flags to enable the debug mode of libstdc++.")
endif()

#

find_package(Qt6 REQUIRED COMPONENTS Network Sql Widgets)

#

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

if(WIN32)
    list(APPEND PROJECT_SOURCES application-icon.rc)
endif()

#

add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:${WDMGR_CXX_FLAGS}>")

if((NOT MSVC) AND WDMGR_ENABLE_GLIBCXX_DEBUG_MODE)
    add_compile_options(-Wp,-D_GLIBCXX_DEBUG -Wp,-D_GLIBCXX_DEBUG_PEDANTIC)
endif()

add_link_options("$<$<LINK_LANGUAGE:CXX>:${WDMGR_LINK_FLAGS}>")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_link_options("$<$<LINK_LANGUAGE:CXX>:${WDMGR_LINK_FLAGS_DEBUG}>")
else()
    add_link_options("$<$<LINK_LANGUAGE:CXX>:${WDMGR_LINK_FLAGS_RELEASE}>")
endif()

#

qt_add_executable(Wachdienst-Manager WIN32 ${PROJECT_SOURCES})

set_target_properties(Wachdienst-Manager PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(Wachdienst-Manager PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)
else()
    set_target_properties(Wachdienst-Manager PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
endif()

target_compile_definitions(Wachdienst-Manager PRIVATE QT_DISABLE_DEPRECATED_UP_TO=0x060600)
target_compile_definitions(Wachdienst-Manager PRIVATE QT_WARN_DEPRECATED_UP_TO=0x060900)

target_link_libraries(Wachdienst-Manager PRIVATE Qt6::Widgets Qt6::Sql Qt6::Network)
