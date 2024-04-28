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

#include "report.h"

#include "databasecache.h"
#include "qualificationchecker.h"

#include <QDateTime>
#include <QFile>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStringList>

#include <functional>
#include <iostream>
#include <list>
#include <stdexcept>
#include <tuple>

/*!
 * \brief Constructor.
 *
 * Creates an empty report with default settings (and an empty boat log with default settings as well).
 *
 * Possibly interesting non-trivial default values:
 * - Serial number set to 1.
 * - All times set to 00:00 and report date set to 01.01.2000.
 * - Duty purpose set to DutyPurpose::_WATCHKEEPING.
 * - Weather conditions set to:
 *  - Aux::Precipitation::_NONE
 *  - Aux::Cloudiness::_CLOUDLESS
 *  - Aux::WindStrength::_CALM
 *  - Aux::WindDirection::_UNKNOWN
 */
Report::Report() :
    fileName(""),
    number(1),
    station(""),
    radioCallName(""),
    comments(""),
    dutyPurpose(DutyPurpose::_WATCHKEEPING),
    dutyPurposeComment(""),
    date(QDate(2000, 1, 1)),
    begin(QTime(0, 0)),
    end(QTime(0, 0)),
    precipitation(Aux::Precipitation::_NONE),
    cloudiness(Aux::Cloudiness::_CLOUDLESS),
    windStrength(Aux::WindStrength::_CALM),
    windDirection(Aux::WindDirection::_UNKNOWN),
    temperatureAir(0),
    temperatureWater(0),
    weatherComments(""),
    operationProtocolsCtr(0),
    patientRecordsCtr(0),
    radioCallLogsCtr(0),
    otherEnclosures(""),
    personnelMinutesCarry(0),
    boatLogPtr(std::make_shared<BoatLog>()),
    assignmentNumber("")
{
    /*
     * Lambda expression to add \p pRescue to \p pRescuesCountMap.
     * To be executed for each available 'RescueOperation'.
     */
    auto tRescOpFunc = [](Report::RescueOperation pRescue, std::map<Report::RescueOperation, int>& pRescuesCountMap) -> void
    {
        pRescuesCountMap[pRescue] = 0;
    };

    //Add a counter for each available rescue operation type
    iterateRescueOperations(tRescOpFunc, rescueOperationsCounts);
}

//Public

/*!
 * \brief Reset to the state of a newly constructed report.
 *
 * Restores the report's initial empty state just as it is right after construction. See also Report().
 */
void Report::reset()
{
    *this = Report();
}

//

/*!
 * \brief Load report from file.
 *
 * Opens the report file \p pFileName as a JSON document (see also save())
 * and reads and sets all report data from this JSON document.
 *
 * Performs several integrity checks such as using validators from Aux to ensure correct formatting
 * of loaded values or checking that crew members from boat loag are also in report's personnel
 * and many other things. Returns false, if any problem is detected.
 *
 * If loading is successful, the report file name (see getFileName()) is set to \p pFileName.
 *
 * Note: All persons of report's personnel are saved with and loaded from report file, such that handling of these
 * persons will be independent of the personnel database unless they are removed from and added to the loaded report again.
 *
 * \param pFileName Path to the file to load the report from.
 * \return If successful.
 */
