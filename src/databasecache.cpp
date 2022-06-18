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

#include "databasecache.h"

//Initialize static class members

bool DatabaseCache::populated = false;
//
std::shared_ptr<QLockFile> DatabaseCache::lockFilePtr = nullptr;
//
std::map<QString, int> DatabaseCache::settingsInt;
std::map<QString, double> DatabaseCache::settingsDbl;
std::map<QString, QString> DatabaseCache::settingsStr;
//
std::map<int, Aux::Station> DatabaseCache::stationsMap;
std::map<int, Aux::Boat> DatabaseCache::boatsMap;
//
std::map<int, Person> DatabaseCache::personnelMap;

//Public

/*!
 * \brief Check, if database can be written.
 *
 * Tries to acquire the database lock file. If this fails, i.e. a lock file is already present,
 * write operations to the database should be prevented and hence true will be returned.
 *
 * \return If database should be considerered read-only because lock file cannot be acquired.
 */
bool DatabaseCache::isReadOnly()
{
    if (lockFilePtr == nullptr)
        return true;

    //Try again to acquire lock file
    if (!lockFilePtr->isLocked())
        lockFilePtr->tryLock(100);

    return !lockFilePtr->isLocked();
}

//

/*!
 * \brief Fill database cache with fields from settings and personnel databases.
 *
 * If this function has not already been called or \p pForce is true,
 * settings will be read from configuration database and loaded in cache and the
 * personnel records will be read from personnel database and also loaded in cache.
 *
 * \p pLockFile must specify a lock file that is used to limit write access to the databases to a single program instance.
 *
 * See also loadIntSettings(), loadDblSettings(), loadStrSettings(), loadStations(), loadBoats(), loadPersonnel().
 *
 * \param pLockFile Pointer to a lock file for the configuration and personnel databases.
 * \param pForce Populate cache even if already populated.
 * \return If already populated or new populate action was successful.
 */
bool DatabaseCache::populate(std::shared_ptr<QLockFile> pLockFile, bool pForce)
{
    if (populated && !pForce)
        return true;

    //Take over lock file pointer
    lockFilePtr = pLockFile;

    //Set to false on query error, but continue and then return 'populated' at the end
    populated = true;

    //Load application settings

    //Integer type settings
    settingsInt.clear();
    populated &= loadIntSettings();

    //Floating-point type settings
    settingsDbl.clear();
    populated &= loadDblSettings();

    //String type settings
    settingsStr.clear();
    populated &= loadStrSettings();

    //Load stations

    stationsMap.clear();
    populated &= loadStations();

    if (stationsMap.empty())
        std::cerr<<"WARNING: No stations found in database!"<<std::endl;

    //Load boats

    boatsMap.clear();
    populated &= loadBoats();

    if (boatsMap.empty())
        std::cerr<<"WARNING: No boats found in database!"<<std::endl;

    //Load personnel

    personnelMap.clear();
    populated &= loadPersonnel();

    if (personnelMap.empty())
        std::cerr<<"WARNING: No personnel found in database!"<<std::endl;

    return populated;
}

//

/*!
 * \brief Get a cached, integer type setting.
 *
 * Looks in cache for setting with name \p pSetting and assigns its value to \p pValue, if found.
 * If the setting is not found in cache, \p pDefault is assigned to \p pValue and the setting with this default value
 * is also inserted to cache and written to database, if \p pCreate is true.
 *
 * If the setting was found, the function return true.
 * If the setting was not found and \p pCreate is false, the function returns false.
 * If the setting was not found and \p pCreate is true, the function returns, whether writing to database was successful.
 *
 * \param pSetting Name of the setting.
 * \param pValue Destination for the setting's value.
 * \param pDefault Default value to use for the setting.
 * \param pCreate Create new database (and cache) record, if setting is not found in cache.
 * \return True, if setting found or database writing successful, and false otherwise. See detailed function description.
 */
bool DatabaseCache::getSetting(const QString& pSetting, int& pValue, int pDefault, bool pCreate)
{
    //Search for setting in cache; if not found then return specified default value
    if (settingsInt.find(pSetting) == settingsInt.end())
    {
        //Use default value
        pValue = pDefault;

        //Only return true, if new setting is (succesfully) created
        bool retVal = false;

        //Automatically create missing setting in database, if requested
        if (pCreate)
            retVal = setSetting(pSetting, pDefault);

        return retVal;
    }
    else
    {
        //Use cached setting
        pValue = settingsInt.at(pSetting);

        return true;
    }

    return false;
}

/*!
 * \brief Get a cached, floating-point type setting.
 *
 * \copydetails getSetting()
 */
bool DatabaseCache::getSetting(const QString& pSetting, double& pValue, double pDefault, bool pCreate)
{
    //Search for setting in cache; if not found then return specified default value
    if (settingsDbl.find(pSetting) == settingsDbl.end())
    {
        //Use default value
        pValue = pDefault;

        //Only return true, if new setting is (succesfully) created
        bool retVal = false;

        //Automatically create missing setting in database, if requested
        if (pCreate)
            retVal = setSetting(pSetting, pDefault);

        return retVal;
    }
    else
    {
        //Use cached setting
        pValue = settingsDbl.at(pSetting);

        return true;
    }

    return false;
}

/*!
 * \brief Get a cached, string type setting.
 *
 * \copydetails getSetting()
 */
bool DatabaseCache::getSetting(const QString& pSetting, QString& pValue, QString pDefault, bool pCreate)
{
    //Search for setting in cache; if not found then return specified default value
    if (settingsStr.find(pSetting) == settingsStr.end())
    {
        //Use default value
        pValue = pDefault;

        //Only return true, if new setting is (succesfully) created
        bool retVal = false;

        //Automatically create missing setting in database, if requested
        if (pCreate)
            retVal = setSetting(pSetting, pDefault);

        return retVal;
    }
    else
    {
        //Use cached setting
        pValue = settingsStr.at(pSetting);

        return true;
    }

    return false;
}

