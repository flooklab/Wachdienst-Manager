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

#ifndef BOATLOG_H
#define BOATLOG_H

#include "boatdrive.h"

#include <list>
#include <iterator>
#include <stdexcept>
#include <functional>

#include <QString>
#include <QTime>

/*!
 * \brief Group together boat-related information of a Report.
 *
 * This class is used by Report to factor out handling of the boat log part of the report,
 * i.e. it contains and handles all general boat-related information and the boat drives.
 * The BoatLog is created by and contained in the Report and must be directly accessed
 * and edited through Report::boatLog().
 */
class BoatLog
{
public:
    BoatLog();                                  ///< Constructor.
    //
    QString getBoat() const;                    ///< Get the name of the boat.
    void setBoat(QString pName);                ///< Set the name of the boat.
    QString getRadioCallName() const;           ///< Get the boat's radio call name.
    void setRadioCallName(QString pName);       ///< Set the boat's radio call name.
    QString getComments() const;                ///< Get comments on the boat.
    void setComments(QString pComments);        ///< Set comments on the boat.
    //
    bool getSlippedInitial() const;             ///< Was boat lowered to water at begin of duty?
    void setSlippedInitial(bool pSlipped);      ///< Set, if boat was lowered to water at begin of duty.
    bool getSlippedFinal() const;               ///< Was boat taken out of water at end of duty?
    void setSlippedFinal(bool pSlipped);        ///< Set, if boat was taken out of water at end of duty.
    //
    QTime getReadyFrom() const;                 ///< Get begin of time frame, in which boat is ready for rescue operations.
    void setReadyFrom(QTime pTime);             ///< Set begin of time frame, in which boat is ready for rescue operations.
    QTime getReadyUntil() const;                ///< Get end of time frame, in which boat is ready for rescue operations.
    void setReadyUntil(QTime pTime);            ///< Set end of time frame, in which boat is ready for rescue operations.
    //
    double getEngineHoursInitial() const;       ///< Get boat engine hours counter at begin of duty.
    void setEngineHoursInitial(double pHours);  ///< Set boat engine hours counter at begin of duty.
    double getEngineHoursFinal() const;         ///< Get boat engine hours counter at end of duty.
    void setEngineHoursFinal(double pHours);    ///< Set boat engine hours counter at end of duty.
    //
    int getFuelInitial() const;                 ///< Get fuel added to the onboard tank at begin of duty.
    void setFuelInitial(int pLiters);           ///< Set fuel added to the onboard tank at begin of duty.
    int getFuelFinal() const;                   ///< Get fuel added to the onboard tank at end of duty.
    void setFuelFinal(int pLiters);             ///< Set fuel added to the onboard tank at end of duty.
    //
    int getBoatMinutesCarry() const;            ///< Get carry for boat drive hours from last report as minutes.
    void setBoatMinutesCarry(int pMinutes);     ///< Set carry for boat drive hours from last report as minutes.
    //
    int getDrivesCount() const;                                             ///< Get the number of boat drives.
    std::list<std::reference_wrapper<const BoatDrive>> getDrives() const;   ///< Get references to all boat drives.
    BoatDrive& getDrive(int pIdx);                                          ///< Get the reference to a boat drive.
    //
    void addDrive(int pIdx, BoatDrive&& pDrive);    ///< Add a boat drive.
    void removeDrive(int pIdx);                     ///< Remove a boat drive.
    void swapDrives(int pIdx1, int pIdx2);          ///< Exchange two boat drives.

private:
    QString boat;                   //Name of the boat
    QString radioCallName;          //Used radio call name
    //
    QString comments;               //General comments on the boat (not referring to a specific drive)
    //
    bool slippedInitial;            //Boat lowered into water at begin of duty?
    bool slippedFinal;              //Boat taken out of water at end of duty?
    QTime readyFrom;                //Time since boat (and crew) are ready for potential rescue operations
    QTime readyUntil;               //Time until boat (and crew) are ready for potential rescue operations
    //
    double engineHoursInitial;      //Bpat engine hours at begin of duty (before first drive)
    double engineHoursFinal;        //Boat engine hours at end of duty (after last drive)
    int fuelInitial;                //Amount of fuel added to the onboard tank at begin of duty (before first drive) in liters
    int fuelFinal;                  //Amount of fuel added to the onboard tank at end of duty (after last drive) in liters
    //
    int boatMinutesCarry;           //Carry of (current season's) total boat drive hours from last report (measured in minutes!)
    //
    std::list<BoatDrive> drives;    //List of boat drives
};

#endif // BOATLOG_H
