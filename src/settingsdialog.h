/*
/////////////////////////////////////////////////////////////////////////////////////////
//
//  This file is part of Wachdienst-Manager, a program to manage DLRG watch duty reports.
//  Copyright (C) 2021–2025 M. Frohne
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

#ifndef WDMGR_SETTINGSDIALOG_H
#define WDMGR_SETTINGSDIALOG_H

#include "auxil.h"

#include <QDialog>
#include <QString>
#include <QWidget>

#include <map>

namespace Ui { class SettingsDialog; }

/*!
 * \brief Change program settings.
 *
 * - General program settings: Defaults for new reports, PDF export options, "password protection", etc.
 * - List of stations and boats to choose from when creating/editing a report.
 * - List of important document shortcuts for the "rescue" tab.
 *
 * If a password is set, this password must be entered in order to be able to change settings.
 * Note: this also applies to the personnel database dialog.
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* pParent = nullptr);    ///< Constructor.
    ~SettingsDialog();                                      ///< Destructor.

private:
    void connectSlots() const;              ///< Connect UI signals to their slots.
    //
    void readDatabase();                    ///< Read the settings from database.
    bool writeDatabase() const;             ///< Write the settings to database.
    //
    void updateStationsBoatsComboBoxes();   ///< Update the entries of station/boat combo boxes.
    void updateStationsInputs();            ///< Update the station inputs according to the selected station combo box entry.
    void updateBoatsInputs();               ///< Update the boat inputs according to the selected boat combo box entry.
    void resizeDocumentsTable();            ///< Fit the documents table height to contents.
    //
    void changeStationIdent(const QString& pOldIdent, Aux::Station&& pNewStation);  ///< Replace a station's changed identifier (key).
    void changeBoatName(const QString& pOldName, Aux::Boat&& pNewBoat);             ///< Replace a boat's changed name (key).
    //
    void disableDefaultStationSelection();                                          ///< Prevent editing default station.
    void disableDefaultBoatSelection();                                             ///< Prevent editing default boat.

private slots:
    void accept() override;                                                     ///< Reimplementation of QDialog::accept().
    //
    void at_settings_tabWidget_currentChanged(int index);                       ///< If documents tab selected, fit table to contents.
    //
    void at_chooseDefaultFilePath_pushButton_pressed();                         ///< Set default file path using a file dialog.
    void at_chooseXelatexPath_pushButton_pressed();                             ///< Set XeLaTeX executable using a file dialog.
    void at_chooseLogoPath_pushButton_pressed();                                ///< Set custom association logo image file path.
    void at_password_lineEdit_textEdited(const QString&);                       ///< Remember that password field was edited.
    //
    void at_stations_comboBox_currentIndexChanged(int);                         ///< Update station inputs with newly selected station.
    void at_addStation_pushButton_pressed();                                    ///< Add a new station.
    void at_removeStation_pushButton_pressed();                                 ///< Remove the selected station.
    void at_stationLocation_lineEdit_textEdited(const QString& arg1);           ///< Change the selected station's location.
    void at_stationName_lineEdit_textEdited(const QString& arg1);               ///< Change the selected station's name.
    void at_localGroup_lineEdit_textEdited(const QString& arg1);                ///< Change the selected station's local group.
    void at_districtAssociation_lineEdit_textEdited(const QString& arg1);       ///< Change the selected station's district association.
    void at_stationRadioCallName_lineEdit_textEdited(const QString& arg1);      ///< Change the selected station's radio call name.
    void at_stationRadioCallNameAlt_lineEdit_textEdited(const QString& arg1);   ///< Change the selected station's alt. radio call name.
    void at_copyStationRadioCallNameAlt_radioButton_toggled(bool checked);      ///< Copy the radio call name to the alternative one.
    //
    void at_boats_comboBox_currentIndexChanged(int);                            ///< Update boat inputs with newly selected boat.
    void at_addBoat_pushButton_pressed();                                       ///< Add a new boat.
    void at_removeBoat_pushButton_pressed();                                    ///< Remove the selected boat.
    void at_boatName_lineEdit_textEdited(const QString& arg1);                  ///< Change the selected boat's name.
    void at_boatAcronym_lineEdit_textEdited(const QString& arg1);               ///< Change the selected boat's acronym.
    void at_boatType_lineEdit_textEdited(const QString& arg1);                  ///< Change the selected boat's type.
    void at_boatFuelType_comboBox_currentTextChanged(const QString& arg1);      ///< Change the selected boat's fuel type.
    void at_boatRadioCallName_lineEdit_textEdited(const QString& arg1);         ///< Change the selected boat's radio call name.
    void at_boatRadioCallNameAlt_lineEdit_textEdited(const QString& arg1);      ///< Change the selected boat's alt. radio call name.
    void at_copyBoatRadioCallNameAlt_radioButton_toggled(bool checked);         ///< Copy the radio call name to the alternative one.
    void at_boatHomeStation_comboBox_currentIndexChanged(int index);            ///< Change the selected boat's home station.
    //
    void at_numDocuments_spinBox_valueChanged(int arg1);                        ///< Adjust the number rows of the documents table.
    void at_chooseDocument_pushButton_pressed();                                ///< Set a document path using a file dialog.
    void at_documents_tableWidget_cellChanged(int row, int column);             ///< \brief Reset table item if it contains a character
                                                                                ///  used to separate documents and names/paths.
    //
    void at_singleInstance_checkBox_stateChanged(int arg1);         ///< Show restart hint when newly activating single instance mode.

private:
    Ui::SettingsDialog* ui;                     //UI
    //
    bool acceptDisabled;                        //Disable accepting the dialog and writing to the database
    //
    bool passwordEdited;                        //Has password input been edited?
    //
    std::map<QString, Aux::Station> stations;   //Map of loaded/added/edited stations with station identifier as key
    std::map<QString, Aux::Boat> boats;         //Map of loaded/added/edited boats with boat name as key
};

#endif // WDMGR_SETTINGSDIALOG_H