/*!
 * \brief Write an integer type setting to cache and database.
 *
 * Writes the setting with name \p pSetting and value \p pValue to the configuration database.
 * If writing to database was successful, the new setting (or the new value for the old setting) is also added to the cache.
 *
 * Returns immediately, if database is read-only.
 *
 * \param pSetting Name of the setting.
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 */
bool DatabaseCache::setSetting(const QString& pSetting, int pValue)
{
    if (isReadOnly())
        return false;

    QSqlDatabase configDb = QSqlDatabase::database("configDb");
    QSqlQuery query(configDb);

    //Insert new row into database, if setting not in cache, else just update existing row
    if (settingsInt.find(pSetting) == settingsInt.end())
    {
        query.prepare("INSERT INTO Application (Setting, Type, ValueInt, ValueDbl, ValueStr) "
                      "VALUES (:setting, :type, :valInt, :valDbl, :valStr);");

        query.bindValue(":setting", pSetting);
        query.bindValue(":type", 0);
        query.bindValue(":valInt", pValue);
        query.bindValue(":valDbl", 0);
        query.bindValue(":valStr", "");
    }
    else
    {
        query.prepare("UPDATE Application SET ValueInt=:valInt WHERE Setting=:setting;");

        query.bindValue(":setting", pSetting);
        query.bindValue(":valInt", pValue);
    }

    if (!query.exec())
    {
        std::cerr<<"ERROR: Could not write setting \"" + pSetting.toStdString() + "\" to configuration database!"<<std::endl;
        return false;
    }
    else
    {
        settingsInt[pSetting] = pValue;
        return true;
    }

    return false;
}

/*!
 * \brief Write a floating-point type setting to cache and database.
 *
 * \copydetails setSetting()
 */
bool DatabaseCache::setSetting(const QString& pSetting, double pValue)
{
    if (isReadOnly())
        return false;

    QSqlDatabase configDb = QSqlDatabase::database("configDb");
    QSqlQuery query(configDb);

    //Insert new row into database, if setting not in cache, else just update existing row
    if (settingsDbl.find(pSetting) == settingsDbl.end())
    {
        query.prepare("INSERT INTO Application (Setting, Type, ValueInt, ValueDbl, ValueStr) "
                      "VALUES (:setting, :type, :valInt, :valDbl, :valStr);");

        query.bindValue(":setting", pSetting);
        query.bindValue(":type", 1);
        query.bindValue(":valInt", 0);
        query.bindValue(":valDbl", pValue);
        query.bindValue(":valStr", "");
    }
    else
    {
        query.prepare("UPDATE Application SET ValueDbl=:valDbl WHERE Setting=:setting;");

        query.bindValue(":setting", pSetting);
        query.bindValue(":valDbl", pValue);
    }

    if (!query.exec())
    {
        std::cerr<<"ERROR: Could not write setting \"" + pSetting.toStdString() + "\" to configuration database!"<<std::endl;
        return false;
    }
    else
    {
        settingsDbl[pSetting] = pValue;
        return true;
    }

    return false;
}

/*!
 * \brief Write a string type setting to cache and database.
 *
 * \copydetails setSetting()
 */
bool DatabaseCache::setSetting(const QString& pSetting, const QString& pValue)
{
    if (isReadOnly())
        return false;

    QSqlDatabase configDb = QSqlDatabase::database("configDb");
    QSqlQuery query(configDb);

    //Insert new row into database, if setting not in cache, else just update existing row
    if (settingsStr.find(pSetting) == settingsStr.end())
    {
        query.prepare("INSERT INTO Application (Setting, Type, ValueInt, ValueDbl, ValueStr) "
                      "VALUES (:setting, :type, :valInt, :valDbl, :valStr);");

        query.bindValue(":setting", pSetting);
        query.bindValue(":type", 2);
        query.bindValue(":valInt", 0);
        query.bindValue(":valDbl", 0);
        query.bindValue(":valStr", pValue);
    }
    else
    {
        query.prepare("UPDATE Application SET ValueStr=:valStr WHERE Setting=:setting;");

        query.bindValue(":setting", pSetting);
        query.bindValue(":valStr", pValue);
    }

    if (!query.exec())
    {
        std::cerr<<"ERROR: Could not write setting \"" + pSetting.toStdString() + "\" to configuration database!"<<std::endl;
        return false;
    }
    else
    {
        settingsStr[pSetting] = pValue;
        return true;
    }

    return false;
}

//

/*!
 * \brief Get the cached available stations.
 *
 * \return Map of available stations (see Aux::Station) with (unique) integer IDs as keys.
 */
std::map<int, Aux::Station> DatabaseCache::stations()
{
    return stationsMap;
}

/*!
 * \brief Get the cached available boats.
 *
 * \return Map of available boats (see Aux::Boat) with (unique) integer IDs as keys.
 */
std::map<int, Aux::Boat> DatabaseCache::boats()
{
    return boatsMap;
}

//

/*!
 * \brief Replace the stations in cache and database.
 *
 * Updates database records for all stations with unchanged name and location (^= identifier), removes and re-adds
 * records for stations with changed name or location, removes removed stations and adds new stations.
 * Finally, if writing to database was successful, clears the stations cache and re-loads
 * the stations into cache to update their IDs (which are simply database row IDs).
 *
 * Returns immediately, if database is read-only.
 *
 * Note: Checks for wrongly formatted strings in station properties and for duplicate stations
 * (see checkStationFormat(), checkStationDuplicates()) and simply returns if there is a problem.
 *
 * \param pStations New/changed list of stations.
 * \return If writing to database and re-loading stations was successful.
 */
