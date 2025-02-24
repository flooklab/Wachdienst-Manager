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

#ifndef WDMGR_REPORTWINDOW_H
#define WDMGR_REPORTWINDOW_H

#include "auxil.h"
#include "report.h"

#include <QCloseEvent>
#include <QDate>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QTimeEdit>
#include <QWidget>

#include <atomic>
#include <list>
#include <map>
#include <memory>
#include <vector>

namespace Ui { class ReportWindow; }

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
 * All persons from the personnel database (internal persons) can be directly added as personnel.
 * External persons can also be individually created and added as personnel for each report. All
 * personnel can be directly used as boatmen or boat crew members according to their set qualifications.
 * External persons that do not belong to the duty personnel can also be manually added as crew members.
 * The used personnel is always stored along with the report for archival purposes, such that old
 * reports can be safely opened/edited independently of the curent state of the database.
 *
 * If the boat log keeping has been disabled via the "app_boatLog_disabled" setting then
 * the boat log tab will be hidden. Note, however, that any boat log information that might
 * have been loaded from a report file with filled boat log will be kept and saved again when
 * saving the report in order to prevent accidental data loss. When exporting the report
 * to PDF then no boat log page will be generated (see PDFExporter::reportToLaTeX()).
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
 * The same applies to the boat drives table as well.
 */
class ReportWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ReportWindow(Report&& pReport, QWidget* pParent = nullptr);        ///< Constructor.
    ~ReportWindow();                                                            ///< Destructor.

private:
    void closeEvent(QCloseEvent* pEvent) override;                              ///< Reimplementation of QMainWindow::closeEvent().
    //
    void dragEnterEvent(QDragEnterEvent* pEvent) override;                      ///< Reimplementation of QMainWindow::dragEnterEvent().
    void dropEvent(QDropEvent* pEvent) override;                                ///< Reimplementation of QMainWindow::dropEvent().
    //
    void connectSlots() const;                                                  ///< Connect UI signals to their slots.
    //
    void loadReportData();                                                      ///< Fill all widgets with the report data.
    //
    void saveReport(const QString& pFileName);                                  ///< Save the report.
    void autoSave();                                                            ///< Save the report to a standard location (as backup).
    void exportReportToFileName(const QString& pFileName, bool pAskOverwrite);  ///< Export the report.
    void exportReport(bool pOpenPDF = false);                                   ///< Ask for a PDF file name and export the report.
    void autoExport();                                      ///< Export to automatic or manual file name depending on setting.
    //
    void setUnsavedChanges(bool pValue = true);             ///< Set whether there are unsaved changes and update title.
    void setUnappliedBoatDriveChanges(bool pValue = true);  ///< Set whether there are not applied boat drive changes and update title.
    //
    bool checkInvalidValues();                              ///< Check for severe mistakes i.e. values that do not make sense.
    bool checkImplausibleValues();                          ///< Check for valid but improbable or forgotten values.
    //
    void updateWindowTitle();                               ///< Update the window title.
    void updatePersonLastNameCompletions();                 ///< Update last name completions according to currently entered first name.
    void updatePersonFirstNameCompletions();                ///< Update first name completions according to currently entered last name.
    void updateTotalPersonnelHours();                       ///< Update the total (carry + new) personnel hours display.
    void updateTotalBoatHours();                            ///< Update the total (carry + new) boat drive hours display.
    void updatePersonnelHours();                            ///< Update the accumulated personnel hours display.
    void updatePersonnelTable(std::vector<QString> pAddToSelection = {});   ///< Update the personnel table widget (and personnel hours).
    void updateBoatDrivesFuel();                            ///< Update the spin box summing up fuel added after individual boat drives.
    void updateBoatDrivesHours();                           ///< Update the accumulated boat drive hours display.
    void updateBoatDrivesTable();                           ///< Update the boat drives table widget (and fuel and boat drive hours).
    void updateBoatDriveAvailablePersons();                 ///< Update the list of persons selectable as boatman or crew member.
    void updateReportResourcesList();                       ///< Set report resources list according to resources entered in UI table.
    void applyBoatDriveChanges(int pRow);                   ///< Apply changed boat drive data to specified boat drive in report.
    //
    void insertBoatCrewTableRow(const Person& pPerson, Person::BoatFunction pFunction); ///< Add a person to the crew member table.
    void addResourcesTableRow(QString pName = "", QTime pBeginTime = QTime(0, 0),
                                                  QTime pEndTime = QTime(0, 0));    ///< Add a resources table row for a new resource.
    //
    void checkPersonInputs();                       ///< Check entered person name and update selectable identifiers list accordingly.
    //
    std::vector<QString> getSelectedPersons() const;            ///< Get all selected personnel table entries' person identifiers.
    //
    void setSerialNumber(int pNumber);                          ///< Set report number if larger zero (and update display and buttons).
    void setPersonnelHoursCarry(int pMinutes);                  ///< Set hours and minutes display of personnel hours carry.
    void setBoatHoursCarry(int pMinutes);                       ///< Set hours and minutes display of boat drive hours carry.
    //
    int countPersonsWithFunction(Person::Function pFunction) const;     ///< Check how often a function is set for persons of personnel.
    bool personWithFunctionPresent(Person::Function pFunction) const;   ///< Check if a function is set for any person of personnel.
    //
    bool personUsedForBoatDrive(const QString& pIdent) const;           ///< Check if a person is boatman or crew member of any drive.
    bool personUsedAsBoatman(const QString& pIdent) const;              ///< Check if a person is boatman of any drive.
    bool personUsedAsBoatCrewMember(const QString& pIdent) const;       ///< Check if a person is crew member of any drive.
    //
    QString personLabelFromIdent(const QString& pIdent) const;          ///< Get a formatted combo box label from a person identifier.
    QString personIdentFromLabel(const QString& pLabel) const;          ///< Get the person identifier from a combo box label.