bool Report::open(const QString& pFileName)
{
    //First make sure that all maps etc. are empty
    reset();

    //Open file
    QFile file(pFileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        file.close();
        std::cerr<<"ERROR: Could not open file for reading!"<<std::endl;
        return false;
    }

    //Read JSON document from file
    QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!jsonDoc.isObject())
    {
        std::cerr<<"ERROR: Could not read report from file!"<<std::endl;
        return false;
    }

    //Main/top JSON object
    QJsonObject jsonObj = jsonDoc.object();

    //Check meta-information

    if (!jsonObj.contains("_magic") || !jsonObj.value("_magic").isString() || jsonObj.value("_magic").toString() != "prg:wd.mgr")
    {
        std::cerr<<"ERROR: File is not a report!"<<std::endl;
        return false;
    }

    bool tUseProgVersionFallback = false;
    QString tFileVerStr, tProgVerStr;

    if (!jsonObj.contains("_fileFormat") || !jsonObj.value("_fileFormat").isString())
    {
        std::cerr<<"WARNING: Could not determine report file format version! "
                   "Using saved program version to check file format compatibility instead."<<std::endl;

        tUseProgVersionFallback = true;
    }
    else
        tFileVerStr = jsonObj.value("_fileFormat").toString();

    if (!jsonObj.contains("_version") || !jsonObj.value("_version").isString())
    {
        if (tUseProgVersionFallback)
        {
            std::cerr<<"ERROR: Could not determine report's program version!"<<std::endl;
            return false;
        }
        else
            std::cerr<<"WARNING: Could not determine report's program version!"<<std::endl;
    }
    else
        tProgVerStr = jsonObj.value("_version").toString();

    //Use saved program version for compatibility check if actual file format version (which could be older) cannot be determined
    QString tVerStr = tFileVerStr;
    if (tUseProgVersionFallback)
        tVerStr = tProgVerStr;

    int tVerMaj = 0, tVerMin = 0, tVerPatch = 0;
    char tVerType = '-';

    int tmpInt = 0;
    if (Aux::programVersionsValidator.validate(tVerStr, tmpInt) != QValidator::State::Acceptable ||
        !Aux::parseProgramVersion(tVerStr, tVerMaj, tVerMin, tVerPatch, tVerType))
    {
        std::cerr<<"ERROR: Could not parse version string for file format compatibility check!"<<std::endl;
        return false;
    }

    //Check version, decide how to process file

    if (tVerStr != Aux::programVersionString)
    {
        if (Aux::compareProgramVersions(tVerMaj, tVerMin, tVerPatch, 1, 0, 0) < 0)
        {
            std::cerr<<"ERROR: Report was saved with incompatible, old program version ("<<tProgVerStr.toStdString()<<")!"<<std::endl;
            return false;
        }
        else if (Aux::compareProgramVersions(tVerMaj, tVerMin, tVerPatch,
                                             Aux::programVersionMajor, Aux::programVersionMinor, Aux::programVersionPatch) > 0)
        {
            std::cerr<<"ERROR: Report was saved with incompatible, newer program version ("<<tProgVerStr.toStdString()<<")!"<<std::endl;
            return false;
        }
    }

    //Need to convert qualifications format and add special personnel identifier handling below for file formats <= 1.4b.0;
    //"_fileFormat" property was added in 1.4.0 (after 1.4b.0), hence simply use 'tUseProgVersionFallback' to determine old format
    bool convertLegacyQualifications = tUseProgVersionFallback;

    //Need to change '_BG' boat function to '_RS' if qualifications do not contain "FA-WRD" since requirement changed from wrong
    //"DRSA-S" (<= v1.4b.0) to correct "FA-WRD" (>= v1.4.0); use 'tUseProgVersionFallback' to determine version (see above)
    bool adaptLegacyBoatFunctions = tUseProgVersionFallback;

    //Check, if the two main objects are present
    if (!jsonObj.contains("reportMain") || !jsonObj.value("reportMain").isObject())
    {
        std::cerr<<"ERROR: File does not contain a report!"<<std::endl;
        return false;
    }
    if (!jsonObj.contains("boatLog") || !jsonObj.value("boatLog").isObject())
    {
        std::cerr<<"ERROR: File does not contain a boat log!"<<std::endl;
        return false;
    }

    //Report data object
    QJsonObject reportObj = jsonObj.value("reportMain").toObject();

    number = reportObj.value("serialNumber").toInt(1);

    station = reportObj.value("stationIdent").toString("");

    //Check station identifier format
    if (station != "" && Aux::stationItentifiersValidator.validate(station, tmpInt) != QValidator::State::Acceptable)
    {
        std::cerr<<"ERROR: Wrong station identifier format!"<<std::endl;
        return false;
    }

    int tStationRowId = 0;
    QString tStationName, tStationLocation;

    //Print warning, if station not in database
    if (station != "" && (!Aux::stationNameLocationFromIdent(station, tStationName, tStationLocation) ||
                          !DatabaseCache::stationRowIdFromNameLocation(tStationName, tStationLocation, tStationRowId)))
    {
        std::cerr<<"WARNING: Could not find station in database!"<<std::endl;
    }

    radioCallName = reportObj.value("stationRadioCallName").toString("");

    //Check radio call name format
    if (radioCallName != "" && Aux::radioCallNamesValidator.validate(radioCallName, tmpInt) != QValidator::State::Acceptable)
    {
        std::cerr<<"ERROR: Wrong radio call name format!"<<std::endl;
        return false;
    }

    //Print warning, if radio call name does not match station's possible radio call names from database
    if (radioCallName != "" && station != "" &&
            DatabaseCache::stationRowIdFromNameLocation(tStationName, tStationLocation, tStationRowId))
    {
        Aux::Station tStation = DatabaseCache::stations().at(tStationRowId);

        if (radioCallName != tStation.radioCallName && radioCallName != tStation.radioCallNameAlt)
            std::cerr<<"WARNING: Radio call name does not match station!"<<std::endl;
    }

    comments = reportObj.value("generalComments").toString("");

    dutyPurpose = static_cast<DutyPurpose>(static_cast<int8_t>(reportObj.value("dutyPurpose").toInt(
                                                                   static_cast<int8_t>(DutyPurpose::_WATCHKEEPING)
                                                                   )));
    dutyPurposeComment = reportObj.value("dutyPurposeComment").toString("");

    //Use date, if valid
    if (!reportObj.contains("date") || !reportObj.value("date").isString() ||
        !QDate::fromString(reportObj.value("date").toString(), Qt::DateFormat::ISODate).isValid())
    {
        std::cerr<<"ERROR: Invalid date."<<std::endl;
        return false;
    }
    date = QDate::fromString(reportObj.value("date").toString(), Qt::DateFormat::ISODate);

    //Use begin time, if valid
    if (!reportObj.contains("beginTime") || !reportObj.value("beginTime").isString() ||
        !QTime::fromString(reportObj.value("beginTime").toString(), "hh:mm").isValid())
    {
        std::cerr<<"ERROR: Invalid time."<<std::endl;
        return false;
    }
    begin = QTime::fromString(reportObj.value("beginTime").toString(), "hh:mm");

    //Use end time, if valid
    if (!reportObj.contains("endTime") || !reportObj.value("endTime").isString() ||
        !QTime::fromString(reportObj.value("endTime").toString(), "hh:mm").isValid())
    {
        std::cerr<<"ERROR: Invalid time."<<std::endl;
        return false;
    }
    end = QTime::fromString(reportObj.value("endTime").toString(), "hh:mm");

    //Weather

    precipitation = static_cast<Aux::Precipitation>(static_cast<int8_t>(reportObj.value("precipitation").toInt(
                                                                            static_cast<int8_t>(Aux::Precipitation::_NONE)
                                                                            )));
    cloudiness = static_cast<Aux::Cloudiness>(static_cast<int8_t>(reportObj.value("cloudiness").toInt(
                                                                      static_cast<int8_t>(Aux::Cloudiness::_CLOUDLESS)
                                                                      )));
    windStrength = static_cast<Aux::WindStrength>(static_cast<int8_t>(reportObj.value("windStrength").toInt(
                                                                          static_cast<int8_t>(Aux::WindStrength::_CALM)
                                                                          )));
    windDirection = static_cast<Aux::WindDirection>(static_cast<int8_t>(reportObj.value("windDirection").toInt(
                                                                          static_cast<int8_t>(Aux::WindDirection::_UNKNOWN)
                                                                          )));

    temperatureAir = reportObj.value("airTemp").toInt(0);
    temperatureWater = reportObj.value("waterTemp").toInt(0);

    weatherComments = reportObj.value("weatherComments").toString("");

    //Enclosures
    operationProtocolsCtr = reportObj.value("numEnclOperationProtocols").toInt(0);
    patientRecordsCtr = reportObj.value("numEnclPatientRecords").toInt(0);
    radioCallLogsCtr = reportObj.value("numEnclRadioCallLogs").toInt(0);
    otherEnclosures = reportObj.value("otherEnclosures").toString("");

    //Numbers of carried out rescue operations

    if (!reportObj.contains("rescueOperations") || !reportObj.value("rescueOperations").isObject())
    {
        std::cerr<<"ERROR: Report does not contain information about rescue operations!"<<std::endl;
        return false;
    }

    QJsonObject rescueOperationsObj = reportObj.value("rescueOperations").toObject();

    bool tGood = false;
    QStringList tKeys = rescueOperationsObj.keys();

    for (int i = 0; i < tKeys.size(); ++i)
    {
        RescueOperation tRescue = static_cast<RescueOperation>(static_cast<int8_t>(tKeys.at(i).toInt(&tGood)));

        if (!tGood)
        {
            std::cerr<<"ERROR: Unknown type of rescue operation!"<<std::endl;
            return false;
        }

        rescueOperationsCounts[tRescue] = rescueOperationsObj.value(tKeys.at(i)).toInt(0);
    }

    int tTotNumRescues = 0;

    for (auto it : rescueOperationsCounts)
    {
        if (it.first == RescueOperation::_MORTAL_DANGER_INVOLVED)
            continue;

        tTotNumRescues += it.second;
    }

    if (rescueOperationsCounts[RescueOperation::_MORTAL_DANGER_INVOLVED] > tTotNumRescues)
        std::cerr<<"WARNING: Number of rescue operations involving mortal danger exceeds total number of rescue operations!"<<std::endl;

    //Assignment number from rescue directing center

    assignmentNumber = reportObj.value("assignmentNumber").toString("");

    if (assignmentNumber != "" &&
        Aux::assignmentNumbersValidator.validate(assignmentNumber, tmpInt) != QValidator::State::Acceptable)
    {
        std::cerr<<"ERROR: Wrong assignment number format!"<<std::endl;
        return false;
    }

    //List of used resources with their begin/end of use times

    if ((Aux::compareProgramVersions(tVerMaj, tVerMin, tVerPatch, 1, 5, 0) < 0 &&
         (!reportObj.contains("vehicles") || !reportObj.value("vehicles").isObject())) ||
        (Aux::compareProgramVersions(tVerMaj, tVerMin, tVerPatch, 1, 5, 0) >= 0 &&
         (!reportObj.contains("resources") || !reportObj.value("resources").isObject())))
    {
        if (Aux::compareProgramVersions(tVerMaj, tVerMin, tVerPatch, 1, 1, 0) < 0)
        {
            std::cerr<<"WARNING: Report was saved with a program version before 1.1.0 "
                       "and does not contain a list of present vehicles. Ignore."<<std::endl;
        }
        else if (Aux::compareProgramVersions(tVerMaj, tVerMin, tVerPatch, 1, 5, 0) < 0)
        {
            std::cerr<<"ERROR: Report does not contain list of present vehicles!"<<std::endl;
            return false;
        }
        else
        {
            std::cerr<<"ERROR: Report does not contain list of used resources!"<<std::endl;
            return false;
        }
    }
    else
    {
        bool useOldVehicles = (Aux::compareProgramVersions(tVerMaj, tVerMin, tVerPatch, 1, 5, 0) < 0);

        QJsonObject resourcesObj = reportObj.value(useOldVehicles ? "vehicles" : "resources").toObject();

        if (!resourcesObj.contains(useOldVehicles ? "vehiclesList" : "resourcesList") ||
                !resourcesObj.value(useOldVehicles ? "vehiclesList" : "resourcesList").isArray())
        {
            if (useOldVehicles)
                std::cerr<<"ERROR: Broken vehicles list!"<<std::endl;
            else
                std::cerr<<"ERROR: Broken resources list!"<<std::endl;
            return false;
        }

        QJsonArray resourcesArray = resourcesObj.value(useOldVehicles ? "vehiclesList" : "resourcesList").toArray();

        //Fill resources list
        for (QJsonArray::iterator it = resourcesArray.begin(); it != resourcesArray.end(); ++it)
        {
            if (!(*it).isObject())
            {
                if (useOldVehicles)
                    std::cerr<<"ERROR: Broken vehicle entry!"<<std::endl;
                else
                    std::cerr<<"ERROR: Broken resource entry!"<<std::endl;
                return false;
            }

            QJsonObject resourceObj = (*it).toObject();

            QString tName = resourceObj.value("radioCallName").toString("");
            QTime tBegin = QTime::fromString(resourceObj.value(useOldVehicles ? "arrive" : "begin").toString(), "hh:mm");
            QTime tEnd = QTime::fromString(resourceObj.value(useOldVehicles ? "leave" : "end").toString(), "hh:mm");

            //Check formatting
            if (Aux::radioCallNamesValidator.validate(tName, tmpInt) != QValidator::State::Acceptable || tName.trimmed() != tName ||
                !tBegin.isValid() || !tEnd.isValid())
            {
                if (useOldVehicles)
                    std::cerr<<"ERROR: Wrong vehicle data formatting!"<<std::endl;
                else
                    std::cerr<<"ERROR: Wrong resource data formatting!"<<std::endl;
                return false;
            }

            resources.push_back({tName, {tBegin, tEnd}});
        }
    }

    //Personnel

    personnelMinutesCarry = reportObj.value("personnelMinutesCarry").toInt(0);

    //Load archived personnel data in order to be independent of the personnel database

    if (!reportObj.contains("personnelData") || !reportObj.value("personnelData").isObject())
    {
        std::cerr<<"ERROR: Report does not contain personnel archive!"<<std::endl;
        return false;
    }

    QJsonObject intExtPersonnelObj = reportObj.value("personnelData").toObject();

    if (!intExtPersonnelObj.contains("intPersonnel") || !intExtPersonnelObj.value("intPersonnel").isArray() ||
        !intExtPersonnelObj.contains("extPersonnel") || !intExtPersonnelObj.value("extPersonnel").isArray())
    {
        std::cerr<<"ERROR: Broken report personnel archive!"<<std::endl;
        return false;
    }

    QJsonArray internalPersonnelArray = intExtPersonnelObj.value("intPersonnel").toArray();
    QJsonArray externalPersonnelArray = intExtPersonnelObj.value("extPersonnel").toArray();

    //Add internal personnel data
    for (QJsonArray::iterator it = internalPersonnelArray.begin(); it != internalPersonnelArray.end(); ++it)
    {
        if (!(*it).isObject())
        {
            std::cerr<<"ERROR: Broken person entry!"<<std::endl;
            return false;
        }

        QJsonObject personObj = (*it).toObject();

        QString tLastName = personObj.value("lastName").toString("");
        QString tFirstName = personObj.value("firstName").toString("");
        QString tQualifications = personObj.value("qualis").toString("");
        QString tMembershipNumber = personObj.value("memberNr").toString("");

        //Check formatting
        if (Aux::personNamesValidator.validate(tLastName, tmpInt) != QValidator::State::Acceptable ||
            Aux::personNamesValidator.validate(tFirstName, tmpInt) != QValidator::State::Acceptable ||
            Aux::membershipNumbersValidator.validate(tMembershipNumber, tmpInt) != QValidator::State::Acceptable ||
            tLastName.trimmed() != tLastName ||
            tFirstName.trimmed() != tFirstName ||
            tQualifications.trimmed() != tQualifications ||
            tMembershipNumber.trimmed() != tMembershipNumber)
        {
            std::cerr<<"ERROR: Wrong person data formatting! Skip.!"<<std::endl;
            return false;
        }

        //Need to convert qualifications first if loading report that was saved with version < 1.4.0
        if (convertLegacyQualifications)
            tQualifications = Person::Qualifications::convertLegacyQualifications(tQualifications);

        //Assume person state always active, if part of personnel of a saved report
        bool tActive = true;

        //Add new person to personnel archive
        addPersonnel(Person(tLastName, tFirstName, Person::createInternalIdent(tLastName, tFirstName, tMembershipNumber),
                            Person::Qualifications(tQualifications), tActive));
    }

    //When loading file format < 1.4.0, need to map legacy external identifiers to actually used new identifiers
    std::map<QString, QString> extIdentsViaLegacyIdents;

    //Add external personnel data
    for (QJsonArray::iterator it = externalPersonnelArray.begin(); it != externalPersonnelArray.end(); ++it)
    {
        if (!(*it).isObject())
        {
            std::cerr<<"ERROR: Broken person entry!"<<std::endl;
            return false;
        }

        QJsonObject personObj = (*it).toObject();

        QString tLastName = personObj.value("lastName").toString("");
        QString tFirstName = personObj.value("firstName").toString("");
        QString tQualifications = personObj.value("qualis").toString("");
        QString tIdentSuffix = personObj.value("identSuffix").toString("");

        //Check formatting
        if (Aux::personNamesValidator.validate(tLastName, tmpInt) != QValidator::State::Acceptable ||
            Aux::personNamesValidator.validate(tFirstName, tmpInt) != QValidator::State::Acceptable ||
            Aux::extIdentSuffixesValidator.validate(tIdentSuffix, tmpInt) != QValidator::State::Acceptable ||
            tLastName.trimmed() != tLastName ||
            tFirstName.trimmed() != tFirstName ||
            tQualifications.trimmed() != tQualifications)
        {
            std::cerr<<"ERROR: Wrong person data formatting!"<<std::endl;
            return false;
        }

        //Need to convert qualifications first if loading report that was saved with version < 1.4.0;
        //also must map personnel identifiers based on original qualifications vs. based on converted qualifications
        if (convertLegacyQualifications)
        {
            QString tLegacyIdent = Person::createLegacyExternalIdent(tLastName, tFirstName, tQualifications, tIdentSuffix);

            tQualifications = Person::Qualifications::convertLegacyQualifications(tQualifications);

            QString tNewIdent = Person::createExternalIdent(tLastName, tFirstName,
                                                            Person::Qualifications(tQualifications), tIdentSuffix);

            extIdentsViaLegacyIdents[tLegacyIdent] = tNewIdent;
        }

        //Assume person state always active, if part of personnel of a saved report
        bool tActive = true;

        //Add new person to personnel archive
        addPersonnel(Person(tLastName, tFirstName,
                            Person::createExternalIdent(tLastName, tFirstName, Person::Qualifications(tQualifications), tIdentSuffix),
                            Person::Qualifications(tQualifications), tActive));
    }

    //Load functions and times of present personnel

    if (!reportObj.contains("personnelList") || !reportObj.value("personnelList").isObject())
    {
        std::cerr<<"ERROR: Report does not contain personnel list!"<<std::endl;
        return false;
    }
    QJsonObject personnelObj = reportObj.value("personnelList").toObject();

    if (!personnelObj.contains("personnel") || !personnelObj.value("personnel").isArray())
    {
        std::cerr<<"ERROR: Broken report personnel list!"<<std::endl;
        return false;
    }
    QJsonArray personnelArray = personnelObj.value("personnel").toArray();

    for (QJsonArray::iterator it = personnelArray.begin(); it != personnelArray.end(); ++it)
    {
        if (!(*it).isObject())
        {
            std::cerr<<"ERROR: Broken person entry!"<<std::endl;
            return false;
        }

        QJsonObject personObj = (*it).toObject();

        QString tIdent = personObj.value("ident").toString("");

        //When loading file format < 1.4.0, use above mapped external identifier instead of legacy external identifier
        if (convertLegacyQualifications && Person::isExternalIdent(tIdent))
            tIdent = extIdentsViaLegacyIdents[tIdent];

        //Person must be part of previously loaded personnel archive
        if (!personnelExists(tIdent))
        {
            std::cerr<<"ERROR: Person not in personnel archive!"<<std::endl;
            return false;
        }

        Person::Function tFunction = static_cast<Person::Function>(static_cast<int8_t>(
                                                                       personObj.value("function").toInt(
                                                                           static_cast<int8_t>(Person::Function::_OTHER))));

        if (!personObj.contains("arrive") || !personObj.value("arrive").isString() ||
            !QTime::fromString(personObj.value("arrive").toString(), "hh:mm").isValid())
        {
            std::cerr<<"ERROR: Invalid time."<<std::endl;
            return false;
        }
        QTime tBeginTime = QTime::fromString(personObj.value("arrive").toString(), "hh:mm");

        if (!personObj.contains("leave") || !personObj.value("leave").isString() ||
            !QTime::fromString(personObj.value("leave").toString(), "hh:mm").isValid())
        {
            std::cerr<<"ERROR: Invalid time."<<std::endl;
            return false;
        }
        QTime tEndTime = QTime::fromString(personObj.value("leave").toString(), "hh:mm");

        const Person& tPerson = getIntOrExtPersonnel(tIdent);

        //Person must be qualified for stated function
        if (!QualificationChecker::checkPersonnelFunction(tFunction, tPerson.getQualifications()))
        {
            std::cerr<<"ERROR: Insufficient qualification for personnel function!"<<std::endl;
            return false;
        }

        addPersonFunctionTimes(tIdent, tFunction, tBeginTime, tEndTime);
    }

    //Boat log data object
    QJsonObject boatObj = jsonObj.value("boatLog").toObject();

    QString tBoatName = boatObj.value("boatName").toString("");

    //Check boat name format
    if (tBoatName != "" && Aux::namesValidator.validate(tBoatName, tmpInt) != QValidator::State::Acceptable)
    {
        std::cerr<<"ERROR: Wrong boat name format!"<<std::endl;
        return false;
    }

    //Print warning, if boat not in database
    int tBoatRowId = 0;
    if (tBoatName != "" && !DatabaseCache::boatRowIdFromName(tBoatName, tBoatRowId))
        std::cerr<<"WARNING: Could not find boat in database!"<<std::endl;

    QString tBoatRadioCallName = boatObj.value("boatRadioCallName").toString("");

    //Check radio call name format
    if (tBoatRadioCallName != "" &&
        Aux::radioCallNamesValidator.validate(tBoatRadioCallName, tmpInt) != QValidator::State::Acceptable)
    {
        std::cerr<<"ERROR: Wrong radio call name format!"<<std::endl;
        return false;
    }

    //Print warning, if radio call name does not match boat's possible radio call names from database
    if (tBoatRadioCallName != "" && tBoatName != "" && DatabaseCache::boatRowIdFromName(tBoatName, tBoatRowId))
    {
        Aux::Boat tBoat = DatabaseCache::boats().at(tBoatRowId);

        if (tBoatRadioCallName != tBoat.radioCallName && tBoatRadioCallName != tBoat.radioCallNameAlt)
            std::cerr<<"WARNING: Radio call name does not match boat!"<<std::endl;
    }

    boatLogPtr->setBoat(tBoatName);
    boatLogPtr->setRadioCallName(tBoatRadioCallName);

    boatLogPtr->setComments(boatObj.value("generalComments").toString(""));

    boatLogPtr->setSlippedInitial(boatObj.value("slippedInitial").toBool(false));
    boatLogPtr->setSlippedFinal(boatObj.value("slippedFinal").toBool(false));

    if (!boatObj.contains("readyFrom") || !boatObj.contains("readyUntil") ||
        !boatObj.value("readyFrom").isString() || !boatObj.value("readyUntil").isString() ||
        !QTime::fromString(boatObj.value("readyFrom").toString(), "hh:mm").isValid() ||
        !QTime::fromString(boatObj.value("readyUntil").toString(), "hh:mm").isValid())
    {
        std::cerr<<"ERROR: Invalid time!"<<std::endl;
        return false;
    }

    boatLogPtr->setReadyFrom(QTime::fromString(boatObj.value("readyFrom").toString(), "hh:mm"));
    boatLogPtr->setReadyUntil(QTime::fromString(boatObj.value("readyUntil").toString(), "hh:mm"));

    boatLogPtr->setEngineHoursInitial(boatObj.value("engineHoursInitial").toDouble(0));
    boatLogPtr->setEngineHoursFinal(boatObj.value("engineHoursFinal").toDouble(0));

    boatLogPtr->setFuelInitial(boatObj.value("addedFuelInitial").toInt(0));
    boatLogPtr->setFuelFinal(boatObj.value("addedFuelFinal").toInt(0));

    //Boat drives

    boatLogPtr->setBoatMinutesCarry(boatObj.value("boatDriveMinutesCarry").toInt(0));

    if (!boatObj.contains("boatDrives") || !boatObj.value("boatDrives").isObject())
    {
        std::cerr<<"ERROR: Report does not contain a boat drives list!"<<std::endl;
        return false;
    }
    QJsonObject drivesObj = boatObj.value("boatDrives").toObject();

    if (!drivesObj.contains("drives") || !drivesObj.value("drives").isArray())
    {
        std::cerr<<"ERROR: Broken report boat drives list!"<<std::endl;
        return false;
    }
    QJsonArray drivesArray = drivesObj.value("drives").toArray();

    int driveIdx = 0;
    for (QJsonArray::iterator it = drivesArray.begin(); it != drivesArray.end(); ++it)
    {
        if (!(*it).isObject())
        {
            std::cerr<<"ERROR: Broken boat drive entry!"<<std::endl;
            return false;
        }

        QJsonObject driveObj = (*it).toObject();

        BoatDrive tDrive;

        tDrive.setPurpose(driveObj.value("purpose").toString(""));
        tDrive.setComments(driveObj.value("comments").toString(""));

        if (!driveObj.contains("beginTime") || !driveObj.value("beginTime").isString() ||
            !driveObj.contains("endTime") || !driveObj.value("endTime").isString() ||
            !QTime::fromString(driveObj.value("beginTime").toString(), "hh:mm").isValid() ||
            !QTime::fromString(driveObj.value("endTime").toString(), "hh:mm").isValid())
        {
            std::cerr<<"EROR: Invalid time!"<<std::endl;
            return false;
        }

        tDrive.setBeginTime(QTime::fromString(driveObj.value("beginTime").toString(), "hh:mm"));
        tDrive.setEndTime(QTime::fromString(driveObj.value("endTime").toString(), "hh:mm"));

        tDrive.setFuel(driveObj.value("addedFuel").toInt(0));

        QString tBoatmanIdent = driveObj.value("boatmanIdent").toString("");

        //Check, if boatman exists in personnel list and if qualifications are sufficient
        if (tBoatmanIdent != "")
        {
            //When loading file format < 1.4.0, use previously mapped external identifier instead of legacy external identifier
            if (convertLegacyQualifications && Person::isExternalIdent(tBoatmanIdent))
                tBoatmanIdent = extIdentsViaLegacyIdents[tBoatmanIdent];

            if (!personInPersonnel(tBoatmanIdent))
            {
                std::cerr<<"ERROR: Boatman not in personnel list!"<<std::endl;
                return false;
            }
            if (!QualificationChecker::checkBoatman(getIntOrExtPersonnel(tBoatmanIdent).getQualifications()))
            {
                std::cerr<<"ERROR: Insufficient qualification for boatman!"<<std::endl;
                return false;
            }
        }

        tDrive.setBoatman(tBoatmanIdent);

        //Boat crew members

        if (!driveObj.contains("boatCrew") || !driveObj.value("boatCrew").isObject())
        {
            std::cerr<<"ERROR: Boat drive does not contain a crew member list!"<<std::endl;
            return false;
        }
        QJsonObject crewObj = driveObj.value("boatCrew").toObject();

        if (!crewObj.contains("crew") || !crewObj.value("crew").isArray())
        {
            std::cerr<<"ERROR: Broken boat drive crew member list!"<<std::endl;
            return false;
        }
        QJsonArray crewArray = crewObj.value("crew").toArray();

        for (QJsonArray::iterator it2 = crewArray.begin(); it2 != crewArray.end(); ++it2)
        {
            if (!(*it2).isObject())
            {
                std::cerr<<"ERROR: Broken crew member entry!"<<std::endl;
                return false;
            }

            QJsonObject crewMemberObj = (*it2).toObject();

            QString tIdent = crewMemberObj.value("crewMemberIdent").toString("");

            Person::BoatFunction tBoatFunction = static_cast<Person::BoatFunction>(
                                                     static_cast<int8_t>(crewMemberObj.value("crewMemberFunction").toInt(
                                                                             static_cast<int8_t>(Person::BoatFunction::_OTHER)
                                                                             )));

            //When loading file format < 1.4.0, use previously mapped external identifier instead of legacy external identifier
            if (convertLegacyQualifications && Person::isExternalIdent(tIdent))
                tIdent = extIdentsViaLegacyIdents[tIdent];

            if (!Person::isOtherIdent(tIdent))
            {
                //Check, if crew member exists in personnel list and if qualifications are sufficient
                if (!personInPersonnel(tIdent))
                {
                    std::cerr<<"ERROR: Crew member not in personnel list!"<<std::endl;
                    return false;
                }
                if (!QualificationChecker::checkBoatFunction(tBoatFunction, getIntOrExtPersonnel(tIdent).getQualifications()))
                {
                    //Automatically fix boat function if failing check is due to too lax qualification requirement in versions < 1.4.0
                    if (adaptLegacyBoatFunctions && tBoatFunction == Person::BoatFunction::_BG &&
                        QualificationChecker::checkBoatFunction(Person::BoatFunction::_RS,
                                                                getIntOrExtPersonnel(tIdent).getQualifications()))
                    {
                        tBoatFunction = Person::BoatFunction::_RS;
                        std::cerr<<"WARNING: Changed boat function from \"BG\" to \"RS\" due to insufficient qualification!"<<std::endl;
                    }
                    else
                    {
                        std::cerr<<"ERROR: Insufficient qualification for boat function!"<<std::endl;
                        return false;
                    }
                }

                tDrive.addCrewMember(tIdent, tBoatFunction);
            }
            else
            {
                QString tLastName = crewMemberObj.value("crewMemberLastName").toString("");
                QString tFirstName = crewMemberObj.value("crewMemberFirstName").toString("");
                QString tIdentSuffix = Person::extractExtSuffix(tIdent);

                //Check formatting
                if (Aux::personNamesValidator.validate(tLastName, tmpInt) != QValidator::State::Acceptable ||
                    Aux::personNamesValidator.validate(tFirstName, tmpInt) != QValidator::State::Acceptable ||
                    Aux::extIdentSuffixesValidator.validate(tIdentSuffix, tmpInt) != QValidator::State::Acceptable ||
                    tLastName.trimmed() != tLastName ||
                    tFirstName.trimmed() != tFirstName)
                {
                    std::cerr<<"ERROR: Wrong external crew member data formatting!"<<std::endl;
                    return false;
                }

                if (Person::createOtherIdent(tLastName, tFirstName, tIdentSuffix) != tIdent)
                {
                    std::cerr<<"ERROR: External crew member name does not match identifier!"<<std::endl;
                    return false;
                }

                tDrive.addExtCrewMember(tIdent, Person::BoatFunction::_EXT, tLastName, tFirstName);
            }
        }

        bool tNoCrewConfirmed = (tDrive.crewSize() == 0 && crewObj.value("noCrewConfirmed").toBool(false));
        tDrive.setNoCrewConfirmed(tNoCrewConfirmed);

        boatLogPtr->addDrive(driveIdx++, std::move(tDrive));
    }

    fileName = pFileName;

    return true;
}