bool DatabaseCache::updateStations(const std::vector<Aux::Station>& pStations)
{
    if (isReadOnly())
        return false;

    //Check stations' formatting first
    for (const Aux::Station& newStation : pStations)
    {
        if (!checkStationFormat(newStation))
        {
            std::cerr<<"ERROR: Wrongly formatted station!"<<std::endl;
            return false;
        }
    }
    for (const Aux::Station& newStation : pStations)
    {
        if (!checkStationDuplicates(newStation, pStations, true))
        {
            std::cerr<<"ERROR: Duplicate station!"<<std::endl;
            return false;
        }
    }

    QSqlDatabase configDb = QSqlDatabase::database("configDb");
    QSqlQuery configQuery(configDb);

    //Removed stations
    for (const auto& it : stationsMap)  //(sic!)
    {
        const Aux::Station& currentStation = it.second;

        //Keep station if in both old and new list of stations
        bool tContinue = false;
        for (const Aux::Station& newStation : pStations)
        {
            if (newStation.location == currentStation.location && newStation.name == currentStation.name)
            {
                tContinue = true;
                break;
            }
        }
        if (tContinue)
            continue;

        //Remove it otherwise
        configQuery.prepare("DELETE FROM Stations WHERE Location=:location AND Name=:name;");
        configQuery.bindValue(":location", currentStation.location);
        configQuery.bindValue(":name", currentStation.name);

        if (!configQuery.exec())
        {
            std::cerr<<"ERROR: Could not remove station from configuration database!"<<std::endl;
            return false;
        }
    }

    //New stations
    for (const Aux::Station& newStation : pStations)    //(sic!)
    {
        //Skip station if in both old and new list of stations
        int tmpRowId = 0;
        if (stationRowIdFromNameLocation(newStation.name, newStation.location, tmpRowId))
            continue;

        configQuery.prepare("INSERT INTO Stations (Location, Name, LocalGroup, DistrictAssociation, "
                                                  "RadioCallName, RadioCallNameAlt) "
                            "VALUES (:location, :name, :group, :district, :radiocall, :radiocallAlt);");
        configQuery.bindValue(":location", newStation.location);
        configQuery.bindValue(":name", newStation.name);
        configQuery.bindValue(":group", newStation.localGroup);
        configQuery.bindValue(":district", newStation.districtAssociation);
        configQuery.bindValue(":radiocall", newStation.radioCallName);
        configQuery.bindValue(":radiocallAlt", newStation.radioCallNameAlt);

        if (!configQuery.exec())
        {
            std::cerr<<"ERROR: Could not add station to configuration database!"<<std::endl;
            return false;
        }
    }

    //Edited stations (do not check, simply update all not removed and not added stations)
    for (const Aux::Station& newStation : pStations)    //(sic!)
    {
        //Skip station if not in both old and new list of stations
        int tmpRowId = 0;
        if (!stationRowIdFromNameLocation(newStation.name, newStation.location, tmpRowId))
            continue;

        configQuery.prepare("UPDATE Stations SET LocalGroup=:group, DistrictAssociation=:district, "
                                                "RadioCallName=:radiocall, RadioCallNameAlt=:radiocallAlt "
                            "WHERE Location=:location AND Name=:name;");
        configQuery.bindValue(":location", newStation.location);
        configQuery.bindValue(":name", newStation.name);
        configQuery.bindValue(":group", newStation.localGroup);
        configQuery.bindValue(":district", newStation.districtAssociation);
        configQuery.bindValue(":radiocall", newStation.radioCallName);
        configQuery.bindValue(":radiocallAlt", newStation.radioCallNameAlt);

        if (!configQuery.exec())
        {
            std::cerr<<"ERROR: Could not update station in configuration database!"<<std::endl;
            return false;
        }
    }

    //Reload stations to obtain new/changed row IDs

    stationsMap.clear();

    return loadStations();
}

/*!
 * \brief Replace the boats in cache and database.
 *
 * Updates database records for all boats with unchanged name (^= identifier), removes and re-adds
 * records for boats with changed name, removes removed boats and adds new boats.
 * Finally, if writing to database was successful, clears the boats cache and re-loads
 * the boats into cache to update their IDs (which are simply database row IDs).
 *
 * Returns immediately, if database is read-only.
 *
 * Note: Checks for wrongly formatted strings in boat properties and for duplicate boats
 * (see checkBoatFormat(), checkBoatDuplicates()) and simply returns if there is a problem.
 *
 * \param pBoats New/changed list of boats.
 * \return If writing to database and re-loading boats was successful.
 */
