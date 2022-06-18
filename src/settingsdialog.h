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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "auxil.h"
#include "databasecache.h"
#include "settingscache.h"

#include <iostream>
#include <map>
#include <thread>
#include <chrono>

#include <QString>
#include <QStringList>
#include <QRegularExpressionValidator>
#include <QMargins>
#include <QRect>
#include <QList>

#include <QDialog>
#include <QFileDialog>
#include <QLayout>
#include <QMessageBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QTimeEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QAbstractButton>
#include <QTabWidget>

namespace Ui {
class SettingsDialog;
}

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
    explicit SettingsDialog(QWidget *const pParent = nullptr);  ///< Constructor.
    ~SettingsDialog();                                          ///< Destructor.

private:
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
    virtual void accept();                                                      ///< Reimplementation of QDialog::accept().
    //
    void on_settings_tabWidget_currentChanged(int index);                       ///< If documents tab selected, fit table to contents.
    //
    void on_chooseDefaultFilePath_pushButton_pressed();                         ///< Set default file path using a file dialog.
    void on_chooseXelatexPath_pushButton_pressed();                             ///< Set XeLaTeX executable using a file dialog.
    void on_password_lineEdit_textEdited(const QString&);                       ///< Remember that password field was edited.
    //
    void on_stations_comboBox_currentIndexChanged(int);                         ///< Update station inputs with newly selected station.
    void on_addStation_pushButton_pressed();                                    ///< Add a new station.
    void on_removeStation_pushButton_pressed();                                 ///< Remove the selected station.
    void on_stationLocation_lineEdit_textEdited(const QString& arg1);           ///< Change the selected station's location.
    void on_stationName_lineEdit_textEdited(const QString& arg1);               ///< Change the selected station's name.
    void on_localGroup_lineEdit_textEdited(const QString& arg1);                ///< Change the selected station's local group.
    void on_districtAssociation_lineEdit_textEdited(const QString& arg1);       ///< Change the selected station's district association.
    void on_stationRadioCallName_lineEdit_textEdited(const QString& arg1);      ///< Change the selected station's radio call name.
    void on_stationRadioCallNameAlt_lineEdit_textEdited(const QString& arg1);   ///< Change the selected station's alt. radio call name.
    void on_copyStationRadioCallNameAlt_radioButton_toggled(bool checked);      ///< Copy the radio call name to the alternative one.
    //
    void on_boats_comboBox_currentIndexChanged(int);                            ///< Update boat inputs with newly selected boat.
    void on_addBoat_pushButton_pressed();                                       ///< Add a new boat.
    void on_removeBoat_pushButton_pressed();                                    ///< Remove the selected boat.
    void on_boatName_lineEdit_textEdited(const QString& arg1);                  ///< Change the selected boat's name.
    void on_boatAcronym_lineEdit_textEdited(const QString& arg1);               ///< Change the selected boat's acronym.
    void on_boatType_lineEdit_textEdited(const QString& arg1);                  ///< Change the selected boat's type.
    void on_boatFuelType_comboBox_currentTextChanged(const QString& arg1);      ///< Change the selected boat's fuel type.
    void on_boatRadioCallName_lineEdit_textEdited(const QString& arg1);         ///< Change the selected boat's radio call name.
    void on_boatRadioCallNameAlt_lineEdit_textEdited(const QString& arg1);      ///< Change the selected boat's alt. radio call name.
    void on_copyBoatRadioCallNameAlt_radioButton_toggled(bool checked);         ///< Copy the radio call name to the alternative one.
    void on_boatHomeStation_comboBox_currentIndexChanged(int index);            ///< Change the selected boat's home station.
    //
    void on_numDocuments_spinBox_valueChanged(int arg1);                        ///< Adjust the number rows of the documents table.
    void on_chooseDocument_pushButton_pressed();                                ///< Set a document path using a file dialog.
    void on_documents_tableWidget_cellChanged(int row, int column);             ///< \brief Reset table item if it contains a character
                                                                                ///  used to separate documents and names/paths.

private:
    Ui::SettingsDialog *ui;                     //UI
    //
    bool acceptDisabled;                        //Disable accepting the dialog and writing to the database
    //
    bool passwordEdited;                        //Has password input been edited?
    //
    std::map<QString, Aux::Station> stations;   //Map of loaded/added/edited stations with station identifier as key
    std::map<QString, Aux::Boat> boats;         //Map of loaded/added/edited boats with boat name as key
};

#endif // SETTINGSDIALOG_H
