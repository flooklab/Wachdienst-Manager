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

#include "report.h"

/*!
 * \brief Constructor.
 *
 * Creates an empty report with default settings (and an empty boat log with default settings as well).
 *
 * Possibly interesting default values:
 * - Serial number set to 1.
 * - All times set to 00:00 and report date set to 01.01.2000.
 * - Duty purpose set to DutyPurpose::_WATCHKEEPING.
 * - Weather conditions set to:
 *  - Aux::Precipitation::_NONE
 *  - Aux::Cloudiness::_CLOUDLESS
 *  - Aux::WindStrength::_CALM
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
    //Open file
    QFile file(pFileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        std::cerr<<"ERROR: Could not open file for reading!"<<std::endl;
        file.close();
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


    if (!jsonObj.contains("_version") || !jsonObj.value("_version").isString())
    {
        std::cerr<<"ERROR: Could not determine report format version!"<<std::endl;
        return false;
    }

    int tVerMaj = 0, tVerMin = 0, tVerPatch = 0;
    char tVerType = '-';

    int tmpInt = 0;
    QString tVerStr = jsonObj.value("_version").toString();
    if (Aux::programVersionsValidator.validate(tVerStr, tmpInt) != QValidator::State::Acceptable ||
        !Aux::parseProgramVersion(jsonObj.value("_version").toString(), tVerMaj, tVerMin, tVerPatch, tVerType))
    {
        std::cerr<<"ERROR: Could not determine report format version!"<<std::endl;
        return false;
    }

    //Check version, decide how to process file
    if (jsonObj.value("_version").toString() != Aux::programVersionString)
    {
        //TODO: add other, more complex checks/logic here for later versions, if document format changes or is extended

        if (tVerMaj < Aux::programVersionMajor)
        {
            std::cerr<<"ERROR: Report was saved with incompatible (older) program version!"<<std::endl;
            return false;
        }
        else if (tVerMaj > Aux::programVersionMajor)
        {
            std::cerr<<"ERROR: Report was saved with incompatible (newer) program version!"<<std::endl;
            return false;
        }
    }

    //Check, if two main objects are present
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

    //Assignment number from rescue directing center

    assignmentNumber = reportObj.value("assignmentNumber").toString("");

    if (assignmentNumber != "" &&
        Aux::assignmentNumbersValidator.validate(assignmentNumber, tmpInt) != QValidator::State::Acceptable)
    {
        std::cerr<<"ERROR: Wrong assignment number format!"<<std::endl;
        return false;
    }

    //Personnel

    personnelMinutesCarry = reportObj.value("personnelMinutesCarry").toInt(0);

    //Load archived personnel data to be independent of the personnel database

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

        //Assume person state always active, if part of personnel of a saved report
        bool tActive = true;

        //Add new person to personnel archive
        addPersonnel(Person(tLastName, tFirstName, Person::createInternalIdent(tLastName, tFirstName, tMembershipNumber),
                            Person::Qualifications(tQualifications), tActive));
    }

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

            //Check, if crew member exists in personnel list and if qualifications are sufficient
            if (!personInPersonnel(tIdent))
            {
                std::cerr<<"ERROR: Crew member not in personnel list!"<<std::endl;
                return false;
            }
            if (!QualificationChecker::checkBoatFunction(tBoatFunction, getIntOrExtPersonnel(tIdent).getQualifications()))
            {
                std::cerr<<"ERROR: Insufficient qualification for boat function!"<<std::endl;
                return false;
            }

            tDrive.addCrewMember(tIdent, tBoatFunction);
        }

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
 * \return If successful.
 */
