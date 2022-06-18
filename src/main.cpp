/*
/////////////////////////////////////////////////////////////////////////////////////////
//
//  This file is part of Wachdienst-Manager, a program to manage DLRG watch duty reports.
//  Copyright (C) 2021 M. Frohne
//
//  Wachdienst-Manager is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License as published
//  by the Free Software Foundation, either version 3 of the License,
//  or (at your option) any later version.
//
//  Wachdienst-Manager is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//  See the GNU Affero General Public License for more details.
//
//  You should have received a copy of the GNU Affero General Public License
//  along with Wachdienst-Manager. If not, see <https://www.gnu.org/licenses/>.
//
/////////////////////////////////////////////////////////////////////////////////////////
*/

#include "databasecreator.h"
#include "databasecache.h"
#include "settingscache.h"
#include "startupwindow.h"
#include "report.h"

#include <iostream>
#include <memory>

#include <QApplication>
#include <QStyleFactory>
#include <QTranslator>
#include <QIcon>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QStandardPaths>
#include <QtSql/QSqlDatabase>
#include <QLockFile>

int main(int argc, char *argv[])
{
    //Create Qt application
    QApplication a(argc, argv);

    //Set nicer "Fusion" style
    a.setStyle(QStyleFactory::create("Fusion"));

    //Display application icon in title bar
    a.setWindowIcon(QIcon(":/resources/icons/application-icon_512x512.png"));

    //Load standard translations for dialog buttons etc.
    QTranslator translator;
    QDir translationsDir(a.applicationDirPath());
    if (!translationsDir.exists() || !translationsDir.cd("translations") ||
        !translator.load("qt_" + QLocale::system().name(), translationsDir.absolutePath()) ||
        !a.installTranslator(&translator))
    {
        std::cerr<<"WARNING: Could not load translations!"<<std::endl;
    }

    //Create application configuration directory at OS specific path if it does not exist

    QStringList standardPaths = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
    if (standardPaths.size() == 0)
    {
        std::cerr<<"ERROR: Could not obtain standard configuration location!"<<std::endl;
        QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Abfragen des Standard-Konfigurationspfades!").exec();
        exit(EXIT_FAILURE);
    }

    QDir configDir(standardPaths[0]);
    if (!configDir.cd("Wachdienst-Manager"))
    {
        if (!configDir.mkpath("Wachdienst-Manager"))
        {
            std::cerr<<"ERROR: Could not create configuration directory!"<<std::endl;
            QMessageBox(QMessageBox::Critical, "Fehler",
                                               "Fehler beim Erstellen des Konfigurations-Verzeichnisses!").exec();
            exit(EXIT_FAILURE);
        }
        if (!configDir.cd("Wachdienst-Manager"))
        {
            std::cerr<<"ERROR: Could not change into the configuration directory!"<<std::endl;
            QMessageBox(QMessageBox::Critical, "Fehler",
                                               "Fehler beim Wechseln in das Konfigurations-Verzeichnis!").exec();
            exit(EXIT_FAILURE);
        }
    }

    //Open general configuration and personnel databases from configuration directory

    QString confDbFileName = configDir.filePath("configuration.sqlite3");
    bool confDbExists = QFile(confDbFileName).exists();

    QString personnelDbFileName = configDir.filePath("personnel.sqlite3");
    bool persDbExists = QFile(personnelDbFileName).exists();

    QSqlDatabase confDatabase = QSqlDatabase::addDatabase("QSQLITE", "configDb");
    confDatabase.setDatabaseName(confDbFileName);

    QSqlDatabase personnelDatabase = QSqlDatabase::addDatabase("QSQLITE", "personnelDb");
    personnelDatabase.setDatabaseName(personnelDbFileName);

    //Note: this is just for "completeness" and not intended to be secure...
    confDatabase.setUserName("DLRG_conf");
    confDatabase.setPassword("password");

    //Note: this is just for "completeness" and not intended to be secure...
    personnelDatabase.setUserName("DLRG_pers");
    personnelDatabase.setPassword("password");

    if (!confDatabase.open())
    {
        std::cerr<<"ERROR: Could not open configuration database!"<<std::endl;
        QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Öffnen der Konfigurations-Datenbank!").exec();
        exit(EXIT_FAILURE);
    }
    if (!personnelDatabase.open())
    {
        std::cerr<<"ERROR: Could not open personnel database!"<<std::endl;
        QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Öffnen der Personal-Datenbank!").exec();
        exit(EXIT_FAILURE);
    }

    //Acquire database lock file (to avoid writing to database from multiple application instances)
    QString lockFileName = configDir.filePath("db.lock");
    std::shared_ptr<QLockFile> lockFilePtr = std::make_shared<QLockFile>(lockFileName);
    lockFilePtr->setStaleLockTime(0);
    lockFilePtr->tryLock(1000);

    //Setup fresh databases if they do not exist

    if (!confDbExists)
    {
        if (DatabaseCreator::createConfigDatabase())
        {
            QMessageBox(QMessageBox::Information, "Datenbank angelegt",
                                                  "Eine neue Konfigurations-Datenbank wurde angelegt!").exec();
        }
        else
        {
            std::cerr<<"ERROR: Could not create configuration database!"<<std::endl;
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Anlegen der Konfigurations-Datenbank!").exec();
            exit(EXIT_FAILURE);
        }
    }

    if (!persDbExists)
    {
        if (DatabaseCreator::createPersonnelDatabase())
        {
            QMessageBox(QMessageBox::Information, "Datenbank angelegt",
                                                  "Eine neue Personal-Datenbank wurde angelegt!").exec();
        }
        else
        {
            std::cerr<<"ERROR: Could not create personnel database!"<<std::endl;
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Anlegen der Personal-Datenbank!").exec();
            exit(EXIT_FAILURE);
        }
    }

    //Check if database versions are supported
    if (!DatabaseCreator::checkConfigVersion() || !DatabaseCreator::checkPersonnelVersion())
    {
        std::cerr<<"ERROR: Unsupported database version!"<<std::endl;
        QMessageBox(QMessageBox::Critical, "Fehler", "Nicht unterstützte Datenbank-Version!").exec();
        exit(EXIT_FAILURE);
    }

    //Cache database entries
    if (!DatabaseCache::populate(lockFilePtr) || !SettingsCache::populate(lockFilePtr))
    {
        std::cerr<<"ERROR: Could not cache database entries!"<<std::endl;
        QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Füllen des Datenbank-Caches!").exec();
        exit(EXIT_FAILURE);
    }

    //Start file dialogs in configured default directory
    QString defaultFileDir = SettingsCache::getStrSetting("app_default_fileDialogDir");
    if (defaultFileDir != "" && QDir().cd(defaultFileDir))
    {
        QFileDialog fileDialog;
        fileDialog.setDirectory(defaultFileDir);
    }

    //Create main window
    StartupWindow startupWindow;

    //Start application in different ways depending on command line arguments

    if (argc > 2)
    {
        std::cerr<<"ERROR: Too many command line arguments!"<<std::endl;
        QMessageBox(QMessageBox::Critical, "Fehler", "Zu viele Kommandozeilenargumente!").exec();
        exit(EXIT_FAILURE);
    }
    else if (argc == 2)
    {
        QString cmdArg1 = argv[1];

        if (cmdArg1.startsWith('-'))    //Expect a parameter
        {
            if (cmdArg1 == "-n")
            {
                //Show the new report assistent dialog immediately
                startupWindow.newReport();
            }
            else
            {
                std::cerr<<"ERROR: Invalid command line argument!"<<std::endl;
                QMessageBox(QMessageBox::Critical, "Fehler", "Ungültiges Kommandozeilenargument!").exec();
                exit(EXIT_FAILURE);
            }
        }
        else    //Expect a file name
        {
            //Check first, if the argument is a file name and if this file exists
            if (!QFile(cmdArg1).exists())
            {
                std::cerr<<"ERROR: File does not exist!"<<std::endl;
                QMessageBox(QMessageBox::Critical, "Fehler", "Zu öffnende Datei existiert nicht!").exec();
                exit(EXIT_FAILURE);
            }

            //Assume file is saved report
            //Open the report and show the report window immediately
            startupWindow.openReport(cmdArg1);
        }
    }
    else
    {
        //No special action requested, simply show startup window
        startupWindow.show();
    }

    return a.exec();
}