private slots:
    void at_rescueOperationSpinBoxValueChanged(int pValue,
                                               Report::RescueOperation pRescue);    ///< Set report rescue operation counter.
    void at_openDocumentPushButtonPressed(const QString& pDocFile);                 ///< Open one of the important documents.
    void at_resourceRemovePushButtonPressed(const QPushButton* pRemoveRowButton);   ///< Remove a used resource from the list.
    void at_resourceLineEditTextEdited(const QPushButton* pRemoveRowButton);        ///< Change the radio call name of a used resource.
    void at_resourceLineEditReturnPressed(const QPushButton* pRemoveRowButton);     ///< Add an empty row for another used resource.
    void at_resourceTimeEditTimeChanged(const QPushButton* pRemoveRowButton);       ///< Change a used resource's begin/end times.
    //
    void at_updateClocksTimerTimeout();                                             ///< Update the time displayed in every tab.
    void at_autoSaveTimerTimeout();                                                 ///< Auto-save the report.
    void at_findPersonShortcutActivated();                                          ///< Select all personnel matching the entered name.
    void at_exportFailed();                                                         ///< Show message box explaining that export failed.
    //
    void at_saveFile_action_triggered();                                            ///< Save the report to (the same) file.
    void at_saveFileAs_action_triggered();                                          ///< Save the report to a (different) file.
    void at_exportFile_action_triggered();                                          ///< Export the report as PDF file.
    void at_exportAndOpenFile_action_triggered();                                   ///< Export the report as PDF file and open the PDF.
    void at_loadCarries_action_triggered();                                         ///< Load old report carryovers from a file.
    void at_newReport_action_triggered();                                           ///< Create a new report in a different window.
    void at_openReport_action_triggered();                                          ///< Open a report from a file in a different window.
    void at_close_action_triggered();                                               ///< Close the report window.
    void at_editPersonnelTableSplit_action_triggered();                             ///< Change the maximum PDF personnel table length.
    void at_editBoatDrivesTableSplit_action_triggered();                            ///< Change the maximum PDF boat drives table length.
    //
    void at_reportTab_calendarWidget_currentPageChanged(int year, int month);       ///< Synchronize the calendar widgets of every tab.
    void at_boatTab_calendarWidget_currentPageChanged(int year, int month);         ///< Synchronize the calendar widgets of every tab.
    void at_rescueTab_calendarWidget_currentPageChanged(int year, int month);       ///< Synchronize the calendar widgets of every tab.
    void at_reportTab_calendarWidget_selectionChanged();                            ///< Synchronize the calendar widgets of every tab.
    void at_boatTab_calendarWidget_selectionChanged();                              ///< Synchronize the calendar widgets of every tab.
    void at_rescueTab_calendarWidget_selectionChanged();                            ///< Synchronize the calendar widgets of every tab.
    //
    void at_reportNumberDecr_radioButton_toggled(bool checked);                     ///< Reduce the report serial number by one.
    void at_reportNumberIncr_radioButton_toggled(bool checked);                     ///< Increase the report serial number by one.
    void at_station_comboBox_currentTextChanged(const QString& arg1);               ///< \brief Set the report station and update
                                                                                    ///  selectable radio call names.
    void at_stationRadioCallName_comboBox_currentTextChanged(const QString& arg1);  ///< Set the report station radio call name.
    void at_dutyPurpose_comboBox_currentTextChanged(const QString& arg1);           ///< Set the report duty purpose.
    void at_dutyPurposeComment_lineEdit_textEdited(const QString& arg1);            ///< Set the report duty purpose comment.
    void at_reportDate_dateEdit_dateChanged(const QDate& date);                     ///< Set the report date.
    void at_dutyTimesBegin_timeEdit_timeChanged(const QTime& time);                 ///< Set the report duty begin time.
    void at_dutyTimesEnd_timeEdit_timeChanged(const QTime& time);                   ///< Set the report duty end time.
    void at_endOnNextDay_checkBox_stateChanged(int arg1);                           ///< Set if the duty end time refers to the next day.
    void at_reportComments_plainTextEdit_textChanged();                             ///< Set the report comments.
    //
    void at_temperatureAir_spinBox_valueChanged(int arg1);                          ///< Set the report air temperature.
    void at_temperatureWater_spinBox_valueChanged(int arg1);                        ///< Set the report water temperature.
    void at_precipitation_comboBox_currentTextChanged(const QString& arg1);         ///< Set the report precipitation type.
    void at_cloudiness_comboBox_currentTextChanged(const QString& arg1);            ///< Set the report level of cloudiness.
    void at_windStrength_comboBox_currentTextChanged(const QString& arg1);          ///< Set the report wind strength.
    void at_windStrength_comboBox_customContextMenuRequested(const QPoint& pos);    ///< Show a tooltip elucidating the wind strengths.
    void at_windDirection_comboBox_currentTextChanged(const QString& arg1);         ///< Set the report wind direction.
    void at_weatherComments_plainTextEdit_textChanged();                            ///< Set the report weather comments.
    //
    void at_operationProtocolsCtr_spinBox_valueChanged(int arg1);           ///< Set the report number of enclosed operation protocols.
    void at_patientRecordsCtr_spinBox_valueChanged(int arg1);               ///< Set the report number of enclosed patient records.
    void at_radioCallLogsCtr_spinBox_valueChanged(int arg1);                ///< Set the report number of enclosed radio call logs.
    void at_otherEnclosures_lineEdit_textEdited(const QString& arg1);       ///< Set the report list of other enclosures.
    void at_otherEnclosures_lineEdit_textChanged(const QString& arg1);      ///< Update line edit tool tip and enclosures list label.
    //
    void at_personLastName_lineEdit_textChanged(const QString&);            ///< Check entered person name and update dependent widgets.
    void at_personFirstName_lineEdit_textChanged(const QString&);           ///< Check entered person name and update dependent widgets.
    void at_personFirstName_lineEdit_returnPressed();                       ///< Add the selected person to the report personnel list.
    void at_personIdent_comboBox_currentTextChanged(const QString& arg1);   ///< Update selectable personnel functions.
    void at_setNewPersonTimeBegin_pushButton_pressed();                     ///< Set new person begin time edit to duty begin time.
    void at_setNewPersonTimeBeginNow_pushButton_pressed();                  ///< Set new person begin time edit to now (nearest quarter).
    void at_personTimeBegin_timeEdit_timeChanged(const QTime& time);        ///< \brief Update highlighting of new person begin
                                                                            ///         and end time edits.
    void at_personTimeEnd_timeEdit_timeChanged(const QTime&);               ///< \brief Update highlighting of new person begin
                                                                            ///         and end time edits.
    void at_setNewPersonTimeEndNow_pushButton_pressed();                    ///< Set new person end time edit to now (nearest quarter).
    void at_setNewPersonTimeEnd_pushButton_pressed();                       ///< Set new person end time edit to duty end time.
    void at_addPerson_pushButton_pressed();                                 ///< Add the selected person to the report personnel list.
    void at_addExtPerson_pushButton_pressed();                              ///< Add an external person to the report personnel list.
    void at_updatePerson_pushButton_pressed();                              ///< Change selected persons' functions and times.
    void at_removePerson_pushButton_pressed();                              ///< Remove selected persons from personnel.
    void at_setPersonTimeBegin_pushButton_pressed();                ///< Set selected persons' begin times to begin time edit's time.
    void at_setPersonTimeEnd_pushButton_pressed();                  ///< Set selected persons' end times to end time edit's time.
    void at_setPersonTimeBeginNow_pushButton_pressed();             ///< Set selected persons' begin times to now (nearest quarter).
    void at_setPersonTimeEndNow_pushButton_pressed();               ///< Set selected persons' end times to now (nearest quarter).
    void at_personQualifications_pushButton_pressed();                      ///< View or change selected persons' qualifications.
    void at_personnel_tableWidget_cellDoubleClicked(int, int);              ///< See at_updatePerson_pushButton_pressed().
    //
    void at_personnelHoursHours_spinBox_valueChanged(int);                  ///< Update total personnel hours display.
    void at_personnelHoursMinutes_spinBox_valueChanged(int);                ///< Update total personnel hours display.
    void at_personnelHoursCarryHours_spinBox_valueChanged(int arg1);        ///< Set personnel hours carry, update total hours display.
    void at_personnelHoursCarryMinutes_spinBox_valueChanged(int arg1);      ///< Set personnel hours carry, update total hours display.
    //
    void at_boat_comboBox_currentTextChanged(const QString& arg1);      ///< Set the report boat and update selectable radio call names.
    void at_boatRadioCallName_comboBox_currentTextChanged(const QString& arg1); ///< Set the report boat radio call name.
    void at_boatSlippedBeginOfDuty_checkBox_stateChanged(int arg1);             ///< Set, if boat was lowered to water at begin of duty.
    void at_boatSlippedEndOfDuty_checkBox_stateChanged(int arg1);               ///< Set, if boat was taken out of water at end of duty.
    void at_boatReadyFrom_timeEdit_timeChanged(const QTime& time);              ///< Set report boat readiness time begin.
    void at_boatReadyUntil_timeEdit_timeChanged(const QTime& time);             ///< Set report boat readiness time end.
    //
    void at_boatComments_plainTextEdit_textChanged();                           ///< Set report boat comments.
    //
    void at_engineHoursBeginOfDuty_doubleSpinBox_valueChanged(double arg1);     ///< Set report engine hours at begin of duty.
    void at_engineHoursEndOfDuty_doubleSpinBox_valueChanged(double arg1);       ///< Set report engine hours at end of duty.
    //
    void at_fuelBeginOfDuty_spinBox_valueChanged(int arg1);     ///< Set report fuel added at begin of duty and update total display.
    void at_fuelAfterDrives_spinBox_valueChanged(int arg1);     ///< Update total added fuel display.
    void at_fuelEndOfDuty_spinBox_valueChanged(int arg1);       ///< Set report fuel added at end of duty and update total display.
    //
    void at_boatDrives_tableWidget_currentCellChanged(int currentRow, int, int previousRow, int);
                                                                    ///< \brief Display data of the newly selected boat drive or revert
                                                                    ///  the table selection in case there are unapplied changes.
    void at_addBoatDrive_pushButton_pressed();                      ///< Add new boat drive to report (after selected one).
    void at_removeBoatDrive_pushButton_pressed();                   ///< Remove the selected boat drive from the report.
    void at_moveBoatDriveUp_pushButton_pressed();                   ///< Change the order of selected drive and drive above.
    void at_moveBoatDriveDown_pushButton_pressed();                 ///< Change the order of selected drive and drive below.
    void at_setBoatDriveTimeBeginNow_pushButton_pressed();          ///< Set displayed boat drive begin time to now (nearest quarter).
    void at_setBoatDriveTimeEndNow_pushButton_pressed();            ///< Set displayed boat drive end time to now (nearest quarter).
    void at_splitBoatDrive_pushButton_pressed();                    ///< Split drive into two drives at current time (nearest quarter).
    void at_applyBoatDriveChanges_pushButton_pressed();             ///< Apply changed boat drive data to selected boat drive in report.
    void at_discardBoatDriveChanges_pushButton_pressed();           ///< Discard changed boat drive data and reload selected drive data.
    //
    void at_boatDrivePurpose_comboBox_currentTextChanged(const QString&);       ///< Set unapplied boat drive changes.
    void at_boatDriveBegin_timeEdit_timeChanged(const QTime& time);             ///< Set unapplied boat drive changes.
    void at_boatDriveEnd_timeEdit_timeChanged(const QTime& time);               ///< Set unapplied boat drive changes.
    void at_boatDriveFuel_spinBox_valueChanged(int);                            ///< Set unapplied boat drive changes.
    void at_boatDriveBoatman_comboBox_currentTextChanged(const QString& arg1);  ///< Check boatman, set unapplied boat drive changes.
    void at_boatDriveComments_plainTextEdit_textChanged();                      ///< Set unapplied boat drive changes.
    void at_boatCrewMember_comboBox_currentTextChanged(const QString& arg1);    ///< \brief Update selectable boat functions according to
                                                                                ///  selected new crew member person's qualifications.
    void at_addBoatCrewMember_pushButton_pressed();     ///< Add crew member to displayed drive's crew member table.
    void at_addExtBoatCrewMember_pushButton_pressed();  ///< Add external crew member to displayed drive's crew member table.
    void at_removeBoatCrewMember_pushButton_pressed();  ///< Remove selected crew member from displayed drive's crew member table.
    void at_noBoatCrew_checkBox_stateChanged(int arg1); ///< Remove crew members from displayed drive's crew member table, if checked.
    //
    void at_boatHoursHours_spinBox_valueChanged(int);               ///< Update total boat hours display.
    void at_boatHoursMinutes_spinBox_valueChanged(int);             ///< Update total boat hours display.
    void at_boatHoursCarryHours_spinBox_valueChanged(int arg1);     ///< Set report boat hours carry and update total boat hours display.
    void at_boatHoursCarryMinutes_spinBox_valueChanged(int arg1);   ///< Set report boat hours carry and update total boat hours display.
    //
    void at_assignmentNumber_lineEdit_textEdited(const QString& arg1);  ///< Set report assignment number.
    //
    void at_clearTimestamps_pushButton_pressed();                       ///< Clear the list of timestamps.
    void at_addTimestamp_pushButton_pressed();                          ///< Add a new timestamp to the list of timestamps.