bool DatabaseCache::updateBoats(const std::vector<Aux::Boat>& pBoats)
{
    if (isReadOnly())
        return false;

    //Check boats' formatting first
    for (const Aux::Boat& newBoat : pBoats)
    {
        if (!checkBoatFormat(newBoat))
        {
            std::cerr<<"ERROR: Wrongly formatted boat!"<<std::endl;
            return false;
        }
    }
    for (const Aux::Boat& newBoat: pBoats)
    {
        if (!checkBoatDuplicates(newBoat, pBoats, true))
        {
            std::cerr<<"ERROR: Duplicate boat!"<<std::endl;
            return false;
        }

        int tHomeStationId = 0;
        QString tName, tLocation;
        if (newBoat.homeStation != "" &&
            (!Aux::stationNameLocationFromIdent(newBoat.homeStation, tName, tLocation) ||
             !stationRowIdFromNameLocation(tName, tLocation, tHomeStationId)))
        {
            std::cerr<<"ERROR: Boat's home station not found in database!"<<std::endl;
            return false;
        }

    }

    QSqlDatabase configDb = QSqlDatabase::database("configDb");
    QSqlQuery configQuery(configDb);

    //Removed boats
    for (const auto& it : boatsMap) //(sic!)
    {
        const Aux::Boat& currentBoat = it.second;

        //Keep boat if in both old and new list of boats
        bool tContinue = false;
        for (const Aux::Boat& newBoat : pBoats)
        {
            if (newBoat.name == currentBoat.name)
            {
                tContinue = true;
                break;
            }
        }
        if (tContinue)
            continue;

        //Remove it otherwise
        configQuery.prepare("DELETE FROM Boats WHERE Name=:name;");
        configQuery.bindValue(":name", currentBoat.name);

        if (!configQuery.exec())
        {
            std::cerr<<"ERROR: Could not remove boat from configuration database!"<<std::endl;
            return false;
        }
    }

    //New boats
    for (const Aux::Boat& newBoat : pBoats) //(sic!)
    {
        //Skip boat if in both old and new list of boats
        int tmpRowId = 0;
        if (boatRowIdFromName(newBoat.name, tmpRowId))
            continue;

        configQuery.prepare("INSERT INTO Boats (Name, Acronym, Type, FuelType, RadioCallName, RadioCallNameAlt, "
                                               "HomeStation) "
                            "VALUES (:name, :acronym, :type, :fuel, :radiocall, :radiocallAlt, :homeStation);");
        configQuery.bindValue(":name", newBoat.name);
        configQuery.bindValue(":acronym", newBoat.acronym);
        configQuery.bindValue(":type", newBoat.type);
        configQuery.bindValue(":fuel", newBoat.fuelType);
        configQuery.bindValue(":radiocall", newBoat.radioCallName);
        configQuery.bindValue(":radiocallAlt", newBoat.radioCallNameAlt);
        configQuery.bindValue(":homeStation", newBoat.homeStation);

        if (!configQuery.exec())
        {
            std::cerr<<"ERROR: Could not add boat to configuration database!"<<std::endl;
            return false;
        }
    }

    //Edited boats (do not check, simply update all not removed and not added boats)
    for (const Aux::Boat& newBoat : pBoats) //(sic!)
    {
        //Skip boat if not in both old and new list of boats
        int tmpRowId = 0;
        if (!boatRowIdFromName(newBoat.name, tmpRowId))
            continue;

        configQuery.prepare("UPDATE Boats SET Acronym=:acronym, Type=:type, FuelType=:fuel, RadioCallName=:radiocall, "
                                             "RadioCallNameAlt=:radiocallAlt, HomeStation=:homeStation "
                            "WHERE Name=:name;");
        configQuery.bindValue(":name", newBoat.name);
        configQuery.bindValue(":acronym", newBoat.acronym);
        configQuery.bindValue(":type", newBoat.type);
        configQuery.bindValue(":fuel", newBoat.fuelType);
        configQuery.bindValue(":radiocall", newBoat.radioCallName);
        configQuery.bindValue(":radiocallAlt", newBoat.radioCallNameAlt);
        configQuery.bindValue(":homeStation", newBoat.homeStation);

        if (!configQuery.exec())
        {
            std::cerr<<"ERROR: Could not update boat in configuration database!"<<std::endl;
            return false;
        }
    }

    //Reload boats to obtain new/changed row IDs

    boatsMap.clear();

    return loadBoats();
}

//

/*!
 * \brief Get the station database row ID from its name and location.
 *
 * Searches for a station (from database cache) with name == \p pName and location == \p pLocation.
 * Its row ID is assigned to \p pRowId, if the station is found.
 *
 * \param pName Name of the station.
 * \param pLocation Location of the station.
 * \param pRowId Destination for the database row ID of the station.
 * \return If the station was found.
 */
bool DatabaseCache::stationRowIdFromNameLocation(const QString& pName, const QString& pLocation, int& pRowId)
{
    for (const auto& it : stationsMap)
    {
        if (it.second.name == pName && it.second.location == pLocation)
        {
            pRowId = it.first;
            return true;
        }
    }

    return false;
}

/*!
 * \brief Get the station name and location from its database row ID.
 *
 * Searches for a station (from database cache) with row ID == \p pRowId.
 * Its name and location are assigned to \p pName and \p pLocation, if the station is found.
 *
 * \param pRowId Database row ID of the station.
 * \param pName Destination for the name of the station.
 * \param pLocation Destination for the location of the station.
 * \return If the station was found.
 */
bool DatabaseCache::stationNameLocationFromRowId(int pRowId, QString& pName, QString& pLocation)
{
    if (stationsMap.find(pRowId) != stationsMap.end())
    {
        pName = stationsMap.at(pRowId).name;
        pLocation = stationsMap.at(pRowId).location;
        return true;
    }

    return false;
}

/*!
 * \brief Get the boat database row ID from its name.
 *
 * Searches for a boat (from database cache) with name == \p pName.
 * Its row ID is assigned to \p pRowId, if the boat is found.
 *
 * \param pName Name of the boat.
 * \param pRowId Destination for the database row ID of the boat.
 * \return If the boat was found.
 */
bool DatabaseCache::boatRowIdFromName(const QString& pName, int& pRowId)
{
    for (const auto& it : boatsMap)
    {
        if (it.second.name == pName)
        {
            pRowId = it.first;
            return true;
        }
    }

    return false;
}

/*!
 * \brief Get the boat name from its database row ID.
 *
 * Searches for a boat (from database cache) with row ID == \p pRowId.
 * Its name is assigned to \p pName, if the boat is found.
 *
 * \param pRowId Database row ID of the boat.
 * \param pName Destination for the name of the boat.
 * \return If the boat was found.
 */
bool DatabaseCache::boatNameFromRowId(int pRowId, QString& pName)
{
    if (boatsMap.find(pRowId) != boatsMap.end())
    {
        pName = boatsMap.at(pRowId).name;
        return true;
    }

    return false;
}

//

/*!
 * \brief Check if person exists in personnel cache.
 *
 * Searches cache for a person with membership number \p pMembershipNumber.
 * The persons' membership numbers should be unique, so use this function before adding/editing a person to the personnel database.
 * Otherwise you can also use personExists() to check for a person using its identifier.
 *
 * \param pMembershipNumber Person's membership number.
 * \return If person found in personnel cache.
 */
bool DatabaseCache::memberNumExists(const QString& pMembershipNumber)
{
    for (const auto& it : personnelMap)
    {
        if (Person::extractMembershipNumber(it.second.getIdent()) == pMembershipNumber)
            return true;
    }

    return false;
}