bool Report::save(const QString& pFileName)
{
    //Open file
    QFile file(pFileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        std::cerr<<"ERROR: Could not open file for writing!"<<std::endl;
        file.close();
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

            crewArray.append(crewMemberObj);
        }

        QJsonObject crewObj;
        crewObj.insert("crew", crewArray);
        driveObj.insert("boatCrew", crewObj);

        drivesArray.append(driveObj);
    }

    QJsonObject drivesObj;
    drivesObj.insert("drives", drivesArray);
    boatObj.insert("boatDrives", drivesObj);

    //Create main/top JSON object
    QJsonObject jsonObj;

    //Add meta-information
    jsonObj.insert("_magic", "prg:wd.mgr");
    jsonObj.insert("_version", Aux::programVersionString);

    //Add main content
    jsonObj.insert("reportMain", reportObj);
    jsonObj.insert("boatLog", boatObj);

    //Create JSON document and write it to the file
    QJsonDocument jsonDoc;
    jsonDoc.setObject(jsonObj);
    if (file.write(jsonDoc.toJson()) == -1)
    {
        std::cerr<<"ERROR: Could not write report to file!"<<std::endl;
        return false;
    }
    file.close();

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
 * - Initial and final boat engine hours are set to last report's final engine hours.
 *
 * Additionally the report serial number is set to last report's serial number plus one.
 *
 * \param pLastReport Last report to load/calculate carryovers from.
 */
