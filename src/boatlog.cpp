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

#include "boatlog.h"

#include <iterator>
#include <stdexcept>

/*!
 * \brief Constructor.
 *
 * Creates an empty boat log.
 *
 * All times are initialized to 00:00.
 */
BoatLog::BoatLog() :
    boat(""),
    radioCallName(""),
    comments(""),
    slippedInitial(false),
    slippedFinal(false),
    readyFrom(QTime(0, 0)),
    readyUntil(QTime(0, 0)),
    engineHoursInitial(0.0),
    engineHoursFinal(0.0),
    fuelInitial(0),
    fuelFinal(0),
    boatMinutesCarry(0)
{
}

//Public

/*!
 * \brief Get the name of the boat.
 *
 * \return Boat name.
 */
QString BoatLog::getBoat() const
{
    return boat;
}

/*!
 * \brief Set the name of the boat.
 *
 * \param pName New boat name.
 */
void BoatLog::setBoat(QString pName)
{
    boat = std::move(pName);
}

/*!
 * \brief Get the boat's radio call name.
 *
 * \return Boat radio call name.
 */
QString BoatLog::getRadioCallName() const
{
    return radioCallName;
}

/*!
 * \brief Set the boat's radio call name.
 *
 * \param pName New boat radio call name.
 */
void BoatLog::setRadioCallName(QString pName)
{
    radioCallName = std::move(pName);
}

/*!
 * \brief Get comments on the boat.
 *
 * \return General boat log comments.
 */
QString BoatLog::getComments() const
{
    return comments;
}

/*!
 * \brief Set comments on the boat.
 *
 * \param pComments New general boat log comments.
 */
void BoatLog::setComments(QString pComments)
{
    comments = std::move(pComments);
}

//

/*!
 * \brief Was boat lowered to water at begin of duty?
 *
 * \return If boat was lowered to water.
 */
bool BoatLog::getSlippedInitial() const
{
    return slippedInitial;
}

/*!
 * \brief Set, if boat was lowered to water at begin of duty.
 *
 * \param pSlipped Boat lowered to water at begin of duty?
 */
void BoatLog::setSlippedInitial(const bool pSlipped)
{
    slippedInitial = pSlipped;
}

/*!
 * \brief Was boat taken out of water at end of duty?
 *
 * \return If boat was taken out of water.
 */
bool BoatLog::getSlippedFinal() const
{
    return slippedFinal;
}

/*!
 * \brief Set, if boat was taken out of water at end of duty.
 *
 * \param pSlipped Boat taken out of water?
 */
void BoatLog::setSlippedFinal(const bool pSlipped)
{
    slippedFinal = pSlipped;
}

//

/*!
 * \brief Get begin of time frame, in which boat is ready for rescue operations.
 *
 * Boat ready means that boat is on the water, rescue backpack has been put on the boat etc.
 *
 * \return From what time the boat is ready.
 */
QTime BoatLog::getReadyFrom() const
{
    return readyFrom;
}

/*!
 * \brief Set begin of time frame, in which boat is ready for rescue operations.
 *
 * Boat ready means that boat is on the water, rescue backpack has been put on the boat etc.
 *
 * \param pTime From what time the boat is ready.
 */
void BoatLog::setReadyFrom(const QTime pTime)
{
    readyFrom = pTime;
}

/*!
 * \brief Get end of time frame, in which boat is ready for rescue operations.
 *
 * See also getReadyFrom().
 *
 * \return Until what time the boat is ready.
 */
QTime BoatLog::getReadyUntil() const
{
    return readyUntil;
}

/*!
 * \brief Set end of time frame, in which boat is ready for rescue operations.
 *
 * See also setReadyFrom().
 *
 * \param pTime Until what time the boat is ready.
 */
void BoatLog::setReadyUntil(const QTime pTime)
{
    readyUntil = pTime;
}

//

/*!
 * \brief Get boat engine hours counter at begin of duty.
 *
 * \return Initial boat engine hours.
 */
double BoatLog::getEngineHoursInitial() const
{
    return engineHoursInitial;
}

/*!
 * \brief Set boat engine hours counter at begin of duty.
 *
 * \param pHours New initial boat engine hours.
 */
void BoatLog::setEngineHoursInitial(const double pHours)
{
    engineHoursInitial = pHours;
}

/*!
 * \brief Get boat engine hours counter at end of duty.
 *
 * \return Final boat engine hours.
 */
double BoatLog::getEngineHoursFinal() const
{
    return engineHoursFinal;
}

/*!
 * \brief Set boat engine hours counter at end of duty.
 *
 * \param pHours New final boat engine hours.
 */
void BoatLog::setEngineHoursFinal(const double pHours)
{
    engineHoursFinal = pHours;
}

//

