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

#ifndef REPORT_H
#define REPORT_H

#include "auxil.h"
#include "person.h"
#include "boatlog.h"
#include "databasecache.h"
#include "qualificationchecker.h"

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <tuple>
#include <stdexcept>
#include <functional>

#include <QString>
#include <QStringList>
#include <QDate>
#include <QTime>
#include <QFile>
#include <QSaveFile>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

/*!
 * \brief Create, edit, open and save a watch duty report.
 *
 * This class can contain all information required to fully characterize a watch duty report.
 * The report can be saved to and loaded from a JSON file via save() and open().
 * For exporting to PDF see PDFExporter.
 *
 * Most information including the duty personnel can be directly accessed and edited via the class interface.
 * The boat log (i.e. all boat-related information and the boat drives) must however be retrieved by
 * boatLog() and then directly accessed and edited through the BoatLog class.
 *
 * The class also defines the enum classes Report::DutyPurpose and Report::RescueOperation,
 * which provide pre-defined values to describe the duty purpose and pre-defined types of
 * rescue operations, respectively. See also iterateDutyPurposes() and iterateRescueOperations().
 */
class Report
{
public:
    enum class DutyPurpose : int8_t;
    enum class RescueOperation : int8_t;

public:
    Report();                                                       ///< Constructor.
    //
    void reset();                                                   ///< Reset to the state of a newly constructed report.
    //
    bool open(const QString& pFileName);                            ///< Load report from file.
    bool save(const QString& pFileName, bool pTempFile = false);    ///< Save report to file.
    //
    QString getFileName() const;                                    ///< Get the file name of opened/saved report file.
    //
    void loadCarryovers(const Report& pLastReport);                 ///< Load/calculate carryovers from the last report.
    //
    int getNumber() const;                                          ///< Get the report's serial number.
    void setNumber(int pNumber);                                    ///< Set the report's serial number.
    //
    QString getStation() const;                                     ///< Get the station identifier.
    void setStation(QString pStation);                              ///< Set the station identifier.
    QString getRadioCallName() const;                               ///< Get the station's radio call name.
    void setRadioCallName(QString pName);                           ///< Set the station's radio call name.
    QString getComments() const;                                    ///< Get general comments on the duty.
    void setComments(QString pComments);                            ///< Set general comments on the duty.
    DutyPurpose getDutyPurpose() const;                             ///< Get the duty purpose.
    void setDutyPurpose(DutyPurpose pPurpose);                      ///< Set the duty purpose.
    QString getDutyPurposeComment() const;                          ///< Get the comment on the duty purpose.
    void setDutyPurposeComment(QString pComment);                   ///< Set the comment on the duty purpose.
    //
    QDate getDate() const;                                          ///< Get the report date.
    void setDate(QDate pDate);                                      ///< Set the report date.
    QTime getBeginTime() const;                                     ///< Get the time when the duty begins.
    void setBeginTime(QTime pTime);                                 ///< Set the time when the duty begins.
    QTime getEndTime() const;                                       ///< Get the time when the duty ends.
    void setEndTime(QTime pTime);                                   ///< Set the time when the duty ends.
    //
    Aux::Precipitation getPrecipitation() const;                    ///< Get the type of precipitation.
    void setPrecipitation(Aux::Precipitation pPrecipitation);       ///< Set the type of precipitation.
    Aux::Cloudiness getCloudiness() const;                          ///< Get the level of cloudiness.
    void setCloudiness(Aux::Cloudiness pCloudiness);                ///< Set the level of cloudiness.
    Aux::WindStrength getWindStrength() const;                      ///< Get the wind strength.
    void setWindStrength(Aux::WindStrength pWindStrength);          ///< Set the wind strength.
    //
    int getAirTemperature() const;                                  ///< Get the air temperature.
    void setAirTemperature(int pTemp);                              ///< Set the air temperature.
    int getWaterTemperature() const;                                ///< Get the water temperature.
    void setWaterTemperature(int pTemp);                            ///< Set the water temperature.
    QString getWeatherComments() const;                             ///< Get additional comments on the weather conditions.
    void setWeatherComments(QString pComments);                     ///< Set additional comments on the weather conditions.
    //
    int getOperationProtocolsCtr() const;                           ///< Get the number of enclosed operation protocols.
    void setOperationProtocolsCtr(int pNumber);                     ///< Set the number of enclosed operation protocols.
    int getPatientRecordsCtr() const;                               ///< Get the number of enclosed patient records.
    void setPatientRecordsCtr(int pNumber);                         ///< Set the number of enclosed patient records.
    int getRadioCallLogsCtr() const;                                ///< Get the number of enclosed radio call logs.
    void setRadioCallLogsCtr(int pNumber);                          ///< Set the number of enclosed radio call logs.
    QString getOtherEnclosures() const;                             ///< Get a string listing other enclosures.
    void setOtherEnclosures(QString pEnclosures);                   ///< Set a string listing other enclosures.
    //
    int getPersonnelMinutesCarry() const;                           ///< Get carry for personnel hours from last report as minutes.
    void setPersonnelMinutesCarry(int pMinutes);                    ///< Set carry for personnel hours from last report as minutes.
    //
    int getPersonnelSize() const;                                   ///< Get the personnel strength.
    std::vector<QString> getPersonnel(bool pSorted = false) const;  ///< Get all personnel identifiers.
    //
    bool personExists(const QString& pIdent) const;                                     ///< Check, if a person is part of personnel.
    bool personIsAmbiguous(const QString& pLastName, const QString& pFirstName) const;  ///< Check, if the person name is ambiguous.
    //
    Person getPerson(const QString& pIdent) const;                                          ///< Get a specific person from personnel.
    void addPerson(Person&& pPerson, Person::Function pFunction, QTime pBegin, QTime pEnd); ///< Add a person to personnel.
    void removePerson(const QString& pIdent);                                               ///< Remove a person from personnel.
    //
    Person::Function getPersonFunction(const QString& pIdent) const;            ///< Get the personnel function of a person.
    void setPersonFunction(const QString& pIdent, Person::Function pFunction);  ///< Set the personnel function of a person.
    QTime getPersonBeginTime(const QString& pIdent) const;                      ///< Get the time a person arrived.
    void setPersonBeginTime(const QString& pIdent, QTime pTime);                ///< Set the time a person arrived.
    QTime getPersonEndTime(const QString& pIdent) const;                        ///< Get the time a person left.
    void setPersonEndTime(const QString& pIdent, QTime pTime);                  ///< Set the time a person left.
    //
    std::shared_ptr<BoatLog> boatLog() const;                                   ///< Get the boat log.
    //
    std::map<RescueOperation, int> getRescueOperationCtrs() const;              ///< Get the numbers of carried out rescue operations.
    int getRescueOperationCtr(RescueOperation pRescue) const;                   ///< Get the number of carried out rescue operations.
    void setRescueOperationCtr(RescueOperation pRescue, int pCount);            ///< Set the number of carried out rescue operations.
    //
    QString getAssignmentNumber() const;                                ///< Get the assignment number of the rescue directing center.
    void setAssignmentNumber(QString pNumber);                          ///< Set the assignment number of the rescue directing center.
    //
    std::vector<std::pair<QString, std::pair<QTime, QTime>>> getVehicles(bool pSorted = false) const;
                                                                                            ///< Get the list of vehicles at the station.
    void setVehicles(std::vector<std::pair<QString, std::pair<QTime, QTime>>> pVehicles);   ///< Set the list of vehicles at the station.
    //
    static QString dutyPurposeToLabel(DutyPurpose pPurpose);                ///< Get the label for a duty purpose.
    static DutyPurpose labelToDutyPurpose(const QString& pPurpose);         ///< Get the duty purpose from its label.
    //
    static QString rescueOperationToLabel(RescueOperation pRescue);         ///< Get the label for a rescue operation type.
    static QString rescueOperationToDocNotice(RescueOperation pRescue); ///< Get the fill document notice for a rescue operation type.
    static RescueOperation labelToRescueOperation(const QString& pRescue);  ///< Get the rescue operation type from its label.

private:
    bool personInPersonnel(const QString& pIdent) const;                ///< \brief Check, if person is part of the personnel,
                                                                        ///  i.e. function and times are defined for the identifier.
    bool personnelExists(const QString& pIdent) const;                  ///< \brief Check, if person exists in report-internal
                                                                        ///  personnel archive.
    const Person& getIntOrExtPersonnel(const QString& pIdent) const;    ///< Get person in report-internal personnel archive.
    //
    void addPersonnel(Person&& pPerson);                                ///< Add person to the report-internal personnel archive.
    void removePersonnel(const QString& pIdent);                        ///< Remove person from the report-internal personnel archive.
    //
    void addPersonFunctionTimes(const QString& pIdent, Person::Function pFunction,
                                QTime pBegin, QTime pEnd);      ///< Add a person's personnel function and times to the personnel list.
    void removePersonFunctionTimes(const QString& pIdent);      ///< Remove a person from the personnel list.

public:
    /*!
     * \brief A number of possible duty purposes.
     *
     * The values correspond to the most common general scenarios.
     * Use DutyPurpose::_OTHER (and set a proper duty purpose comment for the Report), if no scenario fits.
     */
    enum class DutyPurpose : int8_t
    {
        _WATCHKEEPING = 0,      ///< "Wachdienst".
        _SAILING_REGATTA = 1,   ///< "Segelregatta".
        _SAILING_PRACTICE = 2,  ///< "Segeltraining".
        _SWIMMING_PRACTICE = 3, ///< "Training".
        _RESCUE_EXERCISE = 4,   ///< "Übung".
        _RESCUE_OPERATION = 5,  ///< "Einsatz".
        _COURSE = 6,            ///< "Lehrgang".
        _OTHER = 127,           ///< Other purpose.
    };
    /*!
     * \brief A number of categories of frequently carried out rescue operations.
     *
     * The values correspond to the most common rescue operations.
     * Use RescueOperation::_OTHER_ASSISTANCE, if no value fits.
     * Use RescueOperation::_MORTAL_DANGER_INVOLVED to describe a subset of all rescue operations where persons were in mortal danger.
     */
    enum class RescueOperation : int8_t
    {
        _FIRST_AID = 0,                 ///< "Erste-Hilfe".
        _FIRST_AID_MATERIAL = 1,        ///< "Ausgabe von EH-/SAN-Material".
        _SWIMMER_ACROSS = 2,            ///< "Unterstützung/Rettung von Querschwimmer".
        _SWIMMER_GENERAL = 3,           ///< "Anderer Schwimmer-Notfall (z.B. im Uferbereich)".
        _CAPSIZE = 4,                   ///< "Bootskenterung".
        _MATERIAL_RETRIEVAL = 5,        ///< "Sachgutbergung".
        _OTHER_ASSISTANCE = 50,         ///< "Sonstige Hilfeleistung".
        _MORTAL_DANGER_INVOLVED = 100   ///< "... davon unter Lebensgefahr".
    };

public:
    /*!
     * \brief Loop over personnel 'DutyPurpose's and execute a specified function for each purpose.
     *
     * For each 'DutyPurpose' the \p pFunction is called with first parameter being 'DutyPurpose'
     * and perhaps further parameters \p pArgs, i.e. \p pFunction(purpose, pArgs...).
     * Non-void return values will be discarded. You may use use \p pArgs
     * to communicate results of the function calls.
     *
     * \tparam FuncT Specific type of a FunctionObject (\p pFunction) that shall be called for each 'DutyPurpose'.
     * \tparam ...Args Types of all additional parameters for \p pFunction if there are any.
     *
     * \param pFunction Any FunctionObject that accepts at least enum class values of 'DutyPurpose' as the first
     *                  parameter and optionally also further parameters \p pArgs. A lambda expression may be used.
     *                  The function's return value will be discarded.
     *                  Note: (some of) \p pArgs can be passed by reference in order to communicate the results.
     * \param pArgs Optional arguments that will be passed to \p pFunction in addition to the 'DutyPurpose's.
     */
    template <typename FuncT, typename ... Args>
    static void iterateDutyPurposes(FuncT pFunction, Args& ... pArgs)
    {
        for (DutyPurpose purpose : {DutyPurpose::_WATCHKEEPING,
                                    DutyPurpose::_SAILING_REGATTA,
                                    DutyPurpose::_SAILING_PRACTICE,
                                    DutyPurpose::_SWIMMING_PRACTICE,
                                    DutyPurpose::_RESCUE_EXERCISE,
                                    DutyPurpose::_RESCUE_OPERATION,
                                    DutyPurpose::_COURSE,
                                    DutyPurpose::_OTHER})
        {
            pFunction(purpose, pArgs ...);
        }
    }
    /*!
     * \brief Loop over 'RescueOperation's and execute a specified function for each type.
     *
     * For each 'RescueOperation' the \p pFunction is called with first parameter being 'RescueOperation'
     * and perhaps further parameters \p pArgs, i.e. \p pFunction(rescue, pArgs...).
     * Non-void return values will be discarded. You may use use \p pArgs
     * to communicate results of the function calls.
     *
     * \tparam FuncT Specific type of a FunctionObject (\p pFunction) that shall be called for each 'RescueOperation'.
     * \tparam ...Args Types of all additional parameters for \p pFunction if there are any.
     *
     * \param pFunction Any FunctionObject that accepts at least enum class values of 'RescueOperation' as the first
     *                  parameter and optionally also further parameters \p pArgs. A lambda expression may be used.
     *                  The function's return value will be discarded.
     *                  Note: (some of) \p pArgs can be passed by reference in order to communicate the results.
     * \param pArgs Optional arguments that will be passed to \p pFunction in addition to the 'RescueOperation's.
     */
    template <typename FuncT, typename ... Args>
    static void iterateRescueOperations(FuncT pFunction, Args& ... pArgs)
    {
        for (RescueOperation rescue : {RescueOperation::_FIRST_AID,
                                       RescueOperation::_FIRST_AID_MATERIAL,
                                       RescueOperation::_SWIMMER_ACROSS,
                                       RescueOperation::_SWIMMER_GENERAL,
                                       RescueOperation::_CAPSIZE,
                                       RescueOperation::_MATERIAL_RETRIEVAL,
                                       RescueOperation::_OTHER_ASSISTANCE,
                                       RescueOperation::_MORTAL_DANGER_INVOLVED})
        {
            pFunction(rescue, pArgs ...);
        }
    }

private:
    QString fileName;                   //File name of saved report file
    //
    int number;                         //Report serial number
    //
    QString station;                    //Station identifier
    QString radioCallName;              //Used radio call name
    //
    QString comments;                   //General comments on the duty
    //
    DutyPurpose dutyPurpose;            //Purpose of the duty
    QString dutyPurposeComment;         //Further comment on this purpose
    //
    QDate date;                         //Date
    QTime begin, end;                   //Timeframe of the duty
    //
    Aux::Precipitation precipitation;   //Precipitation type
    Aux::Cloudiness cloudiness;         //Cloudiness level
    Aux::WindStrength windStrength;     //Wind strength
    //
    int temperatureAir;                 //Local air temperature
    int temperatureWater;               //Local water temperature
    //
    QString weatherComments;            //Comments on the weather conditions
    //
    int operationProtocolsCtr;          //Number of enclosed operation protocols
    int patientRecordsCtr;              //Number of enclosed patient records
    int radioCallLogsCtr;               //Number of enclosed radio call logs
    QString otherEnclosures;            //Comma-separated list of other enclosures
    //
    int personnelMinutesCarry;          //Carry of (current season's) total personnel hours from last report (measured in minutes!)
    //
    std::map<QString, Person> internalPersonnelMap;  //Save database personell locally along with report for archival purposes
    std::map<QString, Person> externalPersonnelMap;  //Also save external personnel locally along with report
    //
    std::map<QString, std::pair<Person::Function, std::pair<QTime, QTime>>> personnelFunctionTimesMap;  //Personnel functions and times
    //
    std::shared_ptr<BoatLog> boatLogPtr;                    //The boat log (which is handled by a separate class)
    //
    std::map<RescueOperation, int> rescueOperationsCounts;  //Counts of different types of rescue operations
    //
    QString assignmentNumber;                               //Assignment number from rescue directing center
    //
    std::vector<std::pair<QString, std::pair<QTime, QTime>>> vehicles;  //List of present vehicles with their arrival/leaving times
};

#endif // REPORT_H
