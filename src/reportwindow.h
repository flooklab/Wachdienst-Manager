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

#ifndef REPORTWINDOW_H
#define REPORTWINDOW_H

#include "auxil.h"
#include "report.h"
#include "boatdrive.h"
#include "databasecache.h"
#include "settingscache.h"
#include "pdfexporter.h"
#include "qualificationchecker.h"
#include "personneleditordialog.h"
#include "updatereportpersonentrydialog.h"

#include <vector>
#include <map>
#include <thread>
#include <atomic>

#include <QString>
#include <QStringList>
#include <QRegularExpressionValidator>
#include <QTimer>
#include <QShortcut>
#include <QKeySequence>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QDate>
#include <QTime>

#include <QMainWindow>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QTableWidget>
#include <QPlainTextEdit>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QLabel>
#include <QStatusBar>
#include <QCompleter>
#include <QLCDNumber>
#include <QCalendarWidget>
#include <QItemSelectionModel>
#include <QModelIndexList>

namespace Ui {
class ReportWindow;
}

/*!
 * \brief View, fill or edit a Report.
 *
 * Displays the loaded or newly created report.
 * All report data can be edited using this window
 * and then saved and/or exported as PDF.
 *
 * The widget contents and internal report instance are always kept in sync.
 * An exception is the data for the currently selected boat drive,
 * which is only written back when the "Apply" button is pressed.
 *
 * All persons from the personnel database (internal persons)
 * can be directly added as personnel. External persons can also
 * be individually created and added for each report. Both internal
 * and external persons can be used as boatmen or boat crew members
 * according to their set qualifications. The used personnel is
 * always stored along with the report for archival purposes,
 * such that old reports can be safely opened/edited
 * independently of the curent state of the database.
 *
 * Note: If not already done while creating the report,
 * one can also load the carryovers from the last report
 * in this window from the "File" sub-menu.
 *
 * Note: If the personnel table in the exported PDF file is
 * split and continued on the next page with only very few
 * remaining entries in the continued table, then it can be
 * worth a try to squeeze the whole table on the first page by
 * increasing the maximum table length via the "Extras" sub-menu.
 */
class ReportWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ReportWindow(Report&& pReport, QWidget *const pParent = nullptr);  ///< Constructor.
    ~ReportWindow();                                                            ///< Destructor.

private:
    virtual void closeEvent(QCloseEvent* event);                                ///< Reimplementation of QMainWindow::closeEvent().
    //
    void loadReportData();                                                      ///< Fill all widgets with the report data.
    //
    void saveReport(const QString& pFileName);                                  ///< Save the report.
    void exportReport(const QString& pFileName, bool pAskOverwrite);            ///< Export the report.
    void autoExport();                                      ///< Export to automatic or manual file name depending on setting.
    //
    void setUnsavedChanges(bool pValue = true);             ///< Set whether there are unsaved changes and update title.
    void setUnappliedBoatDriveChanges(bool pValue = true);  ///< Set whether there are not applied boat drive changes and update title.
    //
    bool checkInvalidValues();                              ///< Check for severe mistakes i.e. values that do not make sense.
    bool checkImplausibleValues();                          ///< Check for valid but improbable or forgotten values.
    //
    void updateWindowTitle();                               ///< Update the window title.
    void updateTotalPersonnelHours();                       ///< Update the total (carry + new) personnel hours display.
    void updateTotalBoatHours();                            ///< Update the total (carry + new) boat drive hours display.
    void updatePersonnelHours();                            ///< Update the accumulated personnel hours display.
    void updatePersonnelTable();                            ///< Update the personnel table widget (and personnel hours).
    void updateBoatDrivesFuel();                            ///< Update the spin box summing up fuel added after individual boat drives.
    void updateBoatDrivesHours();                           ///< Update the accumulated boat drive hours display.
    void updateBoatDrivesTable();                           ///< Update the boat drives table widget (and fuel and boat drive hours).
    void updateBoatDriveAvailablePersons();                 ///< Update the list of persons selectable as boatman or crew member.
    //
    void insertBoatCrewTableRow(const Person& pPerson, Person::BoatFunction pFunction); ///< Add a person to the crew member table.
    //
    void checkPersonInputs();                       ///< Check entered person name and update selectable identifiers list accordingly.
    //
    std::vector<QString> getSelectedPersons() const;            ///< Get all selected personnel table entries' person identifiers.
    //
    void setSerialNumber(int pNumber);                          ///< Set report number if larger zero (and update display and buttons).
    void setPersonnelHoursCarry(int pMinutes);                  ///< Set hours and minutes display of personnel hours carry.
    void setBoatHoursCarry(int pMinutes);                       ///< Set hours and minutes display of boat drive hours carry.
    //
    bool personWithFunctionPresent(Person::Function pFunction) const;   ///< Check, if a function is set for any person of personnel.
    bool personInUse(const QString& pIdent) const;                      ///< Check, if a person is boatman or crew member of any drive.
    //
    QString personLabelFromIdent(const QString& pIdent) const;          ///< Get a formatted combo box label from a person identifier.
    QString personIdentFromLabel(const QString& pLabel) const;          ///< Get the person identifier from a combo box label.