void Report::loadCarryovers(const Report& pLastReport)
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

        int tMinutes = tBeginTime.secsTo(tEndTime) / 60;
        oldTotalPersonnelMinutes += tMinutes;
    }

    //Sum up gained boat hours for each boat drive's begin/end times from last report

    int oldTotalBoatMinutes = 0;
    auto oldDrives = pLastReport.boatLogPtr->getDrives();

    for (const auto& it : oldDrives)
    {
        const QTime& tBeginTime = it.get().getBeginTime();
        const QTime& tEndTime = it.get().getEndTime();

        int tMinutes = tBeginTime.secsTo(tEndTime) / 60;
        oldTotalBoatMinutes += tMinutes;
    }

    //Set new carryovers to sum of old carryovers plus summed gained time from last report
    int newPersonnelCarry = oldTotalPersonnelMinutes + oldPersonnelCarry;
    int newBoatCarry = oldTotalBoatMinutes + oldBoatCarry;

    //Use final engine hours from last report as new initial value
    double newEngineHoursInitial = pLastReport.boatLogPtr->getEngineHoursFinal();
    double newEngineHoursFinal = newEngineHoursInitial;

    //Copy loaded/calculated values to this report (and increment serial number!)
    number = pLastReport.number + 1;
    personnelMinutesCarry = newPersonnelCarry;
    boatLogPtr->setBoatMinutesCarry(newBoatCarry);
    boatLogPtr->setEngineHoursInitial(newEngineHoursInitial);
    boatLogPtr->setEngineHoursFinal(newEngineHoursFinal);
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
void Report::setNumber(int pNumber)
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
void Report::setDutyPurpose(DutyPurpose pPurpose)
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
void Report::setDate(QDate pDate)
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
void Report::setBeginTime(QTime pTime)
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
void Report::setEndTime(QTime pTime)
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
void Report::setPrecipitation(Aux::Precipitation pPrecipitation)
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
void Report::setCloudiness(Aux::Cloudiness pCloudiness)
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
void Report::setWindStrength(Aux::WindStrength pWindStrength)
{
    windStrength = pWindStrength;
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
void Report::setAirTemperature(int pTemp)
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
void Report::setWaterTemperature(int pTemp)
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
void Report::setOperationProtocolsCtr(int pNumber)
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
void Report::setPatientRecordsCtr(int pNumber)
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
void Report::setRadioCallLogsCtr(int pNumber)
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
void Report::setPersonnelMinutesCarry(int pMinutes)
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
std::vector<QString> Report::getPersonnel(bool pSorted) const
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
void Report::addPerson(Person&& pPerson, Person::Function pFunction, QTime pBegin, QTime pEnd)
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
void Report::setPersonFunction(const QString& pIdent, Person::Function pFunction)
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
void Report::setPersonBeginTime(const QString& pIdent, QTime pTime)
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
void Report::setPersonEndTime(const QString& pIdent, QTime pTime)
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
int Report::getRescueOperationCtr(RescueOperation pRescue) const
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
void Report::setRescueOperationCtr(RescueOperation pRescue, int pCount)
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
 * \brief Get the label for a duty purpose.
 *
 * Get a (unique) nicely formatted label for \p pPurpose to e.g. show it in the application as a combo box item.
 * Converting back is possible using labelToDutyPurpose().
 *
 * \param pPurpose The duty purpose to get a label for.
 * \return The corresponding label for \p pPurpose.
 */
QString Report::dutyPurposeToLabel(DutyPurpose pPurpose)
{
    switch (pPurpose)
    {
        case DutyPurpose::_WATCHKEEPING:
            return "Wachdienst";
        case DutyPurpose::_SAILING_REGATTA:
            return "Begleitung Segelregatta";
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
    else if (pPurpose == "Begleitung Segelregatta")
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
QString Report::rescueOperationToLabel(RescueOperation pRescue)
{
    switch (pRescue)
    {
        case RescueOperation::_FIRST_AID:
            return "Erste-Hilfe-Einsätze";
        case RescueOperation::_FIRST_AID_MATERIAL:
            return "Ausgabe von EH-/SAN-Material";
        case RescueOperation::_SWIMMER_ACROSS:
            return "Schwimmer-Einsätze (Querschwimmer)";
        case RescueOperation::_SWIMMER_INSHORE:
            return "Schwimmer-Einsätze (Uferbereich)";
        case RescueOperation::_CAPSIZE:
            return "Bootskenterungen";
        case RescueOperation::_MATERIAL_RETRIEVAL:
            return "Bergung von Sachgütern";
        case RescueOperation::_OTHER_ASSISTANCE:
            return "Sonstige Hilfeleistungen";
        case RescueOperation::_MORTAL_DANGER:
            return "Hilfeleistungen unter Lebensgefahr";
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
QString Report::rescueOperationToDocNotice(RescueOperation pRescue)
{
    switch (pRescue)
    {
        case RescueOperation::_FIRST_AID:
            return "Verbrauchtes Material in Verbrauchsliste eingetragen?\n"
                   "Patientenprotokoll vollständig und angehängt?";
        case RescueOperation::_FIRST_AID_MATERIAL:
            return "Verbrauchtes Material in Verbrauchsliste eingetragen?";
        case RescueOperation::_SWIMMER_ACROSS:
            return "Details unter Bemerkungen vermerkt oder alternativ\n"
                   "Einsatzprotokoll ausgefüllt und angehängt?";
        case RescueOperation::_SWIMMER_INSHORE:
            return "Einsatzprotokoll ausgefüllt und angehängt?";
        case RescueOperation::_CAPSIZE:
            return "Falls nötig: Einsatzprotokoll ausgefüllt und angehängt?";
        case RescueOperation::_MATERIAL_RETRIEVAL:
            return "";
        case RescueOperation::_OTHER_ASSISTANCE:
            return "Details unter Bemerkungen vermerkt?";
        case RescueOperation::_MORTAL_DANGER:
            return "Einsatzprotokoll ausgefüllt und angehängt?";
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
    if (pRescue == "Erste-Hilfe-Einsätze")
        return RescueOperation::_FIRST_AID;
    else if (pRescue == "Ausgabe von EH-/SAN-Material")
        return RescueOperation::_FIRST_AID_MATERIAL;
    else if (pRescue == "Schwimmer-Einsätze (Querschwimmer)")
        return RescueOperation::_SWIMMER_ACROSS;
    else if (pRescue == "Schwimmer-Einsätze (Uferbereich)")
        return RescueOperation::_SWIMMER_INSHORE;
    else if (pRescue == "Bootskenterungen")
        return RescueOperation::_CAPSIZE;
    else if (pRescue == "Bergung von Sachgütern")
        return RescueOperation::_MATERIAL_RETRIEVAL;
    else if (pRescue == "Sonstige Hilfeleistungen")
        return RescueOperation::_OTHER_ASSISTANCE;
    else if (pRescue == "Hilfeleistungen unter Lebensgefahr")
        return RescueOperation::_MORTAL_DANGER;
    else
        return RescueOperation::_OTHER_ASSISTANCE;
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

    if (ident.startsWith('i'))
        internalPersonnelMap.insert({std::move(ident), std::move(pPerson)});
    else if (ident.startsWith('e'))
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
void Report::addPersonFunctionTimes(const QString& pIdent, Person::Function pFunction, QTime pBegin, QTime pEnd)
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
