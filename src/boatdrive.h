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

#ifndef BOATDRIVE_H
#define BOATDRIVE_H

#include "person.h"

#include <QString>
#include <QTime>

#include <map>
#include <utility>

/*!
 * \brief Information about a boat drive.
 *
 * Describes a single boat drive by defining purpose of the drive, begin and end times,
 * the boatman, all crew members, amount of added fuel and any further comments.
 */
class BoatDrive
{
public:
    BoatDrive();                            ///< Constructor.
    //
    QString getPurpose() const;             ///< Get the drive's purpose.
    void setPurpose(QString pPurpose);      ///< Set the drive's purpose.
    QString getComments() const;            ///< Get the drive's comments.
    void setComments(QString pComments);    ///< Set the drive's comments.
    //
    QTime getBeginTime() const;             ///< Get the drive's begin time.
    void setBeginTime(QTime pTime);         ///< Set the drive's begin time.
    QTime getEndTime() const;               ///< Get the drive's end time.
    void setEndTime(QTime pTime);           ///< Set the drive's end time.
    //
    int getFuel() const;                    ///< Get the amount of added fuel.
    void setFuel(int pLiters);              ///< Set the amount of added fuel.
    //
    QString getBoatman() const;             ///< Get the boatman.
    void setBoatman(QString pIdent);        ///< Set the boatman.
    //
    std::map<QString, Person::BoatFunction> crew() const;                               ///< Get all crew members' functions.
    int crewSize() const;                                                               ///< Get the number of crew members.
    bool getCrewMember(const QString& pIdent, Person::BoatFunction& pFunction) const;   ///< Get the function of a crew member.
    bool getExtCrewMemberName(const QString& pIdent,
                              QString& pLastName, QString& pFirstName) const;           ///< Get the name of an external crew member.
    void addCrewMember(const QString& pIdent, Person::BoatFunction pFunction);          ///< Add a crew member.
    void addExtCrewMember(const QString& pIdent, Person::BoatFunction pFunction,
                          const QString& pLastName, const QString& pFirstName);         ///< Add an external crew member.
    void removeCrewMember(const QString& pIdent);                                       ///< Remove a crew member.
    void clearCrew();                                                                   ///< Remove all crew members.
    //
    bool getNoCrewConfirmed() const;        ///< Check if empty crew (except boatman) was confirmed.
    void setNoCrewConfirmed(bool pNoCrew);  ///< Confirm that empty crew (except boatman) is correct.

private:
    QString purpose;        //Purpose of the boat drive
    QString comments;       //Comments on the boat drive
    //
    QTime begin, end;       //Timeframe of the boat drive
    //
    int fuel;               //Added fuel (during/after this drive) in liters
    //
    QString boatman;        //The boatman
    //
    bool noCrewConfirmed;   //Explicit confirmation that boat crew was left empty intentionally (only boatman aboard)
    std::map<QString, Person::BoatFunction> crewMap;                //The boat crew for this drive (excluding the boatman)
    std::map<QString, std::pair<QString, QString>> crewExtNames;    //Last and first names of external crew members
};

#endif // BOATDRIVE_H
