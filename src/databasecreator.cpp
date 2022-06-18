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
    QSqlDatabase configDb = QSqlDatabase::database("configDb");
    QSqlQuery configQuery(configDb);

    bool success = configQuery.exec("PRAGMA user_version = " + QString::number(CONFIG_DATABASE_USER_VERSION) + ";");

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
    QSqlDatabase personnelDb = QSqlDatabase::database("personnelDb");
    QSqlQuery personnelQuery(personnelDb);

    bool success = personnelQuery.exec("PRAGMA user_version = " + QString::number(PERSONNEL_DATABASE_USER_VERSION) + ";");

    success &= personnelQuery.exec("CREATE TABLE Personnel ("
                                   "LastName TEXT,"
                                   "FirstName TEXT,"
                                   "MembershipNumber TEXT,"
                                   "Qualifications TEXT,"
                                   "Status INT);"
                                   );

    return success;
}

//

/*!
 * \brief Check if the configuration database version is supported.
 *
 * Uses opened configuration database connected with name "configDb".
 *
 * \return If configuration database's 'user_version' matches compiled database version.
 */
bool DatabaseCreator::checkConfigVersion()
{
    QSqlDatabase configDb = QSqlDatabase::database("configDb");
    QSqlQuery configQuery(configDb);

    if (configQuery.exec("PRAGMA user_version;") && configQuery.next())
    {
        if (configQuery.value("user_version").toInt() == CONFIG_DATABASE_USER_VERSION)
            return true;
    }

    return false;
}

/*!
 * \brief Check if the personnel database version is supported.
 *
 * Uses opened personnel database connected with name "personnelDb".
 *
 * \return If personnel database's 'user_version' matches compiled database version.
 */
bool DatabaseCreator::checkPersonnelVersion()
{
    QSqlDatabase personnelDb = QSqlDatabase::database("personnelDb");
    QSqlQuery personnelQuery(personnelDb);

    if (personnelQuery.exec("PRAGMA user_version;") && personnelQuery.next())
    {
        if (personnelQuery.value("user_version").toInt() == PERSONNEL_DATABASE_USER_VERSION)
            return true;
    }

    return false;
}