signals:
    void closed(const ReportWindow* pWindow);       ///< Signal emitted when window closes (for re-showing startup window).
    void exportFailed();                            ///< Signal emitted by export thread on failure to show message box in main thread.
    void openAnotherReportRequested(const QString& pFileName, bool pChooseFile = false);    ///< \brief Signal emitted when another
                                                                                            ///  report window shall be opened
                                                                                            ///  by the startup window.

private:
    Ui::ReportWindow* ui;   //UI
    //
    /*!
     * \brief A row of the rescue operations table.
     *
     * Groups pointers to widgets of the same row of a rescue operations counting table for later access.
     */
    struct RescueOperationsRow
    {
        QSpinBox *const spinBox;            ///< Pointer to a spin box for counting a specific rescue type.
        QLabel *const fillDocNoticeLabel;   ///< Pointer to a label for indicating that a document has to be filled.
    };
    std::map<Report::RescueOperation, const RescueOperationsRow> rescuesTableRows;  //Pointers to dyn. added widgets for each rescue type
    //
    /*!
     * \brief A row of the used resources table.
     *
     * Groups pointers to widgets of the same row of a used resources table for later access.
     * Also contains a list of the widget's signal-slot connections (for later disconnection before possible widget removal).
     */
    struct ResourcesRow
    {
        QPushButton *const removeRowPushButton; ///< Pointer to a push button for removing the row.
        QLineEdit *const resourceNameLineEdit;  ///< Pointer to a line edit for entering the resource's (radio call) name.
        QTimeEdit *const beginTimeEdit;         ///< Pointer to a time edit for entering the resource's begin of use time.
        QTimeEdit *const endTimeEdit;           ///< Pointer to a time edit for entering the resource's end of use time.
        QLabel *const timesSepLabel;            ///< Pointer to a label separating the time edits.
        const std::vector<QMetaObject::Connection> connections; ///< Signal-slot connections for this struct's widgets.
    };
    std::list<ResourcesRow> usedResourcesTableRows; //Pointers to dyn. added widgets for each used resource
    QGridLayout* usedResourcesGroupBoxLayout;       //Layout of group box containing used resources table
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
    std::atomic_bool exporting;                 //Export thread currently running?
    std::atomic_bool latestExportFailed;        //Last export failed?
    //
    int exportPersonnelTableMaxLength;          //Maximum length of exported PDFs personnel table; will be split if length is exceeded
    int exportBoatDrivesTableMaxLength;         //Maximum length of exported PDFs boat drives table; will be split if length is exceeded
    //
    QString loadedStation;                  //Initial station identifier from loaded report, if not found in database (else empty)
    QString loadedStationRadioCallName;     //Initial radio call name from loaded report, if station not found in database (else empty)
    QString loadedBoat;                     //Initial boat name from loaded report, if not found in database (else empty)
    QString loadedBoatRadioCallName;        //Initial radio call name from loaded report, if boat not found in database (else empty)
    //
    QString selectedBoatmanIdent;           //Person identifier of currently selected boatman combo box item
    //
    QStringList databaseLastNames;          //All distinct last names present in personnel database
    QStringList databaseFirstNames;         //All distinct first names present in personnel database
};

#endif // WDMGR_REPORTWINDOW_H