/*!
 * \brief Save report to file.
 *
 * Saves all report data to the file \p pFileName as a JSON document.
 * If writing to file is successful, the report file name (see getFileName()) is set to \p pFileName.
 * The report file name will not be changed, if \p pTempFile is true.
 *
 * Approximate document structure:
 * - Main object
 *  - META
 *  - %Report
 *   - Personnel
 *    - ...
 *   - ...
 *  - Boat log
 *   - Drives
 *    - ...
 *   - ...
 *
 * Note: All persons of report's personnel will also be saved in the report file, such that in a loaded report
 * (see open()) handling of these persons can/will be independent of the personnel database unless they are
 * removed from and added to the loaded report again.
 *
 * \param pFileName Path to the file to write the report to.
 * \param pTempFile Do not change the report instance's file name.
 * \return If successful.
 */
bool Report::save(const QString& pFileName, const bool pTempFile)
{
    //Open file
    QSaveFile file(pFileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        std::cerr<<"ERROR: Could not open file for writing!"<<std::endl;
        return false;
    }

    //Report data object (separate object for boat log below)
    QJsonObject reportObj;

    reportObj.insert("serialNumber", number);

    reportObj.insert("stationIdent", station);
    reportObj.insert("stationRadioCallName", radioCallName);

    reportObj.insert("generalComments", comments);

    reportObj.insert("dutyPurpose", static_cast<int8_t>(dutyPurpose));
    reportObj.insert("dutyPurposeComment", dutyPurposeComment);

    reportObj.insert("date", date.toString(Qt::DateFormat::ISODate));
    reportObj.insert("beginTime", begin.toString("hh:mm"));
    reportObj.insert("endTime", end.toString("hh:mm"));

    reportObj.insert("precipitation", static_cast<int8_t>(precipitation));
    reportObj.insert("cloudiness", static_cast<int8_t>(cloudiness));
    reportObj.insert("windStrength", static_cast<int8_t>(windStrength));
    reportObj.insert("windDirection", static_cast<int8_t>(windDirection));

    reportObj.insert("airTemp", temperatureAir);
    reportObj.insert("waterTemp", temperatureWater);

    reportObj.insert("weatherComments", weatherComments);

    reportObj.insert("numEnclOperationProtocols", operationProtocolsCtr);
    reportObj.insert("numEnclPatientRecords", patientRecordsCtr);
    reportObj.insert("numEnclRadioCallLogs", radioCallLogsCtr);
    reportObj.insert("otherEnclosures", otherEnclosures);

    QJsonObject rescueOperationsObj;
    for (const auto& it : rescueOperationsCounts)
        rescueOperationsObj.insert(QString::number(static_cast<int8_t>(it.first)), it.second);
    reportObj.insert("rescueOperations", rescueOperationsObj);

    reportObj.insert("assignmentNumber", assignmentNumber);

    QJsonArray resourcesArray;
    for (const auto& it : resources)
    {
        QJsonObject resourceObj;

        resourceObj.insert("radioCallName", it.first);
        resourceObj.insert("begin", it.second.first.toString("hh:mm"));
        resourceObj.insert("end", it.second.second.toString("hh:mm"));

        resourcesArray.append(resourceObj);
    }
    QJsonObject resourcesObj;
    resourcesObj.insert("resourcesList", resourcesArray);
    reportObj.insert("resources", resourcesObj);

    reportObj.insert("personnelMinutesCarry", personnelMinutesCarry);

    //Store internal personnel data in report to be independent of future personnel (database) changes

    QJsonArray internalPersonnelArray;

    for (const auto& it : internalPersonnelMap)
    {
        const Person& tPerson(it.second);

        QJsonObject personObj;
        personObj.insert("lastName", tPerson.getLastName());
        personObj.insert("firstName", tPerson.getFirstName());
        personObj.insert("qualis", tPerson.getQualifications().toString());
        personObj.insert("memberNr", Person::extractMembershipNumber(tPerson.getIdent()));

        internalPersonnelArray.append(personObj);
    }

    //Also have to store external personnel, since this is not in the database at all

    QJsonArray externalPersonnelArray;

    for (const auto& it : externalPersonnelMap)
    {
        const Person& tPerson(it.second);

        QJsonObject personObj;
        personObj.insert("lastName", tPerson.getLastName());
        personObj.insert("firstName", tPerson.getFirstName());
        personObj.insert("qualis", tPerson.getQualifications().toString());
        personObj.insert("identSuffix", Person::extractExtSuffix(tPerson.getIdent()));

        externalPersonnelArray.append(personObj);
    }

    //Group internal and external personnel arrays and add to report object
    QJsonObject intExtPersonnelObj;
    intExtPersonnelObj.insert("intPersonnel", internalPersonnelArray);
    intExtPersonnelObj.insert("extPersonnel", externalPersonnelArray);
    reportObj.insert("personnelData", intExtPersonnelObj);

    //Store personnel functions and times separately from actual person data

    QJsonArray personnelArray;

    for (const auto& it : personnelFunctionTimesMap)
    {
        const QString& tIdent = it.first;
        Person::Function tFunction = it.second.first;
        QTime tBeginTime(it.second.second.first);
        QTime tEndTime(it.second.second.second);

        QJsonObject personObj;
        personObj.insert("ident", tIdent);
        personObj.insert("function", static_cast<int8_t>(tFunction));
        personObj.insert("arrive", tBeginTime.toString("hh:mm"));
        personObj.insert("leave", tEndTime.toString("hh:mm"));

        personnelArray.append(personObj);
    }

    QJsonObject personnelObj;
    personnelObj.insert("personnel", personnelArray);
    reportObj.insert("personnelList", personnelObj);

    //Boat log data object
    QJsonObject boatObj;

    boatObj.insert("boatName", boatLogPtr->getBoat());
    boatObj.insert("boatRadioCallName", boatLogPtr->getRadioCallName());

    boatObj.insert("generalComments", boatLogPtr->getComments());

    boatObj.insert("slippedInitial", boatLogPtr->getSlippedInitial());
    boatObj.insert("slippedFinal", boatLogPtr->getSlippedFinal());

    boatObj.insert("readyFrom", boatLogPtr->getReadyFrom().toString("hh:mm"));
    boatObj.insert("readyUntil", boatLogPtr->getReadyUntil().toString("hh:mm"));

    boatObj.insert("engineHoursInitial", boatLogPtr->getEngineHoursInitial());
    boatObj.insert("engineHoursFinal", boatLogPtr->getEngineHoursFinal());

    boatObj.insert("addedFuelInitial", boatLogPtr->getFuelInitial());
    boatObj.insert("addedFuelFinal", boatLogPtr->getFuelFinal());

    boatObj.insert("boatDriveMinutesCarry", boatLogPtr->getBoatMinutesCarry());

    //Store all boat drives in an array

    QJsonArray drivesArray;

    for (const BoatDrive& tDrive : boatLogPtr->getDrives())
    {
        QJsonObject driveObj;

        driveObj.insert("purpose", tDrive.getPurpose());
        driveObj.insert("comments", tDrive.getComments());

        driveObj.insert("beginTime", tDrive.getBeginTime().toString("hh:mm"));
        driveObj.insert("endTime", tDrive.getEndTime().toString("hh:mm"));

        driveObj.insert("addedFuel", tDrive.getFuel());

        driveObj.insert("boatmanIdent", tDrive.getBoatman());

        //Boat crew members

        QJsonArray crewArray;

        for (const auto& it : tDrive.crew())
        {
            const QString& tIdent = it.first;
            Person::BoatFunction tBoatFunction = it.second;

            QJsonObject crewMemberObj;
            crewMemberObj.insert("crewMemberIdent", tIdent);
            crewMemberObj.insert("crewMemberFunction", static_cast<int8_t>(tBoatFunction));

            //Need to store the name as well in case of an external crew member
            if (Person::isOtherIdent(tIdent))
            {
                QString tLastName, tFirstName;
                tDrive.getExtCrewMemberName(tIdent, tLastName, tFirstName);

                crewMemberObj.insert("crewMemberLastName", tLastName);
                crewMemberObj.insert("crewMemberFirstName", tFirstName);
            }

            crewArray.append(crewMemberObj);
        }

        QJsonObject crewObj;
        crewObj.insert("crew", crewArray);
        crewObj.insert("noCrewConfirmed", tDrive.getNoCrewConfirmed());
        driveObj.insert("boatCrew", crewObj);

        drivesArray.append(driveObj);
    }

    QJsonObject drivesObj;
    drivesObj.insert("drives", drivesArray);
    boatObj.insert("boatDrives", drivesObj);

    //Create main/top JSON object
    QJsonObject jsonObj;

    //Add meta information
    jsonObj.insert("_magic", "prg:wd.mgr");
    jsonObj.insert("_version", Aux::programVersionString);
    jsonObj.insert("_fileFormat", Aux::fileFormatVersionString);
    jsonObj.insert("_timestamp", QDateTime::currentDateTimeUtc().toString(Qt::DateFormat::ISODate));

    //Add main content
    jsonObj.insert("reportMain", reportObj);
    jsonObj.insert("boatLog", boatObj);

    //Create JSON document and write it to the file
    QJsonDocument jsonDoc;
    jsonDoc.setObject(jsonObj);
    if (file.write(jsonDoc.toJson()) == -1 || !file.commit())
    {
        std::cerr<<"ERROR: Could not write report to file!"<<std::endl;
        return false;
    }

    if (!pTempFile)
        fileName = pFileName;

    return true;
}