/*!
 * \brief Check if person exists in personnel cache.
 *
 * Searches cache for a person with identifier \p pIdent.
 *
 * See also memberNumExists() for checking, if a membership number exists (which itself should be unique, not only the identifier).
 *
 * \param pIdent Person's identifier.
 * \return If person found in personnel cache.
 */
bool DatabaseCache::personExists(const QString& pIdent)
{
    for (const auto& it : personnelMap)
    {
        if (it.second.getIdent() == pIdent)
            return true;
    }

    return false;
}

/*!
 * \brief Get person from personnel cache.
 *
 * Searches cache for a person with identifier \p pIdent and assigns it to \p pPerson.
 *
 * \param pPerson Destination for the person.
 * \param pIdent Person's identifer.
 * \return If person was found.
 */
bool DatabaseCache::getPerson(Person& pPerson, const QString& pIdent)
{
    for (const auto& it : personnelMap)
    {
        if (it.second.getIdent() == pIdent)
        {
            pPerson = it.second;
            return true;
        }
    }

    return false;
}

/*!
 * \brief Get person from personnel cache.
 *
 * Searches cache for a person with an (internal) identifier generated from \p pLastName, \p pFirstName and \p pMembershipNumber
 * and assigns it to \p pPerson.
 *
 * \param pPerson Destination for the person.
 * \param pLastName Person's last name.
 * \param pFirstName Person's first name.
 * \param pMembershipNumber Person's membership number.
 * \return If person was found.
 */
bool DatabaseCache::getPerson(Person& pPerson, const QString& pLastName, const QString& pFirstName, const QString& pMembershipNumber)
{
    QString tIdent = Person::createInternalIdent(pLastName, pFirstName, pMembershipNumber);

    return getPerson(pPerson, tIdent);
}

/*!
 * \brief Get persons with specified name from personnel cache.
 *
 * Searches cache for all persons with last name \p pLastName and first name \p pFirstName
 * and assigns a list containing those persons to \p pPersons. If \p pActiveOnly is true,
 * only active/enabled persons (see Person::getActive()) will be added to the list.
 *
 * \param pPersons Person's list of found persons.
 * \param pLastName Persons' last name.
 * \param pFirstName Persons' first name.
 * \param pActiveOnly Get only active/enabled persons.
 */
void DatabaseCache::getPersons(std::vector<Person>& pPersons, const QString& pLastName, const QString& pFirstName, bool pActiveOnly)
{
    pPersons.clear();

    for (const auto& it : personnelMap)
    {
        if (it.second.getLastName() == pLastName && it.second.getFirstName() == pFirstName && (!pActiveOnly || it.second.getActive()))
            pPersons.push_back(it.second);
    }
}

/*!
 * \brief Get all persons from personnel cache.
 *
 * Assigns a list of all persons in personnel cache to \p pPersons.
 *
 * \param pPersons Destination for list of persons.
 */
void DatabaseCache::getPersonnel(std::vector<Person>& pPersons)
{
    pPersons.clear();
    pPersons.reserve(personnelMap.size());

    for (auto it : personnelMap)
        pPersons.push_back(std::move(it.second));
}

//

/*!
 * \brief Add new person to personnel cache and database.
 *
 * Adds new person record for \p pPerson to personnel database and, if successful, clears and re-loads the personnel cache.
 * The person is not added, if a person with the same membership number already exists in personnel cache (i.e. in database)
 * or if the person's name or membership number are wrongly formatted (see checkPersonnelDuplicates(), checkPersonFormat()).
 *
 * Returns immediately, if database is read-only.
 *
 * \param pNewPerson New person.
 * \return If writing to database and re-loading personnel cache was successful.
 */
bool DatabaseCache::addPerson(const Person& pNewPerson)
{
    if (isReadOnly())
        return false;

    //Check person's formatting first
    if (!checkPersonFormat(pNewPerson))
    {
        std::cerr<<"ERROR: Wrongly formatted person!"<<std::endl;
        return false;
    }
    if (!checkPersonnelDuplicates(pNewPerson))
    {
        std::cerr<<"ERROR: Duplicate membership number!"<<std::endl;
        return false;
    }

    QSqlDatabase personnelDb = QSqlDatabase::database("personnelDb");
    QSqlQuery personnelQuery(personnelDb);

    personnelQuery.prepare("INSERT INTO Personnel (LastName, FirstName, MembershipNumber, Qualifications, Status) "
                           "VALUES (:lastName, :firstName, :mmbNr, :qualis, :status);");
    personnelQuery.bindValue(":lastName", pNewPerson.getLastName());
    personnelQuery.bindValue(":firstName", pNewPerson.getFirstName());
    personnelQuery.bindValue(":mmbNr", Person::extractMembershipNumber(pNewPerson.getIdent()));
    personnelQuery.bindValue(":qualis", pNewPerson.getQualifications().toString());
    personnelQuery.bindValue(":status", pNewPerson.getActive() ? 0 : 1);

    if (!personnelQuery.exec())
    {
        std::cerr<<"ERROR: Could not add person to personnel database!"<<std::endl;
        return false;
    }

    //Reload personnel to obtain new/changed row IDs

    personnelMap.clear();

    return loadPersonnel();
}

/*!
 * \brief Update person in personnel cache and database.
 *
 * Updates the database record for the person with identifier \p pIdent with \p pNewPerson and, if successful,
 * clears and re-loads the personnel cache. The person is not updated, if the membership number has changed
 * but a different person with the same membership number already exists. The person is also not changed,
 * if the person's name or membership number are wrongly formatted. See also checkPersonnelDuplicates() and checkPersonFormat().
 *
 * Returns immediately, if database is read-only.
 *
 * \param pIdent Identifier of the person to update.
 * \param pNewPerson Changed person.
 * \return If writing to database and re-loading personnel cache was successful.
 */
