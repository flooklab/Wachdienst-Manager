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

#include "singleinstancesynchronizer.h"

#include <chrono>
#include <string>
#include <thread>

//Initialize static class members

bool SingleInstanceSynchronizer::initialized = false;
bool SingleInstanceSynchronizer::master = false;
//
QSharedMemory SingleInstanceSynchronizer::shmCtrl = QSharedMemory("wd.mgr-sync-bus-ctrl");
QSharedMemory SingleInstanceSynchronizer::shmData = QSharedMemory("wd.mgr-sync-bus-data");
//
const std::size_t SingleInstanceSynchronizer::shmDataLength = 4096;

//Public

/*!
 * \brief Initialize the bus connection and determine if master or slave instance.
 *
 * Tries to attach to the shared memory segments used for the bus connection.
 * If no other instance has been attached yet then this instance is going to be
 * the master instance (isMaster() returns true) and is going to be a slave instance
 * otherwise. In case of the master instance the bus signals will be initialized to
 * be in the idle state. If no error occured then isInitialized() will return true.
 *
 * Returns true immediately if already initialized before (and detach() not called since then).
 *
 * \return If successful or already initialized.
 */
bool SingleInstanceSynchronizer::init()
{
    if (initialized)
        return true;

    initialized = false;

    //Try to create the two shared memory segments required for the bus operation
    bool masterCtrl = shmCtrl.create(1, QSharedMemory::AccessMode::ReadWrite);
    bool masterData = shmData.create(shmDataLength*sizeof(wchar_t), QSharedMemory::AccessMode::ReadWrite);

    //Either both segments should already exist (use "slave" mode) or not exist (use "master" mode)
    if (masterCtrl != masterData)
        return false;

    if (!masterCtrl && shmCtrl.error() != QSharedMemory::AlreadyExists)
        return false;

    master = masterCtrl;

    //Initialize bus signals if master or simply attach to bus otherwise

    if (master)
    {
        if (!shmCtrl.lock())
            return false;

        *(static_cast<int8_t*>(shmCtrl.data())) = static_cast<int8_t>(BusCtrlSymbol::_IDLE);

        if (!shmCtrl.unlock())
            return false;

        if (!shmData.lock())
            return false;

        //Set data to empty string
        std::wcsncpy(static_cast<wchar_t*>(shmData.data()), std::wstring(shmDataLength, L'\0').data(), shmDataLength);

        if (!shmData.unlock())
            return false;
    }
    else
    {
        if (!shmCtrl.attach(QSharedMemory::AccessMode::ReadWrite) || !shmData.attach(QSharedMemory::AccessMode::ReadWrite))
            return false;
    }

    initialized = true;

    return initialized;
}

/*!
 * \brief Disconnect the instance from the bus.
 *
 * Disconnects from the bus by detaching the instance from the shared memory segments.
 * In the following, isInitialized() will return false.
 *
 * \return If successful (or false if not initialized).
 */
bool SingleInstanceSynchronizer::detach()
{
    if (!initialized)
        return false;

    initialized = false;

    bool success = shmCtrl.detach();
    success &= shmData.detach();

    return success;
}

//

/*!
 * \brief Check if the bus is initialized and the instance connected to it.
 *
 * Returns true if init() has been called and it succeeded.
 * Returns false otherwise or if detach() has been called after init().
 *
 * \return If initialized (and not detached again).
 */
bool SingleInstanceSynchronizer::isInitialized()
{
    return initialized;
}

/*!
 * \brief Check if the instance connected to the bus is the master instance.
 *
 * Returns true if init() determined the calling instance to be the master instance and false otherwise.
 *
 * \return If current instance is master instance (which controls the bus and processes the received requests).
 */
bool SingleInstanceSynchronizer::isMaster()
{
    return master;
}

//

/*!
 * \brief Send request to start a new report to the master instance via the bus.
 *
 * Waits until the bus is idle and then sends the request to the master instance
 * by setting the bus' control signal accordingly. Returns in case of an error.
 *
 * Returns immediately if bus not initialized or if isMaster().
 */
