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

#ifndef UPDATEREPORTPERSONENTRYDIALOG_H
#define UPDATEREPORTPERSONENTRYDIALOG_H

#include "person.h"

#include <QDialog>
#include <QTime>
#include <QWidget>

namespace Ui {
class UpdateReportPersonEntryDialog;
}

/*!
 * \brief Change the personnel function and begin/end times of a Person.
 *
 * The name and identifier of a person are displayed as well as the
 * function and times initially provided to UpdateReportPersonEntryDialog().
 *
 * The function and times can be edited and then retrieved
 * by calling getFunction(), getBeginTime() and getEndTime().
 *
 * Only functions that comply with the persons qualifications can be selected.
 *
 * Editing of the times can be disabled from the constructor (see UpdateReportPersonEntryDialog()).
 */
class UpdateReportPersonEntryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpdateReportPersonEntryDialog(const Person& pPerson, Person::Function pFunction, QTime pBeginTime, QTime pEndTime,
                                           bool pDisableEditTimes = false, QWidget* pParent = nullptr);         ///< Constructor.
    ~UpdateReportPersonEntryDialog();                                                                           ///< Destructor.
    //
    Person::Function getFunction() const;   ///< Get the currently selected function.
    QTime getBeginTime() const;             ///< Get the currently set begin time.
    QTime getEndTime() const;               ///< Get the currently set end time.

private:
    Ui::UpdateReportPersonEntryDialog* ui;  //UI
};

#endif // UPDATEREPORTPERSONENTRYDIALOG_H