bool DatabaseCache::updatePerson(const QString& pIdent, const Person& pNewPerson)
{
    if (isReadOnly())
        return false;

    //Check person's formatting first
    if (!checkPersonFormat(pNewPerson))
    {
        std::cerr<<"ERROR: Wrongly formatted person!"<<std::endl;
        return false;
    }

    //Check old identifier
    if (!personExists(pIdent))
    {
        std::cerr<<"ERROR: Cannot update person. ID does not exist!"<<std::endl;
        return false;
    }

    //If membership number has changed, need to check that it does not exist yet
    if (Person::extractMembershipNumber(pNewPerson.getIdent()) != Person::extractMembershipNumber(pIdent) &&
            !checkPersonnelDuplicates(pNewPerson))
    {
        std::cerr<<"ERROR: Duplicate membership number!"<<std::endl;
        return false;
    }

    QSqlDatabase personnelDb = QSqlDatabase::database("personnelDb");
    QSqlQuery personnelQuery(personnelDb);

    personnelQuery.prepare("UPDATE Personnel SET LastName=:lastName, FirstName=:firstName, MembershipNumber=:newMmbNr, "
                           "Qualifications=:qualis, Status=:status "
                           "WHERE MembershipNumber=:mmbNr;");
    personnelQuery.bindValue(":lastName", pNewPerson.getLastName());
    personnelQuery.bindValue(":firstName", pNewPerson.getFirstName());
    personnelQuery.bindValue(":mmbNr", Person::extractMembershipNumber(pIdent));
    personnelQuery.bindValue(":newMmbNr", Person::extractMembershipNumber(pNewPerson.getIdent()));
    personnelQuery.bindValue(":qualis", pNewPerson.getQualifications().toString());
    personnelQuery.bindValue(":status", pNewPerson.getActive() ? 0 : 1);

    if (!personnelQuery.exec())
    {
        std::cerr<<"ERROR: Could not update person in personnel database!"<<std::endl;
        return false;
    }

    //Reload personnel to obtain new/changed row IDs

    personnelMap.clear();

    return loadPersonnel();
}

/*!
 * \brief Remove person from personnel cache and database.
 *
 * Removes person with identifier \p pIdent from the database and, if successful, clears and re-loads the personnel cache.
 *
 * Returns immediately, if database is read-only.
 *
 * \param pIdent Identifier of the person to remove.
 * \return If writing to database and re-loading personnel cache was successful.
 */
bool DatabaseCache::removePerson(QString pIdent)
{
    if (isReadOnly())
        return false;

    //Check identifier
    if (!personExists(pIdent))
    {
        std::cerr<<"ERROR: Cannot remove person. ID does not exist!"<<std::endl;
        return false;
    }

    QSqlDatabase personnelDb = QSqlDatabase::database("personnelDb");
    QSqlQuery personnelQuery(personnelDb);

    personnelQuery.prepare("DELETE FROM Personnel WHERE MembershipNumber=:mmbNr;");
    personnelQuery.bindValue(":mmbNr", Person::extractMembershipNumber(pIdent));

    if (!personnelQuery.exec())
    {
        std::cerr<<"ERROR: Could not remove person from personnel database!"<<std::endl;
        return false;
    }

    //Reload personnel to obtain new/changed row IDs

    personnelMap.clear();

    return loadPersonnel();
}

//Private

/*!
 * \brief Load all integer type settings from database into cache.
 *
 * Loads all settings from configuration database into settings cache that have integer type.
 *
 * Note: Does not clear the settings cache before. Values of existing settings are, however, simply overwritten.
 *
 * \return If reading from database was successful.
 */
bool DatabaseCache::loadIntSettings()
{
    QSqlDatabase configDb = QSqlDatabase::database("configDb");
    QSqlQuery configQuery(configDb);

    configQuery.prepare("SELECT Setting, ValueInt FROM Application WHERE Type=:type;");
    configQuery.bindValue(":type", 0);

    if (!configQuery.exec())
        return false;

    while (configQuery.next())
    {
        QString tSetting = configQuery.value("Setting").toString();
        int tValue = configQuery.value("ValueInt").toInt();
        settingsInt[tSetting] = tValue;
    }

    return true;
}

/*!
 * \brief Load all floating-point type settings from database into cache.
 *
 * Loads all settings from configuration database into settings cache that have floating-point type.
 *
 * Note: Does not clear the settings cache before. Values of existing settings are, however, simply overwritten.
 *
 * \return If reading from database was successful.
 */
bool DatabaseCache::loadDblSettings()
{
    QSqlDatabase configDb = QSqlDatabase::database("configDb");
    QSqlQuery configQuery(configDb);

    configQuery.prepare("SELECT Setting, ValueDbl FROM Application WHERE Type=:type;");
    configQuery.bindValue(":type", 1);

    if (!configQuery.exec())
        return false;

    while (configQuery.next())
    {
        QString tSetting = configQuery.value("Setting").toString();
        double tValue = configQuery.value("ValueDbl").toDouble();
        settingsDbl[tSetting] = tValue;
    }

    return true;
}

/*!
 * \brief Load all string type settings from database into cache.
 *
 * Loads all settings from configuration database into settings cache that have string type.
 *
 * Note: Does not clear the settings cache before. Values of existing settings are, however, simply overwritten.
 *
 * \return If reading from database was successful.
 */
bool DatabaseCache::loadStrSettings()
{
    QSqlDatabase configDb = QSqlDatabase::database("configDb");
    QSqlQuery configQuery(configDb);

    configQuery.prepare("SELECT Setting, ValueStr FROM Application WHERE Type=:type;");
    configQuery.bindValue(":type", 2);

    if (!configQuery.exec())
        return false;

    while (configQuery.next())
    {
        QString tSetting = configQuery.value("Setting").toString();
        QString tValue = configQuery.value("ValueStr").toString();
        settingsStr[tSetting] = tValue;
    }

    return true;
}

//

/*!
 * \brief Load all stations from database into cache.
 *
 * Skips stations that are wrongly formatted or duplicate (see checkStationFormat(), checkStationDuplicates()).
 *
 * Note: Does not clear the stations cache before. Stations with the same database row ID are simply overwritten.
 *
 * \return If reading from database was successful.
 */