private slots:
    void on_rescueOperationSpinBoxValueChanged(int pValue,
                                               Report::RescueOperation pRescue);    ///< Set report rescue operation counter.
    void on_openDocumentPushButtonPressed(const QString& pDocFile);                 ///< Open one of the important documents.
    void on_updateClocksTimerTimeout();                                             ///< Update the time displayed in every tab.
    void on_timestampShortcutActivated();                                           ///< Show (non-modal) message box with current time.
    void on_exportFailed();                                                         ///< Show message box explaining that export failed.
    //
    void on_saveFile_action_triggered();                                            ///< Save the report to (the same) file.
    void on_saveFileAs_action_triggered();                                          ///< Save the report to a (different) file.
    void on_exportFile_action_triggered();                                          ///< Export the report as a PDF file.
    void on_loadCarries_action_triggered();                                         ///< Load old report carryovers from a file.
    void on_close_action_triggered();                                               ///< Close the report window.
    void on_editPersonnelListSplit_action_triggered();                              ///< Change the maximum PDF personnel table length.
    //
    void on_reportTab_calendarWidget_currentPageChanged(int year, int month);       ///< Synchronize the calendar widgets of every tab.
    void on_boatTab_calendarWidget_currentPageChanged(int year, int month);         ///< Synchronize the calendar widgets of every tab.
    void on_rescueTab_calendarWidget_currentPageChanged(int year, int month);       ///< Synchronize the calendar widgets of every tab.
    void on_reportTab_calendarWidget_selectionChanged();                            ///< Synchronize the calendar widgets of every tab.
    void on_boatTab_calendarWidget_selectionChanged();                              ///< Synchronize the calendar widgets of every tab.
    void on_rescueTab_calendarWidget_selectionChanged();                            ///< Synchronize the calendar widgets of every tab.
    //
    void on_reportNumberDecr_radioButton_toggled(bool checked);                     ///< Reduce the report serial number by one.
    void on_reportNumberIncr_radioButton_toggled(bool checked);                     ///< Increase the report serial number by one.
    void on_station_comboBox_currentTextChanged(const QString& arg1);               ///< \brief Set the report station and update
                                                                                    ///  selectable radio call names.
    void on_stationRadioCallName_comboBox_currentTextChanged(const QString& arg1);  ///< Set the report station radio call name.
    void on_dutyPurpose_comboBox_currentTextChanged(const QString& arg1);           ///< Set the report duty purpose.
    void on_dutyPurposeComment_lineEdit_textEdited(const QString& arg1);            ///< Set the report duty purpose comment.
    void on_reportDate_dateEdit_dateChanged(const QDate& date);                     ///< Set the report date.
    void on_dutyTimesBegin_timeEdit_timeChanged(const QTime& time);                 ///< Set the report duty begin time.
    void on_dutyTimesEnd_timeEdit_timeChanged(const QTime& time);                   ///< Set the report duty end time.
    void on_reportComments_plainTextEdit_textChanged();                             ///< Set the report comments.
    //
    void on_temperatureAir_spinBox_valueChanged(int arg1);                          ///< Set the report air temperature.
    void on_temperatureWater_spinBox_valueChanged(int arg1);                        ///< Set the report water temperature.
    void on_precipitation_comboBox_currentTextChanged(const QString& arg1);         ///< Set the report precipitation type.
    void on_cloudiness_comboBox_currentTextChanged(const QString& arg1);            ///< Set the report level of cloudiness.
    void on_windStrength_comboBox_currentTextChanged(const QString& arg1);          ///< Set the report wind strength.
    void on_weatherComments_plainTextEdit_textChanged();                            ///< Set the report weather comments.
    //
    void on_operationProtocolsCtr_spinBox_valueChanged(int arg1);           ///< Set the report number of enclosed operation protocols.
    void on_patientRecordsCtr_spinBox_valueChanged(int arg1);               ///< Set the report number of enclosed patient records.
    void on_radioCallLogsCtr_spinBox_valueChanged(int arg1);                ///< Set the report number of enclosed radio call logs.
    void on_otherEnclosures_lineEdit_textEdited(const QString& arg1);       ///< Set the report list of other enclosures.
    void on_otherEnclosures_lineEdit_textChanged(const QString& arg1);      ///< Update line edit tool tip and enclosures list label.
    //
    void on_personLastName_lineEdit_textChanged(const QString&);            ///< Check entered person name and update dependent widgets.
    void on_personFirstName_lineEdit_textChanged(const QString&);           ///< Check entered person name and update dependent widgets.
    void on_personIdent_comboBox_currentTextChanged(const QString& arg1);   ///< Update selectable personnel functions.
    void on_addPerson_pushButton_pressed();                                 ///< Add the selected person to the report personnel list.
    void on_addExtPerson_pushButton_pressed();                              ///< Add an external person to the report personnel list.
    void on_updatePerson_pushButton_pressed();                              ///< Change selected persons' functions and times.
    void on_removePerson_pushButton_pressed();                              ///< Remove selected persons from personnel.
    void on_setPersonTimeBegin_pushButton_pressed();                ///< Set selected persons' begin times to begin time edit's time.
    void on_setPersonTimeEnd_pushButton_pressed();                  ///< Set selected persons' end times to end time edit's time.
    void on_setPersonTimeBeginNow_pushButton_pressed();             ///< Set selected persons' begin times to now (nearest quarter).
    void on_setPersonTimeEndNow_pushButton_pressed();               ///< Set selected persons' end times to now (nearest quarter).
    void on_personnel_tableWidget_cellDoubleClicked(int, int);              ///< See on_updatePerson_pushButton_pressed().
    //
    void on_personnelHoursHours_spinBox_valueChanged(int);                  ///< Update total personnel hours display.
    void on_personnelHoursMinutes_spinBox_valueChanged(int);                ///< Update total personnel hours display.
    void on_personnelHoursCarryHours_spinBox_valueChanged(int arg1);        ///< Set personnel hours carry, update total hours display.
    void on_personnelHoursCarryMinutes_spinBox_valueChanged(int arg1);      ///< Set personnel hours carry, update total hours display.
    //
    void on_boat_comboBox_currentTextChanged(const QString& arg1);      ///< Set the report boat and update selectable radio call names.
    void on_boatRadioCallName_comboBox_currentTextChanged(const QString& arg1); ///< Set the report boat radio call name.
    void on_boatSlippedBeginOfDuty_checkBox_stateChanged(int arg1);             ///< Set, if boat was lowered to water at begin of duty.
    void on_boatSlippedEndOfDuty_checkBox_stateChanged(int arg1);               ///< Set, if boat was taken out of water at end of duty.
    void on_boatReadyFrom_timeEdit_timeChanged(const QTime& time);              ///< Set report boat readiness time begin.
    void on_boatReadyUntil_timeEdit_timeChanged(const QTime& time);             ///< Set report boat readiness time end.
    //
    void on_boatComments_plainTextEdit_textChanged();                           ///< Set report boat comments.
    //
    void on_engineHoursBeginOfDuty_doubleSpinBox_valueChanged(double arg1);     ///< Set report engine hours at begin of duty.
    void on_engineHoursEndOfDuty_doubleSpinBox_valueChanged(double arg1);       ///< Set report engine hours at end of duty.
    //
    void on_fuelBeginOfDuty_spinBox_valueChanged(int arg1);     ///< Set report fuel added at begin of duty and update total display.
    void on_fuelAfterDrives_spinBox_valueChanged(int arg1);     ///< Update total added fuel display.
    void on_fuelEndOfDuty_spinBox_valueChanged(int arg1);       ///< Set report fuel added at end of duty and update total display.
    //
    void on_boatDrives_tableWidget_currentCellChanged(int currentRow, int, int previousRow, int);
                                                                    ///< \brief Display data of the newly selected boat drive or revert
                                                                    ///  the table selection in case there are unapplied changes.
    void on_addBoatDrive_pushButton_pressed();                      ///< Add new boat drive to report (after selected one).
    void on_removeBoatDrive_pushButton_pressed();                   ///< Remove the selected boat drive from the report.
    void on_moveBoatDriveUp_pushButton_pressed();                   ///< Change the order of selected drive and drive above.
    void on_moveBoatDriveDown_pushButton_pressed();                 ///< Change the order of selected drive and drive below.
    void on_setBoatDriveTimeBeginNow_pushButton_pressed();          ///< Set displayed boat drive begin time to now (nearest quarter).
    void on_setBoatDriveTimeEndNow_pushButton_pressed();            ///< Set displayed boat drive end time to now (nearest quarter).
    void on_splitBoatDrive_pushButton_pressed();                    ///< Split drive into two drives at current time (nearest quarter).
    void on_applyBoatDriveChanges_pushButton_pressed();             ///< Apply changed boat drive data to selected boat drive in report.
    void on_discardBoatDriveChanges_pushButton_pressed();           ///< Discard changed boat drive data and reload selected drive data.
    //
    void on_boatDrivePurpose_comboBox_currentTextChanged(const QString&);       ///< Set unapplied boat drive changes.
    void on_boatDriveBegin_timeEdit_timeChanged(const QTime& time);             ///< Set unapplied boat drive changes.
    void on_boatDriveEnd_timeEdit_timeChanged(const QTime& time);               ///< Set unapplied boat drive changes.
    void on_boatDriveFuel_spinBox_valueChanged(int);                            ///< Set unapplied boat drive changes.
    void on_boatDriveBoatman_comboBox_currentTextChanged(const QString& arg1);  ///< Check boatman, set unapplied boat drive changes.
    void on_boatDriveComments_plainTextEdit_textChanged();                      ///< Set unapplied boat drive changes.
    void on_boatCrewMember_comboBox_currentTextChanged(const QString& arg1);    ///< \brief Update selectable boat functions according to
                                                                                ///  selected new crew member person's qualifications.
    void on_addBoatCrewMember_pushButton_pressed();         ///< Add crew member to displayed drive's crew member table.
    void on_removeBoatCrewMember_pushButton_pressed();      ///< Remove selected crew member from displayed drive's crew member table.
    //
    void on_boatHoursHours_spinBox_valueChanged(int);               ///< Update total boat hours display.
    void on_boatHoursMinutes_spinBox_valueChanged(int);             ///< Update total boat hours display.
    void on_boatHoursCarryHours_spinBox_valueChanged(int arg1);     ///< Set report boat hours carry and update total boat hours display.
    void on_boatHoursCarryMinutes_spinBox_valueChanged(int arg1);   ///< Set report boat hours carry and update total boat hours display.
    //
    void on_assignmentNumber_lineEdit_textEdited(const QString& arg1);      ///< Set report assignment number.

