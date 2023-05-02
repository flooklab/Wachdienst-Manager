/*
/////////////////////////////////////////////////////////////////////////////////////////
//
//  This file is part of Wachdienst-Manager, a program to manage DLRG watch duty reports.
//  Copyright (C) 2021–2023 M. Frohne
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

#include "person.h"
#include "version.h"

#include <QString>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

//Public

/*!
 * \brief Create a new, empty configuration database.
 *
 * Uses opened configuration database connected with name "configDb".
 *
 * Sets user_version to compiled value and creates empty tables for application settings, stations and boats.
 *
 * \return If successful.
 */
bool DatabaseCreator::createConfigDatabase()
{
    bool success = setConfigVersion(Version::ConfigDatabaseUserVersion);

    QSqlDatabase configDb = QSqlDatabase::database("configDb");
    QSqlQuery configQuery(configDb);

    success &= configQuery.exec("CREATE TABLE Application ("
                                "Setting TEXT,"
                                "Type INT,"
                                "ValueInt INT,"
                                "ValueDbl DOUBLE,"
                                "ValueStr TEXT);"
                                );

    success &= configQuery.exec("CREATE TABLE Stations ("
                                "Location TEXT,"
                                "Name TEXT,"
                                "LocalGroup TEXT,"
                                "DistrictAssociation TEXT,"
                                "RadioCallName TEXT,"
                                "RadioCallNameAlt TEXT);"
                                );

    success &= configQuery.exec("CREATE TABLE Boats ("
                                "Name TEXT,"
                                "Acronym TEXT,"
                                "Type TEXT,"
                                "FuelType TEXT,"
                                "RadioCallName TEXT,"
                                "RadioCallNameAlt TEXT,"
                                "HomeStation TEXT);"
                                );

    return success;
}

/*!
 * \brief Create a new, empty personnel database.
 *
 * Uses opened personnel database connected with name "personnelDb".
 *
 * Sets user_version to compiled value and creates empty table for personnel records.
 *
 * \return If successful.
 */
bool DatabaseCreator::createPersonnelDatabase()
{
    bool success = setPersonnelVersion(Version::PersonnelDatabaseUserVersion);

    QSqlDatabase personnelDb = QSqlDatabase::database("personnelDb");
    QSqlQuery personnelQuery(personnelDb);

    success &= personnelQuery.exec("CREATE TABLE Personnel ("
                                   "LastName TEXT,"
                                   "FirstName TEXT,"
                                   "MembershipNumber TEXT,"
                                   "Qualifications TEXT,"
                                   "Status INT);"
                                   );

    return success;
}

/*!
 * \brief Upgrade format of old configuration database to the compiled version.
 *
 * Note: Does nothing and returns false, as the configuration database version is still unchanged from the original version.
 *
 * \return If upgrade was required and successful.
 */
bool DatabaseCreator::upgradeConfigDatabase()
{
    return false;
}

/*!
 * \brief Upgrade format of old personnel database to the compiled version.
 *
 * Upgrades personnel databases from version 1 to version 2.
 *
 * \return If upgrade was required and successful.
 */
bool DatabaseCreator::upgradePersonnelDatabase()
{
    if (!checkPersonnelVersionOlder())
        return false;

    if (getPersonnelVersion() == 1 && Version::PersonnelDatabaseUserVersion == 2)
    {        
        QSqlDatabase personnelDb = QSqlDatabase::database("personnelDb");

        QSqlQuery personnelQuery(personnelDb);

        personnelQuery.prepare("SELECT Qualifications, rowid FROM Personnel;");

        if (!personnelQuery.exec())
            return false;

        QSqlQuery updateQualisQuery(personnelDb);

        while (personnelQuery.next())
        {
            QString tNewQualis = Person::Qualifications::convertLegacyQualifications(personnelQuery.value("Qualifications").toString());
            int tRowId = personnelQuery.value("rowid").toInt();

            updateQualisQuery.prepare("UPDATE Personnel SET Qualifications=:qualis WHERE rowid=:row;");
            updateQualisQuery.bindValue(":qualis", tNewQualis);
            updateQualisQuery.bindValue(":row", tRowId);

            if (!updateQualisQuery.exec())
                return false;
        }

        return setPersonnelVersion(Version::PersonnelDatabaseUserVersion);
    }

    return false;
}

