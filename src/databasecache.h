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

#ifndef DATABASECACHE_H
#define DATABASECACHE_H

#include "auxil.h"
#include "person.h"

#include <iostream>
#include <memory>
#include <vector>
#include <map>

#include <QString>
#include <QStringList>
#include <QValidator>
#include <QLockFile>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

/*!
 * \brief Access configuration and personnel database records (using a cache functionality).
 *
 * All configuration and personnel information can (and should only) be read from and written to
 * the databases using this class interface. Before using the DatabaseCache, all database records
 * must be read from the databases by calling populate(). This fills the cache and so reading from
 * databases can be avoided in most of this class's functions. Reading always happens via the cached
 * values. Note that some write functions will clear and then re-load parts of the cache, though.
 * All write functions (set.../update.../etc.) will update the cached values and will
 * also immediately write the new values to the corresponding database.
 *
 * The write functions always check for the database lock file via isReadOnly().
 * If this returns true, the write operation is skipped and the cached value left as is.
 */
class DatabaseCache
{
public:
    DatabaseCache() = delete;   ///< Deleted constructor.
    //
    static bool isReadOnly();   ///< Check, if database can be written.
    //
    static bool populate(std::shared_ptr<QLockFile> pLockFile, bool pForce = false);    ///< \brief Fill database cache with fields
                                                                                        ///  from settings and personnel databases.
    //
    static bool getSetting(const QString& pSetting, int& pValue,
                           int pDefault = 0, bool pCreate = false);     ///< Get a cached, integer type setting.
    static bool getSetting(const QString& pSetting, double& pValue,
                           double pDefault = .0, bool pCreate = false); ///< Get a cached, floating-point type setting.
    static bool getSetting(const QString& pSetting, QString& pValue,
                           QString pDefault = "", bool pCreate = false);    ///< Get a cached, string type setting.
    static bool setSetting(const QString& pSetting, int pValue);        ///< Write an integer type setting to cache and database.
    static bool setSetting(const QString& pSetting, double pValue);     ///< Write a floating-point type setting to cache and database.
    static bool setSetting(const QString& pSetting, const QString& pValue); ///< Write a string type setting to cache and database.
    //
    static std::map<int, Aux::Station> stations();                                  ///< Get the cached available stations.
    static std::map<int, Aux::Boat> boats();                                        ///< Get the cached available boats.
    //
    static bool updateStations(const std::vector<Aux::Station>& pStations);         ///< Replace the stations in cache and database.
    static bool updateBoats(const std::vector<Aux::Boat>& pBoats);                  ///< Replace the boats in cache and database.
    //
    static bool stationRowIdFromNameLocation(const QString& pName, const QString& pLocation,
                                             int& pRowId);                          ///< \brief Get the station database
                                                                                    ///  row ID from its name and location.
    static bool stationNameLocationFromRowId(int pRowId,
                                             QString& pName, QString& pLocation);   ///< \brief Get the station name and
                                                                                    ///  location from its database row ID.
    static bool boatRowIdFromName(const QString& pName, int& pRowId);       ///< Get the boat database row ID from its name.
    static bool boatNameFromRowId(int pRowId, QString& pName);              ///< Get the boat name from its database row ID.
    //
    static bool memberNumExists(const QString& pMembershipNumber);                      ///< Check if person exists in personnel cache.
    static bool personExists(const QString& pIdent);                                    ///< Check if person exists in personnel cache.
    static bool getPerson(Person& pPerson, const QString& pIdent);                      ///< Get person from personnel cache.
    static bool getPerson(Person& pPerson, const QString& pLastName,
                          const QString& pFirstName, const QString& pMembershipNumber); ///< Get person from personnel cache.
    static void getPersons(std::vector<Person>& pPersons, const QString& pLastName,
                           const QString& pFirstName, bool pActiveOnly = false);        ///< \brief Get persons with specified
                                                                                        ///  name from personnel cache.
    static void getPersonnel(std::vector<Person>& pPersons);                            ///< Get all persons from personnel cache.
    //
    static bool addPerson(const Person& pNewPerson);                            ///< Add new person to personnel cache and database.
    static bool updatePerson(const QString& pIdent, const Person& pNewPerson);  ///< Update person in personnel cache and database.
    static bool removePerson(QString pIdent);                                   ///< Remove person from personnel cache and database.

private:
    static bool loadIntSettings();  ///< Load all integer type settings from database into cache.
    static bool loadDblSettings();  ///< Load all floating-point type settings from database into cache.
    static bool loadStrSettings();  ///< Load all string type settings from database into cache.
    //
    static bool loadStations();     ///< Load all stations from database into cache.
    static bool loadBoats();        ///< Load all boats from database into cache.
    //
    static bool loadPersonnel();    ///< Load all personnel from database into cache.
    //
    static bool checkStationFormat(Aux::Station pStation);                          ///< Validate the station properties' formatting.
    static bool checkBoatFormat(Aux::Boat pBoat);                                   ///< Validate the boat properties' formatting.
    static bool checkPersonFormat(const Person& pPerson);                           ///< Validate the person properties' formatting.
    static bool checkStationDuplicates(const Aux::Station& pStation,
                                       const std::vector<Aux::Station>& pStations, bool pOneAllowed);
                                                                                    ///< Check if there are no duplicate stations.
    static bool checkBoatDuplicates(const Aux::Boat& pBoat,
                                    const std::vector<Aux::Boat>& pBoats, bool pOneAllowed);
                                                                                    ///< Check if there are no duplicate boats.
    static bool checkPersonnelDuplicates(const Person& pPerson);                    ///< Check if there are no duplicate persons.

private:
    static bool populated;                              //Database fields loaded into cache from databases by populate()?
    //
    static std::shared_ptr<QLockFile> lockFilePtr;      //Lock file to limit database writing to single application instance
    //
    static std::map<QString, int> settingsInt;          //Cache for integer type settings
    static std::map<QString, double> settingsDbl;       //Cache for floating-point type settings
    static std::map<QString, QString> settingsStr;      //Cache for string type settings
    //
    static std::map<int, Aux::Station> stationsMap;     //Cache for stations (database 'rowid' as key)
    static std::map<int, Aux::Boat> boatsMap;           //Cache for boats (database 'rowid' as key)
    //
    static std::map<int, Person> personnelMap;          //Cache for personnel (database 'rowid' as key)
};

#endif // DATABASECACHE_H