void SingleInstanceSynchronizer::sendNewReport()
{
    if (!initialized)
        return;

    //Only send requests from "slave" to "master"
    if (master)
        return;

    //Wait until bus is idle and then send new report request by changing control signal accordingly
    while (true)
    {
        if (!shmCtrl.lock())
            return;

        int8_t& ctrlVal = *(static_cast<int8_t*>(shmCtrl.data()));

        if (ctrlVal == static_cast<int8_t>(BusCtrlSymbol::_IDLE))
        {
            ctrlVal = static_cast<int8_t>(BusCtrlSymbol::_NEW_REPORT);
            break;
        }

        if (!shmCtrl.unlock())
            return;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    shmCtrl.unlock();
}

/*!
 * \brief Send request to open existing report to the master instance via the bus.
 *
 * Waits until the bus is idle and then sends the request to the master instance by
 * setting the bus' control signal accordingly and setting the bus' data signal
 * to the report's file name \p pFileName. Returns in case of an error.
 *
 * \p pFileName can be 4096 'wchar_t's long (number not including null termination).
 *
 * Returns immediately if bus not initialized or if isMaster().
 *
 * \param pFileName File name of the report.
 */
void SingleInstanceSynchronizer::sendOpenReport(const QString& pFileName)
{
    if (!initialized)
        return;

    //Only send requests from "slave" to "master"
    if (master)
        return;

    //Wait until bus is idle and then send open report request by changing control signal accordingly
    //and setting data signal to the report's file name before releasing the control signal lock

    while (true)
    {
        if (!shmCtrl.lock())
            return;

        int8_t& ctrlVal = *(static_cast<int8_t*>(shmCtrl.data()));

        if (ctrlVal == static_cast<int8_t>(BusCtrlSymbol::_IDLE))
        {
            ctrlVal = static_cast<int8_t>(BusCtrlSymbol::_OPEN_REPORT);
            break;
        }

        if (!shmCtrl.unlock())
            return;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!shmData.lock())
        return;

    std::wstring fileNameWStr = pFileName.toStdWString();
    fileNameWStr.resize(shmDataLength);

    std::wcsncpy(static_cast<wchar_t*>(shmData.data()), fileNameWStr.c_str(), shmDataLength);

    shmData.unlock();

    shmCtrl.unlock();
}

//

/*!
 * \brief Control the bus and continuously process all incoming requests.
 *
 * Starts an infinite loop to process incoming events. In each loop iteration, waits until the bus is
 * not idle (incoming request) and then reads the request type and additional information (file name)
 * from the bus' control and data signals, respectively. Finally, resets the bus to its idle state and,
 * depending on the request type, uses \p pStartupWindow to create a report or open the specified report
 * (see StartupWindow::emitOpenAnotherReportRequested(); see also sendNewReport() and sendOpenReport()).
 *
 * When \p pStopListening becomes true the processing of the current request is completed and then the function returns.
 *
 * Returns immediately if bus not initialized or if not isMaster().
 *
 * Returns as soon as reading or writing of a bus signal fails.
 *
 * \param pStartupWindow StartupWindow that shall be used to open the report windows.
 * \param pStopListening Control variable for stopping the infinite loop.
 */
void SingleInstanceSynchronizer::listen(StartupWindow& pStartupWindow, const std::atomic_bool& pStopListening)
{
    if (!initialized)
        return;

    //Only receive requests from "slave"s by "master"
    if (!master)
        return;

    //Repeatedly check bus control signal for requests as long as 'pStopListening' is false;
    //in case of a request, process it using request type from the control signal and additional
    //information from the data signal; reset control and data signals after processing each request

    while (true)
    {
        //Wait until a request arrived on the bus; return from this function immediately if 'pStopListening' becomes true
        while (true)
        {
            if (pStopListening.load() == true)
                return;

            if (!shmCtrl.lock())
                return;

            int8_t ctrlVal = *(static_cast<const int8_t*>(shmCtrl.constData()));

            if (ctrlVal != static_cast<int8_t>(BusCtrlSymbol::_IDLE))
                break;

            if (!shmCtrl.unlock())
                return;

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        //Read request type from control signal and additional information from data signal

        if (!shmData.lock())
            return;

        std::wstring wStr(static_cast<const wchar_t*>(shmData.constData()), shmDataLength);

        wStr.resize(std::wcslen(wStr.c_str()));

        //Reset data to empty string
        std::wcsncpy(static_cast<wchar_t*>(shmData.data()), std::wstring(shmDataLength, L'\0').data(), shmDataLength);

        int8_t ctrlVal = *(static_cast<const int8_t*>(shmCtrl.constData()));

        //Reset control signal to idle state (i.e. no current request)
        *(static_cast<int8_t*>(shmCtrl.data())) = static_cast<int8_t>(BusCtrlSymbol::_IDLE);

        //Forward processed request to 'pStartupWindow'

        if (ctrlVal == static_cast<int8_t>(BusCtrlSymbol::_NEW_REPORT))
            pStartupWindow.emitOpenAnotherReportRequested("");
        else if (ctrlVal == static_cast<int8_t>(BusCtrlSymbol::_OPEN_REPORT))
            pStartupWindow.emitOpenAnotherReportRequested(QString::fromStdWString(wStr));

        if (!shmData.unlock())
            return;

        if (!shmCtrl.unlock())
            return;
    }
}
