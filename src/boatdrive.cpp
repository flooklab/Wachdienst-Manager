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

#include "boatdrive.h"

/*!
 * \brief Constructor.
 *
 * Creates a boat drive with begin and end times equal to the current time but otherwise empty.
 */
BoatDrive::BoatDrive() :
    purpose(""),
    comments(""),
    begin(QTime::currentTime()),
    end(QTime::currentTime()),
    fuel(0),
    boatman("")
{
}

//Public

/*!
 * \brief Get the drive's purpose.
 *
 * \return Drive purpose.
 */
QString BoatDrive::getPurpose() const
{
    return purpose;
}

/*!
 * \brief Set the drive's purpose.
 *
 * \param pPurpose New drive purpose.
 */
void BoatDrive::setPurpose(QString pPurpose)
{
    purpose = std::move(pPurpose);
}

/*!
 * \brief Get the drive's comments.
 *
 * \return Drive comments.
 */
QString BoatDrive::getComments() const
{
    return comments;
}

/*!
 * \brief Set the drive's comments.
 *
 * \param pComments New drive comments.
 */
void BoatDrive::setComments(QString pComments)
{
    comments = std::move(pComments);
}

//

/*!
 * \brief Get the drive's begin time.
 *
 * \return Drive begin time.
 */
QTime BoatDrive::getBeginTime() const
{
    return begin;
}

/*!
 * \brief Set the drive's begin time.
 *
 * \param pTime New drive begin time.
 */
void BoatDrive::setBeginTime(QTime pTime)
{
    begin = pTime;
}

/*!
 * \brief Get the drive's end time.
 *
 * \return Drive end time.
 */
QTime BoatDrive::getEndTime() const
{
    return end;
}

/*!
 * \brief Set the drive's end time.
 *
 * \param pTime New drive end time.
 */
void BoatDrive::setEndTime(QTime pTime)
{
    end = pTime;
}

//

/*!
 * \brief Get the amount of added fuel.
 *
 * \return Added fuel in liters.
 */
int BoatDrive::getFuel() const
{
    return fuel;
}

/*!
 * \brief Set the amount of added fuel.
 *
 * \param pLiters New amount in liters.
 */
void BoatDrive::setFuel(int pLiters)
{
    fuel = pLiters;
}

//

/*!
 * \brief Get the boatman.
 *
 * \return Boatman identifier.
 */
QString BoatDrive::getBoatman() const
{
    return boatman;
}

/*!
 * \brief Set the boatman.
 *
 * \param pIdent New boatman identifier.
 */
void BoatDrive::setBoatman(QString pIdent)
{
    boatman = std::move(pIdent);
}

//

/*!
 * \brief Get the number of crew members.
 *
 * \return Current number of crew members.
 */
int BoatDrive::crewSize() const
{
    return crewMap.size();
}

/*!
 * \brief Get all crew members.
 *
 * \return Map with person identifiers as keys and their boat functions as values.
 */
std::map<QString, Person::BoatFunction> BoatDrive::crew() const
{
    return crewMap;
}

/*!
 * \brief Remove all crew members.
 */
void BoatDrive::clearCrew()
{
    crewMap.clear();
}

/*!
 * \brief Get the function of a crew member.
 *
 * If person \p pIdent is a crew member, its boat function is assigned to \p pFunction.
 *
 * \param pIdent Requested person's identifier.
 * \param pFunction Destination for person's boat function.
 * \return If person was found (and \p pFunction was assigned).
 */
bool BoatDrive::getCrewMember(const QString& pIdent, Person::BoatFunction& pFunction) const
{
    if (crewMap.find(pIdent) != crewMap.end())
    {
        pFunction = crewMap.at(pIdent);
        return true;
    }

    return false;
}

/*!
 * \brief Add a crew member.
 *
 * Register \p pIdent as crew member and assign its boat function the value of \p pFunction.
 *
 * \param pIdent New crew member's identifier.
 * \param pFunction New crew member's boat function.
 */
void BoatDrive::addCrewMember(const QString& pIdent, Person::BoatFunction pFunction)
{
    crewMap[pIdent] = pFunction;
}
