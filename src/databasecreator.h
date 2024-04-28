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

#ifndef DATABASECREATOR_H
#define DATABASECREATOR_H

/*!
 * \brief Basic database handling.
 *
 * Create new tables for the configuration and personnel databases (createConfigDatabase(), createPersonnelDatabase())
 * and check existing database versions (checkConfigVersion(), checkPersonnelVersion()). If databases use incompatible
 * formats from older software versions (checkConfigVersionOlder(), checkPersonnelVersionOlder()) it might be possible
 * to convert their format to the current version via upgradeConfigDatabase() and upgradePersonnelDatabase().
 *
 * Note: Database connections with names "configDb" and "personnelDb" must
 * already exist and these databases must be opened before using this class.
 */
class DatabaseCreator
{
public:
    DatabaseCreator() = delete;                     ///< Deleted constructor.
    //
    static bool createConfigDatabase();             ///< Create a new, empty configuration database.
    static bool createPersonnelDatabase();          ///< Create a new, empty personnel database.
    static bool upgradeConfigDatabase();            ///< Upgrade format of old configuration database to the compiled version.
    static bool upgradePersonnelDatabase();         ///< Upgrade format of old personnel database to the compiled version.
    //
    static bool checkConfigVersion();               ///< Check if the configuration database version matches the compiled version.
    static bool checkPersonnelVersion();            ///< Check if the personnel database version matches the compiled version.
    static bool checkConfigVersionOlder();          ///< Check if the configuration database version is older than the compiled version.
    static bool checkPersonnelVersionOlder();       ///< Check if the personnel database version is older than the compiled version.

private:
    static int getConfigVersion();                  ///< Read the configuration database version.
    static bool setConfigVersion(int pVersion);     ///< Write the configuration database version.
    static int getPersonnelVersion();               ///< Read the personnel database version.
    static bool setPersonnelVersion(int pVersion);  ///< Write the personnel database version.
};

#endif // DATABASECREATOR_H