//

/*!
 * \brief Get the file name of opened/saved report file.
 *
 * \return File name set by open() or save() functions.
 */
QString Report::getFileName() const
{
    return fileName;
}

//

/*!
 * \brief Load/calculate carryovers from the last report.
 *
 * Sets carry values in this report to corresponding final/total values from last report \p pLastReport:
 * - Personnel hours carry is set to last report's total personnel hours
 * (which in turn is last report's gained personnel hours plus last report's personnel hours carry)
 * - Boat drive hours carry is set to last report's total boat drive hours
 * (which in turn is last report's gained boat drive hours plus last report's boat drive hours carry)
 * - Initial (and final, if still zero) boat engine hours are set to last report's final engine hours.
 *
 * Additionally the report serial number is set to last report's serial number plus one.
 *
 * \param pLastReport Last report to load/calculate carryovers from.
 * \return If previous carryovers were changed.
 */
bool Report::loadCarryovers(const Report& pLastReport)
{
    //Need to include old carryovers from last report
    int oldPersonnelCarry = pLastReport.personnelMinutesCarry;
    int oldBoatCarry = pLastReport.boatLogPtr->getBoatMinutesCarry();

    //Sum up gained personnel hours for each person's arrival/leaving times from last report

    int oldTotalPersonnelMinutes = 0;
    const auto& oldFuncsTimes = pLastReport.personnelFunctionTimesMap;

    for (const auto& it : oldFuncsTimes)
    {
        const QTime& tBeginTime = it.second.second.first;
        const QTime& tEndTime = it.second.second.second;

        int dMinutes = tBeginTime.secsTo(tEndTime) / 60;

        if (dMinutes < 0)
            dMinutes += 24 * 60;

        oldTotalPersonnelMinutes += dMinutes;
    }

    //Sum up gained boat hours for each boat drive's begin/end times from last report

    int oldTotalBoatMinutes = 0;
    auto oldDrives = pLastReport.boatLogPtr->getDrives();

    for (const auto& it : oldDrives)
    {
        const QTime& tBeginTime = it.get().getBeginTime();
        const QTime& tEndTime = it.get().getEndTime();

        int dMinutes = tBeginTime.secsTo(tEndTime) / 60;

        if (dMinutes < 0)
            dMinutes += 24 * 60;

        oldTotalBoatMinutes += dMinutes;
    }

    //Set new carryovers to sum of old carryovers plus summed gained time from last report
    int newPersonnelCarry = oldTotalPersonnelMinutes + oldPersonnelCarry;
    int newBoatCarry = oldTotalBoatMinutes + oldBoatCarry;

    //Use final engine hours from last report as new initial value
    double newEngineHoursInitial = pLastReport.boatLogPtr->getEngineHoursFinal();

    //If final engine hours are still zero (prevent unwanted destructive overwrite), set them equal to the new initial value
    double newEngineHoursFinal = boatLogPtr->getEngineHoursFinal();
    if (newEngineHoursFinal == 0)
        newEngineHoursFinal = newEngineHoursInitial;

    //Increment serial number
    int newSerialNumber = pLastReport.number + 1;

    //Check if new carryovers are different from old ones
    bool valuesChanged = number != newSerialNumber ||
                         personnelMinutesCarry != newPersonnelCarry ||
                         boatLogPtr->getBoatMinutesCarry() != newBoatCarry ||
                         boatLogPtr->getEngineHoursInitial() != newEngineHoursInitial ||
                         boatLogPtr->getEngineHoursFinal() != newEngineHoursFinal;

    //Copy loaded/calculated values to this report
    number = newSerialNumber;
    personnelMinutesCarry = newPersonnelCarry;
    boatLogPtr->setBoatMinutesCarry(newBoatCarry);
    boatLogPtr->setEngineHoursInitial(newEngineHoursInitial);
    boatLogPtr->setEngineHoursFinal(newEngineHoursFinal);

    return valuesChanged;
}