/*!
 * \brief Get fuel added to the onboard tank at begin of duty.
 *
 * \return Added fuel in liters.
 */
int BoatLog::getFuelInitial() const
{
    return fuelInitial;
}

/*!
 * \brief Set fuel added to the onboard tank at begin of duty.
 *
 * \param pLiters Added fuel in liters.
 */
void BoatLog::setFuelInitial(const int pLiters)
{
    fuelInitial = pLiters;
}

/*!
 * \brief Get fuel added to the onboard tank at end of duty.
 *
 * \return Added fuel in liters.
 */
int BoatLog::getFuelFinal() const
{
    return fuelFinal;
}

/*!
 * \brief Set fuel added to the onboard tank at end of duty.
 *
 * \param pLiters Added fuel in liters.
 */
void BoatLog::setFuelFinal(const int pLiters)
{
    fuelFinal = pLiters;
}

//

/*!
 * \brief Get carry for boat drive hours from last report as minutes.
 *
 * \return Boat drive hours carry in minutes.
 */
int BoatLog::getBoatMinutesCarry() const
{
    return boatMinutesCarry;
}

/*!
 * \brief Set carry for boat drive hours from last report as minutes.
 *
 * \param pMinutes New boat drive hours carry in minutes.
 */
void BoatLog::setBoatMinutesCarry(const int pMinutes)
{
    boatMinutesCarry = pMinutes;
}

//

/*!
 * \brief Get the number of boat drives.
 *
 * \return Number of boat drives.
 */
int BoatLog::getDrivesCount() const
{
    return static_cast<int>(drives.size());
}

/*!
 * \brief Get references to all boat drives.
 *
 * \return List of references to all boat drives.
 */
std::list<std::reference_wrapper<const BoatDrive>> BoatLog::getDrives() const
{
    std::list<std::reference_wrapper<const BoatDrive>> driveRefs;

    for (const BoatDrive& tDrive : drives)
        driveRefs.push_back(std::cref(tDrive));

    return driveRefs;
}

/*!
 * \brief Get the reference to a boat drive.
 *
 * \param pIdx Number of the boat drive.
 * \return Reference to boat drive number \p pIdx.
 *
 * \throws std::out_of_range \p pIdx out of range.
 */
BoatDrive& BoatLog::getDrive(const int pIdx)
{
    auto idx = static_cast<std::list<BoatDrive>::size_type>(pIdx);

    if (idx >= drives.size())
        throw std::out_of_range("No boat drive with this index!");

    auto it = drives.begin();
    std::advance(it, idx);
    return *it;
}

//

/*!
 * \brief Add a boat drive.
 *
 * Adds \p pDrive to the list of drives at position/number \p pIdx.
 * Possible positions are in [0, getDrivesCount()].
 *
 * Note: If \p pIdx is larger than getDrivesCount(), it will be automatically set to getDrivesCount().
 *
 * \param pIdx Number/position of the new drive.
 * \param pDrive New drive to be added.
 */
void BoatLog::addDrive(const int pIdx, BoatDrive&& pDrive)
{
    auto idx = static_cast<std::list<BoatDrive>::size_type>(pIdx);

    if (idx > drives.size())
        idx = drives.size();

    auto it = drives.begin();
    std::advance(it, idx);

    drives.insert(it, std::move(pDrive));
}

/*!
 * \brief Remove a boat drive.
 *
 * Removes the drive at position \p pIdx.
 * Possible positions are in [0, getDrivesCount()-1].
 *
 * Note: If \p pIdx is larger than getDrivesCount()-1 (last drive), the last drive will be removed.
 *
 * \param pIdx Number/position of the drive to be removed.
 */
void BoatLog::removeDrive(const int pIdx)
{
    if (drives.empty())
        return;

    auto idx = static_cast<std::list<BoatDrive>::size_type>(pIdx);

    if (idx >= drives.size())
        idx = drives.size()-1;

    auto it = drives.begin();
    std::advance(it, idx);

    drives.erase(it);
}

/*!
 * \brief Exchange two boat drives.
 *
 * Exchanges the positions of teh two drives at \p pIdx1 and \p pIdx2.
 * Possible positions are in [0, getDrivesCount()-1].
 *
 * \param pIdx1 Number/position of the first of two drives to be exchanged.
 * \param pIdx2 Number/position of the second of two drives to be exchanged.
 */
void BoatLog::swapDrives(const int pIdx1, const int pIdx2)
{
    if (pIdx1 == pIdx2)
        return;

    auto idx1 = static_cast<std::list<BoatDrive>::size_type>(pIdx1);
    auto idx2 = static_cast<std::list<BoatDrive>::size_type>(pIdx2);

    auto it1 = drives.begin();
    auto it2 = drives.begin();

    std::advance(it1, idx1);
    std::advance(it2, idx2);

    std::swap(*it1, *it2);
}
