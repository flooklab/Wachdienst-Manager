/*
/////////////////////////////////////////////////////////////////////////////////////////
//
//  This file is part of Wachdienst-Manager, a program to manage DLRG watch duty reports.
//  Copyright (C) 2021–2022 M. Frohne
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
#include "pdfexporter.h"
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
#include <QFileInfo>
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

    QStringList standardPaths = QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation);
    if (standardPaths.size() == 0)
    {
        std::cerr<<"ERROR: Could not obtain standard configuration location!"<<std::endl;
        QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Abfragen des Standard-Konfigurationspfades!").exec();
        return EXIT_FAILURE;
    }

    QDir configDir(standardPaths[0]);
    if (!configDir.cd("Wachdienst-Manager"))
    {
        if (!configDir.mkpath("Wachdienst-Manager"))
        {
            std::cerr<<"ERROR: Could not create configuration directory!"<<std::endl;
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Erstellen des Konfigurations-Verzeichnisses!").exec();
            return EXIT_FAILURE;
        }
        if (!configDir.cd("Wachdienst-Manager"))
        {
            std::cerr<<"ERROR: Could not change into the configuration directory!"<<std::endl;
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Wechseln in das Konfigurations-Verzeichnis!").exec();
            return EXIT_FAILURE;
        }
    }

    //If config directory contains a file 'dbPath.conf', read alternative config directory from the file and use that in the following
    if (QFileInfo::exists(configDir.filePath("dbPath.conf")))
    {
        QFile dbPathConfFile(configDir.filePath("dbPath.conf"));

        if (!dbPathConfFile.open(QIODevice::ReadOnly))
        {
            dbPathConfFile.close();
            std::cerr<<"ERROR: Could not read alternative configuration directory path from \"dbPath.conf\"!"<<std::endl;
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Lesen des alternativen Konfigurations-Verzeichnis-Pfads!").exec();
            return EXIT_FAILURE;
        }

        QString alternativeDbPath = dbPathConfFile.readLine().trimmed();

        dbPathConfFile.close();

        if (alternativeDbPath != "")
        {
            QDir alternativeDbDir(alternativeDbPath);

            if (!alternativeDbDir.exists())
            {
                if (!QDir().mkpath(alternativeDbDir.absolutePath()))
                {
                    std::cerr<<"ERROR: Could not create alternative configuration directory!"<<std::endl;
                    QMessageBox(QMessageBox::Critical, "Fehler",
                                                       "Fehler beim Erstellen des alternativen Konfigurations-Verzeichnisses!").exec();
                    return EXIT_FAILURE;
                }
                else
                {
                    QMessageBox(QMessageBox::Information, "Verzeichnis angelegt",
                                                          "Ein alternatives Konfigurations-Verzeichnis wurde angelegt!").exec();
                }
            }

            if (!configDir.cd(alternativeDbDir.absolutePath()))
            {
                std::cerr<<"ERROR: Could not change into the alternative configuration directory!"<<std::endl;
                QMessageBox(QMessageBox::Critical, "Fehler",
                                                   "Fehler beim Wechseln in das alternative Konfigurations-Verzeichnis!").exec();
                return EXIT_FAILURE;
            }
        }
    }

    //Open general configuration and personnel databases from configuration directory

    QString confDbFileName = configDir.filePath("configuration.sqlite3");
    bool confDbExists = QFileInfo::exists(confDbFileName);

    QString personnelDbFileName = configDir.filePath("personnel.sqlite3");
    bool persDbExists = QFileInfo::exists(personnelDbFileName);

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
        return EXIT_FAILURE;
    }
    if (!personnelDatabase.open())
    {
        std::cerr<<"ERROR: Could not open personnel database!"<<std::endl;
        QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Öffnen der Personal-Datenbank!").exec();
        return EXIT_FAILURE;
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
            return EXIT_FAILURE;
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
            return EXIT_FAILURE;
        }
    }

    //Check if database versions are supported
    if (!DatabaseCreator::checkConfigVersion() || !DatabaseCreator::checkPersonnelVersion())
    {
        std::cerr<<"ERROR: Unsupported database version!"<<std::endl;
        QMessageBox(QMessageBox::Critical, "Fehler", "Nicht unterstützte Datenbank-Version!").exec();
        return EXIT_FAILURE;
    }

    //Cache database entries
    if (!DatabaseCache::populate(lockFilePtr) || !SettingsCache::populate(lockFilePtr))
    {
        std::cerr<<"ERROR: Could not cache database entries!"<<std::endl;
        QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Füllen des Datenbank-Caches!").exec();
        return EXIT_FAILURE;
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

    if (argc == 2)
    {
        const QString& cmdArg1 = argv[1];

        if (cmdArg1.startsWith('-'))    //Expecting one option
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
                return EXIT_FAILURE;
            }
        }
        else    //Expecting one file name
        {
            //Assume file exists and is saved report; open the report and show the report window immediately
            if (!startupWindow.openReport(cmdArg1))
                return EXIT_FAILURE;
        }
    }
    else if (argc > 2)   //Expecting an option plus a number of file names
    {
        const QString& cmdArg1 = argv[1];

        if (cmdArg1 != "-E" && cmdArg1 != "-F")
        {
            std::cerr<<"ERROR: Too many or invalid command line arguments!"<<std::endl;
            QMessageBox(QMessageBox::Critical, "Fehler", "Zu viele oder ungültige Kommandozeilenargumente!").exec();
            return EXIT_FAILURE;
        }

        QStringList fileNames(argv+2, argv+argc);

        if (cmdArg1 == "-E")    //Automatically iterate file list and load and export each report to PDF (replacing extension with .pdf)
        {
            std::cout<<"Load and export each report from the following file list to PDF:\n\n";
            for (const QString& tFileName : fileNames)
                std::cout<<tFileName.toStdString()<<"\n";

            std::cout<<"\nUsing the same file names with extensions being replaced by \".pdf\".\n";
            std::cout<<"\nExported PDF files will be overwritten without asking! Continue? [y/N]\n";

            char inputChar = 'n';
            std::cin>>inputChar;

            std::cout<<std::endl;

            if (inputChar != 'y' && inputChar != 'Y')
                return EXIT_SUCCESS;

            Report report;

            for (const QString& tFileName : fileNames)
            {
                std::cout<<"\nLoading report from file \""<<tFileName.toStdString()<<"\"..."<<std::endl;

                if (!report.open(tFileName))
                    return EXIT_FAILURE;

                QFileInfo fileInfo(tFileName);
                QString pdfFileName = fileInfo.path() + "/" + fileInfo.completeBaseName() + ".pdf";

                std::cout<<"Exporting report to \""<<pdfFileName.toStdString()<<"\"..."<<std::endl;

                if (!PDFExporter::exportPDF(report, pdfFileName))
                    return EXIT_FAILURE;
            }

            std::cout<<"\nFinished exporting all reports."<<std::endl;
        }
        else if (cmdArg1 == "-F")   //Iteratively fix all carryovers by loading first report from file list, applying its carryovers to
        {                           //second report and saving second report; then applying its carryovers to third report and so forth

            std::cout<<"Fix all reports' carryovers by iterating over the following file list:\n\n";
            for (const QString& tFileName : fileNames)
                std::cout<<tFileName.toStdString()<<"\n";

            if (fileNames.size() < 2)
            {
                std::cerr<<"WARNING: Nothing to be done!"<<std::endl;
                return EXIT_SUCCESS;
            }

            std::cout<<"\nFirst file remains unmodified. Fixed reports will be saved under their old file names.\n";
            std::cout<<"\nFiles will be overwritten without asking! Continue? [y/N]\n";

            char inputChar = 'n';
            std::cin>>inputChar;

            std::cout<<std::endl;

            if (inputChar != 'y' && inputChar != 'Y')
                return EXIT_SUCCESS;

            Report firstReport, secondReport;

            std::cout<<"Loading report from file \""<<fileNames[0].toStdString()<<"\"..."<<std::endl;

            if (!secondReport.open(fileNames[0]))
                return EXIT_FAILURE;

            for (int i = 1; i < fileNames.size(); ++i)
            {
                firstReport = secondReport;

                std::cout<<"\nLoading report from file \""<<fileNames[i].toStdString()<<"\"..."<<std::endl;

                if (!secondReport.open(fileNames[i]))
                    return EXIT_FAILURE;

                std::cout<<"Applying carryovers from report \""<<fileNames[i-1].toStdString()<<"\"..."<<std::endl;

                secondReport.loadCarryovers(firstReport);

                std::cout<<"Saving report \""<<fileNames[i].toStdString()<<"\"..."<<std::endl;

                if (!secondReport.save(secondReport.getFileName()))
                    return EXIT_FAILURE;
            }

            std::cout<<"\nFinished fixing all reports' carryovers."<<std::endl;
        }

        return EXIT_SUCCESS;
    }
    else
    {
        //No special action requested, simply show startup window
        startupWindow.show();
    }

    return a.exec();
}
