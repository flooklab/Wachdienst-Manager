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

#ifndef SINGLEINSTANCESYNCHRONIZER_H
#define SINGLEINSTANCESYNCHRONIZER_H

#include "startupwindow.h"

#include <QSharedMemory>
#include <QString>

#include <atomic>
#include <cwchar>

/*!
 * \brief Interface between a single "master" application instance and multiple "slave" instances that automatically exit again.
 *
 * Implements a virtual "bus" for communicating requests from multiple application instances to execute different
 * tasks in a main ("master") instance instead of the instance that sends the request ("slave"). The bus is based
 * on shared memory segments and is initialized by init(). The first of all instances that calls init() initially
 * creates the shared memory segments and becomes the master instance (see also isMaster()), which has to control
 * the bus and process the requests (see listen()). If a master instance is already running then init() simply attaches
 * to the bus and the instance becomes a slave instance. A slave instance can send requests to create a new Report in
 * the master instance (see sendNewReport()) or to open an existing Report in the master instance (see sendOpenReport()).
 *
 * Note: If a slave instance does not need to send further requests (or a master instance does not want to be
 * one but rather exit) but still has to keep running for some reason then it can use detach() to detach from
 * the bus before exiting in order not to be wrongly recognized as master instance while it is still running.
 */
class SingleInstanceSynchronizer
{
public:
    SingleInstanceSynchronizer() = delete;                  ///< Deleted constructor.
    //
    static bool init();                                     ///< Initialize the bus connection and determine if master or slave instance.
    static bool detach();                                   ///< Disconnect the instance from the bus.
    //
    static bool isInitialized();                            ///< Check if the bus is initialized and the instance connected to it.
    static bool isMaster();                                 ///< Check if the instance connected to the bus is the master instance.
    //
    static void sendNewReport();                            ///< Send request to start a new report to the master instance via the bus.
    static void sendOpenReport(const QString& pFileName);   ///< Send request to open existing report to the master instance via the bus.
    //
    static void listen(StartupWindow& pStartupWindow, const std::atomic_bool& pStopListening);  ///< \brief Control the bus and
                                                                                                ///  continuously process all
                                                                                                ///  incoming requests.

private:
    /*!
     * \brief Symbols that can be used for the bus control signal.
     *
     * The values define different states of the bus. If the bus is BusCtrlSymbol::_IDLE then the master
     * instance is waiting for requests from slave instances (see listen()). In the idle state slave instances
     * can send requests on the bus if they can acquire the lock for the control signal. The request is sent
     * by setting the control signal to a symbol different from BusCtrlSymbol::_IDLE and releasing the lock.
     * Additional data that need to be sent via the data signal must be set before releasing the control lock.
     */
    enum class BusCtrlSymbol : int8_t
    {
        _IDLE = 0,          ///< Idle state.
        _NEW_REPORT = 1,    ///< Create a new report and show it in another report window. See also sendNewReport().
        _OPEN_REPORT = 2    ///< Open an existing report in another report window. See also sendOpenReport().
    };

private:
    static bool initialized;        //Shared memory set up and master/slave mode determined
    static bool master;             //Operate in master or slave mode
    //
    static QSharedMemory shmCtrl;   //Shared memory segment for the "control signal" of a "bus" for communication between instances
    static QSharedMemory shmData;   //Shared memory segment for the "data signal" of a "bus" for communication between instances
    //
    static const std::size_t shmDataLength; //Maximum number of wide characters to be stored in the "data signal" shared memory segment
};

#endif // SINGLEINSTANCESYNCHRONIZER_H