//

/*!
 * \brief Get the report's serial number.
 *
 * \return Report serial number.
 */
int Report::getNumber() const
{
    return number;
}

/*!
 * \brief Set the report's serial number.
 *
 * \param pNumber New report serial number.
 */
void Report::setNumber(const int pNumber)
{
    number = pNumber;
}

//

/*!
 * \brief Get the station identifier.
 *
 * \return Station identifier.
 */
QString Report::getStation() const
{
    return station;
}

/*!
 * \brief Set the station identifier.
 *
 * \param pStation Identifier of new station.
 */
void Report::setStation(QString pStation)
{
    station = std::move(pStation);
}

/*!
 * \brief Get the station's radio call name.
 *
 * \return Station radio call name.
 */
QString Report::getRadioCallName() const
{
    return radioCallName;
}

/*!
 * \brief Set the station's radio call name.
 *
 * \param pName New station radio call name.
 */
void Report::setRadioCallName(QString pName)
{
    radioCallName = std::move(pName);
}

/*!
 * \brief Get general comments on the duty.
 *
 * \return General comments.
 */
QString Report::getComments() const
{
    return comments;
}

/*!
 * \brief Set general comments on the duty.
 *
 * \param pComments New general comments.
 */
void Report::setComments(QString pComments)
{
    comments = std::move(pComments);
}

/*!
 * \brief Get the duty purpose.
 *
 * \return Duty purpose.
 */
Report::DutyPurpose Report::getDutyPurpose() const
{
    return dutyPurpose;
}

/*!
 * \brief Set the duty purpose.
 *
 * \param pPurpose New duty purpose.
 */
void Report::setDutyPurpose(const DutyPurpose pPurpose)
{
    dutyPurpose = pPurpose;
}

/*!
 * \brief Get the comment on the duty purpose.
 *
 * \return Duty purpose comment.
 */
QString Report::getDutyPurposeComment() const
{
    return dutyPurposeComment;
}

/*!
 * \brief Set the comment on the duty purpose.
 *
 * \param pComment New duty purpose comment.
 */
void Report::setDutyPurposeComment(QString pComment)
{
    dutyPurposeComment = std::move(pComment);
}

//

/*!
 * \brief Get the report date.
 *
 * \return Report date.
 */
QDate Report::getDate() const
{
    return date;
}

/*!
 * \brief Set the report date.
 *
 * \param pDate New report date.
 */
void Report::setDate(const QDate pDate)
{
    date = pDate;
}

/*!
 * \brief Get the time when the duty begins.
 *
 * \return Duty begin time.
 */
QTime Report::getBeginTime() const
{
    return begin;
}

/*!
 * \brief Set the time when the duty begins.
 *
 * \param pTime New duty begin time.
 */
void Report::setBeginTime(const QTime pTime)
{
    begin = pTime;
}

/*!
 * \brief Get the time when the duty ends.
 *
 * \return Duty end time.
 */
QTime Report::getEndTime() const
{
    return end;
}

/*!
 * \brief Set the time when the duty ends.
 *
 * \param pTime New duty end time.
 */
void Report::setEndTime(const QTime pTime)
{
    end = pTime;
}

//

/*!
 * \brief Get the type of precipitation.
 *
 * \return Precipitation type.
 */
Aux::Precipitation Report::getPrecipitation() const
{
    return precipitation;
}

/*!
 * \brief Set the type of precipitation.
 *
 * \param pPrecipitation New precipitation type.
 */
void Report::setPrecipitation(const Aux::Precipitation pPrecipitation)
{
    precipitation = pPrecipitation;
}

/*!
 * \brief Get the level of cloudiness.
 *
 * \return Cloudiness level.
 */
Aux::Cloudiness Report::getCloudiness() const
{
    return cloudiness;
}