bool DatabaseCache::loadStations()
{
    QSqlDatabase configDb = QSqlDatabase::database("configDb");
    QSqlQuery configQuery(configDb);

    configQuery.prepare("SELECT Location, Name, LocalGroup, DistrictAssociation, RadioCallName, RadioCallNameAlt, "
                        "rowid FROM Stations;");

    if (!configQuery.exec())
        return false;

    std::vector<Aux::Station> tStationList;

    while (configQuery.next())
    {
        Aux::Station tStation {configQuery.value("Location").toString(),
                               configQuery.value("Name").toString(),
                               configQuery.value("LocalGroup").toString(),
                               configQuery.value("DistrictAssociation").toString(),
                               configQuery.value("RadioCallName").toString(),
                               configQuery.value("RadioCallNameAlt").toString()};

        int tRowId = configQuery.value("rowid").toInt();

        if (!checkStationFormat(tStation))
        {
            std::cerr<<"WARNING: Wrongly formatted station record! Skip."<<std::endl;
            continue;
        }

        if (!checkStationDuplicates(tStation, tStationList, false))
        {
            std::cerr<<"WARNING: Duplicate station record! Skip."<<std::endl;
            continue;
        }

        tStationList.push_back(tStation);
        stationsMap[tRowId] = std::move(tStation);
    }

    return true;
}

/*!
 * \brief Load all boats from database into cache.
 *
 * Skips boats that are wrongly formatted or duplicate (see checkBoatFormat(), checkBoatDuplicates()).
 *
 * Note: Does not clear the boats cache before. Boats with the same database row ID are simply overwritten.
 *
 * \return If reading from database was successful.
 */
bool DatabaseCache::loadBoats()
{
    QSqlDatabase configDb = QSqlDatabase::database("configDb");
    QSqlQuery configQuery(configDb);

    configQuery.prepare("SELECT Name, Acronym, Type, FuelType, RadioCallName, RadioCallNameAlt, HomeStation, rowid "
                        "FROM Boats;");

    if (!configQuery.exec())
        return false;

    std::vector<Aux::Boat> tBoatList;

    while (configQuery.next())
    {
        Aux::Boat tBoat {configQuery.value("Name").toString(),
                         configQuery.value("Acronym").toString(),
                         configQuery.value("Type").toString(),
                         configQuery.value("FuelType").toString(),
                         configQuery.value("RadioCallName").toString(),
                         configQuery.value("RadioCallNameAlt").toString(),
                         configQuery.value("HomeStation").toString()};

        int tRowId = configQuery.value("rowid").toInt();

        if (!checkBoatFormat(tBoat))
        {
            std::cerr<<"WARNING: Wrongly formatted boat record! Skip."<<std::endl;
            continue;
        }

        if (!checkBoatDuplicates(tBoat, tBoatList, false))
        {
            std::cerr<<"WARNING: Duplicate boat record! Skip."<<std::endl;
            continue;
        }

        int tHomeStationId = 0;
        QString tName, tLocation;
        if (tBoat.homeStation != "" &&
            (!Aux::stationNameLocationFromIdent(tBoat.homeStation, tName, tLocation) ||
             !stationRowIdFromNameLocation(tName, tLocation, tHomeStationId)))
        {
            std::cerr<<"WARNING: Boat's home station not found in database! Skip."<<std::endl;
            continue;
        }

        tBoatList.push_back(tBoat);
        boatsMap[tRowId] = std::move(tBoat);
    }

    return true;
}

//

/*!
 * \brief Load all personnel from database into cache.
 *
 * Skips persons that are wrongly formatted or duplicate (see checkPersonFormat(), checkPersonnelDuplicates()).
 *
 * Note: Does not clear the personnel cache before. Persons with the same database row ID are skipped.
 *
 * \return If reading from database was successful.
 */
bool DatabaseCache::loadPersonnel()
{
    QSqlDatabase personnelDb = QSqlDatabase::database("personnelDb");
    QSqlQuery personnelQuery(personnelDb);

    personnelQuery.prepare("SELECT LastName, FirstName, MembershipNumber, Qualifications, Status, rowid FROM Personnel;");

    if (!personnelQuery.exec())
        return false;

    std::vector<Person> tPersonsList;

    while (personnelQuery.next())
    {
        Person tPerson(personnelQuery.value("LastName").toString(),
                       personnelQuery.value("FirstName").toString(),
                       Person::createInternalIdent(
                           personnelQuery.value("LastName").toString(),
                           personnelQuery.value("FirstName").toString(),
                           personnelQuery.value("MembershipNumber").toString()
                           ),
                       Person::Qualifications(
                           personnelQuery.value("Qualifications").toString()
                           ),
                       personnelQuery.value("Status").toInt() == 0
                       );

        int tRowId = personnelQuery.value("rowid").toInt();

        if (!checkPersonFormat(tPerson))
        {
            std::cerr<<"WARNING: Wrongly formatted person record! Skip."<<std::endl;
            continue;
        }

        if (!checkPersonnelDuplicates(tPerson))
        {
            std::cerr<<"WARNING: Duplicate person record! Skip."<<std::endl;
            continue;
        }

        tPersonsList.push_back(tPerson);
        personnelMap.insert({tRowId, std::move(tPerson)});
    }

    return true;
}

//

/*!
 * \brief Validate the station properties' formatting.
 *
 * Checks format of Aux::Station members using the proper validators from Aux.
 * Also checks that trimmed location/name matches location/name, i.e. there are no leading or trailing spaces.
 *
 * \param pStation Station to check.
 * \return If formatting is ok.
 */
