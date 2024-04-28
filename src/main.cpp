/*
/////////////////////////////////////////////////////////////////////////////////////////
//
//  This file is part of Wachdienst-Manager, a program to manage DLRG watch duty reports.
//  Copyright (C) 2021–2024 M. Frohne
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

#include "databasecache.h"
#include "databasecreator.h"
#include "pdfexporter.h"
#include "report.h"
#include "settingscache.h"
#include "singleinstancesynchronizer.h"
#include "startupwindow.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QLockFile>
#include <QMessageBox>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QStyleFactory>
#include <QTranslator>
#include <QtSql/QSqlDatabase>

#include <atomic>
#include <iostream>
#include <memory>
#include <thread>

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

    QDir personnelDir = configDir;

    //If config directory contains a file 'dbPath.conf', read alternative config directory from the file and use that in the following;
    //if the file contains two paths, use the second path as distinct directory for the personnel database

    QString dbPathConfFileName = configDir.filePath("dbPath.conf");
    const bool dbPathConfFileExists = QFileInfo::exists(dbPathConfFileName);

    if (dbPathConfFileExists)
    {
        QFile dbPathConfFile(dbPathConfFileName);

        if (!dbPathConfFile.open(QIODevice::ReadOnly))
        {
            dbPathConfFile.close();
            std::cerr<<"ERROR: Could not read alternative configuration directory path from \"dbPath.conf\"!"<<std::endl;
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Lesen des alternativen Datenbank-Verzeichnis-Pfads!").exec();
            return EXIT_FAILURE;
        }

        QString alternativeDbPath = dbPathConfFile.readLine().trimmed();
        QString alternativeDbPath2 = dbPathConfFile.readLine().trimmed();

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
                                                       "Fehler beim Erstellen des alternativen Datenbank-Verzeichnisses!").exec();
                    return EXIT_FAILURE;
                }
                else
                {
                    QMessageBox(QMessageBox::Information, "Verzeichnis angelegt",
                                                          "Ein alternatives Datenbank-Verzeichnis wurde angelegt!").exec();
                }
            }

            if (!configDir.cd(alternativeDbDir.absolutePath()))
            {
                std::cerr<<"ERROR: Could not change into the alternative configuration directory!"<<std::endl;
                QMessageBox(QMessageBox::Critical, "Fehler",
                                                   "Fehler beim Wechseln in das alternative Datenbank-Verzeichnis!").exec();
                return EXIT_FAILURE;
            }
        }

        if (alternativeDbPath2 != "")
        {
            QDir alternativeDbDir2(alternativeDbPath2);

            if (!alternativeDbDir2.exists())
            {
                if (!QDir().mkpath(alternativeDbDir2.absolutePath()))
                {
                    std::cerr<<"ERROR: Could not create alternative personnel directory!"<<std::endl;
                    QMessageBox(QMessageBox::Critical, "Fehler",
                                "Fehler beim Erstellen des alternativen Personal-Datenbank-Verzeichnisses!").exec();
                    return EXIT_FAILURE;
                }
                else
                {
                    QMessageBox(QMessageBox::Information, "Verzeichnis angelegt",
                                                          "Ein alternatives Personal-Datenbank-Verzeichnis wurde angelegt!").exec();
                }
            }

            if (!personnelDir.cd(alternativeDbDir2.absolutePath()))
            {
                std::cerr<<"ERROR: Could not change into the alternative personnel directory!"<<std::endl;
                QMessageBox(QMessageBox::Critical, "Fehler",
                                                   "Fehler beim Wechseln in das alternative Personal-Datenbank-Verzeichnis!").exec();
                return EXIT_FAILURE;
            }
        }
        else
        {
            personnelDir = configDir;
        }
    }

    //Check if database files exist

    QString confDbFileName = configDir.filePath("configuration.sqlite3");
    const bool confDbExists = QFileInfo::exists(confDbFileName);

    QString personnelDbFileName = personnelDir.filePath("personnel.sqlite3");
    const bool persDbExists = QFileInfo::exists(personnelDbFileName);

    //If neither of 'dbPath.conf' or database files exist assume first startup and ask for alternative config directory (for dbPath.conf)
    if (!dbPathConfFileExists && !confDbExists && !persDbExists)
    {
        QMessageBox msgBox(QMessageBox::Question, "Datenbank-Verzeichnis", "Soll ein vom Standard-Verzeichnis abweichendes "
                           "Verzeichnis für die Konfigurations- und Personal-Datenbanken verwendet werden?",
                           QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);

        if (msgBox.exec() == QMessageBox::Yes)
        {
            QString alternativeDbPath = QFileDialog::getExistingDirectory(nullptr, "Datenbank-Verzeichnis auswählen", "",
                                                                          QFileDialog::ShowDirsOnly);

            if (alternativeDbPath != "")
            {
                QFile dbPathConfFile(dbPathConfFileName);

                if (!dbPathConfFile.open(QIODevice::WriteOnly) || dbPathConfFile.write(alternativeDbPath.toUtf8()) == -1)
                {
                    dbPathConfFile.close();
                    std::cerr<<"ERROR: Could not write alternative configuration directory path to \"dbPath.conf\"!"<<std::endl;
                    QMessageBox(QMessageBox::Critical, "Fehler",
                                "Fehler beim Schreiben des alternativen Datenbank-Verzeichnis-Pfads!").exec();
                    return EXIT_FAILURE;
                }

                dbPathConfFile.close();

                QMessageBox(QMessageBox::Information, "Neustart", "Das Programm wird jetzt beendet und kann danach "
                            "neu gestartet werden! Es wird dann das alternative Datenbank-Verzeichnis \"" +
                            alternativeDbPath + "\" verwendet.").exec();

                return EXIT_SUCCESS;
            }
            else
            {
                std::cerr<<"ERROR: No valid path was chosen!"<<std::endl;
                QMessageBox(QMessageBox::Critical, "Fehler", "Kein gültiges Verzeichnis ausgewählt!").exec();
                return EXIT_FAILURE;
            }
        }
    }

    //Open general configuration and personnel databases from configuration directory

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

    //Use additional lock file for personnel database in case of different database directories; use the same lock file otherwise

    std::shared_ptr<QLockFile> lockFilePtr2 = lockFilePtr;

    if (personnelDir.absolutePath() != configDir.absolutePath())
    {
        QString lockFileName2 = personnelDir.filePath("db.lock");
        lockFilePtr2 = std::make_shared<QLockFile>(lockFileName2);
        lockFilePtr2->setStaleLockTime(0);
        lockFilePtr2->tryLock(1000);
    }

    //Set up fresh databases if they do not exist

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

    //Check if database versions are supported, upgrade them if necessary

    if (!DatabaseCreator::checkConfigVersion())
    {
        std::cerr<<"ERROR: Unsupported configuration database version!"<<std::endl;
        QMessageBox(QMessageBox::Critical, "Fehler", "Nicht unterstützte Konfigurations-Datenbank-Version!").exec();

        if (DatabaseCreator::checkConfigVersionOlder())
        {
            QMessageBox msgBox(QMessageBox::Question, "Datenbank-Upgrade", "Die Konfigurations-Datenbank-Version ist älter als die "
                               "aktuelle Version. Es kann daher versucht werden, die Datenbank in das aktuelle Format zu "
                               "konvertieren. Soll das Upgrade jetzt durchgeführt werden? (Backup empfohlen!)",
                               QMessageBox::Yes | QMessageBox::Abort);
            msgBox.setDefaultButton(QMessageBox::Abort);

            if (msgBox.exec() == QMessageBox::Yes)
            {
                if (DatabaseCreator::upgradeConfigDatabase())
                {
                    QMessageBox(QMessageBox::Information, "Upgrade erfolgreich",
                                "Die Konfigurations-Datenbank wurde erfolgreich in das aktuelle Format konvertiert!").exec();
                }
                else
                {
                    QMessageBox(QMessageBox::Critical, "Fehler",
                                                       "Das Upgrade der Konfigurations-Datenbank war nicht erfolgreich!").exec();
                    return EXIT_FAILURE;
                }
            }
            else
                return EXIT_FAILURE;
        }
        else
            return EXIT_FAILURE;
    }

    if (!DatabaseCreator::checkPersonnelVersion())
    {
        std::cerr<<"ERROR: Unsupported personnel database version!"<<std::endl;
        QMessageBox(QMessageBox::Critical, "Fehler", "Nicht unterstützte Personal-Datenbank-Version!").exec();

        if (DatabaseCreator::checkPersonnelVersionOlder())
        {
            QMessageBox msgBox(QMessageBox::Question, "Datenbank-Upgrade", "Die Personal-Datenbank-Version ist älter als die "
                               "aktuelle Version. Es kann daher versucht werden, die Datenbank in das aktuelle Format zu "
                               "konvertieren. Soll das Upgrade jetzt durchgeführt werden? (Backup empfohlen!)",
                               QMessageBox::Yes | QMessageBox::Abort);
            msgBox.setDefaultButton(QMessageBox::Abort);

            if (msgBox.exec() == QMessageBox::Yes)
            {
                if (DatabaseCreator::upgradePersonnelDatabase())
                {
                    QMessageBox(QMessageBox::Information, "Upgrade erfolgreich",
                                "Die Personal-Datenbank wurde erfolgreich in das aktuelle Format konvertiert!").exec();
                }
                else
                {
                    QMessageBox(QMessageBox::Critical, "Fehler", "Das Upgrade der Personal-Datenbank war nicht erfolgreich!").exec();
                    return EXIT_FAILURE;
                }
            }
            else
                return EXIT_FAILURE;
        }
        else
            return EXIT_FAILURE;
    }

    //Cache database entries
    if (!DatabaseCache::populate(lockFilePtr, lockFilePtr2) || !SettingsCache::populate(lockFilePtr, lockFilePtr2))
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

    //Determine whether to run in single instance mode and, if so, whether to proceed in "master" or "slave" mode

    bool singleInstance = SettingsCache::getBoolSetting("app_singleInstance");

    if (singleInstance && !SingleInstanceSynchronizer::init())
    {
        std::cerr<<"ERROR: Could not set up application instance synchronization! "
                 <<"Disabling single instance mode for this instance."<<std::endl;
        QMessageBox(QMessageBox::Critical, "Fehler", "Konnte keine Programm-Instanz-Synchronisation herstellen!\n"
                                                     "Einzel-Instanz-Modus wird für diese Instanz deaktiviert.").exec();

        singleInstance = false;
    }

    const bool singleInstanceMaster = singleInstance && SingleInstanceSynchronizer::isMaster();

    //Create main window
    StartupWindow startupWindow;

    //In single instance "master" mode run a listener thread to receive requests from "slave" instances

    std::thread masterListenerThread;
    std::atomic_bool stopListenerThread(false);

    if (singleInstance && singleInstanceMaster)
    {
        masterListenerThread = std::thread([&startupWindow, &stopListenerThread]() -> void
                                           {
                                               SingleInstanceSynchronizer::listen(startupWindow, stopListenerThread);
                                           });
    }

    //Start application in different ways depending on command line arguments; if running in single instance "slave" mode then
    //just forward corresponding requests to running "master" instance and exit (except in case of "-E" or "-F" options!)

    const QStringList cmdArgs = a.arguments();
    const int cmdArgsCount = cmdArgs.count();

    if (cmdArgsCount == 2)
    {
        const QString& cmdArg1 = cmdArgs[1];

        if (cmdArg1.startsWith('-'))    //Expecting one option
        {
            if (cmdArg1 == "-n")
            {
                //Show the new report assistent dialog immediately

                if (singleInstance && !singleInstanceMaster)
                    SingleInstanceSynchronizer::sendNewReport();
                else
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

            if (singleInstance && !singleInstanceMaster)
                SingleInstanceSynchronizer::sendOpenReport(cmdArg1);
            else
            {
                if (!startupWindow.openReport(cmdArg1))
                    return EXIT_FAILURE;
            }
        }
    }
    else if (cmdArgsCount > 2)   //Expecting either a list of file names or an option plus a number of file names
    {
        const QString& cmdArg1 = cmdArgs[1];

        if (cmdArg1.startsWith('-') && cmdArg1 != "-E" && cmdArg1 != "-F")
        {
            std::cerr<<"ERROR: Too many or invalid command line arguments!"<<std::endl;
            QMessageBox(QMessageBox::Critical, "Fehler", "Zu viele oder ungültige Kommandozeilenargumente!").exec();
            return EXIT_FAILURE;
        }

        QStringList fileNames(cmdArgs.begin()+2, cmdArgs.end());

        if (cmdArg1 == "-E")    //Automatically iterate file list and load and export each report to PDF (replacing extension with .pdf)
        {
            //As following instructions may take some time but the program will exit afterwards, try to detach instance already
            //now and stop listener thread if "master"; hence neither any "slave" requests will be processed by this instance
            //nor this instance, if "slave", will be accidentally recognized as "master" by new other instances
            if (singleInstance)
            {
                if (singleInstanceMaster)
                {
                    stopListenerThread.store(true);
                    masterListenerThread.join();
                }
                SingleInstanceSynchronizer::detach();
            }

            QMessageBox msgBox(QMessageBox::Question, "Alle exportieren?",
                               "Alle angegebenen Wachberichte (siehe Details) werden nacheinander geladen und als PDF exportiert. "
                               "Dazu wird jeweils die Dateiendung des Wachberichtes durch \".pdf\" ersetzt. Bestehende Dateien "
                               "werden ohne weiteres Nachfragen überschrieben. Fortfahren?", QMessageBox::Abort | QMessageBox::Yes);

            QString detailedText  = "Folgende Wachberichte werden exportiert:";

            for (const QString& tFileName : fileNames)
                detailedText.append("\n- \"" + tFileName + "\"");

            msgBox.setDetailedText(detailedText);

            msgBox.setDefaultButton(QMessageBox::Abort);

            if (msgBox.exec() != QMessageBox::Yes)
                return EXIT_SUCCESS;

            Report report;

            for (const QString& tFileName : fileNames)
            {
                if (!report.open(tFileName))
                {
                    std::cerr<<"ERROR: Could not load report \""<<tFileName.toStdString()<<"\"!"<<std::endl;
                    QMessageBox(QMessageBox::Critical, "Fehler", "Konnte Wachbericht \"" + tFileName + "\" nicht laden!").exec();
                    return EXIT_FAILURE;
                }

                QFileInfo fileInfo(tFileName);
                QString pdfFileName = fileInfo.path() + "/" + fileInfo.completeBaseName() + ".pdf";

                if (!PDFExporter::exportPDF(report, pdfFileName))
                {
                    std::cerr<<"ERROR: Could not export report to \""<<pdfFileName.toStdString()<<"\"!"<<std::endl;
                    QMessageBox(QMessageBox::Critical, "Fehler",
                                                       "Konnte Wachbericht nicht nach \"" + pdfFileName + "\" exportieren!").exec();
                    return EXIT_FAILURE;
                }
            }

            QMessageBox(QMessageBox::Information, "Exportieren erfolgreich", "Es wurden alle Wachberichte exportiert!").exec();

            return EXIT_SUCCESS;
        }
        else if (cmdArg1 == "-F")   //Iteratively fix all carryovers by loading first report from file list, applying its carryovers to
        {                           //second report and saving second report; then applying its carryovers to third report and so forth

            //As following instructions may take some time but the program will exit afterwards, try to detach instance already
            //now and stop listener thread if "master"; hence neither any "slave" requests will be processed by this instance
            //nor this instance, if "slave", will be accidentally recognized as "master" by new other instances
            if (singleInstance)
            {
                if (singleInstanceMaster)
                {
                    stopListenerThread.store(true);
                    masterListenerThread.join();
                }
                SingleInstanceSynchronizer::detach();
            }

            if (fileNames.size() < 2)
            {
                std::cerr<<"WARNING: Nothing to be done!"<<std::endl;
                QMessageBox(QMessageBox::Warning, "Warnung", "Es gibt nichts zu tun!").exec();
                return EXIT_SUCCESS;
            }

            QMessageBox msgBox(QMessageBox::Question, "Alle korrigieren?",
                               "Alle angegebenen Wachberichte (siehe Details) werden nacheinander geladen und nach Korrektur der "
                               "Überträge mittels des jeweils vorherigen Wachberichtes wieder unter demselben Dateinamen gespeichert. "
                               "Der erste Wachbericht bleibt unverändert. Die bestehenden Dateien werden ohne weiteres Nachfragen "
                               "überschrieben.  Fortfahren?", QMessageBox::Abort | QMessageBox::Yes);

            QString detailedText  = "Für die folgenden Wachberichte werden in angegebener Reihenfolge die Überträge korrigiert:";

            for (const QString& tFileName : fileNames)
                detailedText.append("\n- \"" + tFileName + "\"");

            msgBox.setDetailedText(detailedText);

            msgBox.setDefaultButton(QMessageBox::Abort);

            if (msgBox.exec() != QMessageBox::Yes)
                return EXIT_SUCCESS;

            QStringList correctedFiles;

            Report firstReport, secondReport;

            if (!secondReport.open(fileNames[0]))
            {
                std::cerr<<"ERROR: Could not load report \""<<fileNames[0].toStdString()<<"\"!"<<std::endl;
                QMessageBox(QMessageBox::Critical, "Fehler", "Konnte Wachbericht \"" + fileNames[0] + "\" nicht laden!").exec();
                return EXIT_FAILURE;
            }

            for (int i = 1; i < fileNames.size(); ++i)
            {
                firstReport = secondReport;

                if (!secondReport.open(fileNames[i]))
                {
                    std::cerr<<"ERROR: Could not load report \""<<fileNames[i].toStdString()<<"\"!"<<std::endl;
                    QMessageBox(QMessageBox::Critical, "Fehler", "Konnte Wachbericht \"" + fileNames[i] + "\" nicht laden!").exec();
                    return EXIT_FAILURE;
                }

                if (secondReport.loadCarryovers(firstReport))
                {
                    correctedFiles.append(fileNames[i]);

                    if (!secondReport.save(secondReport.getFileName()))
                    {
                        std::cerr<<"ERROR: Could not save report \""<<secondReport.getFileName().toStdString()<<"\"!"<<std::endl;
                        QMessageBox(QMessageBox::Critical,
                                    "Fehler", "Konnte Wachbericht \"" + secondReport.getFileName() + "\" nicht speichern!").exec();
                        return EXIT_FAILURE;
                    }
                }
            }

            if (correctedFiles.isEmpty())
            {
                QMessageBox(QMessageBox::Information, "Korrektur beendet", "Es waren keine Korrekturen erforderlich!").exec();
            }
            else
            {
                QMessageBox msgBox2(QMessageBox::Information, "Korrektur beendet",
                                   "Es wurden Überträge korrigiert! Dies betrifft alle unter Details angegebenen Wachberichte. "
                                   "Hinweis: Für diese ist ein erneuter Export erforderlich.");

                QString detailedText2  = "Bei den folgenden Wachberichten wurden Überträge korrigiert:";

                for (const QString& tFileName : correctedFiles)
                    detailedText2.append("\n- \"" + tFileName + "\"");

                msgBox2.setDetailedText(detailedText2);

                msgBox2.exec();
            }

            return EXIT_SUCCESS;
        }
        else
        {
            fileNames.push_front(cmdArg1);

            //Assume each file exists and is saved report; open each report and show them in individual report windows

            if (singleInstance && !singleInstanceMaster)
            {
                for (const QString& tFileName : fileNames)
                    SingleInstanceSynchronizer::sendOpenReport(tFileName);
            }
            else
            {
                bool allFailed = true;

                for (const QString& tFileName : fileNames)
                    if (startupWindow.openReport(tFileName))
                        allFailed = false;

                if (allFailed)
                    startupWindow.show();
            }
        }
    }
    else
    {
        //No special action requested, simply show startup window

        if (!singleInstance || singleInstanceMaster)
            startupWindow.show();
    }

    //Wait for application being exited and return; in single instance "master" mode additionally
    //stop the listener thread again; in single instance "slave" mode, instead, exit immediately

    if (singleInstance && singleInstanceMaster)
    {
        int exitCode = a.exec();

        stopListenerThread.store(true);
        masterListenerThread.join();

        return exitCode;
    }
    else if (singleInstance && !singleInstanceMaster)
        return EXIT_SUCCESS;
    else
        return a.exec();
}