/*!
 * \brief Set the level of cloudiness.
 *
 * \param pCloudiness New cloudiness level.
 */
void Report::setCloudiness(const Aux::Cloudiness pCloudiness)
{
    cloudiness = pCloudiness;
}

/*!
 * \brief Get the wind strength.
 *
 * \return Wind strength.
 */
Aux::WindStrength Report::getWindStrength() const
{
    return windStrength;
}

/*!
 * \brief Set the wind strength.
 *
 * \param pWindStrength New wind strength.
 */
void Report::setWindStrength(const Aux::WindStrength pWindStrength)
{
    windStrength = pWindStrength;
}

/*!
 * \brief Get the wind direction.
 *
 * \return Wind direction.
 */
Aux::WindDirection Report::getWindDirection() const
{
    return windDirection;
}

/*!
 * \brief Set the wind direction.
 *
 * \param pWindDirection New wind direction.
 */
void Report::setWindDirection(const Aux::WindDirection pWindDirection)
{
    windDirection = pWindDirection;
}

//

/*!
 * \brief Get the air temperature.
 *
 * \return Air temperature in degrees Celsius.
 */
int Report::getAirTemperature() const
{
    return temperatureAir;
}

/*!
 * \brief Set the air temperature.
 *
 * \param pTemp New air temperature in degrees Celsius.
 */
void Report::setAirTemperature(const int pTemp)
{
    temperatureAir = pTemp;
}

/*!
 * \brief Get the water temperature.
 *
 * \return Water temperature in degrees Celsius.
 */
int Report::getWaterTemperature() const
{
    return temperatureWater;
}

/*!
 * \brief Set the water temperature.
 *
 * \param pTemp New water temperature in degrees Celsius.
 */
void Report::setWaterTemperature(const int pTemp)
{
    temperatureWater = pTemp;
}

/*!
 * \brief Get additional comments on the weather conditions.
 *
 * \return Weather comments.
 */
QString Report::getWeatherComments() const
{
    return weatherComments;
}

/*!
 * \brief Set additional comments on the weather conditions.
 *
 * \param pComments New weather comments.
 */
void Report::setWeatherComments(QString pComments)
{
    weatherComments = std::move(pComments);
}

//

/*!
 * \brief Get the number of enclosed operation protocols.
 *
 * \return Number of operation protocols.
 */
int Report::getOperationProtocolsCtr() const
{
    return operationProtocolsCtr;
}

/*!
 * \brief Set the number of enclosed operation protocols.
 *
 * \param pNumber New number of operation protocols.
 */
void Report::setOperationProtocolsCtr(const int pNumber)
{
    operationProtocolsCtr = pNumber;
}

/*!
 * \brief Get the number of enclosed patient records.
 *
 * \return Number of patient records.
 */
int Report::getPatientRecordsCtr() const
{
    return patientRecordsCtr;
}

/*!
 * \brief Set the number of enclosed patient records.
 *
 * \param pNumber New number of patient records.
 */
void Report::setPatientRecordsCtr(const int pNumber)
{
    patientRecordsCtr = pNumber;
}

/*!
 * \brief Get the number of enclosed radio call logs.
 *
 * \return Number of radio call logs.
 */
int Report::getRadioCallLogsCtr() const
{
    return radioCallLogsCtr;
}

/*!
 * \brief Set the number of enclosed radio call logs.
 *
 * \param pNumber New number of radio call logs.
 */
void Report::setRadioCallLogsCtr(const int pNumber)
{
    radioCallLogsCtr = pNumber;
}

/*!
 * \brief Get a string listing other enclosures.
 *
 * \return Other enclosures.
 */
QString Report::getOtherEnclosures() const
{
    return otherEnclosures;
}

/*!
 * \brief Set a string listing other enclosures.
 *
 * \param pEnclosures Changed other enclosures.
 */
void Report::setOtherEnclosures(QString pEnclosures)
{
    otherEnclosures = std::move(pEnclosures);
}

//

/*!
 * \brief Get carry for personnel hours from last report as minutes.
 *
 * \return Personnel hours carry in minutes.
 */
int Report::getPersonnelMinutesCarry() const
{
    return personnelMinutesCarry;
}

/*!
 * \brief Set carry for personnel hours from last report as minutes.
 *
 * \param pMinutes New personnel hours carry in minutes.
 */
void Report::setPersonnelMinutesCarry(const int pMinutes)
{
    personnelMinutesCarry = pMinutes;
}

//

/*!
 * \brief Get the personnel strength.
 *
 * Gets the current size of the personnel list.
 *
 * \return Number of persons in report's personnel.
 */
int Report::getPersonnelSize() const
{
    return personnelFunctionTimesMap.size();
}

/*!
 * \brief Get all personnel identifiers.
 *
 * If \p pSorted is true, the identifiers will be sorted by first the person's function (see Person::functionOrder()),
 * then arrival time (earliest first), then name (alphabetically), then identifier (alphabetically).
 *
 * \param pSorted Sort the identifiers by persons' properties?
 * \return List of person identifiers.
 */
std::vector<QString> Report::getPersonnel(const bool pSorted) const
{
    std::vector<QString> tIdents;

    if (!pSorted)
    {
        for (const auto& it : personnelFunctionTimesMap)
            tIdents.push_back(it.first);
    }
    else
    {
        //Define lambda to compare/sort persons by function, then arrival/leaving times, then name, then identifier
        auto cmp = [](const std::tuple<std::reference_wrapper<const Person>, Person::Function, QTime, QTime>& pA,
                      const std::tuple<std::reference_wrapper<const Person>, Person::Function, QTime, QTime>& pB) -> bool
        {
            if (Person::functionOrder(std::get<1>(pA), std::get<1>(pB)) == 1)
                return true;
            else if (Person::functionOrder(std::get<1>(pA), std::get<1>(pB)) == -1)
                return false;
            else
            {
                if (std::get<2>(pA).secsTo(std::get<2>(pB)) > 0)
                    return true;
                else if (std::get<2>(pA).secsTo(std::get<2>(pB)) < 0)
                    return false;
                else
                {
                    if (QString::localeAwareCompare(std::get<0>(pA).get().getLastName(), std::get<0>(pB).get().getLastName()) < 0)
                        return true;
                    else if (QString::localeAwareCompare(std::get<0>(pA).get().getLastName(), std::get<0>(pB).get().getLastName()) > 0)
                        return false;
                    else
                    {
                        if (QString::localeAwareCompare(std::get<0>(pA).get().getFirstName(),
                                                        std::get<0>(pB).get().getFirstName()) < 0)
                        {
                            return true;
                        }
                        else if (QString::localeAwareCompare(std::get<0>(pA).get().getFirstName(),
                                                             std::get<0>(pB).get().getFirstName()) > 0)
                        {
                            return false;
                        }
                        else
                        {
                            if (QString::localeAwareCompare(std::get<0>(pA).get().getIdent(), std::get<0>(pB).get().getIdent()) < 0)
                                return true;
                            else
                                return false;
                        }
                    }
                }
            }
        };

        //Use temporary set to sort persons using above custom sort lambda

        std::set<std::tuple<std::reference_wrapper<const Person>, Person::Function, QTime, QTime>, decltype(cmp)> tSet(cmp);

        for (const auto& it : personnelFunctionTimesMap)
            tSet.insert({std::cref(getIntOrExtPersonnel(it.first)), it.second.first, it.second.second.first, it.second.second.second});

        for (const auto& it : tSet)
            tIdents.push_back(std::get<0>(it).get().getIdent());
    }

    return tIdents;
}

//

/*!
 * \brief Check, if a person is part of personnel.
 *
 * Checks, if a person with identifier \p pIdent is found in this report's personnel list.
 *
 * \param pIdent Person's identifier.
 * \return If person found.
 */
bool Report::personExists(const QString& pIdent) const
{
    return personInPersonnel(pIdent) && personnelExists(pIdent);
}

/*!
 * \brief Check, if the person name is ambiguous.
 *
 * Checks, if multiple persons with last name \p pLastName and first name \p pFirstName
 * are found in this report's personnel list.
 *
 * \param pLastName Persons' last name.
 * \param pFirstName Persons' first name.
 * \return If multiple persons found.
 */
bool Report::personIsAmbiguous(const QString& pLastName, const QString& pFirstName) const
{
    int count = 0;
    for (const auto& it : internalPersonnelMap)
    {
        if (it.second.getLastName() == pLastName && it.second.getFirstName() == pFirstName)
            ++count;

        if (count > 1)
            break;
    }
    for (const auto& it : externalPersonnelMap)
    {
        if (count > 1)
            break;

        if (it.second.getLastName() == pLastName && it.second.getFirstName() == pFirstName)
            ++count;
    }

    return (count > 1);
}

//

/*!
 * \brief Get a specific person from personnel.
 *
 * Gets the person with identifier \p pIdent from report's personnel list.
 *
 * Note: If \p pIdent is not part of personnel, Person::dummyPerson() will be returned.
 *
 * \param pIdent Person's identifier.
 * \return Person with identifier \p pIdent (or Person::dummyPerson(), if not found).
 */
Person Report::getPerson(const QString& pIdent) const
{
    try
    {
        return getIntOrExtPersonnel(pIdent);
    }
    catch (const std::out_of_range&)
    {
        std::cerr<<"ERROR: Could not find person in personnel list! Returning dummy person."<<std::endl;
        return Person::dummyPerson();
    }
}

/*!
 * \brief Add a person to personnel.
 *
 * Adds person \p pPerson to personnel list and sets its personnel function to \p pFunction,
 * its arrival time to \p pBegin and its leaving time to \p pEnd.
 *
 * If a person with the same identifier is already in the personnel list, nothing will be changed.
 *
 * \param pPerson Person to be added.
 * \param pFunction Personnel function.
 * \param pBegin Arrival time.
 * \param pEnd Leaving time.
 */
void Report::addPerson(Person&& pPerson, const Person::Function pFunction, const QTime pBegin, const QTime pEnd)
{
    addPersonFunctionTimes(pPerson.getIdent(), pFunction, pBegin, pEnd);
    addPersonnel(std::move(pPerson));
}

/*!
 * \brief Remove a person from personnel.
 *
 * Removes person with identifier \p pIdent from personnel list.
 *
 * \param pIdent Person's identifier.
 */
void Report::removePerson(const QString& pIdent)
{
    removePersonFunctionTimes(pIdent);
    removePersonnel(pIdent);
}

//

/*!
 * \brief Get the personnel function of a person.
 *
 * Returns Person::Function::_OTHER, if \p pIdent not found.
 *
 * \param pIdent Person's identifier.
 * \return Person's personnel function.
 */