bool DatabaseCache::checkStationFormat(Aux::Station pStation)
{
    int tmpInt = 0;
    if (Aux::locationsValidator.validate(pStation.location, tmpInt) != QValidator::State::Acceptable ||
        Aux::namesValidator.validate(pStation.name, tmpInt) != QValidator::State::Acceptable ||
        Aux::namesValidator.validate(pStation.localGroup, tmpInt) != QValidator::State::Acceptable ||
        Aux::namesValidator.validate(pStation.districtAssociation, tmpInt) != QValidator::State::Acceptable ||
        Aux::radioCallNamesValidator.validate(pStation.radioCallName, tmpInt) != QValidator::State::Acceptable ||
        Aux::radioCallNamesValidator.validate(pStation.radioCallNameAlt, tmpInt) != QValidator::State::Acceptable)
    {
        return false;
    }

    if (pStation.location.trimmed() != pStation.location || pStation.name.trimmed() != pStation.name)
        return false;

    return true;
}

/*!
 * \brief Validate the boat properties' formatting.
 *
 * Checks format of Aux::Boat members using the proper validators from Aux.
 * Also checks that trimmed name matches name, i.e. there are no leading or trailing spaces.
 *
 * \param pBoat Boat to check.
 * \return If formatting is ok.
 */
bool DatabaseCache::checkBoatFormat(Aux::Boat pBoat)
{
    int tmpInt = 0;
    if (Aux::namesValidator.validate(pBoat.name, tmpInt) != QValidator::State::Acceptable ||
        Aux::boatAcronymsValidator.validate(pBoat.acronym, tmpInt) == QValidator::State::Invalid || //State::Intermediate OK here
        Aux::namesValidator.validate(pBoat.type, tmpInt) != QValidator::State::Acceptable ||
        Aux::fuelTypesValidator.validate(pBoat.fuelType, tmpInt) != QValidator::State::Acceptable ||
        Aux::radioCallNamesValidator.validate(pBoat.radioCallName, tmpInt) != QValidator::State::Acceptable ||
        Aux::radioCallNamesValidator.validate(pBoat.radioCallNameAlt, tmpInt) != QValidator::State::Acceptable ||
    (pBoat.homeStation != "" && Aux::stationItentifiersValidator.validate(pBoat.homeStation, tmpInt) != QValidator::State::Acceptable))
    {
        return false;
    }

    if (pBoat.name.trimmed() != pBoat.name)
        return false;

    return true;
}

/*!
 * \brief Validate the person properties' formatting.
 *
 * Checks format of Person name and membership number (extracted from identifier) using the proper validators from Aux.
 * Also checks that trimmed first/last name matches first/last name, i.e. there are no leading or trailing spaces.
 *
 * \param pPerson Person to check.
 * \return If formatting is ok.
 */
bool DatabaseCache::checkPersonFormat(const Person& pPerson)
{
    QString lName = pPerson.getLastName();
    QString fName = pPerson.getFirstName();
    QString mmbNr = Person::extractMembershipNumber(pPerson.getIdent());

    int tmpInt = 0;
    if (Aux::personNamesValidator.validate(lName, tmpInt) != QValidator::State::Acceptable ||
        Aux::personNamesValidator.validate(fName, tmpInt) != QValidator::State::Acceptable ||
        Aux::membershipNumbersValidator.validate(mmbNr, tmpInt) != QValidator::State::Acceptable)
    {
        return false;
    }

    if (lName.trimmed() != lName || fName.trimmed() != fName)
        return false;

    return true;
}

/*!
 * \brief Check if there are no duplicate stations.
 *
 * Searches for stations in \p pStations with the same name and location as \p pStation.
 * Maximally one occurrence of \p pStation in \p pStations is allowed, if \p pOneAllowed is true, and zero else.
 * The function returns false, if there are more occurrences than allowed, and true else.
 *
 * \param pStation Station to search for.
 * \param pStations List of stations to search in for \p pStation.
 * \param pOneAllowed One occurrence of \p pStation allowed (zero else)?
 * \return If there are no occurrences of \p pStation in \p pStations (or maximally one, if \p pOneAllowed is true).
 */
bool DatabaseCache::checkStationDuplicates(const Aux::Station& pStation, const std::vector<Aux::Station>& pStations, bool pOneAllowed)
{
    int tmpCount = 0;
    for (const Aux::Station& tStation : pStations)
    {
        if (tStation.location == pStation.location && tStation.name == pStation.name)
            ++tmpCount;

        if (tmpCount > (pOneAllowed ? 1 : 0))
            break;
    }
    if (tmpCount > (pOneAllowed ? 1 : 0))
        return false;

    return true;
}

/*!
 * \brief Check if there are no duplicate boats.
 *
 * Searches for boats in \p pBoats with the same name as \p pBoat.
 * Maximally one occurrence of \p pBoat in \p pBoats is allowed, if \p pOneAllowed is true, and zero else.
 * The function returns false, if there are more occurrences than allowed, and true else.
 *
 * \param pBoat Boat to search for.
 * \param pBoats List of boats to search in for \p pBoat.
 * \param pOneAllowed One occurrence of \p pBoat allowed? Zero else.
 * \return If there are no occurrences of \p pBoat in \p pBoats (or maximally one, if \p pOneAllowed is true).
 */
bool DatabaseCache::checkBoatDuplicates(const Aux::Boat& pBoat, const std::vector<Aux::Boat>& pBoats, bool pOneAllowed)
{
    int tmpCount = 0;
    for (const Aux::Boat& tBoat : pBoats)
    {
        if (tBoat.name == pBoat.name)
            ++tmpCount;

        if (tmpCount > (pOneAllowed ? 1 : 0))
            break;
    }
    if (tmpCount > (pOneAllowed ? 1 : 0))
        return false;

    return true;
}

/*!
 * \brief Check if there are no duplicate persons.
 *
 * Searches the personnel cache for persons with the same membership number as \p pPerson.
 * The function returns false, if such a person is found, and true else.
 *
 * \param pPerson Person to search for in cache.
 * \return If no person with membership number of \p pPerson found.
 */
bool DatabaseCache::checkPersonnelDuplicates(const Person& pPerson)
{
    QString tMmbNr = Person::extractMembershipNumber(pPerson.getIdent());

    for (const auto& it : personnelMap)
        if (Person::extractMembershipNumber(it.second.getIdent()) == tMmbNr)
            return false;

    return true;
}
