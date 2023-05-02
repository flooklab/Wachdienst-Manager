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

#ifndef NEWREPORTDIALOG_H
#define NEWREPORTDIALOG_H

#include "auxil.h"
#include "report.h"

#include <QDialog>
#include <QString>
#include <QWidget>

#include <map>

namespace Ui {
class NewReportDialog;
}

/*!
 * \brief Create a new Report with default settings.
 *
 * Creates a new report and loads and displays the default report
 * configuration options from the database. Basic configuration
 * can be changed using some input widgets and will be applied
 * to the new report on accepting the dialog. When accepted,
 * the configured new report can be obtained by getReport().
 *
 * Configuration options are:
 * - Date, time
 * - Duty purpose, comment
 * - Used station
 * - Used boat
 * - Carryovers from last report
 */
class NewReportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewReportDialog(QWidget* pParent = nullptr);           ///< Constructor.
    ~NewReportDialog();                                             ///< Destructor.
    //
    void getReport(Report& pReport) const;                          ///< Get the new report.

private slots:
    void accept() override;                                         ///< Reimplementation of QDialog::accept().
    //
    void on_stackedWidget_currentChanged(int arg1);                 ///< Update progress bar value/label and navigation button labels.
    void on_previous_pushButton_pressed();                          ///< Go to previous page or reject the dialog, if on first page.
    void on_next_pushButton_pressed();                              ///< Go to next page or accept the dialog, if on last page.
    //
    void on_station_comboBox_currentTextChanged(const QString& arg1);   ///< Update selectable radio call names from selected station.
    void on_boat_comboBox_currentTextChanged(const QString& arg1);      ///< Update selectable radio call names from selected boat.
    void on_clearStation_radioButton_toggled(bool checked);             ///< Clear station selection (and reset the radio button).
    void on_clearBoat_radioButton_toggled(bool checked);                ///< Clear boat selection (and reset the radio button).
    //
    void on_noLoadLastReportCarries_radioButton_toggled(bool checked);  ///< Reset file name selected/displayed for last report.
    void on_loadLastReportCarries_radioButton_toggled(bool checked);    ///< Select a file name to load last report's carryovers from.
    void on_loadLastReportCarries_radioButton_pressed();                ///< See on_loadLastReportCarries_radioButton_toggled().

private:
    Ui::NewReportDialog* ui;                    //UI
    //
    Report report;                              //The new report
    //
    std::map<QString, Aux::Station> stations;   //Map of stations with station identifier as key
    std::map<QString, Aux::Boat> boats;         //Map of boats with boat name as key
};

#endif // NEWREPORTDIALOG_H