Person::Function Report::getPersonFunction(const QString& pIdent) const
{
    if (personnelFunctionTimesMap.find(pIdent) != personnelFunctionTimesMap.end())
        return personnelFunctionTimesMap.at(pIdent).first;

    std::cerr<<"ERROR: Could not find person in personnel list! Returning dummy function."<<std::endl;

    return Person::Function::_OTHER;
}

/*!
 * \brief Set the personnel function of a person.
 *
 * \param pIdent Person's identifier.
 * \param pFunction New personnel function.
 */
void Report::setPersonFunction(const QString& pIdent, const Person::Function pFunction)
{
    if (personnelFunctionTimesMap.find(pIdent) != personnelFunctionTimesMap.end())
        personnelFunctionTimesMap.at(pIdent).first = pFunction;
    else
        std::cerr<<"ERROR: Could not find person in personnel list!"<<std::endl;
}

/*!
 * \brief Get the time a person arrived.
 *
 * Returns time 00:00, if \p pIdent not found.
 *
 * \param pIdent Person's identifier.
 * \return Person's arrival time.
 */
QTime Report::getPersonBeginTime(const QString& pIdent) const
{
    if (personnelFunctionTimesMap.find(pIdent) != personnelFunctionTimesMap.end())
        return personnelFunctionTimesMap.at(pIdent).second.first;

    std::cerr<<"ERROR: Could not find person in personnel list! Returning 00:00 time."<<std::endl;

    return QTime(0, 0);
}

/*!
 * \brief Set the time a person arrived.
 *
 * \param pIdent Person's identifier.
 * \param pTime New arrival time.
 */
void Report::setPersonBeginTime(const QString& pIdent, const QTime pTime)
{
    if (personnelFunctionTimesMap.find(pIdent) != personnelFunctionTimesMap.end())
        personnelFunctionTimesMap.at(pIdent).second.first = pTime;
    else
        std::cerr<<"ERROR: Could not find person in personnel list!"<<std::endl;
}

/*!
 * \brief Get the time a person left.
 *
 * Returns time 00:00, if \p pIdent not found.
 *
 * \param pIdent Person's identifier.
 * \return Person's leaving time.
 */
QTime Report::getPersonEndTime(const QString& pIdent) const
{
    if (personnelFunctionTimesMap.find(pIdent) != personnelFunctionTimesMap.end())
        return personnelFunctionTimesMap.at(pIdent).second.second;

    std::cerr<<"ERROR: Could not find person in personnel list! Returning 00:00 time."<<std::endl;

    return QTime(0, 0);
}

/*!
 * \brief Set the time a person left.
 *
 * \param pIdent Person's identifier.
 * \param pTime New leaving time.
 */
void Report::setPersonEndTime(const QString& pIdent, const QTime pTime)
{
    if (personnelFunctionTimesMap.find(pIdent) != personnelFunctionTimesMap.end())
        personnelFunctionTimesMap.at(pIdent).second.second = pTime;
    else
        std::cerr<<"ERROR: Could not find person in personnel list!"<<std::endl;
}

//

/*!
 * \brief Get the boat log.
 *
 * \return Shared pointer to the boat log.
 */
std::shared_ptr<BoatLog> Report::boatLog() const
{
    return boatLogPtr;
}

//

/*!
 * \brief Get the numbers of carried out rescue operations.
 *
 * \return Map of numbers of rescue operations with available rescue operation types as keys.
 */
std::map<Report::RescueOperation, int> Report::getRescueOperationCtrs() const
{
    return rescueOperationsCounts;
}

/*!
 * \brief Get the number of carried out rescue operations.
 *
 * Gets the number of carried out rescue operations of type \p pRescue.
 *
 * \param pRescue Rescue operation type.
 * \return Number of rescue operations of type \p pRescue.
 */
int Report::getRescueOperationCtr(const RescueOperation pRescue) const
{
    if (rescueOperationsCounts.find(pRescue) != rescueOperationsCounts.end())
        return rescueOperationsCounts.at(pRescue);

    return 0;
}

/*!
 * \brief Set the number of carried out rescue operations.
 *
 * Sets the number of carried out rescue operations of type \p pRescue.
 *
 * \param pRescue Rescue operation type.
 * \param pCount New number of rescue operations of type \p pRescue.
 */
void Report::setRescueOperationCtr(const RescueOperation pRescue, const int pCount)
{
    if (rescueOperationsCounts.find(pRescue) != rescueOperationsCounts.end())
        rescueOperationsCounts.at(pRescue) = pCount;
}

//

/*!
 * \brief Get the assignment number of the rescue directing center.
 *
 * \return Assignment number.
 */
QString Report::getAssignmentNumber() const
{
    return assignmentNumber;
}

/*!
 * \brief Set the assignment number of the rescue directing center.
 *
 * \param pNumber New assignment number.
 */
void Report::setAssignmentNumber(QString pNumber)
{
    assignmentNumber = std::move(pNumber);
}

//

/*!
 * \brief Get the list of resources used for the duty.
 *
 * Returns a list of resources (their (radio call) names) used in the
 * course of the duty together with their begin of use and end of use times.
 * If \p pSorted is true, the returned vector is sorted by begin time (before (radio call) name, before end time).
 *
 * \param pSorted Get a sorted vector.
 * \return Vector of resources as {{NAME, {T_BEGIN, T_END}}, ...}.
 */
std::vector<std::pair<QString, std::pair<QTime, QTime>>> Report::getResources(const bool pSorted) const
{
    if (!pSorted)
        return resources;

    //Define lambda to compare/sort resources by begin time, then name, then end time
    auto cmp = [](const std::pair<QString, std::pair<QTime, QTime>>& pA,
                  const std::pair<QString, std::pair<QTime, QTime>>& pB) -> bool
    {
        if (pA.second.first.secsTo(pB.second.first) > 0)
            return true;
        else if (pA.second.first.secsTo(pB.second.first) < 0)
            return false;
        else
        {
            if (QString::localeAwareCompare(pA.first, pB.first) < 0)
                return true;
            else if (QString::localeAwareCompare(pA.first, pB.first) > 0)
                return false;
            else
            {
                if (pA.second.second.secsTo(pB.second.second) > 0)
                    return true;
                else
                    return false;
            }
        }
    };

    //Use temporary set to sort resources using above custom sort lambda

    std::set<std::pair<QString, std::pair<QTime, QTime>>, decltype(cmp)> tSet(cmp);

    for (const auto& it : resources)
        tSet.insert(it);

    std::vector<std::pair<QString, std::pair<QTime, QTime>>> resourcesSorted;

    for (const auto& it : tSet)
        resourcesSorted.push_back(it);

    return resourcesSorted;
}

/*!
 * \brief Set the list of resources used for the duty.
 *
 * Sets a list of resources (their (radio call) names) used in the
 * course of the duty together with their arrival and leaving times.
 *
 * \param pResources New resources list as vector {{NAME, {T_BEGIN, T_END}}, ...}.
 */
void Report::setResources(std::vector<std::pair<QString, std::pair<QTime, QTime>>> pResources)
{
    resources.swap(pResources);
}

//

/*!
 * \brief Get the label for a duty purpose.
 *
 * Get a (unique) nicely formatted label for \p pPurpose to e.g. show it in the application as a combo box item.
 * Converting back is possible using labelToDutyPurpose().
 *
 * \param pPurpose The duty purpose to get a label for.
 * \return The corresponding label for \p pPurpose.
 */
QString Report::dutyPurposeToLabel(const DutyPurpose pPurpose)
{
    switch (pPurpose)
    {
        case DutyPurpose::_WATCHKEEPING:
            return "Wachdienst";
        case DutyPurpose::_SAILING_REGATTA:
            return "Begleitung Regatta";
        case DutyPurpose::_SAILING_PRACTICE:
            return "Begleitung Segeltraining";
        case DutyPurpose::_SWIMMING_PRACTICE:
            return "Training";
        case DutyPurpose::_RESCUE_EXERCISE:
            return "Übung";
        case DutyPurpose::_RESCUE_OPERATION:
            return "Einsatz";
        case DutyPurpose::_COURSE:
            return "Lehrgang";
        case DutyPurpose::_OTHER:
            return "Sonstiges";
        default:
            return "Sonstiges";
    }
}

/*!
 * \brief Get the duty purpose from its label.
 *
 * Get the duty purpose corresponding to a (unique) label \p pPurpose,
 * which can in turn be obtained from dutyPurposeToLabel().
 *
 * \param pPurpose The label representing the requested duty purpose.
 * \return The duty purpose corresponding to label \p pPurpose.
 */
Report::DutyPurpose Report::labelToDutyPurpose(const QString& pPurpose)
{
    if (pPurpose == "Wachdienst")
        return DutyPurpose::_WATCHKEEPING;
    else if (pPurpose == "Begleitung Regatta")
        return DutyPurpose::_SAILING_REGATTA;
    else if (pPurpose == "Begleitung Segeltraining")
        return DutyPurpose::_SAILING_PRACTICE;
    else if (pPurpose == "Training")
        return DutyPurpose::_SWIMMING_PRACTICE;
    else if (pPurpose == "Übung")
        return DutyPurpose::_RESCUE_EXERCISE;
    else if (pPurpose == "Einsatz")
        return DutyPurpose::_RESCUE_OPERATION;
    else if (pPurpose == "Lehrgang")
        return DutyPurpose::_COURSE;
    else if (pPurpose == "Sonstiges")
        return DutyPurpose::_OTHER;
    else
        return DutyPurpose::_OTHER;
}

//

/*!
 * \brief Get the label for a rescue operation type.
 *
 * Get a (unique) nicely formatted label/description for \p pRescue to e.g. show it in the application as a label.
 * Converting back is possible using labelToRescueOperation().
 *
 * \param pRescue The rescue operation type to get a label for.
 * \return The corresponding label for \p pRescue.
 */