//

/*!
 * \brief Check if the configuration database version matches the compiled version.
 *
 * Uses opened configuration database connected with name "configDb".
 *
 * \return If configuration database's 'user_version' matches compiled database version.
 */
bool DatabaseCreator::checkConfigVersion()
{
    int tVersion = getConfigVersion();
    return tVersion != -1 && tVersion == Version::ConfigDatabaseUserVersion;
}

/*!
 * \brief Check if the personnel database version matches the compiled version.
 *
 * Uses opened personnel database connected with name "personnelDb".
 *
 * \return If personnel database's 'user_version' matches compiled database version.
 */
bool DatabaseCreator::checkPersonnelVersion()
{
    int tVersion = getPersonnelVersion();
    return tVersion != -1 && tVersion == Version::PersonnelDatabaseUserVersion;
}

/*!
 * \brief Check if the configuration database version is older than the compiled version.
 *
 * Uses opened configuration database connected with name "configDb".
 *
 * \return If configuration database's 'user_version' is valid but older than the compiled database version.
 */
bool DatabaseCreator::checkConfigVersionOlder()
{
    int tVersion = getConfigVersion();
    return tVersion != -1 && tVersion < Version::ConfigDatabaseUserVersion;
}

/*!
 * \brief Check if the personnel database version is older than the compiled version.
 *
 * Uses opened personnel database connected with name "personnelDb".
 *
 * \return If personnel database's 'user_version' is valid but older than the compiled database version.
 */
bool DatabaseCreator::checkPersonnelVersionOlder()
{
    int tVersion = getPersonnelVersion();
    return tVersion != -1 && tVersion < Version::PersonnelDatabaseUserVersion;
}

//Private

/*!
 * \brief Read the configuration database version.
 *
 * \return Configuration database's 'user_version', if query successful, and -1 otherwise.
 */
int DatabaseCreator::getConfigVersion()
{
    QSqlDatabase configDb = QSqlDatabase::database("configDb");
    QSqlQuery configQuery(configDb);

    if (configQuery.exec("PRAGMA user_version;") && configQuery.next())
        return configQuery.value("user_version").toInt();
    else
        return -1;
}

/*!
 * \brief Write the configuration database version.
 *
 * \param pVersion New value for configuration database's 'user_version'.
 * \return If successful.
 */
bool DatabaseCreator::setConfigVersion(const int pVersion)
{
    QSqlDatabase configDb = QSqlDatabase::database("configDb");
    QSqlQuery configQuery(configDb);

    return configQuery.exec("PRAGMA user_version = " + QString::number(pVersion) + ";");
}

/*!
 * \brief Read the personnel database version.
 *
 * \return Personnel database's 'user_version', if query successful, and -1 otherwise.
 */
int DatabaseCreator::getPersonnelVersion()
{
    QSqlDatabase personnelDb = QSqlDatabase::database("personnelDb");
    QSqlQuery personnelQuery(personnelDb);

    if (personnelQuery.exec("PRAGMA user_version;") && personnelQuery.next())
        return personnelQuery.value("user_version").toInt();
    else
        return -1;
}

/*!
 * \brief Write the personnel database version.
 *
 * \param pVersion New value for personnel database's 'user_version'.
 * \return If successful.
 */
bool DatabaseCreator::setPersonnelVersion(const int pVersion)
{
    QSqlDatabase personnelDb = QSqlDatabase::database("personnelDb");
    QSqlQuery personnelQuery(personnelDb);

    return personnelQuery.exec("PRAGMA user_version = " + QString::number(pVersion) + ";");
}