signals:
    void closed();          ///< Signal emitted when window closes (for re-showing startup dialog).
    void exportFailed();    ///< Signal emitted by export thread on failure to show message box in main thread.

private:
    Ui::ReportWindow *ui;   //UI
    //
    std::map<Report::RescueOperation, QSpinBox *const> rescuesSpinBoxes;            //Pointers to a spin box for each rescue type
    std::map<Report::RescueOperation, QLabel *const> rescuesFillDocNoticeLabels;    //Pointers to a label for each rescue type
    //
    QLabel* statusBarLabel;                     //Permanent label in the status bar
    //
    std::map<QString, Aux::Station> stations;   //Map of stations with station identifier as key
    std::map<QString, Aux::Boat> boats;         //Map of boats with boat name as key
    //
    Report report;                              //The report instance
    std::shared_ptr<BoatLog> boatLogPtr;        //Shared pointer to the report's boat log
    //
    bool unsavedChanges;                        //Any changes not saved to file yet?
    bool unappliedBoatDriveChanges;             //Any not applied changes to currently selected boat drive?
    //
    bool revertingDriveSelection;               //Switch to help reverting selected boat drive table row in case of unapplied changes
    //
    std::atomic_bool exporting;                 //Export thread currently running?
    //
    int exportPersonnelTableMaxLength;          //Maximum length of exported PDFs personnel table; will be split if length is exceeded
    //
    QString loadedStation;                  //Initial station identifier from loaded report, if not found in database (else empty)
    QString loadedStationRadioCallName;     //Initial radio call name from loaded report, if station not found in database (else empty)
    QString loadedBoat;                     //Initial boat name from loaded report, if not found in database (else empty)
    QString loadedBoatRadioCallName;        //Initial radio call name from loaded report, if boat not found in database (else empty)
    //
    QString selectedBoatmanIdent;           //Person identifier of currently selected boatman combo box item
};

#endif // REPORTWINDOW_H