QString Report::rescueOperationToLabel(const RescueOperation pRescue)
{
    switch (pRescue)
    {
        case RescueOperation::_FIRST_AID:
            return "Erste-Hilfe-Einsatz";
        case RescueOperation::_FIRST_AID_MATERIAL:
            return "Ausgabe von EH-/SAN-Material";
        case RescueOperation::_WATER_PREVENTIVE_MEASURES:
            return "Vorbeugende Maßnahmen Wassersportler";
        case RescueOperation::_WATER_RESCUE_GENERAL:
            return "Rettung von Personen vor Ertrinken";
        case static_cast<RescueOperation>(RescueOperation_CAPSIZE_deprecated):  //Note: deprecated option; keep this here
            return "Bootskenterung";                                            //for compatibility with old saved reports
        case RescueOperation::_MATERIAL_RETRIEVAL:
            return "Bergung von Sachgut";
        case RescueOperation::_CAPSIZE_WATER_RESCUE:
            return "Bootskenterung (mit Hilfe Personen)";
        case RescueOperation::_CAPSIZE_TECH_ASSISTANCE:
            return "Bootskenterung (technische Hilfe)";
        case RescueOperation::_MISSING_PERSON:
            return "Personensuche";
        case RescueOperation::_OTHER_ASSISTANCE:
            return "Sonstige Hilfeleistungen";
        case RescueOperation::_MORTAL_DANGER_INVOLVED:
            return "Rettungen aus Lebensgefahr";
        default:
            return "Sonstige Hilfeleistungen";
    }
}

/*!
 * \brief Get the fill document notice for a rescue operation type.
 *
 * Get a nicely formatted notice string for \p pRescue to ask for or remind about certain actions
 * such as filling a patient record etc., if a rescue operation of type \p pRescue has been carried out.
 *
 * \param pRescue The rescue operation type to get a notice label for.
 * \return The corresponding notice label for \p pRescue.
 */
QString Report::rescueOperationToDocNotice(const RescueOperation pRescue)
{
    switch (pRescue)
    {
        case RescueOperation::_FIRST_AID:
        {
            return "Patientenprotokoll vollständig und angehängt?\n"
                   "Verbrauchtes Material in Verbrauchsliste eingetragen?";
        }
        case RescueOperation::_FIRST_AID_MATERIAL:
        {
            return "Verbrauchtes Material in Verbrauchsliste eingetragen?";
        }
        case RescueOperation::_WATER_PREVENTIVE_MEASURES:
        {
            return "Details unter Bemerkungen vermerkt oder, falls nötig,\n"
                   "Einsatzprotokoll ausgefüllt und angehängt?";
        }
        case RescueOperation::_WATER_RESCUE_GENERAL:
        {
            return "Einsatz- und Patientenprotokoll ausgefüllt und angehängt?\n"
                   "Leiter Einsatz informiert?";
        }
        case static_cast<RescueOperation>(RescueOperation_CAPSIZE_deprecated):  //Note: deprecated option; keep this here
        {                                                                       //for compatibility with old saved reports
            return "Details unter Bemerkungen vermerkt oder, falls nötig,\n"
                   "Einsatz-/Patientenprotokoll ausgefüllt und angehängt?";
        }
        case RescueOperation::_MATERIAL_RETRIEVAL:
        {
            return "Details unter Bemerkungen vermerkt?";
        }
        case RescueOperation::_CAPSIZE_WATER_RESCUE:
        {
            return "Einsatz-/Patientenprotokoll ausgefüllt und angehängt?\n"
                   "Leiter Einsatz informiert?";
        }
        case RescueOperation::_CAPSIZE_TECH_ASSISTANCE:
        {
            return "Details unter Bemerkungen vermerkt oder, falls nötig,\n"
                   "Einsatzprotokoll ausgefüllt und angehängt?";
        }
        case RescueOperation::_MISSING_PERSON:
        {
            return "Einsatzprotokoll und Suchmeldung ausgefüllt und angehängt?\n"
                   "Leiter Einsatz informiert?";
        }
        case RescueOperation::_OTHER_ASSISTANCE:
        {
            return "Details unter Bemerkungen vermerkt oder, falls nötig,\n"
                   "Einsatzprotokoll ausgefüllt und angehängt?";
        }
        case RescueOperation::_MORTAL_DANGER_INVOLVED:
        {
            return "Einsatz- und Patientenprotokoll ausgefüllt und angehängt?\n"
                   "Leiter Einsatz informiert?";
        }
        default:
            return "";
    }
}

/*!
 * \brief Get the rescue operation type from its label.
 *
 * Get the rescue operation type corresponding to a (unique) label \p pRescue,
 * which can in turn be obtained from rescueOperationToLabel().
 *
 * \param pRescue The label representing the requested rescue operation type.
 * \return The rescue operation type corresponding to label \p pRescue.
 */
Report::RescueOperation Report::labelToRescueOperation(const QString& pRescue)
{
    if (pRescue == "Erste-Hilfe-Einsatz")
        return RescueOperation::_FIRST_AID;
    else if (pRescue == "Ausgabe von EH-/SAN-Material")
        return RescueOperation::_FIRST_AID_MATERIAL;
    else if (pRescue == "Vorbeugende Maßnahmen Wassersportler")
        return RescueOperation::_WATER_PREVENTIVE_MEASURES;
    else if (pRescue == "Rettung von Personen vor Ertrinken")
        return RescueOperation::_WATER_RESCUE_GENERAL;
    else if (pRescue == "Bootskenterung")                                           //Note: deprecated option; keep this here
        return static_cast<RescueOperation>(RescueOperation_CAPSIZE_deprecated);    //for compatibility with old saved reports
    else if (pRescue == "Bergung von Sachgut")
        return RescueOperation::_MATERIAL_RETRIEVAL;
    else if (pRescue == "Bootskenterung (mit Hilfe Personen)")
        return RescueOperation::_CAPSIZE_WATER_RESCUE;
    else if (pRescue == "Bootskenterung (technische Hilfe)")
        return RescueOperation::_CAPSIZE_TECH_ASSISTANCE;
    else if (pRescue == "Personensuche")
        return RescueOperation::_MISSING_PERSON;
    else if (pRescue == "Sonstige Hilfeleistungen")
        return RescueOperation::_OTHER_ASSISTANCE;
    else if (pRescue == "Rettungen aus Lebensgefahr")
        return RescueOperation::_MORTAL_DANGER_INVOLVED;
    else
        return RescueOperation::_OTHER_ASSISTANCE;
}

//

/*!
 * \brief Get all available (non-deprecated) rescue operation types.
 *
 * Returns a set of all of those RescueOperation types that iterateRescueOperations() would loop over.
 *
 * \return Set of available rescue operation types.
 */
std::set<Report::RescueOperation> Report::getAvailableRescueOperations()
{
    //Lambda expression to add a rescue operation to a set of rescue operations
    auto accAvailRescOps = [](RescueOperation pRescue, std::set<RescueOperation>& pRescues) -> void { pRescues.insert(pRescue); };

    //Use the 'RescueOperation' iteration function to accumulate the available rescue operation types
    std::set<RescueOperation> availRescOps;
    iterateRescueOperations(accAvailRescOps, availRescOps);

    return availRescOps;
}

//Private

/*!
 * \brief Check, if person is part of the personnel, i.e. function and times are defined for the identifier.
 *
 * Checks, if a person with identifier \p pIdent is part of the personnel, i.e. function and times are defined for the identifier.
 *
 * \param pIdent Person's identifier.
 * \return If person found.
 */
bool Report::personInPersonnel(const QString& pIdent) const
{
    if (personnelFunctionTimesMap.find(pIdent) != personnelFunctionTimesMap.end())
        return true;

    return false;
}

/*!
 * \brief Check, if person exists in report-internal personnel archive.
 *
 * Checks, if a Person with identifier \p pIdent is present in either internal or external personnel maps.
 *
 * \param pIdent Person's identifier.
 * \return If person found.
 */
bool Report::personnelExists(const QString& pIdent) const
{
    if (internalPersonnelMap.find(pIdent) != internalPersonnelMap.end() ||
        externalPersonnelMap.find(pIdent) != externalPersonnelMap.end())
    {
        return true;
    }

    return false;
}

/*!
 * \brief Get person in report-internal personnel archive.
 *
 * Searches for a person with identifier \p pIdent in both internal and external personnel maps and returns a reference.
 *
 * \param pIdent Person's identifier.
 * \return Reference to the person.
 *
 * \throws std::out_of_range No person with identifier \p pIdent found.
 */
const Person& Report::getIntOrExtPersonnel(const QString& pIdent) const
{
    if (internalPersonnelMap.find(pIdent) != internalPersonnelMap.end())
        return internalPersonnelMap.at(pIdent);
    else if (externalPersonnelMap.find(pIdent) != externalPersonnelMap.end())
        return externalPersonnelMap.at(pIdent);

    throw std::out_of_range("Could not find person with this identifier!");
}

//

/*!
 * \brief Add person to the report-internal personnel archive.
 *
 * Adds person \p pPerson to the report-internal personnel archive. An internal person is added
 * to the internal personnel map and an external person is added to the external personnel map.
 *
 * \param pPerson Person to be added.
 */
void Report::addPersonnel(Person&& pPerson)
{
    QString ident = pPerson.getIdent();

    if (Person::isInternalIdent(ident))
        internalPersonnelMap.insert({std::move(ident), std::move(pPerson)});
    else if (Person::isExternalIdent(ident))
        externalPersonnelMap.insert({std::move(ident), std::move(pPerson)});
}

/*!
 * \brief Remove person from the report-internal personnel archive.
 *
 * Removes a person with identifier \p pIdent from (either internal or external) personnel map.
 *
 * \param pIdent Person's identifier.
 */
void Report::removePersonnel(const QString& pIdent)
{
    if (internalPersonnelMap.find(pIdent) != internalPersonnelMap.end())
        internalPersonnelMap.erase(pIdent);
    else if (externalPersonnelMap.find(pIdent) != externalPersonnelMap.end())
        externalPersonnelMap.erase(pIdent);
}

//

/*!
 * \brief Add a person's personnel function and times to the personnel list.
 *
 * Adds personnel function \p pFunction and arrival and leaving times \p pBegin and \p pEnd
 * for person identifier \p pIdent to the personnel list.
 *
 * \param pIdent Person's identifier.
 * \param pFunction Personnel function.
 * \param pBegin Arrival time.
 * \param pEnd Leaving time.
 */
void Report::addPersonFunctionTimes(const QString& pIdent, const Person::Function pFunction, const QTime pBegin, const QTime pEnd)
{
    personnelFunctionTimesMap.insert({pIdent, {pFunction, {pBegin, pEnd}}});
}

/*!
 * \brief Remove a person from the personnel list.
 *
 * \param pIdent Person's identifier.
 */
void Report::removePersonFunctionTimes(const QString& pIdent)
{
    if (personnelFunctionTimesMap.find(pIdent) != personnelFunctionTimesMap.end())
        personnelFunctionTimesMap.erase(pIdent);
}
