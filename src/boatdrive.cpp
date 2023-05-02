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

#include "boatdrive.h"

#include <stdexcept>

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
    boatman(""),
    noCrewConfirmed(false)
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
void BoatDrive::setBeginTime(const QTime pTime)
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
void BoatDrive::setEndTime(const QTime pTime)
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
void BoatDrive::setFuel(const int pLiters)
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
 * \brief Get all crew members' functions.
 *
 * \return Map with person identifiers as keys and their boat functions as values.
 */
std::map<QString, Person::BoatFunction> BoatDrive::crew() const
{
    return crewMap;
}

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
 * \brief Get the function of a crew member.
 *
 * If person \p pIdent is a crew member, its boat function is assigned to \p pFunction.
 *
 * \param pIdent Requested person's identifier.
 * \param pFunction Destination for person's boat function.
 * \return If person was found.
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
 * \brief Get the name of an external crew member.
 *
 * If person \p pIdent is an external crew member, its last and first name are assigned to \p pLastName and \p pFirstName.
 *
 * \param pIdent Requested person's identifier.
 * \param pLastName Destination for person's last name.
 * \param pFirstName Destination for person's first name.
 * \return If person was found.
 */
bool BoatDrive::getExtCrewMemberName(const QString& pIdent, QString& pLastName, QString& pFirstName) const
{
    if (crewExtNames.find(pIdent) != crewExtNames.end())
    {
        pLastName = crewExtNames.at(pIdent).first;
        pFirstName = crewExtNames.at(pIdent).second;
        return true;
    }

    return false;
}

/*!
 * \brief Add a crew member.
 *
 * Registers \p pIdent as crew member and assigns its boat function the value of \p pFunction.
 *
 * As this means that crewSize() is then larger than zero, the "empty crew
 * confirmation state" (see getNoCrewConfirmed()) is set to false.
 *
 * \param pIdent New crew member's identifier.
 * \param pFunction New crew member's boat function.
 */
void BoatDrive::addCrewMember(const QString& pIdent, const Person::BoatFunction pFunction)
{
    crewMap[pIdent] = pFunction;
    noCrewConfirmed = false;
}

/*!
 * \brief Add an external crew member.
 *
 * Adds an external boat crew member (which is not part of the duty personnel). See addCrewMember().
 *
 * Additionally registers \p pLastName and \p pFirstName and links them to \p pIdent.
 *
 * \param pIdent New crew member's identifier.
 * \param pFunction New crew member's boat function.
 * \param pLastName External crew member's last name.
 * \param pFirstName External crew member's first name.
 */
void BoatDrive::addExtCrewMember(const QString& pIdent, const Person::BoatFunction pFunction,
                                 const QString& pLastName, const QString& pFirstName)
{
    addCrewMember(pIdent, pFunction);

    crewExtNames[pIdent] = {pLastName, pFirstName};
}

/*!
 * \brief Remove a crew member.
 *
 * Removes the crew member \p pIdent and, if it is an external crew member, its associated name.
 *
 * \param pIdent Person's identifier.
 */
void BoatDrive::removeCrewMember(const QString& pIdent)
{
    crewMap.erase(pIdent);
    crewExtNames.erase(pIdent);
}

/*!
 * \brief Remove all crew members.
 *
 * Removes all crew members and additionally removes all added names of the external crew members.
 */
void BoatDrive::clearCrew()
{
    crewMap.clear();
    crewExtNames.clear();
}

//

/*!
 * \brief Check if empty crew (except boatman) was confirmed.
 *
 * \return If crewSize() is zero and this was explicitly confirmed as being correct by setNoCrewConfirmed().
 */
bool BoatDrive::getNoCrewConfirmed() const
{
    return (crewSize() == 0) && noCrewConfirmed;
}

/*!
 * \brief Confirm that empty crew (except boatman) is correct.
 *
 * If the drive really had no crew members other than the boatman then it can be confirmed with this function.
 * This only works if crewSize() is actually zero. The confirmation state will be set to false otherwise.
 *
 * Note: The confirmation state will also be automatically set to false whenever addCrewMember() is called.
 *
 * \param pNoCrew True if an empty crewSize() shall be considered intentional.
 */
void BoatDrive::setNoCrewConfirmed(const bool pNoCrew)
{
    noCrewConfirmed = ((crewSize() == 0) && pNoCrew);
}
