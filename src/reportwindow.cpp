/*
/////////////////////////////////////////////////////////////////////////////////////////
//
//  This file is part of Wachdienst-Manager, a program to manage DLRG watch duty reports.
//  Copyright (C) 2021–2022 M. Frohne
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

#include "reportwindow.h"
#include "ui_reportwindow.h"

/*!
 * \brief Constructor.
 *
 * Creates the window.
 *
 * Adds combo box items for duty purpose and weather conditions according to available values in
 * Report::DutyPurpose, Aux::Precipitation, Aux::Cloudiness and Aux::WindStrength.
 *
 * Adds example combo box items for boat drive purpose from Aux::boatDrivePurposePresets.
 *
 * Adds available stations and boats from configuration database to respective combo boxes.
 * Also adds station and/or boat set in \p pReport, if this is not contained in database,
 * such that these can always get selected.
 *
 * Adds a spin box for each value of Report::RescueOperation to count the carried out rescue operations.
 * Connects each 'valueChanged' signal to the parameterized slot on_rescueOperationSpinBoxValueChanged().
 *
 * Adds a push button for each "important document" listed in settings.
 * Connects each 'pressed' signal to slot on_openDocumentPushButtonPressed().
 *
 * Sets validators from Aux for peronnel name and assignment number line edits.
 *
 * Adds completers for personnel name line edits containing the avaliable names from personnel database.
 *
 * Configures personnel table, boat drive table and crew member table.
 *
 * Sets a timer to update clock displays of all tabs every second.
 *
 * Finally fills widget contents with data of report \p pReport (see loadReportData()).
 *
 * \param pReport The report to display/edit.
 * \param pParent The parent widget.
 */
ReportWindow::ReportWindow(Report&& pReport, QWidget *const pParent) :
    QMainWindow(pParent),
    ui(new Ui::ReportWindow),
    vehiclesGroupBoxLayout(nullptr),
    statusBarLabel(new QLabel(this)),
    report(),
    boatLogPtr(report.boatLog()),
    unsavedChanges(false),
    unappliedBoatDriveChanges(false),
    exporting(false),
    exportPersonnelTableMaxLength(15),
    loadedStation(""),
    loadedStationRadioCallName(""),
    loadedBoat(""),
    loadedBoatRadioCallName(""),
    selectedBoatmanIdent("")
{
    ui->setupUi(this);

    setWindowState(Qt::WindowState::WindowMaximized);

    //Add spin boxes to count the different types of rescue operations

    QGridLayout* tRescuesGroupBoxLayout = new QGridLayout(ui->rescues_groupBox);

    /*
     * Lambda expression for adding a spin box (plus label and "fill document" notice label) for
     * rescue operation type \p pRescue to \p pGroupBox with \p pGroupBoxLayout. Additional pointers to
     * spin boxes and notice labels are added to \p pRescuesTableRows.
     * To be executed for each available 'RescueOperation'.
     */
    auto tRescueFunc = [](Report::RescueOperation pRescue, QGroupBox *const pGroupBox, QGridLayout *const pGroupBoxLayout,
                          std::map<Report::RescueOperation, const RescueOperationsRow>& pRescuesTableRows) -> void
    {
        //Create widgets
        QLabel* tLabel = new QLabel(Report::rescueOperationToLabel(pRescue), pGroupBox);
        QLabel* tLabel2 = new QLabel("", pGroupBox);
        tLabel2->setStyleSheet("color: #FF0000;");
        QSpinBox* tSpinBox = new QSpinBox(pGroupBox);
        tSpinBox->setMinimum(0);
        tSpinBox->setMaximum(999);

        //Add widgets to new layout row
        int row = pGroupBoxLayout->rowCount();
        pGroupBoxLayout->addWidget(tLabel, row, 0);
        pGroupBoxLayout->addWidget(tSpinBox, row, 1);
        pGroupBoxLayout->addWidget(tLabel2, row, 2);

        //Add pointers to map
        pRescuesTableRows.insert({pRescue, {tSpinBox, tLabel2}});
    };

    //Add spin boxes to layout
    Report::iterateRescueOperations(tRescueFunc, ui->rescues_groupBox, tRescuesGroupBoxLayout, rescuesTableRows);

    //Set the layout
    ui->rescues_groupBox->setLayout(tRescuesGroupBoxLayout);

    //Connect 'valueChanged' signal of each spin box to a single, parameterized slot
    for (const auto& it : rescuesTableRows)
    {
        Report::RescueOperation tRescue = it.first;
        const QSpinBox *const tSpinBox = it.second.spinBox;

        connect(tSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
                [this, tRescue](int pValue) -> void {
                    this->on_rescueOperationSpinBoxValueChanged(pValue, tRescue);
                });
    }

    //Add push buttons for opening important or frequently used documents

    QGridLayout* tDocsGroupBoxLayout = new QGridLayout(ui->documents_groupBox);

    std::vector<std::pair<QString, QString>> tDocs = Aux::parseDocumentListString(
                                                         SettingsCache::getStrSetting("app_documentLinks_documentList"));

    for (const std::pair<QString, QString>& tPair : tDocs)
    {
        const QString& tDocName = tPair.first;
        const QString& tDocFile = tPair.second;

        //Create button with document name as label
        QPushButton* tButton = new QPushButton(tDocName, ui->documents_groupBox);

        //Add button to layout
        int row = tDocsGroupBoxLayout->rowCount();
        tDocsGroupBoxLayout->addWidget(tButton, row, 0);

        //Connect 'pressed' signal of each document button to a single slot to open the document
        connect(tButton, &QPushButton::pressed, this,
                [this, tDocFile](void) -> void {
                    this->on_openDocumentPushButtonPressed(tDocFile);
                });
    }

    //Set the layout
    ui->documents_groupBox->setLayout(tDocsGroupBoxLayout);

    //Get layout of vehicles table group box
    vehiclesGroupBoxLayout = dynamic_cast<QGridLayout*>(ui->vehicles_groupBox->layout());

    //Add combo box items from available enum class values of 'DutyPurpose'/'Precipitation'/'Cloudiness'/'WindStrength'

    /*
     * Define auxiliary lambda expressions first, which all add a combo box item for \p pPurpose/pPrecip/pClouds/pWind to \p pComboBox.
     * They have to be executed for each available value of 'DutyPurpose'/'Precipitation'/'Cloudiness'/'WindStrength'.
     */
    auto tPurposeFunc = [](Report::DutyPurpose pPurpose, QComboBox *const pComboBox) -> void {
        pComboBox->insertItem(pComboBox->count(), Report::dutyPurposeToLabel(pPurpose));
    };
    auto tPrecipFunc = [](Aux::Precipitation pPrecip, QComboBox *const pComboBox) -> void {
        pComboBox->insertItem(pComboBox->count(), Aux::precipitationToLabel(pPrecip));
    };
    auto tCloudsFunc = [](Aux::Cloudiness pClouds, QComboBox *const pComboBox) -> void {
        pComboBox->insertItem(pComboBox->count(), Aux::cloudinessToLabel(pClouds));
    };
    auto tWindFunc = [](Aux::WindStrength pWind, QComboBox *const pComboBox) -> void {
        pComboBox->insertItem(pComboBox->count(), Aux::windStrengthToLabel(pWind));
    };

    //Add the items
    Report::iterateDutyPurposes(tPurposeFunc, ui->dutyPurpose_comboBox);
    Aux::iteratePrecipitationTypes(tPrecipFunc, ui->precipitation_comboBox);
    Aux::iterateCloudinessLevels(tCloudsFunc, ui->cloudiness_comboBox);
    Aux::iterateWindStrengths(tWindFunc, ui->windStrength_comboBox);

    //Add example boat drive purposes
    ui->boatDrivePurpose_comboBox->addItems(Aux::boatDrivePurposePresets);

    //Set line edit validators

    ui->personLastName_lineEdit->setValidator(new QRegularExpressionValidator(Aux::personNamesValidator.regularExpression(),
                                                                              ui->personLastName_lineEdit));
    ui->personFirstName_lineEdit->setValidator(new QRegularExpressionValidator(Aux::personNamesValidator.regularExpression(),
                                                                               ui->personFirstName_lineEdit));

    ui->assignmentNumber_lineEdit->setValidator(new QRegularExpressionValidator(Aux::assignmentNumbersValidator.regularExpression(),
                                                                                ui->assignmentNumber_lineEdit));

    //Disable manual checking of enclosures check boxes

    ui->operationProtocols_checkBox->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui->operationProtocols_checkBox->setFocusPolicy(Qt::NoFocus);

    ui->patientRecords_checkBox->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui->patientRecords_checkBox->setFocusPolicy(Qt::NoFocus);

    ui->radioCallLogs_checkBox->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui->radioCallLogs_checkBox->setFocusPolicy(Qt::NoFocus);

    ui->otherEnclosures_checkBox->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui->otherEnclosures_checkBox->setFocusPolicy(Qt::NoFocus);

    //Disable tab key input for plain text edits
    ui->reportComments_plainTextEdit->setTabChangesFocus(true);
    ui->weatherComments_plainTextEdit->setTabChangesFocus(true);
    ui->boatComments_plainTextEdit->setTabChangesFocus(true);
    ui->boatDriveComments_plainTextEdit->setTabChangesFocus(true);

    //Format table headers and configure selection modes

    ui->personnel_tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->personnel_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->personnel_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->personnel_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->personnel_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->personnel_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->personnel_tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    ui->personnel_tableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    ui->personnel_tableWidget->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);

    ui->boatDrives_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->boatDrives_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->boatDrives_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->boatCrew_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->boatCrew_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->boatCrew_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->boatCrew_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->boatCrew_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->boatCrew_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->boatCrew_tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    //Add status bar label to status bar
    ui->statusbar->addPermanentWidget(statusBarLabel);

    //Show error message from main thread always when export thread signals an export failure
    connect(this, SIGNAL(exportFailed()), this, SLOT(on_exportFailed()));

    //Set timer to update clock displays of all tabs every second
    on_updateClocksTimerTimeout();
    QTimer* clockTimer = new QTimer(this);
    connect(clockTimer, SIGNAL(timeout()), this, SLOT(on_updateClocksTimerTimeout()));
    clockTimer->start(1000);

    //Set timer to auto-save the report every 10 minutes
    QTimer* autoSaveTimer = new QTimer(this);
    connect(autoSaveTimer, SIGNAL(timeout()), this, SLOT(on_autoSaveTimerTimeout()));
    autoSaveTimer->start(10 * 60 * 1000);

    //Add shortcut to record the current time and display it in a non-modal message box
    QShortcut* timestampShortcut = new QShortcut(QKeySequence("Ctrl+T"), this);
    connect(timestampShortcut, SIGNAL(activated()), this, SLOT(on_timestampShortcutActivated()));

    //Add stations and boats to combo boxes

    //Use station identifier instead of 'rowid' as key
    for (auto it : DatabaseCache::stations())
    {
        QString tStationIdent;
        Aux::stationIdentFromNameLocation(it.second.name, it.second.location, tStationIdent);
        stations.insert({std::move(tStationIdent), std::move(it.second)});
    }

    //Use boat name instead of 'rowid' as key
    for (const auto& it : DatabaseCache::boats())
        boats.insert({it.second.name, std::move(it.second)});

    for (const auto& it : stations)
        ui->station_comboBox->insertItem(ui->station_comboBox->count(), Aux::stationLabelFromIdent(it.first));

    for (const auto& it : boats)
        ui->boat_comboBox->insertItem(ui->boat_comboBox->count(), it.first);

    //Add completers for personnel name line edits

    std::vector<Person> dbPersonnel;
    DatabaseCache::getPersonnel(dbPersonnel);

    QStringList lastNameCompletions, firstNameCompletions;

    for (const Person& tPerson : dbPersonnel)
    {
        QString tLastName = tPerson.getLastName();
        QString tFirstName = tPerson.getFirstName();

        if (!lastNameCompletions.contains(tLastName))
            lastNameCompletions.push_back(std::move(tLastName));

        if (!firstNameCompletions.contains(tFirstName))
            firstNameCompletions.push_back(std::move(tFirstName));
    }

    QCompleter* lastNameCompleter = new QCompleter(lastNameCompletions, ui->personLastName_lineEdit);
    lastNameCompleter->setCompletionMode(QCompleter::PopupCompletion);
    lastNameCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    ui->personLastName_lineEdit->setCompleter(lastNameCompleter);

    QCompleter* firstNameCompleter = new QCompleter(firstNameCompletions, ui->personFirstName_lineEdit);
    firstNameCompleter->setCompletionMode(QCompleter::PopupCompletion);
    firstNameCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    ui->personFirstName_lineEdit->setCompleter(firstNameCompleter);

    //Enable drag and drop in order to open further reports being dropped on the window
    setAcceptDrops(true);

    //Set report only now because adding combo box items above overwrites values
    report = std::move(pReport);
    boatLogPtr = report.boatLog();

    //Fill the widgets with report's data
    loadReportData();

    //If no vehicle added from loaded report, add one empty row to vehicles table so that user can start filling the table
    if (vehiclesTableRows.empty())
        addVehiclesTableRow("", report.getBeginTime(), report.getEndTime());
}

/*!
 * \brief Destructor.
 */
ReportWindow::~ReportWindow()
{
    delete ui;
}

//Private

/*!
 * \brief Reimplementation of QMainWindow::closeEvent().
 *
 * Reimplements QMainWindow::closeEvent().
 *
 * Ignores \p pEvent and returns if an export is still running.
 *
 * Warns about unsaved changes. User can choose, if closing is ok. If not, \p pEvent is ignored and function returns.
 *
 * Emits closed() signal with this pointer as argument before accepting the close event.
 *
 * \param pEvent The close event.
 */
void ReportWindow::closeEvent(QCloseEvent* pEvent)
{
    //Check for still running export thread
    if (exporting.load() == true)
    {
        QMessageBox(QMessageBox::Warning, "Exportiervorgang nicht abgeschlossen", "Es läuft noch ein Exportiervorgang!",
                    QMessageBox::Ok, this).exec();

        pEvent->ignore();
        return;
    }

    if (unappliedBoatDriveChanges)
    {
        QMessageBox msgBox(QMessageBox::Question, "Ungespeicherte Änderungen",
                           "Nicht übernommene/gespeicherte Änderungen in selektierter Bootsfahrt.\nTrotzdem schließen?",
                           QMessageBox::Abort | QMessageBox::Yes, this);

        if (msgBox.exec() != QMessageBox::Yes)
        {
            pEvent->ignore();
            return;
        }
    }

    if (unsavedChanges)
    {
        QMessageBox msgBox(QMessageBox::Question, "Ungespeicherte Änderungen",
                           "Ungespeicherte Änderungen im Wachbericht.\nTrotzdem schließen?",
                           QMessageBox::Abort | QMessageBox::Yes, this);

        if (msgBox.exec() != QMessageBox::Yes)
        {
            pEvent->ignore();
            return;
        }
    }

    emit closed(this);

    pEvent->accept();
}

//

/*!
 * \brief Reimplementation of QMainWindow::dragEnterEvent().
 *
 * Reimplements QMainWindow::dragEnterEvent().
 *
 * \p pEvent is accepted, if the window was entered by a drag and drop action that represents a single file,
 * and is ignored otherwise. Before accepting, the event's proposed action is changed to Qt::DropAction::LinkAction.
 *
 * \param pEvent The event containing information about the drag and drop action that entered the window.
 */
void ReportWindow::dragEnterEvent(QDragEnterEvent* pEvent)
{
    if (pEvent->mimeData()->hasUrls() && pEvent->mimeData()->urls().length() == 1)
    {
        pEvent->setDropAction(Qt::DropAction::LinkAction);
        pEvent->accept();
    }
    else
        pEvent->ignore();
}

/*!
 * \brief Reimplementation of QMainWindow::dropEvent().
 *
 * Reimplements QMainWindow::dropEvent().
 *
 * \p pEvent is accepted (using event action Qt::DropAction::LinkAction), if a single
 * file was dropped on the window, and is ignored otherwise. See also dragEnterEvent().
 *
 * The signal openAnotherReportRequested() is emitted with the dropped file name as argument.
 *
 * See also StartupWindow::on_openAnotherReportRequested(), which may be connected to the signal
 * in order to open a report from the dropped file name and show it in a different report window.
 *
 * \param pEvent The event containing information about the drag and drop action that was dropped on the window.
 */
void ReportWindow::dropEvent(QDropEvent* pEvent)
{
    if (pEvent->mimeData()->hasUrls() && pEvent->mimeData()->urls().length() == 1)
    {
        pEvent->setDropAction(Qt::DropAction::LinkAction);
        pEvent->accept();

        emit openAnotherReportRequested(pEvent->mimeData()->urls().at(0).toLocalFile());
    }
    else
        pEvent->ignore();
}

//

/*!
 * \brief Fill all widgets with the report data.
 *
 * Also displays the report's file name in the status bar, if not empty.
 *
 * Note: Setting the widget contents triggers some of the (auto-connected) slots,
 * which will write their values back to the report (this is generally not a problem)
 * and also set the 'unsaved changes' or 'unapplied boat drive changes' switches
 * (see setUnsavedChanges() and setUnappliedBoatDriveChanges()). Since the data was just
 * applied from the report the two switches will be reset at the end of this function.
 */
void ReportWindow::loadReportData()
{
    if (report.getFileName() != "")
        statusBarLabel->setText("Datei: " + report.getFileName());

    setSerialNumber(report.getNumber());

    if (report.getStation() != "")
    {
        //Remember radio call name since gets overwritten by station combo box slot
        QString tRadioCallName = report.getRadioCallName();

        QString tStationLabel = Aux::stationLabelFromIdent(report.getStation());

        //Make possible to select (old) station not in or removed from database
        if (stations.find(report.getStation()) == stations.end())
        {
            loadedStation = report.getStation();
            loadedStationRadioCallName = tRadioCallName;

            ui->station_comboBox->insertItem(ui->station_comboBox->count(), tStationLabel);
        }

        ui->station_comboBox->setCurrentIndex(ui->station_comboBox->findText(tStationLabel));

        //Call slot in case index has not changed
        on_station_comboBox_currentTextChanged(tStationLabel);

        ui->stationRadioCallName_comboBox->setCurrentIndex(ui->stationRadioCallName_comboBox->findText(tRadioCallName));

        //Call slot in case index has not changed
        on_stationRadioCallName_comboBox_currentTextChanged(ui->stationRadioCallName_comboBox->currentText());
    }
    else
    {
        ui->station_comboBox->setCurrentIndex(-1);

        on_station_comboBox_currentTextChanged("");
        on_stationRadioCallName_comboBox_currentTextChanged("");
    }

    ui->dutyPurpose_comboBox->setCurrentIndex(ui->dutyPurpose_comboBox->findText(Report::dutyPurposeToLabel(report.getDutyPurpose())));
    ui->dutyPurposeComment_lineEdit->setText(report.getDutyPurposeComment());

    ui->reportDate_dateEdit->setDate(report.getDate());
    ui->dutyTimesBegin_timeEdit->setTime(report.getBeginTime());
    ui->dutyTimesEnd_timeEdit->setTime(report.getEndTime());

    ui->reportComments_plainTextEdit->setPlainText(report.getComments());

    ui->temperatureAir_spinBox->setValue(report.getAirTemperature());
    ui->temperatureWater_spinBox->setValue(report.getWaterTemperature());

    ui->precipitation_comboBox->setCurrentIndex(ui->precipitation_comboBox->findText(Aux::precipitationToLabel(
                                                                                         report.getPrecipitation())));
    ui->cloudiness_comboBox->setCurrentIndex(ui->cloudiness_comboBox->findText(Aux::cloudinessToLabel(
                                                                                   report.getCloudiness())));
    ui->windStrength_comboBox->setCurrentIndex(ui->windStrength_comboBox->findText(Aux::windStrengthToLabel(
                                                                                       report.getWindStrength())));

    ui->weatherComments_plainTextEdit->setPlainText(report.getWeatherComments());

    ui->operationProtocolsCtr_spinBox->setValue(report.getOperationProtocolsCtr());
    ui->patientRecordsCtr_spinBox->setValue(report.getPatientRecordsCtr());
    ui->radioCallLogsCtr_spinBox->setValue(report.getRadioCallLogsCtr());

    ui->otherEnclosures_lineEdit->setText(report.getOtherEnclosures());

    updatePersonnelTable();
    updateBoatDriveAvailablePersons();

    setPersonnelHoursCarry(report.getPersonnelMinutesCarry());

    if (boatLogPtr->getBoat() != "")
    {
        //Remember radio call name since gets overwritten by boat combo box slot
        QString tRadioCallName = boatLogPtr->getRadioCallName();

        QString tBoatName = boatLogPtr->getBoat();

        //Make possible to select (old) boat not in or removed from database
        if (boats.find(tBoatName) == boats.end())
        {
            loadedBoat = tBoatName;
            loadedBoatRadioCallName = tRadioCallName;

            ui->boat_comboBox->insertItem(ui->boat_comboBox->count(), tBoatName);
        }

        ui->boat_comboBox->setCurrentIndex(ui->boat_comboBox->findText(tBoatName));

        //Call slot in case index has not changed
        on_boat_comboBox_currentTextChanged(tBoatName);

        ui->boatRadioCallName_comboBox->setCurrentIndex(ui->boatRadioCallName_comboBox->findText(tRadioCallName));

        //Call slot in case index has not changed
        on_boatRadioCallName_comboBox_currentTextChanged(ui->boatRadioCallName_comboBox->currentText());
    }
    else
    {
        ui->boat_comboBox->setCurrentIndex(-1);

        on_boat_comboBox_currentTextChanged("");
        on_boatRadioCallName_comboBox_currentTextChanged("");
    }

    ui->boatSlippedBeginOfDuty_checkBox->setChecked(boatLogPtr->getSlippedInitial());
    ui->boatSlippedEndOfDuty_checkBox->setChecked(boatLogPtr->getSlippedFinal());

    //Remember time since gets overwritten by time edit spin box slot
    QTime tReadyUntil = boatLogPtr->getReadyUntil();

    ui->boatReadyFrom_timeEdit->setTime(boatLogPtr->getReadyFrom());
    ui->boatReadyUntil_timeEdit->setTime(tReadyUntil);

    ui->boatComments_plainTextEdit->setPlainText(boatLogPtr->getComments());

    ui->engineHoursBeginOfDuty_doubleSpinBox->setValue(boatLogPtr->getEngineHoursInitial());
    ui->engineHoursEndOfDuty_doubleSpinBox->setValue(boatLogPtr->getEngineHoursFinal());

    ui->fuelBeginOfDuty_spinBox->setValue(boatLogPtr->getFuelInitial());
    ui->fuelEndOfDuty_spinBox->setValue(boatLogPtr->getFuelFinal());

    updateBoatDrivesTable();

    //Select no boat drive
    ui->boatDrives_tableWidget->setCurrentCell(-1, 0);
    on_boatDrives_tableWidget_currentCellChanged(-1, 0, -1, 0);

    setBoatHoursCarry(boatLogPtr->getBoatMinutesCarry());

    for (const auto& it : rescuesTableRows)
    {
        Report::RescueOperation tRescue = it.first;
        QSpinBox *const tSpinBox = it.second.spinBox;

        tSpinBox->setValue(report.getRescueOperationCtr(tRescue));
    }

    for (const auto& it : report.getVehicles())
        addVehiclesTableRow(it.first, it.second.first, it.second.second);

    ui->assignmentNumber_lineEdit->setText(report.getAssignmentNumber());

    //Fix widget highlighting (slots not called when values not changed, e.g. if stays zero)

    on_dutyTimesBegin_timeEdit_timeChanged(ui->dutyTimesBegin_timeEdit->time());
    on_dutyTimesEnd_timeEdit_timeChanged(ui->dutyTimesEnd_timeEdit->time());
    on_temperatureAir_spinBox_valueChanged(ui->temperatureAir_spinBox->value());
    on_temperatureWater_spinBox_valueChanged(ui->temperatureWater_spinBox->value());
    on_personnelHoursCarryHours_spinBox_valueChanged(ui->personnelHoursCarryHours_spinBox->value());
    on_personnelHoursCarryMinutes_spinBox_valueChanged(ui->personnelHoursCarryMinutes_spinBox->value());

    on_boatReadyFrom_timeEdit_timeChanged(ui->boatReadyFrom_timeEdit->time());
    on_boatReadyUntil_timeEdit_timeChanged(ui->boatReadyUntil_timeEdit->time());
    on_engineHoursBeginOfDuty_doubleSpinBox_valueChanged(ui->engineHoursBeginOfDuty_doubleSpinBox->value());
    on_engineHoursEndOfDuty_doubleSpinBox_valueChanged(ui->engineHoursEndOfDuty_doubleSpinBox->value());
    on_fuelAfterDrives_spinBox_valueChanged(ui->fuelAfterDrives_spinBox->value());
    on_fuelEndOfDuty_spinBox_valueChanged(ui->fuelEndOfDuty_spinBox->value());
    on_boatDriveBegin_timeEdit_timeChanged(ui->boatDriveBegin_timeEdit->time());
    on_boatDriveEnd_timeEdit_timeChanged(ui->boatDriveEnd_timeEdit->time());
    on_boatHoursCarryHours_spinBox_valueChanged(ui->boatHoursCarryHours_spinBox->value());
    on_boatHoursCarryMinutes_spinBox_valueChanged(ui->boatHoursCarryMinutes_spinBox->value());

    //Reset unsaved changes switches
    setUnappliedBoatDriveChanges(false);
    setUnsavedChanges(false);
}

//

/*!
 * \brief Save the report.
 *
 * Saves the report to file \p pFileName. See also Report::save().
 *
 * If there are not yet applied changes to the selected boat drive, the user is warned before the report is saved
 * and can choose either to temporarily ignore these changes and save anyway or to skip/abort saving.
 *
 * Similarly the user is also warned about and asked how to handle (ignore or abort) possible invalid values (see checkInvalidValues()).
 *
 * If writing the file was successful, the displayed file name is updated and the 'unsaved changes' switch is reset.
 * Also, if an automatic export on save is configured in the settings, autoExport() will be called at the end of the function.
 *
 * \param pFileName Path to write the report file to.
 */
void ReportWindow::saveReport(const QString& pFileName)
{
    //Not include newest boat drive changes?
    if (unappliedBoatDriveChanges)
    {
        if (SettingsCache::getBoolSetting("app_reportWindow_autoApplyBoatDriveChanges"))
            on_applyBoatDriveChanges_pushButton_pressed();
        else
        {
            QMessageBox msgBox(QMessageBox::Question, "Nicht übernommene Änderungen",
                               "Nicht übernommene Änderungen in ausgewählter Bootsfahrt.\nTrotzdem speichern?",
                               QMessageBox::Abort | QMessageBox::Yes, this);
            if (msgBox.exec() != QMessageBox::Yes)
                return;
        }
    }

    //Ask the user, if invalid values shall be ignored
    if (!checkInvalidValues())
        return;

    //Automatically export as PDF after saving?
    bool tAutoExport = SettingsCache::getBoolSetting("app_export_autoOnSave");

    ui->statusbar->showMessage("Speichere als \"" + pFileName + "\"...");

    bool success = report.save(pFileName);

    ui->statusbar->clearMessage();

    //Save
    if (!success)
    {
        QMessageBox(QMessageBox::Warning, "Fehler", "Fehler beim Speichern!", QMessageBox::Ok, this).exec();

        if (tAutoExport)
        {
            QMessageBox(QMessageBox::Warning, "Warnung", "Wachbericht nicht exportiert aufgrund von Fehler beim Speichern!",
                        QMessageBox::Ok, this).exec();
        }
    }
    else
    {
        //Show file name in status bar on success
        statusBarLabel->setText("Datei: " + pFileName);

        //No unsaved changes anymore...
        setUnsavedChanges(false);

        if (tAutoExport)
            autoExport();
    }
}

/*!
 * \brief Save the report to a standard location (as backup).
 *
 * Silently saves the report to file "report-autosave.wbr" in a subdirectory ("Wachdienst-Manager-autosave")
 * of QStandardPaths::AppLocalDataLocation (typically itself a subdirectory of "%AppData%\Local" on Windows).
 *
 * See also Report::save().
 *
 * The internal file name of Report will not be changed. Also, contrary to saveReport(), there is no additional logic.
 *
 * Note: Any errors that occur while saving the file are simply ignored.
 */
void ReportWindow::autoSave()
{
    //Try to silently auto-save to an OS specific application data directory; ignore all errors

    QStringList standardPaths = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);

    if (standardPaths.size() == 0)
        return;

    QDir localDir(standardPaths[0]);

    if (!localDir.cd("Wachdienst-Manager-autosave"))
    {
        if (!localDir.mkpath("Wachdienst-Manager-autosave"))
            return;

        if (!localDir.cd("Wachdienst-Manager-autosave"))
            return;
    }

    report.save(localDir.filePath("report-autosave.wbr"), true);
}

/*!
 * \brief Export the report.
 *
 * Exports the report to file \p pFileName. See also PDFExporter::exportPDF().
 *
 * The function returns immediately, if an export thread is already/still running.
 *
 * If there are not yet applied changes to the selected boat drive, the user is warned before the report is exported
 * and can choose either to temporarily ignore these changes and export anyway or to skip/abort exporting.
 *
 * Similarly the user is also warned about and asked how to handle (ignore or abort) possible invalid and implausible values
 * (see checkInvalidValues() and checkImplausibleValues()).
 *
 * If \p pAskOverwrite is true and \p pFileName already exists, the user is asked, if the file should be overwritten.
 * The function returns before starting the export otherwise.
 *
 * The export itself (see PDFExporter::exportPDF()) can take a few seconds and is therefore run in a separate, detached thread.
 * If the export fails, the exportFailed() signal is emitted.
 *
 * \param pFileName Path to write the report file to.
 * \param pAskOverwrite Ask before overwriting existing file?
 */
void ReportWindow::exportReport(const QString& pFileName, bool pAskOverwrite)
{
    //Check for still running export thread
    if (exporting.load() == true)
    {
        QMessageBox(QMessageBox::Warning, "Exportiervorgang nicht abgeschlossen",
                    "Exportieren nicht möglich, da noch ein Exportiervorgang läuft!", QMessageBox::Ok, this).exec();
        return;
    }

    if (unappliedBoatDriveChanges)
    {
        if (SettingsCache::getBoolSetting("app_reportWindow_autoApplyBoatDriveChanges"))
            on_applyBoatDriveChanges_pushButton_pressed();
        else
        {
            QMessageBox msgBox(QMessageBox::Question, "Nicht übernommene Änderungen",
                               "Nicht übernommene Änderungen in ausgewählter Bootsfahrt.\nTrotzdem exportieren?",
                               QMessageBox::Abort | QMessageBox::Yes, this);
            if (msgBox.exec() != QMessageBox::Yes)
                return;
        }
    }

    //Ask the user, if invalid and implausible values shall be ignored
    if (!checkInvalidValues() || !checkImplausibleValues())
        return;

    if (pAskOverwrite && QFileInfo::exists(pFileName))
    {
        QMessageBox msgBox(QMessageBox::Question, "Überschreiben?", "Datei überschreiben?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return;
    }

    ui->statusbar->showMessage("Exportiere nach \"" + pFileName + "\"...");

    exporting.store(true);

    //Call export function (and clearing of status bar label) in detached thread to keep UI responsive
    std::thread exportThread(
                [this, pFileName]() -> void
                {
                    if (!PDFExporter::exportPDF(report, pFileName, exportPersonnelTableMaxLength))
                        emit exportFailed();

                    ui->statusbar->clearMessage();
                    exporting.store(false);
                });
    exportThread.detach();
}

/*!
 * \brief Export to automatic or manual file name depending on setting.
 *
 * Exports the report via on_exportFile_action_triggered(), if report file name empty or automatic export
 * shall always ask for file name (a setting). Otherwise the report is exported via exportReport() to an
 * automatically chosen file name (report file name with extension replaced by ".pdf"; asks before replacing existing file).
 */
void ReportWindow::autoExport()
{
    if (SettingsCache::getBoolSetting("app_export_autoOnSave_askForFileName") || report.getFileName() == "")
        on_exportFile_action_triggered();
    else
    {
        //Use report save file name with extension replaced by ".pdf"
        QString tFileName = report.getFileName();
        tFileName = QDir(QFileInfo(tFileName).absolutePath()).filePath(QFileInfo(tFileName).completeBaseName() + ".pdf");

        exportReport(tFileName, true);  //Ask before replacing the file because of automaticly generated file name
    }
}

//

/*!
 * \brief Set whether there are unsaved changes and update title.
 *
 * \param pValue Unsaved changes?
 */
void ReportWindow::setUnsavedChanges(bool pValue)
{
    unsavedChanges = pValue;

    updateWindowTitle();
}

/*!
 * \brief Set whether there are not applied boat drive changes and update title.
 *
 * \param pValue Unapplied boat drive changes?
 */
void ReportWindow::setUnappliedBoatDriveChanges(bool pValue)
{
    unappliedBoatDriveChanges = pValue;

    ui->applyBoatDriveChanges_pushButton->setEnabled(pValue);
    ui->discardBoatDriveChanges_pushButton->setEnabled(pValue);

    updateWindowTitle();
}

//

/*!
 * \brief Check for severe mistakes i.e. values that do not make sense.
 *
 * Checks for instance that a station is set, that end times are larger than corresponding begin times, etc.
 *
 * If a problem is found, the user is asked whether to proceed (ignore the problem) or not.
 * If not, the function directly returns false (do not proceed). True is returned at the end
 * of the function (proceed), i.e. no problem was found or all problems were ignored.
 *
 * \return If no problems found or all found problems ignored.
 */
bool ReportWindow::checkInvalidValues()
{
    if (report.getStation() == "")
    {
        QMessageBox msgBox(QMessageBox::Warning, "Keine Wachstation", "Wachstation nicht gesetzt.\nTrotzdem fortfahren?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }

    if (report.getRadioCallName() == "")
    {
        QMessageBox msgBox(QMessageBox::Warning, "Kein Funkrufname", "Stations-Funkrufname nicht gesetzt.\nTrotzdem fortfahren?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }

    if (boatLogPtr->getBoat() == "")
    {
        QMessageBox msgBox(QMessageBox::Warning, "Kein Boot", "Boot nicht gesetzt.\nTrotzdem fortfahren?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }

    if (boatLogPtr->getRadioCallName() == "")
    {
        QMessageBox msgBox(QMessageBox::Warning, "Kein Funkrufname", "Boots-Funkrufname nicht gesetzt.\nTrotzdem fortfahren?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }

    if (report.getBeginTime().secsTo(report.getEndTime()) < 0)
    {
        QMessageBox msgBox(QMessageBox::Warning, "Ungültige Dienst-Zeiten",
                           "Dienst-Ende liegt vor Dienst-Beginn.\nTrotzdem fortfahren?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }

    for (const QString& tIdent : report.getPersonnel())
    {
        if (report.getPersonBeginTime(tIdent).secsTo(report.getPersonEndTime(tIdent)) < 0)
        {
            QMessageBox msgBox(QMessageBox::Warning, "Ungültige Personal-Zeiten",
                               "Personal-Dienstzeit-Ende liegt vor Personal-Dienstzeit-Beginn.\nTrotzdem fortfahren?",
                               QMessageBox::Abort | QMessageBox::Yes, this);
            if (msgBox.exec() != QMessageBox::Yes)
                return false;
        }
    }

    if (boatLogPtr->getReadyUntil() != QTime(0, 0) && boatLogPtr->getReadyFrom().secsTo(boatLogPtr->getReadyUntil()) < 0)
    {
        QMessageBox msgBox(QMessageBox::Warning, "Ungültige Boots-Bereitschaftszeiten",
                           "Boots-Einsatzbereitschafts-Ende liegt vor Boots-Einsatzbereitschafts-Beginn.\nTrotzdem fortfahren?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }

    if (boatLogPtr->getEngineHoursInitial() > boatLogPtr->getEngineHoursFinal())
    {
        QMessageBox msgBox(QMessageBox::Warning, "Ungültiger Betriebsstundenzählerstand",
                           "Betriebsstundenzähler-Start größer als -Ende.\nTrotzdem fortfahren?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }

    bool firstDrive = true;
    QTime latestEndTime;

    for (const BoatDrive& tDrive : boatLogPtr->getDrives())
    {
        if (tDrive.getPurpose().trimmed() == "")
        {
            QMessageBox msgBox(QMessageBox::Warning, "Kein Fahrt-Zweck", "Kein Bootsfahrt-Zweck angegeben.\nTrotzdem fortfahren?",
                               QMessageBox::Abort | QMessageBox::Yes, this);
            if (msgBox.exec() != QMessageBox::Yes)
                return false;
        }

        if (tDrive.getBoatman() == "")
        {
            QMessageBox msgBox(QMessageBox::Warning, "Kein Bootsführer", "Bootsfahrt hat keinen Bootsführer.\nTrotzdem fortfahren?",
                               QMessageBox::Abort | QMessageBox::Yes, this);
            if (msgBox.exec() != QMessageBox::Yes)
                return false;
        }

        if (tDrive.getBeginTime().secsTo(tDrive.getEndTime()) < 0)
        {
            QMessageBox msgBox(QMessageBox::Warning, "Ungültige Bootsfahrt-Zeiten",
                               "Bootsfahrt-Ende liegt vor Bootsfahrt-Beginn.\nTrotzdem fortfahren?",
                               QMessageBox::Abort | QMessageBox::Yes, this);
            if (msgBox.exec() != QMessageBox::Yes)
                return false;
        }

        if (firstDrive)
            firstDrive = false;
        else
        {
            if (latestEndTime.secsTo(tDrive.getBeginTime()) < 0)
            {
                QMessageBox msgBox(QMessageBox::Warning, "Ungültige Bootsfahrt-Zeiten",
                                   "Bootsfahrt-Zeiten in falscher Reihenfolge oder überschneiden sich.\nTrotzdem fortfahren?",
                                   QMessageBox::Abort | QMessageBox::Yes, this);
                if (msgBox.exec() != QMessageBox::Yes)
                    return false;
            }
        }
        latestEndTime = tDrive.getEndTime();
    }

    for (const auto& it : report.getVehicles())
    {
        if (it.second.first.secsTo(it.second.second) < 0)
        {
            QMessageBox msgBox(QMessageBox::Warning, "Einsatzfahrzeuge",
                               "Fahrzeug-Abfahrtszeit liegt vor Fahrzeug-Ankunftszeit.\nTrotzdem fortfahren?",
                               QMessageBox::Abort | QMessageBox::Yes, this);
            if (msgBox.exec() != QMessageBox::Yes)
                return false;

            break;
        }
    }

    return true;
}

/*!
 * \brief Check for valid but improbable or forgotten values.
 *
 * Checks for values that in principle do make sense but are nevertheless unlikely or unwanted
 * such as values equal to zero (preset values) or no added fuel although there were boat drives etc.
 *
 * If a problem is found, the user is asked whether to proceed (ignore the problem) or not.
 * If not, the function directly returns false (do not proceed). True is returned at the end
 * of the function (proceed), i.e. no problem was found or all problems were ignored.
 *
 * \return If no problems found or all found problems ignored.
 */
bool ReportWindow::checkImplausibleValues()
{
    if (report.getNumber() == 1)
    {
        QMessageBox msgBox(QMessageBox::Warning, "Laufende Nummer", "Laufende Nummer ist 1.\nKorrekt?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }

    if (report.getDate() != QDate::currentDate())
    {
        QMessageBox msgBox(QMessageBox::Warning, "Datum", "Datum ist nicht heute.\nTrotzdem fortfahren?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }

    if (report.getAirTemperature() == 0)
    {
        QMessageBox msgBox(QMessageBox::Warning, "Lufttemperatur", "Lufttemperatur ist 0°C.\nTrotzdem fortfahren?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }

    if (report.getWaterTemperature() == 0)
    {
        QMessageBox msgBox(QMessageBox::Warning, "Wassertemperatur", "Wassertemperatur ist 0°C.\nTrotzdem fortfahren?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }

    if (report.getPersonnelSize() == 0)
    {
        QMessageBox msgBox(QMessageBox::Warning, "Kein Personal", "Kein Personal eingetragen.\nTrotzdem fortfahren?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }

    if (report.getPersonnelMinutesCarry() == 0)
    {
        QMessageBox msgBox(QMessageBox::Warning, "Personalstunden-Übertrag", "Personalstunden-Übertrag ist 0.\nTrotzdem fortfahren?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }

    if (boatLogPtr->getBoatMinutesCarry() == 0)
    {
        QMessageBox msgBox(QMessageBox::Warning, "Bootsstunden-Übertrag", "Bootsstunden-Übertrag ist 0.\nTrotzdem fortfahren?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }

    if (boatLogPtr->getEngineHoursInitial() == 0)
    {
        QMessageBox msgBox(QMessageBox::Warning, "Betriebsstundenzähler", "Betriebsstundenzähler-Start ist 0.\nTrotzdem fortfahren?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }

    if (boatLogPtr->getEngineHoursFinal() == 0)
    {
        QMessageBox msgBox(QMessageBox::Warning, "Betriebsstundenzähler", "Betriebsstundenzähler-Ende ist 0.\nTrotzdem fortfahren?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }

    for (const BoatDrive& tDrive : boatLogPtr->getDrives())
    {
        if (tDrive.crewSize() == 0)
        {
            QMessageBox msgBox(QMessageBox::Warning, "Keine Bootsbesatzung",
                               "Bootsfahrt hat außer dem Bootsführer keine Bootsbesatzung.\nTrotzdem fortfahren?",
                               QMessageBox::Abort | QMessageBox::Yes, this);
            if (msgBox.exec() != QMessageBox::Yes)
                return false;
        }
    }

    if (boatLogPtr->getDrivesCount() > 0)
    {
        int tFuelTotal = boatLogPtr->getFuelInitial() + boatLogPtr->getFuelFinal();

        for (const BoatDrive& tDrive : boatLogPtr->getDrives())
            tFuelTotal += tDrive.getFuel();

        if (tFuelTotal == 0)
        {
            QMessageBox msgBox(QMessageBox::Warning, "Getankt?", "Nichts getankt!?!?.\nTrotzdem fortfahren?",
                               QMessageBox::Abort | QMessageBox::Yes, this);
            if (msgBox.exec() != QMessageBox::Yes)
                return false;
        }
        else if (boatLogPtr->getFuelFinal() == 0)
        {
            QMessageBox msgBox(QMessageBox::Warning, "Getankt?", "Bei Dienstende nicht vollgetankt?\nTrotzdem fortfahren?",
                               QMessageBox::Abort | QMessageBox::Yes, this);
            if (msgBox.exec() != QMessageBox::Yes)
                return false;
        }

        if (boatLogPtr->getEngineHoursFinal() == boatLogPtr->getEngineHoursInitial())
        {
            QMessageBox msgBox(QMessageBox::Warning, "Betriebsstundenzähler",
                               "Betriebsstundenzähler-Ende trotz Fahrten gleich -Start.\nTrotzdem fortfahren?",
                               QMessageBox::Abort | QMessageBox::Yes, this);
            if (msgBox.exec() != QMessageBox::Yes)
                return false;
        }
    }

    if (boatLogPtr->getReadyFrom() == QTime(0, 0))
    {
        QMessageBox msgBox(QMessageBox::Warning, "Boots-Bereitschaftszeiten",
                           "Boots-Einsatzbereitschafts-Beginn ist 00:00 Uhr.\nTrotzdem fortfahren?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }

    if (boatLogPtr->getReadyUntil() == QTime(0, 0) && boatLogPtr->getReadyFrom().secsTo(boatLogPtr->getReadyUntil()) < 0)
    {
        QMessageBox msgBox(QMessageBox::Warning, "Boots-Bereitschaftszeiten",
                           "Boots-Einsatzbereitschafts-Ende liegt vor Boots-Einsatzbereitschafts-Beginn.\nTrotzdem fortfahren?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }
    else if (boatLogPtr->getReadyFrom() == boatLogPtr->getReadyUntil())
    {
        QMessageBox msgBox(QMessageBox::Warning, "Boot nicht einsatzbereit?",
                           "Boot in keinem Zeitraum einsatzbereit.\nTrotzdem fortfahren?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }

    if (personWithFunctionPresent(Person::Function::_FUD) && report.getAssignmentNumber() == "")
    {
        QMessageBox msgBox(QMessageBox::Warning, "Einsatznummer?",
                           "Person im Führungsdienst aber keine Einsatznummer eingetragen.\nTrotzdem fortfahren?",
                           QMessageBox::Abort | QMessageBox::Yes, this);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
    }

    for (const auto& it : report.getVehicles())
    {
        if (it.second.second == it.second.first)
        {
            QMessageBox msgBox(QMessageBox::Warning, "Einsatzfahrzeuge",
                               "Fahrzeug-Abfahrtszeit gleich Fahrzeug-Ankunftszeit.\nTrotzdem fortfahren?",
                               QMessageBox::Abort | QMessageBox::Yes, this);
            if (msgBox.exec() != QMessageBox::Yes)
                return false;

            break;
        }
    }

    return true;
}

//

/*!
 * \brief Update the window title.
 *
 * Sets the window title according to report file name, report date, unsaved changes and unapplied boat drive changes.
 * The file name is only used to show "[Template]" in the title, if it is empty.
 */
void ReportWindow::updateWindowTitle()
{
    QString title = "Wachbericht ";

    //Add hint, if not saved to file yet
    if (report.getFileName() == "")
        title.append("[Vorlage] ");

    title.append("- " + report.getDate().toString("dd.MM.yyyy"));

    //Show hint for unsaved changes
    //Use a different hint for (additionally) not applied changes to the selected boat drive
    if (unsavedChanges || unappliedBoatDriveChanges)
    {
        title.prepend("* ");

        if (unappliedBoatDriveChanges)
            title.prepend("*");
    }

    setWindowTitle(title);
}

/*!
 * \brief Update the total (carry + new) personnel hours display.
 */
void ReportWindow::updateTotalPersonnelHours()
{
    int hours = ui->personnelHoursHours_spinBox->value() + ui->personnelHoursCarryHours_spinBox->value();
    int minutes = ui->personnelHoursMinutes_spinBox->value() + ui->personnelHoursCarryMinutes_spinBox->value();

    int modMinutes = minutes % 60;
    int modHours = hours + ((minutes - modMinutes) / 60);

    ui->personnelHoursTotalHours_spinBox->setValue(modHours);
    ui->personnelHoursTotalMinutes_spinBox->setValue(modMinutes);
}

/*!
 * \brief Update the total (carry + new) boat drive hours display.
 */
void ReportWindow::updateTotalBoatHours()
{
    int hours = ui->boatHoursHours_spinBox->value() + ui->boatHoursCarryHours_spinBox->value();
    int minutes = ui->boatHoursMinutes_spinBox->value() + ui->boatHoursCarryMinutes_spinBox->value();

    int modMinutes = minutes % 60;
    int modHours = hours + ((minutes - modMinutes) / 60);

    ui->boatHoursTotalHours_spinBox->setValue(modHours);
    ui->boatHoursTotalMinutes_spinBox->setValue(modMinutes);
}

/*!
 * \brief Update the accumulated personnel hours display.
 */
void ReportWindow::updatePersonnelHours()
{
    std::vector<QString> tIdents = report.getPersonnel();

    int tPersonnelMinutes = 0;
    for (const QString& tIdent : tIdents)
        tPersonnelMinutes += report.getPersonBeginTime(tIdent).secsTo(report.getPersonEndTime(tIdent)) / 60;

    int modMinutes = tPersonnelMinutes % 60;
    int modHours = (tPersonnelMinutes - modMinutes) / 60;

    ui->personnelHoursHours_spinBox->setValue(modHours);
    ui->personnelHoursMinutes_spinBox->setValue(modMinutes);
}

/*!
 * \brief Update the personnel table widget (and personnel hours).
 *
 * Clears and re-fills the personnel table with current report personnel. Also calls updatePersonnelHours().
 */
void ReportWindow::updatePersonnelTable()
{
    std::vector<QString> tIdents = report.getPersonnel(true);

    std::set<QString> tSelectedIdents;
    for (QString tIdent : getSelectedPersons())
        tSelectedIdents.insert(std::move(tIdent));

    std::vector<int> tNewSelectedRows;

    ui->personnel_tableWidget->setRowCount(0);

    int rowCount = 0;
    for (const QString& tIdent : tIdents)
    {
        Person tPerson = report.getPerson(tIdent);

        ui->personnel_tableWidget->insertRow(rowCount);

        ui->personnel_tableWidget->setItem(rowCount, 0, new QTableWidgetItem(tPerson.getLastName()));
        ui->personnel_tableWidget->setItem(rowCount, 1, new QTableWidgetItem(tPerson.getFirstName()));
        ui->personnel_tableWidget->setItem(rowCount, 2, new QTableWidgetItem(Person::functionToLabel(
                                                                                 report.getPersonFunction(tIdent))));
        ui->personnel_tableWidget->setItem(rowCount, 3, new QTableWidgetItem(report.getPersonBeginTime(tIdent).toString("hh:mm")));
        ui->personnel_tableWidget->setItem(rowCount, 4, new QTableWidgetItem(report.getPersonEndTime(tIdent).toString("hh:mm")));
        ui->personnel_tableWidget->setItem(rowCount, 5, new QTableWidgetItem(tIdent));

        if (tSelectedIdents.find(tIdent) != tSelectedIdents.end())
            tNewSelectedRows.push_back(rowCount);

        ++rowCount;
    }

    //Restore selection
    ui->personnel_tableWidget->setSelectionMode(QAbstractItemView::SelectionMode::MultiSelection);
    for (int row : tNewSelectedRows)
        ui->personnel_tableWidget->selectRow(row);
    ui->personnel_tableWidget->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

    //Also update personnel hours
    updatePersonnelHours();
}

/*!
 * \brief Update the spin box summing up fuel added after individual boat drives.
 */
void ReportWindow::updateBoatDrivesFuel()
{
    auto tDrives = boatLogPtr->getDrives();

    int tDrivesFuel = 0;
    for (const BoatDrive& tDrive : tDrives)
        tDrivesFuel += tDrive.getFuel();

    ui->fuelAfterDrives_spinBox->setValue(tDrivesFuel);
}

/*!
 * \brief Update the accumulated boat drive hours display.
 */
void ReportWindow::updateBoatDrivesHours()
{
    auto tDrives = boatLogPtr->getDrives();

    int tDrivesMinutes = 0;
    for (const BoatDrive& tDrive : tDrives)
        tDrivesMinutes += tDrive.getBeginTime().secsTo(tDrive.getEndTime()) / 60;

    int modMinutes = tDrivesMinutes % 60;
    int modHours = (tDrivesMinutes - modMinutes) / 60;

    ui->boatHoursHours_spinBox->setValue(modHours);
    ui->boatHoursMinutes_spinBox->setValue(modMinutes);
}

/*!
 * \brief Update the boat drives table widget (and fuel and boat drive hours).
 *
 * Clears and re-fills the boat drives table widget with current boat log boat drives.
 * Also calls updateBoatDrivesFuel() and updateBoatDrivesHours().
 */
void ReportWindow::updateBoatDrivesTable()
{
    //Try to restore selected row after update
    int tSelectedRow = ui->boatDrives_tableWidget->currentRow();

    ui->boatDrives_tableWidget->setRowCount(0);

    int rowCount = 0;
    bool firstDrive = true;
    QTime latestEndTime;

    for (const BoatDrive& tDrive : boatLogPtr->getDrives())
    {
        //Add hint to table entry if important information such as the end time appears to be (still) missing
        bool driveDataIncomplete = false;

        if (tDrive.getPurpose().trimmed() == "" || tDrive.getBoatman() == "" || tDrive.crewSize() == 0 ||
            tDrive.getBeginTime().secsTo(tDrive.getEndTime()) <= 0 || (!firstDrive && latestEndTime.secsTo(tDrive.getBeginTime()) < 0))
        {
            driveDataIncomplete = true;
        }

        if (firstDrive)
            firstDrive = false;

        latestEndTime = tDrive.getEndTime();

        ui->boatDrives_tableWidget->insertRow(rowCount);

        ui->boatDrives_tableWidget->setItem(rowCount, 0, new QTableWidgetItem("#" + QString::number(rowCount+1) + " [" +
                                                                              tDrive.getBeginTime().toString("hh:mm") + " - " +
                                                                              tDrive.getEndTime().toString("hh:mm") +
                                                                              "] " + (driveDataIncomplete ? "TO" "DO " : "") +
                                                                              "- " + tDrive.getPurpose()));

        ++rowCount;
    }

    if (tSelectedRow < boatLogPtr->getDrivesCount())
        ui->boatDrives_tableWidget->setCurrentCell(tSelectedRow, 0);

    updateBoatDrivesFuel();
    updateBoatDrivesHours();
}

/*!
 * \brief Update the list of persons selectable as boatman or crew member.
 *
 * Remembers selected boatman and sets it again after combo box updated.
 * Since this triggers on_boatDriveBoatman_comboBox_currentTextChanged(),
 * the current unapplied boat drive changes state is also remembered and set accordingly again.
 */
void ReportWindow::updateBoatDriveAvailablePersons()
{
    //Remember selected boatman and restore at end of function
    QString tBoatmanIdent = selectedBoatmanIdent;

    //Temporarily block signals to prevent handling of transient wrong boatman selection while updating combo box items
    ui->boatDriveBoatman_comboBox->blockSignals(true);

    ui->boatDriveBoatman_comboBox->clear();
    ui->boatCrewMember_comboBox->clear();

    for (const QString& tIdent : report.getPersonnel(true))
    {
        QString tLabel = personLabelFromIdent(tIdent);

        ui->boatCrewMember_comboBox->insertItem(ui->boatCrewMember_comboBox->count(), tLabel);

        if (QualificationChecker::checkBoatman(report.getPerson(tIdent).getQualifications()))
            ui->boatDriveBoatman_comboBox->insertItem(ui->boatDriveBoatman_comboBox->count(), tLabel);
    }

    QString tBoatmanLabel = "";
    if (tBoatmanIdent != "")
        tBoatmanLabel = personLabelFromIdent(tBoatmanIdent);

    ui->boatDriveBoatman_comboBox->setCurrentIndex(ui->boatDriveBoatman_comboBox->findText(tBoatmanLabel));

    //Stop temporary blocking of signals again
    ui->boatDriveBoatman_comboBox->blockSignals(false);
}

/*!
 * \brief Set report vehicles list according to vehicles entered in UI table.
 *
 * The list of all vehicles (with non-empty name field) currently
 * entered in the dynamic UI table is written to the report.
 *
 * Sets the 'unsaved changes' switch, if new list differs from old list.
 */
void ReportWindow::updateReportVehiclesList()
{
    std::vector<std::pair<QString, std::pair<QTime, QTime>>> vehicles;
    std::vector<std::pair<QString, std::pair<QTime, QTime>>> vehiclesOld = report.getVehicles();

    for (const auto& row : vehiclesTableRows)
    {
        if (row.vehicleNameLineEdit->text().trimmed() == "")    //Skip rows without vehicle name
            continue;

        vehicles.push_back({row.vehicleNameLineEdit->text().trimmed(), {row.arriveTimeEdit->time(), row.leaveTimeEdit->time()}});
    }

    if (vehicles != vehiclesOld)
    {
        report.setVehicles(vehicles);
        setUnsavedChanges();
    }
}

/*!
 * \brief Apply changed boat drive data to specified boat drive in report.
 *
 * The drive data is applied to the drive corresponding to the boat drives table row \p pRow.
 * Note: Returns immediately, if \p pRow == -1.
 *
 * Then the boat drives table is updated (see updateBoatDrivesTable()) to display the new drive purpose in case this has changed.
 *
 * Resets setUnappliedBoatDriveChanges().
 *
 * Sets setUnsavedChanges() (if \p pRow was not -1).
 *
 * \param pRow Boat drive number to which to apply the changed data.
 */
void ReportWindow::applyBoatDriveChanges(int pRow)
{
    if (pRow == -1)
        return;

    BoatDrive& tDrive = boatLogPtr->getDrive(pRow);

    tDrive.setPurpose(ui->boatDrivePurpose_comboBox->currentText());
    tDrive.setBeginTime(ui->boatDriveBegin_timeEdit->time());
    tDrive.setEndTime(ui->boatDriveEnd_timeEdit->time());
    tDrive.setFuel(ui->boatDriveFuel_spinBox->value());
    tDrive.setComments(ui->boatDriveComments_plainTextEdit->toPlainText());

    if (ui->boatDriveBoatman_comboBox->currentText() != "")
        tDrive.setBoatman(personIdentFromLabel(ui->boatDriveBoatman_comboBox->currentText()));
    else
        tDrive.setBoatman("");

    tDrive.clearCrew();

    for (int row = 0; row < ui->boatCrew_tableWidget->rowCount(); ++row)
    {
        QString tIdent = ui->boatCrew_tableWidget->item(row, 3)->text();

        //Double-check that person is still part of personnel (because of "table widget cache")
        if (!report.personExists(tIdent))
            continue;

        Person::BoatFunction tBoatFunction = Person::labelToBoatFunction(ui->boatCrew_tableWidget->item(row, 2)->text());

        //Double-check that person's qualifications still allow the function (might have changed; because of "table widget cache")
        if (!QualificationChecker::checkBoatFunction(tBoatFunction, report.getPerson(tIdent).getQualifications()))
            continue;

        tDrive.addCrewMember(tIdent, tBoatFunction);
    }

    setUnappliedBoatDriveChanges(false);
    setUnsavedChanges();

    updateBoatDrivesTable();
}

//

/*!
 * \brief Add a person to the crew member table.
 *
 * \param pPerson The person.
 * \param pFunction Person's boat drive function (qualifications not checked here!).
 */
void ReportWindow::insertBoatCrewTableRow(const Person& pPerson, Person::BoatFunction pFunction)
{
    int rowCount = ui->boatCrew_tableWidget->rowCount();

    ui->boatCrew_tableWidget->insertRow(rowCount);

    ui->boatCrew_tableWidget->setItem(rowCount, 0, new QTableWidgetItem(pPerson.getLastName()));
    ui->boatCrew_tableWidget->setItem(rowCount, 1, new QTableWidgetItem(pPerson.getFirstName()));
    ui->boatCrew_tableWidget->setItem(rowCount, 2, new QTableWidgetItem(Person::boatFunctionToLabel(pFunction)));
    ui->boatCrew_tableWidget->setItem(rowCount, 3, new QTableWidgetItem(pPerson.getIdent()));
}

/*!
 * \brief Add a vehicles table row for a new vehicle.
 *
 * Adds a new row to the dynamic vehicles UI table (adds push button, line edit, label, two time edits)
 * with pre-filled vehicle (radio call) name \p pName, arrival time \p pArrivalTime and leaving time \p pLeavingTime.
 * The time edits are disabled, if (trimmed) \p pName is empty.
 *
 * Note: Connects a number of slots to widgets' signals, which can/will be
 * disconnected again by one of those slots, on_vehicleRemovePushButtonPressed().
 *
 * \param pName Radio call name of the vehicle.
 * \param pArrivalTime Arrival time.
 * \param pLeavingTime Leaving time.
 */
void ReportWindow::addVehiclesTableRow(QString pName, QTime pArrivalTime, QTime pLeavingTime)
{
    //Create widgets
    QPushButton* tPushButton = new QPushButton("x", ui->vehicles_groupBox);
    QLineEdit* tLineEdit = new QLineEdit(pName.trimmed(), ui->vehicles_groupBox);
    QTimeEdit* tArrivalTimeEdit = new QTimeEdit(pArrivalTime, ui->vehicles_groupBox);
    QTimeEdit* tLeavingTimeEdit = new QTimeEdit(pLeavingTime, ui->vehicles_groupBox);
    QLabel* tLabel = new QLabel("-", ui->vehicles_groupBox);

    tPushButton->setMaximumWidth(30);
    tLineEdit->setValidator(new QRegularExpressionValidator(Aux::radioCallNamesValidator.regularExpression(), tLineEdit));
    tArrivalTimeEdit->setEnabled(pName.trimmed() != "");
    tLeavingTimeEdit->setEnabled(pName.trimmed() != "");

    //Explicitly set font so that time edit height does not change when style sheet is changed (for red background color)
    tArrivalTimeEdit->setFont(QFont("Tahoma", 8));
    tLeavingTimeEdit->setFont(QFont("Tahoma", 8));

    //Add widgets to new layout row
    int row = vehiclesGroupBoxLayout->rowCount();
    vehiclesGroupBoxLayout->addWidget(tPushButton, row, 0);
    vehiclesGroupBoxLayout->addWidget(tLineEdit, row, 1);
    vehiclesGroupBoxLayout->addWidget(tArrivalTimeEdit, row, 2);
    vehiclesGroupBoxLayout->addWidget(tLabel, row, 3);
    vehiclesGroupBoxLayout->addWidget(tLeavingTimeEdit, row, 4);

    //Connect signals to slots

    std::vector<QMetaObject::Connection> connections {
        connect(tPushButton, &QPushButton::pressed, this,
                [this, tPushButton]() -> void {
                    this->on_vehicleRemovePushButtonPressed(tPushButton);
                }),
        connect(tLineEdit, &QLineEdit::textEdited, this,
                [this, tPushButton](const QString&) -> void {
                    this->on_vehicleLineEditTextEdited(tPushButton);
                }),
        connect(tLineEdit, &QLineEdit::returnPressed, this,
                [this, tPushButton]() -> void {
                    this->on_vehicleLineEditReturnPressed(tPushButton);
                }),
        connect(tArrivalTimeEdit, &QTimeEdit::timeChanged, this,
                [this, tPushButton]() -> void {
                    this->on_vehicleTimeEditTimeChanged(tPushButton);
                }),
        connect(tLeavingTimeEdit, &QTimeEdit::timeChanged, this,
                [this, tPushButton]() -> void {
                    this->on_vehicleTimeEditTimeChanged(tPushButton);
                })
    };

    //Add pointers to list
    vehiclesTableRows.push_back({tPushButton, tLineEdit, tArrivalTimeEdit, tLeavingTimeEdit, tLabel, std::move(connections)});

    //Update leaving time edit color
    on_vehicleTimeEditTimeChanged(tPushButton);
}

//

/*!
 * \brief Check entered person name and update selectable identifiers list accordingly.
 *
 * Checks, if at least one person with the entered person name exists in the personnel database.
 * If so, inserts identifiers of all matching persons into the person identifier combo box and enables the add person button.
 * Clears the combo box and disables the button else.
 */
void ReportWindow::checkPersonInputs()
{
    ui->personIdent_comboBox->clear();
    ui->personFunction_comboBox->clear();

    ui->personLastName_lineEdit->setStyleSheet("");
    ui->personFirstName_lineEdit->setStyleSheet("");

    ui->addPerson_pushButton->setEnabled(false);

    if (ui->personLastName_lineEdit->text() == "" && ui->personFirstName_lineEdit->text() == "")
        return;

    std::vector<Person> tPersons;
    DatabaseCache::getPersons(tPersons, ui->personLastName_lineEdit->text(), ui->personFirstName_lineEdit->text(), true);

    if (tPersons.size() == 0)
    {
        ui->personLastName_lineEdit->setStyleSheet("QLineEdit { color: red; }");
        ui->personFirstName_lineEdit->setStyleSheet("QLineEdit { color: red; }");
    }
    else
    {
        for (const Person& tPerson : tPersons)
            ui->personIdent_comboBox->insertItem(ui->personIdent_comboBox->count(), tPerson.getIdent());

        ui->personIdent_comboBox->setCurrentIndex(0);

        ui->addPerson_pushButton->setEnabled(true);
    }
}

//

/*!
 * \brief Get all selected personnel table entries' person identifiers.
 *
 * \return List of identifiers corresponding to the selected persons of the personnel table.
 */
std::vector<QString> ReportWindow::getSelectedPersons() const
{
    std::vector<QString> tSelectedIdents;

    QModelIndexList idxList = ui->personnel_tableWidget->selectionModel()->selectedRows();
    for (auto it = idxList.begin(); it != idxList.end(); ++it)
        tSelectedIdents.push_back(ui->personnel_tableWidget->item((*it).row(), 5)->text());

    return tSelectedIdents;
}

//

/*!
 * \brief Set report number if larger zero (and update display and buttons).
 *
 * Sets the report serial number to \p pNumber, if larger zero, and updates the report number LCD number display.
 * Disables the decrement report number radio button, iff \p pNumber is 1.
 *
 * \param pNumber New serial number.
 */
void ReportWindow::setSerialNumber(int pNumber)
{
    if (pNumber <= 0)
    {
        QMessageBox(QMessageBox::Warning, "Warnung", "Laufende Nummer muss positiv sein!", QMessageBox::Ok, this).exec();
        return;
    }

    if (pNumber == 1)
    {
        ui->reportNumberDecr_radioButton->setEnabled(false);
        ui->reportNumber_lcdNumber->setStyleSheet("QLCDNumber { background-color: red; }");
    }
    else
    {
        ui->reportNumberDecr_radioButton->setEnabled(true);
        ui->reportNumber_lcdNumber->setStyleSheet("");
    }

    ui->reportNumber_lcdNumber->display(QString::number(pNumber));

    report.setNumber(pNumber);
}

/*!
 * \brief Set hours and minutes display of personnel hours carry.
 *
 * \param pMinutes New personnel hours carry in minutes.
 */
void ReportWindow::setPersonnelHoursCarry(int pMinutes)
{
    int modMinutes = pMinutes % 60;
    int modHours = (pMinutes - modMinutes) / 60;

    ui->personnelHoursCarryHours_spinBox->setValue(modHours);
    ui->personnelHoursCarryMinutes_spinBox->setValue(modMinutes);
}

/*!
 * \brief Set hours and minutes display of boat drive hours carry.
 *
 * \param pMinutes New boat hours carry in minutes.
 */
void ReportWindow::setBoatHoursCarry(int pMinutes)
{
    int modMinutes = pMinutes % 60;
    int modHours = (pMinutes - modMinutes) / 60;

    ui->boatHoursCarryHours_spinBox->setValue(modHours);
    ui->boatHoursCarryMinutes_spinBox->setValue(modMinutes);
}

//

/*!
 * \brief Check, if a function is set for any person of personnel.
 *
 * Searches the report personnel list for a person with personnel function \p pFunction.
 *
 * \param pFunction Personnel function to search for.
 * \return If any person with function \p pFunction found.
 */
bool ReportWindow::personWithFunctionPresent(Person::Function pFunction) const
{
    std::vector<QString> tPersonnel = report.getPersonnel();

    for (const QString& tIdent : tPersonnel)
    {
        if (report.getPersonFunction(tIdent) == pFunction)
            return true;
    }

    return false;
}

/*!
 * \brief Check, if a person is boatman or crew member of any drive.
 *
 * Searches boat drives' boatmen and crew members for a person with identifier \p pIdent.
 *
 * \param pIdent Person's identifier.
 * \return If person \p pPerson is listed in any boat drive.
 */
bool ReportWindow::personInUse(const QString& pIdent) const
{
    //Check for person in all boat drives
    for (const BoatDrive& tDrive : boatLogPtr->getDrives())
    {
        if (tDrive.getBoatman() == pIdent)
            return true;

        for (const auto& it : tDrive.crew())
            if (it.first == pIdent)
                return true;
    }

    //Also check for person in unapplied changes of selected drive
    if (ui->boatDrives_tableWidget->currentRow() != -1 && unappliedBoatDriveChanges)
    {
        if (ui->boatDriveBoatman_comboBox->currentIndex() != -1)
            if (personIdentFromLabel(ui->boatDriveBoatman_comboBox->currentText()) == pIdent)
                return true;

        for (int row = 0; row < ui->boatCrew_tableWidget->rowCount(); ++row)
            if (ui->boatCrew_tableWidget->item(row, 3)->text() == pIdent)
                return true;
    }

    return false;
}

//

/*!
 * \brief Get a formatted combo box label from a person identifier.
 *
 * Takes the first and last name of the report person with identifier \p pIdent
 * and returns a label either with format "Last, First" or with format "Last, First [Ident]".
 * The latter format is chosen, if the report personnel contains multiple persons with the person's name.
 * This makes the labels look clean but be unique at the same time, if all used labels are
 * always re-generated whenever the personnel changes.
 *
 * \param pIdent Identifier of the person.
 * \return Label containing the person's name (and possibly \p pIdent).
 */
QString ReportWindow::personLabelFromIdent(const QString& pIdent) const
{
    Person tPerson = report.getPerson(pIdent);
    QString tLastName = tPerson.getLastName();
    QString tFirstName = tPerson.getFirstName();

    bool ambiguous = report.personIsAmbiguous(tLastName, tFirstName);

    return tPerson.getLastName() + ", " + tPerson.getFirstName() + (ambiguous ? (" [" + pIdent + "]") : "");
}

/*!
 * \brief Get the person identifier from a combo box label.
 *
 * Extracts a person name (and identifier) from label \p pLabel generated by personLabelFromIdent().
 * If the label contains an identifier, this one is simply returned.
 * If the label only contains the name, the person name is assumed to be unique
 * and searched for in the report personnel list. If the person is found,
 * its identifier is returned and an empty string else.
 *
 * \param pLabel Label for a person generated by personLabelFromIdent().
 * \return Identifier of the person represented by \p pLabel.
 */
QString ReportWindow::personIdentFromLabel(const QString& pLabel) const
{
    QStringList tSplit = pLabel.split('[');

    if (tSplit.size() == 2)
        return tSplit.at(1).split(']').front();
    else
    {
        QStringList tNameParts = tSplit.at(0).split(',');
        QString tLastName = tNameParts.at(0);
        QString tFirstName = tNameParts.at(1).trimmed();

        for (const QString& tIdent : report.getPersonnel())
        {
            Person tPerson = report.getPerson(tIdent);
            if (tPerson.getLastName() == tLastName && tPerson.getFirstName() == tFirstName)
                return tIdent;
        }

        return "";
    }
}

//Private slots

/*!
 * \brief Set report rescue operation counter.
 *
 * Sets the report's rescue operation counter for rescue operation type \p pRescue to \p pValue.
 *
 * Sets setUnsavedChanges().
 *
 * \param pValue New number.
 * \param pRescue Type of rescue operation.
 */
void ReportWindow::on_rescueOperationSpinBoxValueChanged(int pValue, Report::RescueOperation pRescue)
{
    report.setRescueOperationCtr(pRescue, pValue);
    setUnsavedChanges();

    //Show "fill document" notice, if count is non-zero
    if (pValue > 0)
        rescuesTableRows.at(pRescue).fillDocNoticeLabel->setText(Report::rescueOperationToDocNotice(pRescue));
    else
        rescuesTableRows.at(pRescue).fillDocNoticeLabel->setText("");

    //Set maximum number of _MORTAL_DANGER_INVOLVED spin box to sum of all other rescue operations as it just describes subset of those

    int tNumRescues = 0;

    for (auto it : report.getRescueOperationCtrs())
    {
        if (it.first == Report::RescueOperation::_MORTAL_DANGER_INVOLVED)
            continue;

        tNumRescues += it.second;
    }

    for (const auto& it : rescuesTableRows)
    {
        if (it.first == Report::RescueOperation::_MORTAL_DANGER_INVOLVED)
        {
            it.second.spinBox->setMaximum(tNumRescues);
            break;
        }
    }
}

/*!
 * \brief Open one of the important documents.
 *
 * Opens the document at path \p pDocFile.
 *
 * \param pDocFile Path to the document-
 */
void ReportWindow::on_openDocumentPushButtonPressed(const QString& pDocFile)
{
    if (!QFileInfo::exists(pDocFile))
    {
        QMessageBox(QMessageBox::Critical, "Dokument existiert nicht", "Die Datei \"" + pDocFile + "\" existiert nicht!",
                    QMessageBox::Ok, this).exec();
        return;
    }

    //Open file using OS default application
    QDesktopServices::openUrl(QUrl::fromLocalFile(pDocFile).url());
}

/*!
 * \brief Remove a rescue vehicle from the list.
 *
 * Removes the UI table row (widgets and corresponding signal-slot connections),
 * whose remove button is \p pRemoveButton. Calls updateReportVehiclesList(),
 * which then removes the corresponding vehicle from the report vehicles list.
 *
 * If the row is the last remaining row, the row will not be removed but instead the vehicle's
 * name field will be reset (such that new vehicles can still be added). The effect on the
 * report vehicles list stays the same (see also on_vehicleLineEditTextEdited()).
 *
 * \param pRemoveRowButton "Remove row" button of the row that is to be removed (no index available).
 */
void ReportWindow::on_vehicleRemovePushButtonPressed(QPushButton* pRemoveRowButton)
{
    for (auto it = vehiclesTableRows.begin(); it != vehiclesTableRows.end(); ++it)
    {
        if ((*it).removeRowPushButton == pRemoveRowButton)
        {
            if (vehiclesTableRows.size() < 2)   //Keep one empty row, just remove name (vehicle list then updated below)
            {
                (*it).vehicleNameLineEdit->setText("");
                on_vehicleLineEditTextEdited(pRemoveRowButton);

                //Can skip update step below (called by slot already)
                return;
            }
            else
            {
                //Disconnect all slots
                for (const QMetaObject::Connection& conn : (*it).connections)
                    disconnect(conn);

                //Remove widgets from layout
                vehiclesGroupBoxLayout->removeWidget((*it).removeRowPushButton);
                vehiclesGroupBoxLayout->removeWidget((*it).vehicleNameLineEdit);
                vehiclesGroupBoxLayout->removeWidget((*it).arriveTimeEdit);
                vehiclesGroupBoxLayout->removeWidget((*it).leaveTimeEdit);
                vehiclesGroupBoxLayout->removeWidget((*it).timesSepLabel);

                //Delete widgets
                delete (*it).removeRowPushButton;
                delete (*it).vehicleNameLineEdit;
                delete (*it).arriveTimeEdit;
                delete (*it).leaveTimeEdit;
                delete (*it).timesSepLabel;

                //Erase pointers
                vehiclesTableRows.erase(it);
            }

            updateReportVehiclesList();

            return;
        }
    }
}

/*!
 * \brief Change the radio call name of a rescue vehicle.
 *
 * Updates the report vehicles list (see updateReportVehiclesList()).
 * The time edits in the row whose remove button is \p pRemoveButton (should refer to the edited row)
 * are reset and disabled, if the row's (trimmed) radio call name is empty (no vehicle), and enabled otherwise.
 *
 * Note: The row's radio call name (the line edit text) will be reset, if it contains only whitespaces.
 *
 * \param pRemoveRowButton "Remove row" button of the edited row (no index available).
 */
void ReportWindow::on_vehicleLineEditTextEdited(QPushButton* pRemoveRowButton)
{
    for (const auto& row : vehiclesTableRows)
    {
        if (row.removeRowPushButton == pRemoveRowButton)
        {
            //Disable/reset time edits, if vehicle name removed
            if (row.vehicleNameLineEdit->text().trimmed() == "")
            {
                //Trim text
                if (row.vehicleNameLineEdit->text() != "")
                    row.vehicleNameLineEdit->setText("");

                row.arriveTimeEdit->setEnabled(false);
                row.leaveTimeEdit->setEnabled(false);

                row.arriveTimeEdit->setTime(report.getBeginTime());
                row.leaveTimeEdit->setTime(report.getEndTime());

                row.leaveTimeEdit->setStyleSheet("");
            }
            else
            {
                row.arriveTimeEdit->setEnabled(true);
                row.leaveTimeEdit->setEnabled(true);
            }

            updateReportVehiclesList();

            on_vehicleTimeEditTimeChanged(pRemoveRowButton);

            return;
        }
    }
}

/*!
 * \brief Add an empty row for another rescue vehicle.
 *
 * Adds an empty UI table row (widgets and corresponding signal-slot connections) for entering
 * another vehicle, if 'return' was pressed in the \e last \e row's line edit (determined by
 * row of \p pRemoveRowButton), and if last row's line edit already represents
 * a vehicle (i.e. is not empty). See also addVehiclesTableRow().
 *
 * \param pRemoveRowButton "Remove row" button of the row in which 'return' was pressed (no index available).
 */
void ReportWindow::on_vehicleLineEditReturnPressed(QPushButton* pRemoveRowButton)
{
    for (auto it = vehiclesTableRows.begin(); it != vehiclesTableRows.end(); ++it)
    {
        if ((*it).removeRowPushButton == pRemoveRowButton)
        {
            //Only add row, if return pressed in last row's line edit and if this row's name is not empty
            if (it != --(vehiclesTableRows.end()) || (*it).vehicleNameLineEdit->text().trimmed() == "")
                return;

            addVehiclesTableRow("", report.getBeginTime(), report.getEndTime());

            (--vehiclesTableRows.end())->vehicleNameLineEdit->setFocus();

            return;
        }
    }
}

/*!
 * \brief Change a vehicle's arrival/leaving times.
 *
 * Updates the report vehicles list (see updateReportVehiclesList()), if the (trimmed)
 * radio call name in the row of \p pRemoveRowButton is not empty (an actual vehicle entry).
 *
 * \param pRemoveRowButton "Remove row" button of the edited row (no index available).
 */
void ReportWindow::on_vehicleTimeEditTimeChanged(QPushButton* pRemoveRowButton)
{
    for (const auto& row : vehiclesTableRows)
    {
        if (row.removeRowPushButton == pRemoveRowButton)
        {
            if (row.vehicleNameLineEdit->text().trimmed() != "")
            {
                updateReportVehiclesList();

                if (row.leaveTimeEdit->isEnabled() && row.arriveTimeEdit->time().secsTo(row.leaveTimeEdit->time()) <= 0)
                    row.leaveTimeEdit->setStyleSheet("QTimeEdit { background-color: red; }");
                else
                    row.leaveTimeEdit->setStyleSheet("");
            }

            return;
        }
    }
}

//

/*!
 * \brief Update the time displayed in every tab.
 */
void ReportWindow::on_updateClocksTimerTimeout()
{
    QString timeText = QTime::currentTime().toString("hh:mm:ss");

    ui->reportTabTime_lcdNumber->display(timeText);
    ui->boatTabTime_lcdNumber->display(timeText);
    ui->rescueTabTime_lcdNumber->display(timeText);
}

/*!
 * \brief Auto-save the report.
 *
 * See autoSave().
 *
 * Only calls autoSave() if there are unsaved changes.
 */
void ReportWindow::on_autoSaveTimerTimeout()
{
    if (unsavedChanges)
        autoSave();
}

/*!
 * \brief Show (non-modal) message box with current time.
 */
void ReportWindow::on_timestampShortcutActivated()
{
    QMessageBox* msgBox = new QMessageBox(QMessageBox::Information, "Zeitstempel", "Zeit: " + QTime::currentTime().toString("hh:mm:ss"),
                                          QMessageBox::Ok, this);

    //Non-modal message box
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->setWindowModality(Qt::NonModal);
    msgBox->show();
}

/*!
 * \brief Show message box explaining that export failed.
 */
void ReportWindow::on_exportFailed()
{
    QMessageBox(QMessageBox::Warning, "Exportieren fehlgeschlagen", "Fehler beim Exportieren!", QMessageBox::Ok, this).exec();
}

//

/*!
 * \brief Save the report to (the same) file.
 *
 * Saves the report to the current report file name (see saveReport()).
 * Asks for a file name, if this is empty (see on_saveFileAs_action_triggered()).
 */
void ReportWindow::on_saveFile_action_triggered()
{
    if (report.getFileName() == "")
        on_saveFileAs_action_triggered();
    else
    {
        if (QFileInfo::exists(report.getFileName()))
        {
            QMessageBox msgBox(QMessageBox::Question, "Überschreiben?", "Datei überschreiben?",
                               QMessageBox::Abort | QMessageBox::Ok, this);
            if(msgBox.exec() != QMessageBox::Ok)
                return;
        }

        saveReport(report.getFileName());
    }
}

/*!
 * \brief Save the report to a (different) file.
 *
 * Asks for a file name and saves the report to this file.
 * See also saveReport().
 */
void ReportWindow::on_saveFileAs_action_triggered()
{
    QFileDialog fileDialog(this, "Wachbericht speichern", "", "Wachberichte (*.wbr)");
    fileDialog.setDefaultSuffix("wbr");
    fileDialog.setFileMode(QFileDialog::FileMode::AnyFile);
    fileDialog.setAcceptMode(QFileDialog::AcceptMode::AcceptSave);

    if (fileDialog.exec() != QDialog::DialogCode::Accepted)
        return;

    QStringList tFileNames = fileDialog.selectedFiles();

    if (tFileNames.empty() || tFileNames.at(0) == "")
    {
        QMessageBox(QMessageBox::Warning, "Kein Ordner", "Bitte Datei auswählen!", QMessageBox::Ok, this).exec();
        return;
    }
    else if (tFileNames.size() > 1)
    {
        QMessageBox(QMessageBox::Warning, "Mehrere Dateien", "Bitte nur eine Datei auswählen!", QMessageBox::Ok, this).exec();
        return;
    }

    QString tFileName = tFileNames.at(0);

    saveReport(tFileName);
}

/*!
 * \brief Export the report as a PDF file.
 *
 * Asks for a file name and exports the report to this file.
 * See also exportReport().
 */
void ReportWindow::on_exportFile_action_triggered()
{
    QFileDialog fileDialog(this, "Wachbericht exportieren", "", "PDF-Dateien (*.pdf)");
    fileDialog.setDefaultSuffix("pdf");
    fileDialog.setFileMode(QFileDialog::FileMode::AnyFile);
    fileDialog.setAcceptMode(QFileDialog::AcceptMode::AcceptSave);

    QString reportFileName = report.getFileName();

    //Pre-select a PDF file name that equals the report file name except for replaced extension
    if (reportFileName != "")
    {
        QFileInfo reportFileInfo(reportFileName);
        fileDialog.selectFile(reportFileInfo.completeBaseName() + ".pdf");
    }

    if (fileDialog.exec() != QDialog::DialogCode::Accepted)
        return;

    QStringList tFileNames = fileDialog.selectedFiles();

    if (tFileNames.empty() || tFileNames.at(0) == "")
    {
        QMessageBox(QMessageBox::Warning, "Kein Ordner", "Bitte Datei auswählen!", QMessageBox::Ok, this).exec();
        return;
    }
    else if (tFileNames.size() > 1)
    {
        QMessageBox(QMessageBox::Warning, "Mehrere Dateien", "Bitte nur eine Datei auswählen!", QMessageBox::Ok, this).exec();
        return;
    }

    QString tFileName = tFileNames.at(0);

    if (!tFileName.endsWith(".pdf"))
    {
        QMessageBox(QMessageBox::Critical, "Kein PDF", "Kann nur als PDF exportieren!", QMessageBox::Ok, this).exec();
        return;
    }

    exportReport(tFileName, false); //Do not need to ask before overwrite here
}

/*!
 * \brief Load old report carryovers from a file.
 *
 * Asks for an old report file name and loads the report's carryovers from this old report (see Report::loadCarryovers()).
 * Updates the changed widget contents.
 */
void ReportWindow::on_loadCarries_action_triggered()
{
    QString tFileName = QFileDialog::getOpenFileName(this, "Letzten Wachbericht öffnen", "", "Wachberichte (*.wbr)");

    if (tFileName == "")
        return;

    Report oldReport;
    if (!oldReport.open(tFileName))
    {
        QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Laden des letzten Wachberichts!", QMessageBox::Ok, this).exec();
        return;
    }

    report.loadCarryovers(oldReport);

    setSerialNumber(report.getNumber());

    setPersonnelHoursCarry(report.getPersonnelMinutesCarry());
    setBoatHoursCarry(boatLogPtr->getBoatMinutesCarry());

    ui->engineHoursBeginOfDuty_doubleSpinBox->setValue(boatLogPtr->getEngineHoursInitial());
    ui->engineHoursEndOfDuty_doubleSpinBox->setValue(boatLogPtr->getEngineHoursFinal());
}

/*!
 * \brief Create a new report in a different window.
 *
 * Emits the signal openAnotherReportRequested() with "" as file name argument.
 *
 * See also StartupWindow::on_openAnotherReportRequested(), which may be connected to the
 * signal in order to actually create the new report and show it in a different report window.
 */
void ReportWindow::on_newReport_action_triggered()
{
    emit openAnotherReportRequested("");
}

/*!
 * \brief Open a report from a file in a different window.
 *
 * Emits the signal openAnotherReportRequested() with "" as file name argument and 'true' as the second, boolean argument.
 *
 * See also StartupWindow::on_openAnotherReportRequested(), which may be connected to the signal
 * in order to actually ask for a file name, open the report and show it in a different report window.
 */
void ReportWindow::on_openReport_action_triggered()
{
    emit openAnotherReportRequested("", true);
}

/*!
 * \brief Close the report window.
 *
 * See also closeEvent().
 */
void ReportWindow::on_close_action_triggered()
{
    close();
}

/*!
 * \brief Change the maximum PDF personnel table length.
 *
 * This value is used for PDFExporter::exportPDF() by exportReport().
 */
void ReportWindow::on_editPersonnelListSplit_action_triggered()
{
    bool ok = false;

    int value = QInputDialog::getInt(this, "Personallisten-Trennung", "Maximale Länge der Personalliste:",
                                     exportPersonnelTableMaxLength, 1, 99, 1, &ok);

    if (ok)
        exportPersonnelTableMaxLength = value;
}

//

/*!
 * \brief Synchronize the calendar widgets of every tab.
 *
 * Sets the displayed year and month of the other two widgets to \p year and \p month.
 *
 * \param year New displayed year.
 * \param month New displayed month.
 */
void ReportWindow::on_reportTab_calendarWidget_currentPageChanged(int year, int month)
{
    ui->boatTab_calendarWidget->setCurrentPage(year, month);
    ui->rescueTab_calendarWidget->setCurrentPage(year, month);
}

/*!
 * \brief Synchronize the calendar widgets of every tab.
 *
 * \copydetails on_reportTab_calendarWidget_currentPageChanged()
 */
void ReportWindow::on_boatTab_calendarWidget_currentPageChanged(int year, int month)
{
    ui->reportTab_calendarWidget->setCurrentPage(year, month);
    ui->rescueTab_calendarWidget->setCurrentPage(year, month);
}

/*!
 * \brief Synchronize the calendar widgets of every tab.
 *
 * \copydetails on_reportTab_calendarWidget_currentPageChanged()
 */
void ReportWindow::on_rescueTab_calendarWidget_currentPageChanged(int year, int month)
{
    ui->reportTab_calendarWidget->setCurrentPage(year, month);
    ui->boatTab_calendarWidget->setCurrentPage(year, month);
}

/*!
 * \brief Synchronize the calendar widgets of every tab.
 *
 * Sets the selected date of the other two widgets to this widget's selected date.
 */
void ReportWindow::on_reportTab_calendarWidget_selectionChanged()
{
    ui->boatTab_calendarWidget->setSelectedDate(ui->reportTab_calendarWidget->selectedDate());
    ui->rescueTab_calendarWidget->setSelectedDate(ui->reportTab_calendarWidget->selectedDate());
}

/*!
 * \brief Synchronize the calendar widgets of every tab.
 *
 * \copydetails on_reportTab_calendarWidget_selectionChanged()
 */
void ReportWindow::on_boatTab_calendarWidget_selectionChanged()
{
    ui->reportTab_calendarWidget->setSelectedDate(ui->boatTab_calendarWidget->selectedDate());
    ui->rescueTab_calendarWidget->setSelectedDate(ui->boatTab_calendarWidget->selectedDate());
}

/*!
 * \brief Synchronize the calendar widgets of every tab.
 *
 * \copydetails on_reportTab_calendarWidget_selectionChanged()
 */
void ReportWindow::on_rescueTab_calendarWidget_selectionChanged()
{
    ui->reportTab_calendarWidget->setSelectedDate(ui->rescueTab_calendarWidget->selectedDate());
    ui->boatTab_calendarWidget->setSelectedDate(ui->rescueTab_calendarWidget->selectedDate());
}

//

/*!
 * \brief Reduce the report serial number by one.
 *
 * Reduces the report serial number by one and resets button's check state (not auto-exclusive), if \p checked.
 * See also setSerialNumber().
 *
 * Sets setUnsavedChanges(), if \p checked.
 *
 * \param checked Button checked (i.e. pressed)?
 */
void ReportWindow::on_reportNumberDecr_radioButton_toggled(bool checked)
{
    if (checked)
    {
        ui->reportNumberDecr_radioButton->setChecked(false);
        setSerialNumber(report.getNumber()-1);
        setUnsavedChanges();
    }
}

/*!
 * \brief Increase the report serial number by one.
 *
 * Increases the report serial number by one and resets button's check state (not auto-exclusive), if \p checked.
 * See also setSerialNumber().
 *
 * Sets setUnsavedChanges(), if \p checked.
 *
 * \param checked Button checked (i.e. pressed)?
 */
void ReportWindow::on_reportNumberIncr_radioButton_toggled(bool checked)
{
    if (checked)
    {
        ui->reportNumberIncr_radioButton->setChecked(false);
        setSerialNumber(report.getNumber()+1);
        setUnsavedChanges();
    }
}

/*!
 * \brief Set the report station and update selectable radio call names.
 *
 * The selectable radio call names are updated according to the selected station's values from the database.
 * If the station is not in the database (e.g. old station), the originally loaded radio call name from the
 * report (remembered by loadReportData()) will be inserted into the combo box instead.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 Label representing the selected station.
 */
void ReportWindow::on_station_comboBox_currentTextChanged(const QString& arg1)
{
    if (arg1 != "")
        report.setStation(Aux::stationIdentFromLabel(arg1));
    else
        report.setStation("");

    setUnsavedChanges();

    //List matching radio call names for the selected station

    ui->stationRadioCallName_comboBox->clear();

    if (arg1 != "")
    {
        if (stations.find(Aux::stationIdentFromLabel(arg1)) != stations.end())
        {
            const Aux::Station& tStation = stations.at(Aux::stationIdentFromLabel(arg1));

            ui->stationRadioCallName_comboBox->insertItem(0, tStation.radioCallName);
            ui->stationRadioCallName_comboBox->insertItem(1, tStation.radioCallNameAlt);
        }
        else if (loadedStation != "")
            ui->stationRadioCallName_comboBox->insertItem(0, loadedStationRadioCallName);
        else
            return;

        ui->stationRadioCallName_comboBox->setCurrentIndex(0);
    }
}

/*!
 * \brief Set the report station radio call name.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New radio call name.
 */
void ReportWindow::on_stationRadioCallName_comboBox_currentTextChanged(const QString& arg1)
{
    report.setRadioCallName(arg1);
    setUnsavedChanges();
}

/*!
 * \brief Set the report duty purpose.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New duty purpose as label.
 */
void ReportWindow::on_dutyPurpose_comboBox_currentTextChanged(const QString& arg1)
{
    report.setDutyPurpose(Report::labelToDutyPurpose(arg1));
    setUnsavedChanges();
}

/*!
 * \brief Set the report duty purpose comment.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New duty purpose comment.
 */
void ReportWindow::on_dutyPurposeComment_lineEdit_textEdited(const QString& arg1)
{
    report.setDutyPurposeComment(arg1);
    setUnsavedChanges();
}

/*!
 * \brief Set the report date.
 *
 * Sets setUnsavedChanges().
 *
 * \param date New date.
 */
void ReportWindow::on_reportDate_dateEdit_dateChanged(const QDate& date)
{
    report.setDate(date);
    setUnsavedChanges();
}

/*!
 * \brief Set the report duty begin time.
 *
 * Sets the time edit defining a new person's begin time to the same time.
 *
 * Sets setUnsavedChanges().
 *
 * \param time New begin time.
 */
void ReportWindow::on_dutyTimesBegin_timeEdit_timeChanged(const QTime& time)
{
    report.setBeginTime(time);
    setUnsavedChanges();

    if (time.secsTo(ui->dutyTimesEnd_timeEdit->time()) <= 0)
    {
        ui->dutyTimesBegin_timeEdit->setStyleSheet("QTimeEdit { background-color: red; }");
        ui->dutyTimesEnd_timeEdit->setStyleSheet("QTimeEdit { background-color: red; }");
    }
    else
    {
        ui->dutyTimesBegin_timeEdit->setStyleSheet("");
        ui->dutyTimesEnd_timeEdit->setStyleSheet("");
    }

    //Use the new time as new default personnel arrive time
    ui->personTimeBegin_timeEdit->setTime(time);
}

/*!
 * \brief Set the report duty end time.
 *
 * Sets the time edit defining a new person's end time to the same time.
 *
 * Sets setUnsavedChanges().
 *
 * \param time New end time.
 */
void ReportWindow::on_dutyTimesEnd_timeEdit_timeChanged(const QTime& time)
{
    report.setEndTime(time);
    setUnsavedChanges();

    if (ui->dutyTimesBegin_timeEdit->time().secsTo(time) <= 0)
    {
        ui->dutyTimesBegin_timeEdit->setStyleSheet("QTimeEdit { background-color: red; }");
        ui->dutyTimesEnd_timeEdit->setStyleSheet("QTimeEdit { background-color: red; }");
    }
    else
    {
        ui->dutyTimesBegin_timeEdit->setStyleSheet("");
        ui->dutyTimesEnd_timeEdit->setStyleSheet("");
    }

    //Use the new time as new default personnel leave time
    ui->personTimeEnd_timeEdit->setTime(time);
}

/*!
 * \brief Set the report comments.
 *
 * Sets the report comments to plain text edit's plain text.
 *
 * Sets setUnsavedChanges().
 */
void ReportWindow::on_reportComments_plainTextEdit_textChanged()
{
    report.setComments(ui->reportComments_plainTextEdit->toPlainText());
    setUnsavedChanges();
}

//

/*!
 * \brief Set the report air temperature.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New temperature in degrees Celsius.
 */
void ReportWindow::on_temperatureAir_spinBox_valueChanged(int arg1)
{
    report.setAirTemperature(arg1);
    setUnsavedChanges();

    if (arg1 == 0)
        ui->temperatureAir_spinBox->setStyleSheet("QSpinBox { background-color: red; }");
    else
        ui->temperatureAir_spinBox->setStyleSheet("");
}

/*!
 * \brief Set the report water temperature.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New temperature in degrees Celsius.
 */
void ReportWindow::on_temperatureWater_spinBox_valueChanged(int arg1)
{
    report.setWaterTemperature(arg1);
    setUnsavedChanges();

    if (arg1 == 0)
        ui->temperatureWater_spinBox->setStyleSheet("QSpinBox { background-color: red; }");
    else
        ui->temperatureWater_spinBox->setStyleSheet("");
}

/*!
 * \brief Set the report precipitation type.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New precipitation type as label.
 */
void ReportWindow::on_precipitation_comboBox_currentTextChanged(const QString& arg1)
{
    report.setPrecipitation(Aux::labelToPrecipitation(arg1));
    setUnsavedChanges();
}

/*!
 * \brief Set the report level of cloudiness.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New cloudiness level as label.
 */
void ReportWindow::on_cloudiness_comboBox_currentTextChanged(const QString& arg1)
{
    report.setCloudiness(Aux::labelToCloudiness(arg1));
    setUnsavedChanges();
}

/*!
 * \brief Set the report wind strength.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New wind strength as label.
 */
void ReportWindow::on_windStrength_comboBox_currentTextChanged(const QString& arg1)
{
    report.setWindStrength(Aux::labelToWindStrength(arg1));
    setUnsavedChanges();
}

/*!
 * \brief Show a tooltip elucidating the wind strengths.
 *
 * Displays short descriptions of effects that can be observed at different wind strengths,
 * which should help determine the approximate wind strength without an anemometer.
 *
 * \param pos Position of the context menu request in the widget's coordinate system.
 */
void ReportWindow::on_windStrength_comboBox_customContextMenuRequested(const QPoint& pos)
{
    const static std::map<int, std::pair<QString, QString>> windStrengthExplanations {
        { 0, {"Keine Luftbewegung.",
                "Spiegelglatte See."}},
        { 1, {"Kaum merklich.",
                "Leichte Kräuselwellen."}},
        { 2, {"Blätter rascheln;<br>Wind im Gesicht spürbar.",
                "Kleine kurze Wellen;<br>Oberfläche glasig."}},
        { 3, {"Blätter und dünne Zweige bewegen sich; Wimpel werden gestreckt.",
                "Anfänge der Schaumbildung."}},
        { 4, {"Zweige bewegen sich; loses Papier wird vom Boden gehoben.",
                "Kleine länger werdende Wellen; regelmäßige Schaumköpfe."}},
        { 5, {"Größere Zweige und Bäume<br>bewegen sich; Wind deutlich hörbar.",
                "Mäßige lange Wellen;<br>überall Schaumköpfe."}},
        { 6, {"Dicke Äste bewegen sich;<br>hörbares Pfeifen an Drahtseilen.",
                "Größere Wellen mit brechenden Köpfen; überall weiße Schaumflecken."}},
        { 7, {"Bäume schwanken;<br>Widerstand beim Gehen.",
                "Weißer Schaum legt sich in<br>Schaumstreifen in die Windrichtung."}},
        { 8, {"Große Bäume schwanken;<br>Zweige brechen ab;<br>große Behinderung beim Gehen.",
                "Höhere Wellenberge, deren Köpfe<br>verweht werden; überall Schaumstreifen."}},
        { 9, {"Äste brechen; Ziegel heben<br>von Dächern ab; Gartenmöbel<br>werden umgeworfen.",
                "Hohe Wellen mit verwehter Gischt."}},
        {10, {"Bäume werden entwurzelt; Baumstämme brechen;<br>Gartenmöbel werden weggeweht.",
                "Sehr hohe Wellen;<br>weiße Flecken auf dem Wasser;<br>überbrechende Kämme."}},
        {11, {"Schwere Waldschäden;<br>Dächer werden abgedeckt;<br>Gehen unmöglich.",
                "Wasser wird waagerecht weggeweht;<br>starke Sichtverminderung."}},
        {12, {"Schwerste Sturmschäden<br>und Verwüstungen.",
                "See völlig weiß; Luft mit Schaum und Gischt gefüllt; keine Sicht mehr."}}
    };

    QString toolTipText = "<table cellspacing=\"5\">";
    for (const auto& it : windStrengthExplanations)
    {
        toolTipText.append("<tr><td style=\"text-align: right;\"> <span style=\"font-weight: bold;\">" +
                           QString::number(it.first) + " Bft:</span> </td><td>" +
                           it.second.first + "</td><td>" + it.second.second + "</td></tr>");
    }
    toolTipText.append("</table>");

    QToolTip::showText(ui->windStrength_comboBox->mapToGlobal(pos), toolTipText, ui->windStrength_comboBox, {}, 30000);
}

/*!
 * \brief Set the report weather comments.
 *
 * Sets setUnsavedChanges().
 *
 * Sets the report weather comments to plain text edit's plain text.
 */
void ReportWindow::on_weatherComments_plainTextEdit_textChanged()
{
    report.setWeatherComments(ui->weatherComments_plainTextEdit->toPlainText());
    setUnsavedChanges();
}

//

/*!
 * \brief Set the report number of enclosed operation protocols.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New number of operation protocols.
 */
void ReportWindow::on_operationProtocolsCtr_spinBox_valueChanged(int arg1)
{
    report.setOperationProtocolsCtr(arg1);
    setUnsavedChanges();

    ui->operationProtocols_checkBox->setChecked(arg1 > 0);
}

/*!
 * \brief Set the report number of enclosed patient records.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New number of patient records.
 */
void ReportWindow::on_patientRecordsCtr_spinBox_valueChanged(int arg1)
{
    report.setPatientRecordsCtr(arg1);
    setUnsavedChanges();

    ui->patientRecords_checkBox->setChecked(arg1 > 0);
}

/*!
 * \brief Set the report number of enclosed radio call logs.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New number of radio call logs.
 */
void ReportWindow::on_radioCallLogsCtr_spinBox_valueChanged(int arg1)
{
    report.setRadioCallLogsCtr(arg1);
    setUnsavedChanges();

    ui->radioCallLogs_checkBox->setChecked(arg1 > 0);
}

/*!
 * \brief Set the report list of other enclosures.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New list of other enclosures.
 */
void ReportWindow::on_otherEnclosures_lineEdit_textEdited(const QString& arg1)
{
    report.setOtherEnclosures(arg1);
    setUnsavedChanges();
}

/*!
 * \brief Update line edit tool tip and enclosures list label.
 *
 * \param arg1 New list of other enclosures.
 */
void ReportWindow::on_otherEnclosures_lineEdit_textChanged(const QString& arg1)
{
    QString tEnclosures;

    //Split comma-separated list
    for (const QString& tEncl : arg1.split(',', Qt::SkipEmptyParts))
    {
        if (tEncl.trimmed() == "")
            continue;

        tEnclosures.append("- " + tEncl.trimmed() + "\n");
    }

    if (tEnclosures != "")
        tEnclosures.chop(1);

    ui->otherEnclosures_label->setText(tEnclosures);

    ui->otherEnclosures_checkBox->setChecked(arg1 != "");
    ui->otherEnclosures_lineEdit->setToolTip(arg1);
}

//

/*!
 * \brief Check entered person name and update dependent widgets.
 *
 * See checkPersonInputs().
 */
void ReportWindow::on_personLastName_lineEdit_textChanged(const QString&)
{
    checkPersonInputs();
}

/*!
 * \brief Check entered person name and update dependent widgets.
 *
 * See checkPersonInputs().
 */
void ReportWindow::on_personFirstName_lineEdit_textChanged(const QString&)
{
    checkPersonInputs();
}

/*!
 * \brief Add the selected person to the report personnel list.
 *
 * See on_addPerson_pushButton_pressed().
 *
 * Does not do anything if the line edit's completer popup is still open.
 */
void ReportWindow::on_personFirstName_lineEdit_returnPressed()
{
    if (ui->addPerson_pushButton->isEnabled() && !ui->personFirstName_lineEdit->completer()->popup()->isVisible())
        on_addPerson_pushButton_pressed();
}

/*!
 * \brief Update selectable personnel functions.
 *
 * Inserts every function the selected new person is qualified for into the function combo box.
 *
 * The highest-priority function is always pre-selected, except for Person::Function::_WF
 * (only if Person::Function::_WF not already assigned) and Person::Function::_SL
 * (only if neither Person::Function::_WF nor Person::Function::_SL already assigned).
 *
 * \param arg1 Identifier of the person to be added.
 */
void ReportWindow::on_personIdent_comboBox_currentTextChanged(const QString& arg1)
{
    ui->personFunction_comboBox->clear();

    if (arg1 == "")
        return;

    /*
     * Lambda expression for adding \p pFunction and corresponding label to \p pFunctions and \p pFunctionsLabels,
     * respectively, if the qualifications required for the function are present in \p pQualis.
     * To be executed for each available 'Function'.
     */
    auto tFunc = [](Person::Function pFunction, const struct Person::Qualifications& pQualis,
                    std::set<Person::Function>& pFunctions, QStringList& pFunctionsLabels) -> void
    {
        if (QualificationChecker::checkPersonnelFunction(pFunction, pQualis))
        {
            pFunctions.insert(pFunction);
            pFunctionsLabels.append(Person::functionToLabel(pFunction));
        }
    };

    Person tPerson = Person::dummyPerson();

    DatabaseCache::getPerson(tPerson, arg1);

    Person::Qualifications tQualis = tPerson.getQualifications();

    std::set<Person::Function> availableFunctions;
    QStringList availableFunctionsLabels;

    Person::iterateFunctions(tFunc, tQualis, availableFunctions, availableFunctionsLabels);

    ui->personFunction_comboBox->insertItems(0, availableFunctionsLabels);

    //Pre-select highest-priority function that is available; however, do not pre-select '_WF' if already
    //person with '_WF' present and do not pre-select '_SL' if already person with '_SL' or '_WF' present;
    //use that Person::Function is sorted by priority and that Person::iterateFunctions() works in order of priority

    bool skipWf = personWithFunctionPresent(Person::Function::_WF);
    bool skipWfSl = skipWf || personWithFunctionPresent(Person::Function::_SL);

    Person::Function tDefaultFunction = Person::Function::_PR;

    for (Person::Function tFunction : availableFunctions)
    {
        if ((tFunction == Person::Function::_WF && skipWf) || (tFunction == Person::Function::_SL && skipWfSl))
            continue;

        tDefaultFunction = tFunction;
        break;
    }

    ui->personFunction_comboBox->setCurrentIndex(ui->personFunction_comboBox->findText(Person::functionToLabel(tDefaultFunction)));
}

/*!
 * \brief Add the selected person to the report personnel list.
 *
 * Adds the person from database that has the selected identifier to the personnel list,
 * with personnel function and begin/end times set to the current values of the corresponding widgets.
 *
 * Then updates the personnel table (see updatePersonnelTable()) and also calls updateBoatDriveAvailablePersons().
 *
 * Sets setUnsavedChanges(), if person is added.
 */
void ReportWindow::on_addPerson_pushButton_pressed()
{
    std::vector<Person> tPersons;
    DatabaseCache::getPersons(tPersons, ui->personLastName_lineEdit->text(), ui->personFirstName_lineEdit->text());

    if (tPersons.size() == 0)
        return;

    QString addedPersonIdent;

    if (tPersons.size() == 1)
    {
        addedPersonIdent = tPersons.at(0).getIdent();

        if (report.personExists(addedPersonIdent))
            return;

        report.addPerson(std::move(tPersons.at(0)), Person::labelToFunction(ui->personFunction_comboBox->currentText()),
                         ui->personTimeBegin_timeEdit->time(), ui->personTimeEnd_timeEdit->time());

        setUnsavedChanges();
    }
    else
    {
        addedPersonIdent = ui->personIdent_comboBox->currentText();

        for (Person& tPerson : tPersons)
        {
            if (tPerson.getIdent() == addedPersonIdent)
            {
                if (report.personExists(tPerson.getIdent()))
                    return;

                report.addPerson(std::move(tPerson), Person::labelToFunction(ui->personFunction_comboBox->currentText()),
                                 ui->personTimeBegin_timeEdit->time(), ui->personTimeEnd_timeEdit->time());

                setUnsavedChanges();

                break;
            }
        }
    }

    ui->personLastName_lineEdit->clear();
    ui->personFirstName_lineEdit->clear();

    ui->personLastName_lineEdit->setFocus();

    updatePersonnelTable();
    updateBoatDriveAvailablePersons();

    //Select added person
    for (int row = 0; row < ui->personnel_tableWidget->rowCount(); ++row)
        if (ui->personnel_tableWidget->item(row, 5)->text() == addedPersonIdent)
            ui->personnel_tableWidget->selectRow(row);
}

/*!
 * \brief Add an external person to the report personnel list.
 *
 * Shows PersonnelEditorDialog with name fields preset to the names entered in this window's person name fields.
 * The created external person is added to the personnel list, with begin/end times set to the current times of
 * the corresponding widgets. The personnel function and times can then be chosen via the UpdateReportPersonEntryDialog.
 * The highest-priority function (except for Person::Function::_WF and Person::Function::_SL) is pre-selected.
 *
 * Then updates the personnel table (see updatePersonnelTable()) and also calls updateBoatDriveAvailablePersons().
 *
 * Sets setUnsavedChanges(), if person is added.
 */
void ReportWindow::on_addExtPerson_pushButton_pressed()
{
    QString tLastName = ui->personLastName_lineEdit->text().trimmed();
    QString tFirstName = ui->personFirstName_lineEdit->text().trimmed();

    Person tPerson(tLastName, tFirstName, "", Person::Qualifications(""), true);

    PersonnelEditorDialog editorDialog(tPerson, true, this);

    if (editorDialog.exec() != QDialog::Accepted)
        return;

    tPerson = editorDialog.getPerson();

    //Add suffix, if identifier already exists.
    if (report.personExists(tPerson.getIdent()))
    {
        //Try 99 different suffixes (should be sufficient to find an unused identifier...)
        for (int i = 1; i < 100; ++i)
        {
            QString tIdent = Person::createExternalIdent(tPerson.getLastName(), tPerson.getFirstName(),
                                                         tPerson.getQualifications(), QString::number(i));

            if (!report.personExists(tIdent))
            {
                tPerson = Person(tPerson.getLastName(), tPerson.getFirstName(), tIdent,
                                 tPerson.getQualifications(), tPerson.getActive());
                break;
            }
        }

        //Return, if could not find any unused identifier
        if (report.personExists(tPerson.getIdent()))
        {
            QMessageBox(QMessageBox::Warning, "ID schon in Benutzung", "Kann Person nicht hinzufügen, da ID schon vorhanden!",
                        QMessageBox::Ok, this).exec();
            return;
        }
    }

    /*
     * Lambda expression for adding \p pFunction to \p pFunctions,
     * if the qualifications required for the function are present in \p pQualis.
     * To be executed for each available 'Function'.
     */
    auto tFunc = [](Person::Function pFunction, const struct Person::Qualifications& pQualis,
                    std::set<Person::Function>& pFunctions) -> void
    {
        if (QualificationChecker::checkPersonnelFunction(pFunction, pQualis))
            pFunctions.insert(pFunction);
    };

    Person::Qualifications tQualis = tPerson.getQualifications();

    std::set<Person::Function> availableFunctions;

    Person::iterateFunctions(tFunc, tQualis, availableFunctions);

    //Pre-select highest-priority function that is available, but do not pre-select '_WF' or '_SL' for external personnel;
    //use that Person::Function is sorted by priority and that Person::iterateFunctions() works in order of priority

    Person::Function tDefaultFunction = Person::Function::_PR;

    for (Person::Function tFunction : availableFunctions)
    {
        if (tFunction != Person::Function::_WF && tFunction != Person::Function::_SL)
        {
            tDefaultFunction = tFunction;
            break;
        }
    }

    UpdateReportPersonEntryDialog updateDialog(tPerson, tDefaultFunction,
                                               ui->personTimeBegin_timeEdit->time(), ui->personTimeEnd_timeEdit->time());

    if (updateDialog.exec() != QDialog::Accepted)
        return;

    QString addedPersonIdent = tPerson.getIdent();

    report.addPerson(std::move(tPerson), updateDialog.getFunction(), updateDialog.getBeginTime(), updateDialog.getEndTime());

    ui->personLastName_lineEdit->clear();
    ui->personFirstName_lineEdit->clear();

    ui->personLastName_lineEdit->setFocus();

    setUnsavedChanges();

    updatePersonnelTable();
    updateBoatDriveAvailablePersons();

    //Select added person
    for (int row = 0; row < ui->personnel_tableWidget->rowCount(); ++row)
        if (ui->personnel_tableWidget->item(row, 5)->text() == addedPersonIdent)
            ui->personnel_tableWidget->selectRow(row);
}

/*!
 * \brief Change selected persons' functions and times.
 *
 * Shows UpdateReportPersonEntryDialog for each selected person to change function and begin/end times.
 * Then updates the personnel table (see updatePersonnelTable()).
 *
 * Sets setUnsavedChanges(), if update dialog is accepted for at least one person.
 */
void ReportWindow::on_updatePerson_pushButton_pressed()
{
    for (QString tIdent : getSelectedPersons())
    {
        UpdateReportPersonEntryDialog dialog(report.getPerson(tIdent), report.getPersonFunction(tIdent),
                                             report.getPersonBeginTime(tIdent), report.getPersonEndTime(tIdent), this);

        if (dialog.exec() != QDialog::Accepted)
            continue;

        report.setPersonFunction(tIdent, dialog.getFunction());
        report.setPersonBeginTime(tIdent, dialog.getBeginTime());
        report.setPersonEndTime(tIdent, dialog.getEndTime());

        setUnsavedChanges();
    }

    updatePersonnelTable();
}

/*!
 * \brief Remove selected persons from personnel.
 *
 * Removes the persons and then updates the personnel table (see updatePersonnelTable())
 * and also calls updateBoatDriveAvailablePersons().
 *
 * If a person is still listed in a boat drive (see personInUse()), this person will not be removed.
 *
 * Sets setUnsavedChanges(), if any person was removed.
 */
void ReportWindow::on_removePerson_pushButton_pressed()
{
    for (QString tIdent : getSelectedPersons())
    {
        //Check that person not still entered as boat crew member or boatman
        if (personInUse(tIdent))
        {
            QMessageBox(QMessageBox::Warning,
                        "Person in Benutzung", "Kann Person nicht entfernen, da noch als Bootsgast oder Bootsführer "
                        "einer Fahrt eingetragen!", QMessageBox::Ok, this).exec();

            continue;
        }

        report.removePerson(tIdent);

        setUnsavedChanges();
    }

    updatePersonnelTable();
    updateBoatDriveAvailablePersons();
}

/*!
 * \brief Set selected persons' begin times to begin time edit's time.
 *
 * Sets times and updates the personnel table (see updatePersonnelTable()).
 * Does not show any dialog.
 *
 * Sets setUnsavedChanges(), if any person was selected (i.e. its time changed).
 */
void ReportWindow::on_setPersonTimeBegin_pushButton_pressed()
{
    for (QString tIdent : getSelectedPersons())
    {
        report.setPersonBeginTime(tIdent, ui->personTimeBegin_timeEdit->time());

        setUnsavedChanges();
    }

    updatePersonnelTable();
}

/*!
 * \brief Set selected persons' end times to end time edit's time.
 *
 * Sets times and updates the personnel table (see updatePersonnelTable()).
 * Does not show any dialog.
 *
 * Sets setUnsavedChanges(), if any person was selected (i.e. its time changed).
 */
void ReportWindow::on_setPersonTimeEnd_pushButton_pressed()
{
    for (QString tIdent : getSelectedPersons())
    {
        report.setPersonEndTime(tIdent, ui->personTimeEnd_timeEdit->time());

        setUnsavedChanges();
    }

    updatePersonnelTable();
}

/*!
 * \brief Set selected persons' begin times to now (nearest quarter).
 *
 * Rounds current time to next quarter and sets this time as all selected persons' begin time.
 * Then updates the personnel table (see updatePersonnelTable()).
 * Does not show any dialog.
 *
 * Sets setUnsavedChanges(), if any person was selected (i.e. its time changed).
 */
void ReportWindow::on_setPersonTimeBeginNow_pushButton_pressed()
{
    for (QString tIdent : getSelectedPersons())
    {
        report.setPersonBeginTime(tIdent, Aux::roundQuarterHour(QTime::currentTime()));

        setUnsavedChanges();
    }

    updatePersonnelTable();
}

/*!
 * \brief Set selected persons' end times to now (nearest quarter).
 *
 * Rounds current time to next quarter and sets this time as all selected persons' end time.
 * Then updates the personnel table (see updatePersonnelTable()).
 * Does not show any dialog.
 *
 * Sets setUnsavedChanges(), if any person was selected (i.e. its time changed).
 */
void ReportWindow::on_setPersonTimeEndNow_pushButton_pressed()
{
    for (QString tIdent : getSelectedPersons())
    {
        report.setPersonEndTime(tIdent, Aux::roundQuarterHour(QTime::currentTime()));

        setUnsavedChanges();
    }

    updatePersonnelTable();
}

/*!
 * \brief See on_updatePerson_pushButton_pressed().
 */
void ReportWindow::on_personnel_tableWidget_cellDoubleClicked(int, int)
{
    on_updatePerson_pushButton_pressed();
}

//

/*!
 * \brief Update total boat hours display.
 *
 * See updateTotalPersonnelHours().
 */
void ReportWindow::on_personnelHoursHours_spinBox_valueChanged(int)
{
    updateTotalPersonnelHours();
}

/*!
 * \brief Update total boat hours display.
 *
 * See updateTotalPersonnelHours().
 */
void ReportWindow::on_personnelHoursMinutes_spinBox_valueChanged(int)
{
    updateTotalPersonnelHours();
}

/*!
 * \brief Set personnel hours carry, update total hours display.
 *
 * See updateTotalPersonnelHours().
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New carry hours value.
 */
void ReportWindow::on_personnelHoursCarryHours_spinBox_valueChanged(int arg1)
{
    report.setPersonnelMinutesCarry(arg1 * 60 + ui->personnelHoursCarryMinutes_spinBox->value());
    setUnsavedChanges();

    if (arg1 == 0 && ui->personnelHoursCarryMinutes_spinBox->value() == 0)
    {
        ui->personnelHoursCarryHours_spinBox->setStyleSheet("QSpinBox { background-color: red; }");
        ui->personnelHoursCarryMinutes_spinBox->setStyleSheet("QSpinBox { background-color: red; }");
    }
    else
    {
        ui->personnelHoursCarryHours_spinBox->setStyleSheet("");
        ui->personnelHoursCarryMinutes_spinBox->setStyleSheet("");
    }

    updateTotalPersonnelHours();
}

/*!
 * \brief Set personnel hours carry, update total hours display.
 *
 * See updateTotalPersonnelHours().
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New carry minutes value.
 */
void ReportWindow::on_personnelHoursCarryMinutes_spinBox_valueChanged(int arg1)
{
    report.setPersonnelMinutesCarry(ui->personnelHoursCarryHours_spinBox->value() * 60 + arg1);
    setUnsavedChanges();

    if (ui->personnelHoursCarryHours_spinBox->value() == 0 && arg1 == 0)
    {
        ui->personnelHoursCarryHours_spinBox->setStyleSheet("QSpinBox { background-color: red; }");
        ui->personnelHoursCarryMinutes_spinBox->setStyleSheet("QSpinBox { background-color: red; }");
    }
    else
    {
        ui->personnelHoursCarryHours_spinBox->setStyleSheet("");
        ui->personnelHoursCarryMinutes_spinBox->setStyleSheet("");
    }

    updateTotalPersonnelHours();
}

//

/*!
 * \brief Set the report boat and update selectable radio call names.
 *
 * The selectable radio call names are updated according to the selected boat's values from the database.
 * If the boat is not in the database (e.g. old boat), the originally loaded radio call name from the
 * report (remembered by loadReportData()) will be inserted into the combo box instead.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 Name of the selected boat.
 */
void ReportWindow::on_boat_comboBox_currentTextChanged(const QString& arg1)
{
    boatLogPtr->setBoat(arg1);
    setUnsavedChanges();

    //List matching radio call names or the selected boat

    ui->boatRadioCallName_comboBox->clear();

    if (arg1 != "")
    {
        if (boats.find(arg1) != boats.end())
        {
            const Aux::Boat& tBoat = boats.at(arg1);

            ui->boatRadioCallName_comboBox->insertItem(0, tBoat.radioCallName);
            ui->boatRadioCallName_comboBox->insertItem(1, tBoat.radioCallNameAlt);
        }
        else if (loadedBoat != "")
            ui->boatRadioCallName_comboBox->insertItem(0, loadedBoatRadioCallName);
        else
            return;

        ui->boatRadioCallName_comboBox->setCurrentIndex(0);
    }
}

/*!
 * \brief Set the report boat radio call name.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New radio call name.
 */
void ReportWindow::on_boatRadioCallName_comboBox_currentTextChanged(const QString& arg1)
{
    boatLogPtr->setRadioCallName(arg1);
    setUnsavedChanges();
}

/*!
 * \brief Set, if boat was lowered to water at begin of duty.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 Check box check state.
 */
void ReportWindow::on_boatSlippedBeginOfDuty_checkBox_stateChanged(int arg1)
{
    boatLogPtr->setSlippedInitial(arg1 == Qt::CheckState::Checked);
    setUnsavedChanges();
}

/*!
 * \brief Set, if boat was taken out of water at end of duty.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 Check box check state.
 */
void ReportWindow::on_boatSlippedEndOfDuty_checkBox_stateChanged(int arg1)
{
    boatLogPtr->setSlippedFinal(arg1 == Qt::CheckState::Checked);
    setUnsavedChanges();
}

/*!
 * \brief Set report boat readiness time begin.
 *
 * Sets setUnsavedChanges().
 *
 * \param time New time.
 */
void ReportWindow::on_boatReadyFrom_timeEdit_timeChanged(const QTime& time)
{
    boatLogPtr->setReadyFrom(time);
    setUnsavedChanges();

    if (time == QTime(0, 0))
        ui->boatReadyFrom_timeEdit->setStyleSheet("QTimeEdit { background-color: red; }");
    else
        ui->boatReadyFrom_timeEdit->setStyleSheet("");

    if (time.secsTo(ui->boatReadyUntil_timeEdit->time()) <= 0)
        ui->boatReadyUntil_timeEdit->setStyleSheet("QTimeEdit { background-color: red; }");
    else
        ui->boatReadyUntil_timeEdit->setStyleSheet("");
}

/*!
 * \brief Set report boat readiness time end.
 *
 * Sets setUnsavedChanges().
 *
 * \param time New time.
 */
void ReportWindow::on_boatReadyUntil_timeEdit_timeChanged(const QTime& time)
{
    boatLogPtr->setReadyUntil(time);
    setUnsavedChanges();

    if (ui->boatReadyFrom_timeEdit->time().secsTo(time) <= 0)
        ui->boatReadyUntil_timeEdit->setStyleSheet("QTimeEdit { background-color: red; }");
    else
        ui->boatReadyUntil_timeEdit->setStyleSheet("");
}

//

/*!
 * \brief Set report boat comments.
 *
 * Sets setUnsavedChanges().
 *
 * Sets report boat comments to plain text edit's plain text.
 */
void ReportWindow::on_boatComments_plainTextEdit_textChanged()
{
    boatLogPtr->setComments(ui->boatComments_plainTextEdit->toPlainText());
    setUnsavedChanges();
}

//

/*!
 * \brief Set report engine hours at begin of duty.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New engine hours.
 */
void ReportWindow::on_engineHoursBeginOfDuty_doubleSpinBox_valueChanged(double arg1)
{
    boatLogPtr->setEngineHoursInitial(arg1);
    setUnsavedChanges();

    if (arg1 == 0)
        ui->engineHoursBeginOfDuty_doubleSpinBox->setStyleSheet("QDoubleSpinBox { background-color: red; }");
    else
        ui->engineHoursBeginOfDuty_doubleSpinBox->setStyleSheet("");

    if (ui->engineHoursEndOfDuty_doubleSpinBox->value() <= arg1)
        ui->engineHoursEndOfDuty_doubleSpinBox->setStyleSheet("QDoubleSpinBox { background-color: red; }");
    else
        ui->engineHoursEndOfDuty_doubleSpinBox->setStyleSheet("");

    ui->engineHoursDiff_doubleSpinBox->setValue(ui->engineHoursEndOfDuty_doubleSpinBox->value() - arg1);
}

/*!
 * \brief Set report engine hours at end of duty.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New engine hours.
 */
void ReportWindow::on_engineHoursEndOfDuty_doubleSpinBox_valueChanged(double arg1)
{
    boatLogPtr->setEngineHoursFinal(arg1);
    setUnsavedChanges();

    if (arg1 <= ui->engineHoursBeginOfDuty_doubleSpinBox->value())
        ui->engineHoursEndOfDuty_doubleSpinBox->setStyleSheet("QDoubleSpinBox { background-color: red; }");
    else
        ui->engineHoursEndOfDuty_doubleSpinBox->setStyleSheet("");

    ui->engineHoursDiff_doubleSpinBox->setValue(arg1 - ui->engineHoursBeginOfDuty_doubleSpinBox->value());
}

//

/*!
 * \brief Set report fuel added at begin of duty and update total display.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 Fuel added at begin of duty in liters.
 */
void ReportWindow::on_fuelBeginOfDuty_spinBox_valueChanged(int arg1)
{
    boatLogPtr->setFuelInitial(arg1);
    setUnsavedChanges();

    ui->fuelTotal_spinBox->setValue(arg1 + ui->fuelAfterDrives_spinBox->value() + ui->fuelEndOfDuty_spinBox->value());
}

/*!
 * \brief Update total added fuel display.
 *
 * \param arg1 Fuel added after drives in liters.
 */
void ReportWindow::on_fuelAfterDrives_spinBox_valueChanged(int arg1)
{
    if (ui->fuelEndOfDuty_spinBox->value() == 0)
    {
        if (arg1 != 0)
            ui->fuelEndOfDuty_spinBox->setStyleSheet("QSpinBox { background-color: yellow; }");
        else
            ui->fuelEndOfDuty_spinBox->setStyleSheet("QSpinBox { background-color: red; }");
    }
    else
        ui->fuelEndOfDuty_spinBox->setStyleSheet("");

    ui->fuelTotal_spinBox->setValue(ui->fuelBeginOfDuty_spinBox->value() + arg1 + ui->fuelEndOfDuty_spinBox->value());
}

/*!
 * \brief Set report fuel added at end of duty and update total display.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 Fuel added at end of duty in liters.
 */
void ReportWindow::on_fuelEndOfDuty_spinBox_valueChanged(int arg1)
{
    boatLogPtr->setFuelFinal(arg1);
    setUnsavedChanges();

    if (arg1 == 0)
    {
        if (ui->fuelAfterDrives_spinBox->value() != 0)
            ui->fuelEndOfDuty_spinBox->setStyleSheet("QSpinBox { background-color: yellow; }");
        else
            ui->fuelEndOfDuty_spinBox->setStyleSheet("QSpinBox { background-color: red; }");
    }
    else
        ui->fuelEndOfDuty_spinBox->setStyleSheet("");

    ui->fuelTotal_spinBox->setValue(ui->fuelBeginOfDuty_spinBox->value() + ui->fuelAfterDrives_spinBox->value() + arg1);
}

//

/*!
 * \brief Display data of the newly selected boat drive or revert the table selection in case there are unapplied changes.
 *
 * Displays the selected boat drive's data or clears the boat drive display, if no drive selected.
 * Note: Has to reset setUnappliedBoatDriveChanges() again after setting/changing all the boat drive widget contents.
 *
 * If, before the selection changed, there were not yet applied changes to the selected drive,
 * the user is asked whether to discard these changes or to skip selecting the new drive (i.e. revert the selection again).
 *
 * If the selection shall be reverted, the selected row is simply set to \p previousRow. For this purpose
 * the table widget's signals will be temporarily blocked since that would otherwise trigger this slot again.
 *
 * \param currentRow Number of newly selected drive (or -1).
 * \param previousRow Number of previously selected drive (or -1).
 */
void ReportWindow::on_boatDrives_tableWidget_currentCellChanged(int currentRow, int, int previousRow, int)
{
    if (unappliedBoatDriveChanges)
    {
        if (SettingsCache::getBoolSetting("app_reportWindow_autoApplyBoatDriveChanges"))
            applyBoatDriveChanges(previousRow);
        else
        {
            //Ask before discarding unapplied changes of previously selected drive

            QMessageBox msgBox(QMessageBox::Question, "Nicht übernommene Änderungen",
                               "Nicht übernommene Änderungen in ausgewählter Bootsfahrt.\nVerwerfen?",
                               QMessageBox::Abort | QMessageBox::Yes, this);

            if (msgBox.exec() != QMessageBox::Yes)
            {
                //Set previous selection again; temporarily block signals to prevent repeated call of this slot

                ui->boatDrives_tableWidget->blockSignals(true);
                ui->boatDrives_tableWidget->setCurrentCell(previousRow, 0);
                ui->boatDrives_tableWidget->blockSignals(false);

                return;
            }
        }
    }

    if (currentRow == -1)
    {
        ui->boatDrive_scrollArea->setEnabled(false);

        ui->boatDrivePurpose_comboBox->setCurrentText("");
        ui->boatDriveBegin_timeEdit->setTime(QTime(0, 0));
        ui->boatDriveEnd_timeEdit->setTime(QTime(0, 0));
        ui->boatDriveFuel_spinBox->setValue(0);
        ui->boatDriveBoatman_comboBox->setCurrentIndex(-1);
        ui->boatDriveComments_plainTextEdit->setPlainText(0);
        ui->boatCrew_tableWidget->setRowCount(0);
    }
    else
    {
        ui->boatDrive_scrollArea->setEnabled(true);

        const BoatDrive& tDrive = boatLogPtr->getDrive(currentRow);

        ui->boatDrivePurpose_comboBox->setCurrentText(tDrive.getPurpose());
        ui->boatDriveBegin_timeEdit->setTime(tDrive.getBeginTime());
        ui->boatDriveEnd_timeEdit->setTime(tDrive.getEndTime());
        ui->boatDriveFuel_spinBox->setValue(tDrive.getFuel());
        ui->boatDriveComments_plainTextEdit->setPlainText(tDrive.getComments());

        //Clear table already here because setting boatman could conflict with previous contents of (i.e. persons in) table
        ui->boatCrew_tableWidget->setRowCount(0);

        if (tDrive.getBoatman() == "")
            ui->boatDriveBoatman_comboBox->setCurrentIndex(-1);
        else
        {
            ui->boatDriveBoatman_comboBox->setCurrentIndex(ui->boatDriveBoatman_comboBox->findText(
                                                               personLabelFromIdent(tDrive.getBoatman())));
        }

        //Define lambda to compare/sort crew members by name, then identifier
        auto cmp = [](const Person& pPersonA, const Person& pPersonB) -> bool
        {
            if (QString::localeAwareCompare(pPersonA.getLastName(), pPersonB.getLastName()) < 0)
                return true;
            else if (QString::localeAwareCompare(pPersonA.getLastName(), pPersonB.getLastName()) > 0)
                return false;
            else
            {
                if (QString::localeAwareCompare(pPersonA.getFirstName(), pPersonB.getFirstName()) < 0)
                    return true;
                else if (QString::localeAwareCompare(pPersonA.getFirstName(), pPersonB.getFirstName()) > 0)
                    return false;
                else
                {
                    if (QString::localeAwareCompare(pPersonA.getIdent(), pPersonB.getIdent()) < 0)
                        return true;
                    else
                        return false;
                }
            }
        };

        //Get boat drive crew

        std::map<QString, Person::BoatFunction> tCrew = tDrive.crew();

        std::vector<Person> tCrewPersons;
        for (const auto& it : tCrew)
            tCrewPersons.push_back(report.getPerson(it.first));

        //Use temporary set to sort persons using above custom sort lambda

        std::set<std::reference_wrapper<const Person>, decltype(cmp)> tCrewPersonsSorted(cmp);

        for (const Person& tPerson : tCrewPersons)
            tCrewPersonsSorted.insert(std::cref(tPerson));

        for (const Person& tPerson : tCrewPersonsSorted)
            insertBoatCrewTableRow(tPerson, tCrew.at(tPerson.getIdent()));
    }

    on_boatDriveEnd_timeEdit_timeChanged(ui->boatDriveEnd_timeEdit->time());
    on_boatDriveBoatman_comboBox_currentTextChanged(ui->boatDriveBoatman_comboBox->currentText());

    setUnappliedBoatDriveChanges(false);
}

/*!
 * \brief Add new boat drive to report (after selected one).
 *
 * If a drive is selected, the new drive will be inserted after this one.
 *
 * Updates the boat drives table (see updateBoatDrivesTable()) and selects the new drive.
 *
 * If there are not applied changes to the currently selected drive, the user is asked whether to discard these changes.
 * If the changes shall be kept, this function returns immediately (does nothing). Resets setUnappliedBoatDriveChanges() else.
 *
 * Sets setUnsavedChanges(), if drive was added.
 */
void ReportWindow::on_addBoatDrive_pushButton_pressed()
{
    if (unappliedBoatDriveChanges)
    {
        if (SettingsCache::getBoolSetting("app_reportWindow_autoApplyBoatDriveChanges"))
            on_applyBoatDriveChanges_pushButton_pressed();
        else
        {
            QMessageBox msgBox(QMessageBox::Question, "Nicht übernommene Änderungen",
                               "Nicht übernommene Änderungen in ausgewählter Bootsfahrt.\nVerwerfen?",
                               QMessageBox::Abort | QMessageBox::Yes, this);

            if (msgBox.exec() != QMessageBox::Yes)
                return;
        }
    }

    BoatDrive tDrive;

    if (Aux::boatDrivePurposePresets.size() > 0)
        tDrive.setPurpose(Aux::boatDrivePurposePresets.at(0));

    tDrive.setBeginTime(Aux::roundQuarterHour(QTime::currentTime()));
    tDrive.setEndTime(tDrive.getBeginTime());

    //Insert new drive at the end
    int tNewRowNumber = boatLogPtr->getDrivesCount();

    boatLogPtr->addDrive(tNewRowNumber, std::move(tDrive));

    setUnappliedBoatDriveChanges(false);
    setUnsavedChanges();

    updateBoatDrivesTable();

    ui->boatDrives_tableWidget->selectRow(tNewRowNumber);
}

/*!
 * \brief Remove the selected boat drive from the report.
 *
 * Resets setUnappliedBoatDriveChanges().
 *
 * Sets setUnsavedChanges(), if drive was removed.
 *
 * Then updates the boat drives table (see updateBoatDrivesTable()).
 */
void ReportWindow::on_removeBoatDrive_pushButton_pressed()
{
    if (ui->boatDrives_tableWidget->currentRow() == -1)
        return;

    boatLogPtr->removeDrive(ui->boatDrives_tableWidget->currentRow());

    setUnappliedBoatDriveChanges(false);
    setUnsavedChanges();

    updateBoatDrivesTable();
}

/*!
 * \brief Change the order of selected drive and drive above.
 *
 * Then updates the boat drives table (see updateBoatDrivesTable()) and selects the currently selected drive again.
 *
 * If there are not applied changes to the currently selected drive, the user is asked whether to discard these changes.
 * If the changes shall be kept, this function returns immediately (does nothing). Resets setUnappliedBoatDriveChanges() else.
 *
 * Sets setUnsavedChanges(), if drives were swapped.
 */
void ReportWindow::on_moveBoatDriveUp_pushButton_pressed()
{
    int tSelectedRow = ui->boatDrives_tableWidget->currentRow();

    if (tSelectedRow == -1)
        return;

    if (tSelectedRow < 1)
        return;

    if (unappliedBoatDriveChanges)
    {
        if (SettingsCache::getBoolSetting("app_reportWindow_autoApplyBoatDriveChanges"))
            on_applyBoatDriveChanges_pushButton_pressed();
        else
        {
            QMessageBox msgBox(QMessageBox::Question, "Nicht übernommene Änderungen",
                               "Nicht übernommene Änderungen in ausgewählter Bootsfahrt.\nVerwerfen?",
                               QMessageBox::Abort | QMessageBox::Yes, this);

            if (msgBox.exec() != QMessageBox::Yes)
                return;
        }
    }

    boatLogPtr->swapDrives(tSelectedRow, tSelectedRow-1);

    setUnappliedBoatDriveChanges(false);
    setUnsavedChanges();

    updateBoatDrivesTable();

    ui->boatDrives_tableWidget->selectRow(tSelectedRow-1);
}

/*!
 * \brief Change the order of selected drive and drive below.
 *
 * Then updates the boat drives table (see updateBoatDrivesTable()) and selects the currently selected drive again.
 *
 * If there are not applied changes to the currently selected drive, the user is asked whether to discard these changes.
 * If the changes shall be kept, this function returns immediately (does nothing). Resets setUnappliedBoatDriveChanges() else.
 *
 * Sets setUnsavedChanges(), if drives were swapped.
 */
void ReportWindow::on_moveBoatDriveDown_pushButton_pressed()
{
    int tSelectedRow = ui->boatDrives_tableWidget->currentRow();

    if (tSelectedRow == -1)
        return;

    if (tSelectedRow >= boatLogPtr->getDrivesCount()-1)
        return;

    if (unappliedBoatDriveChanges)
    {
        if (SettingsCache::getBoolSetting("app_reportWindow_autoApplyBoatDriveChanges"))
            on_applyBoatDriveChanges_pushButton_pressed();
        else
        {
            QMessageBox msgBox(QMessageBox::Question, "Nicht übernommene Änderungen",
                               "Nicht übernommene Änderungen in ausgewählter Bootsfahrt.\nVerwerfen?",
                               QMessageBox::Abort | QMessageBox::Yes, this);

            if (msgBox.exec() != QMessageBox::Yes)
                return;
        }
    }

    boatLogPtr->swapDrives(tSelectedRow, tSelectedRow+1);

    setUnappliedBoatDriveChanges(false);
    setUnsavedChanges();

    updateBoatDrivesTable();

    ui->boatDrives_tableWidget->selectRow(tSelectedRow+1);
}

/*!
 * \brief Set displayed boat drive begin time to now (nearest quarter).
 *
 * Sets the boat drive begin time edit's time to the current time rounded to the nearest quarter.
 */
void ReportWindow::on_setBoatDriveTimeBeginNow_pushButton_pressed()
{
    if (ui->boatDrives_tableWidget->currentRow() == -1)
        return;

    ui->boatDriveBegin_timeEdit->setTime(Aux::roundQuarterHour(QTime::currentTime()));
}

/*!
 * \brief Set displayed boat drive end time to now (nearest quarter).
 *
 * Sets the boat drive end time edit's time to the current time rounded to the nearest quarter.
 */
void ReportWindow::on_setBoatDriveTimeEndNow_pushButton_pressed()
{
    if (ui->boatDrives_tableWidget->currentRow() == -1)
        return;

    ui->boatDriveEnd_timeEdit->setTime(Aux::roundQuarterHour(QTime::currentTime()));
}

/*!
 * \brief Split drive into two drives at current time (nearest quarter).
 *
 * Sets the end time of the currently selected drive to the current time rounded to the nearest quarter,
 * copies the drive and inserts the copy after this one. The new drive's end time is set to its begin time,
 * the amount of added fuel and comments are reset. The purpose, boatman and crew are kept.
 *
 * Then updates the boat drives table (see updateBoatDrivesTable()) and selects the new drive.
 *
 * If there are not applied changes to the currently selected drive, the user is asked whether to discard these changes.
 * If the changes shall be kept, this function returns immediately (does nothing). Resets setUnappliedBoatDriveChanges() else.
 *
 * Sets setUnsavedChanges(), if drive was added.
 */
void ReportWindow::on_splitBoatDrive_pushButton_pressed()
{
    int tSelectedRow = ui->boatDrives_tableWidget->currentRow();

    if (tSelectedRow == -1)
        return;

    if (unappliedBoatDriveChanges)
    {
        if (SettingsCache::getBoolSetting("app_reportWindow_autoApplyBoatDriveChanges"))
            on_applyBoatDriveChanges_pushButton_pressed();
        else
        {
            QMessageBox msgBox(QMessageBox::Question, "Nicht übernommene Änderungen",
                               "Nicht übernommene Änderungen in ausgewählter Bootsfahrt.\nVerwerfen?",
                               QMessageBox::Abort | QMessageBox::Yes, this);

            if (msgBox.exec() != QMessageBox::Yes)
                return;
        }
    }

    BoatDrive& oldDrive = boatLogPtr->getDrive(tSelectedRow);

    //Ask whether to set end time of to be continued drive and begin time of new drive to current time (keep old end time otherwise)

    QMessageBox msgBox(QMessageBox::Question, "Zeit ändern?",
                       "Ende der alten Fahrt und Anfang der neuen Fahrt auf aktuelle Uhrzeit setzen?",
                       QMessageBox::Yes | QMessageBox::No, this);

    if (msgBox.exec() == QMessageBox::Yes)
        oldDrive.setEndTime(Aux::roundQuarterHour(QTime::currentTime()));

    //Copy the old drive, but change times and reset comments and fuel value

    BoatDrive newDrive = oldDrive;

    newDrive.setBeginTime(newDrive.getEndTime());
    newDrive.setFuel(0);
    newDrive.setComments("");

    //Insert new drive after old drive
    boatLogPtr->addDrive(tSelectedRow+1, std::move(newDrive));

    setUnappliedBoatDriveChanges(false);
    setUnsavedChanges();

    updateBoatDrivesTable();

    ui->boatDrives_tableWidget->selectRow(tSelectedRow+1);
}

/*!
 * \brief Apply changed boat drive data to selected boat drive in report.
 *
 * Applies the drive data to the drive corresponding to the currently selected boat drives table row.
 * See applyBoatDriveChanges().
 */
void ReportWindow::on_applyBoatDriveChanges_pushButton_pressed()
{
    applyBoatDriveChanges(ui->boatDrives_tableWidget->currentRow());
}

/*!
 * \brief Discard changed boat drive data and reload selected drive data.
 *
 * Resets setUnappliedBoatDriveChanges().
 */
void ReportWindow::on_discardBoatDriveChanges_pushButton_pressed()
{
    if (ui->boatDrives_tableWidget->currentRow() == -1)
        return;

    setUnappliedBoatDriveChanges(false);

    on_boatDrives_tableWidget_currentCellChanged(ui->boatDrives_tableWidget->currentRow(), 0,
                                                 ui->boatDrives_tableWidget->currentRow(), 0);
}

//

/*!
 * \brief Set unapplied boat drive changes.
 *
 * Sets setUnappliedBoatDriveChanges(), if a boat drive selected.
 */
void ReportWindow::on_boatDrivePurpose_comboBox_currentTextChanged(const QString&)
{
    if (ui->boatDrives_tableWidget->currentRow() == -1)
        return;

    setUnappliedBoatDriveChanges();
}

/*!
 * \brief Set unapplied boat drive changes.
 *
 * Sets setUnappliedBoatDriveChanges(), if a boat drive selected.
 *
 * \param time New boat drive begin time.
 */
void ReportWindow::on_boatDriveBegin_timeEdit_timeChanged(const QTime& time)
{
    if (ui->boatDrives_tableWidget->currentRow() == -1)
        return;

    setUnappliedBoatDriveChanges();

    if (time.secsTo(ui->boatDriveEnd_timeEdit->time()) <= 0)
        ui->boatDriveEnd_timeEdit->setStyleSheet("QTimeEdit { background-color: red; }");
    else
        ui->boatDriveEnd_timeEdit->setStyleSheet("");
}

/*!
 * \brief Set unapplied boat drive changes.
 *
 * Sets setUnappliedBoatDriveChanges(), if a boat drive selected.
 *
 * \param time New boat drive end time.
 */
void ReportWindow::on_boatDriveEnd_timeEdit_timeChanged(const QTime& time)
{
    if (ui->boatDrives_tableWidget->currentRow() == -1)
    {
        ui->boatDriveEnd_timeEdit->setStyleSheet("");
        return;
    }

    setUnappliedBoatDriveChanges();

    if (ui->boatDriveBegin_timeEdit->time().secsTo(time) <= 0)
        ui->boatDriveEnd_timeEdit->setStyleSheet("QTimeEdit { background-color: red; }");
    else
        ui->boatDriveEnd_timeEdit->setStyleSheet("");
}

/*!
 * \brief Set unapplied boat drive changes.
 *
 * Sets setUnappliedBoatDriveChanges(), if a boat drive selected.
 */
void ReportWindow::on_boatDriveFuel_spinBox_valueChanged(int)
{
    if (ui->boatDrives_tableWidget->currentRow() == -1)
        return;

    setUnappliedBoatDriveChanges();
}

/*!
 * \brief Check boatman, set unapplied boat drive changes.
 *
 * Checks, if new selected boatman is not already listed in displayed crew member table.
 * If not, selected combo box item is reset to none.
 *
 * Sets setUnappliedBoatDriveChanges(), if a boat drive selected and boatman accepted.
 *
 * Note: Also remembers new boatman identifer(!) such that updateBoatDriveAvailablePersons() can re-select proper boatman
 * (since label format can change, if persons added/removed).
 *
 * \param arg1 Person label of new boatman.
 */
void ReportWindow::on_boatDriveBoatman_comboBox_currentTextChanged(const QString& arg1)
{
    //Remember selected person's identifier in case labels change on update (e.g. ambiguous person added)
    if (arg1 != "")
        selectedBoatmanIdent = personIdentFromLabel(arg1);
    else
        selectedBoatmanIdent = "";

    if (ui->boatDrives_tableWidget->currentRow() == -1)
    {
        ui->boatDriveBoatman_comboBox->setPalette(QApplication::style()->standardPalette());
        return;
    }

    //Ensure that new boatman is not already crew member and reset otherwise
    if (arg1 != "")
    {
        QString tIdent = personIdentFromLabel(arg1);

        for (int row = 0; row < ui->boatCrew_tableWidget->rowCount(); ++row)
        {
            if (ui->boatCrew_tableWidget->item(row, 3)->text() == tIdent)
            {
                QMessageBox(QMessageBox::Warning, "Person schon Bootsgast", "Person ist schon als Bootsgast eingetragen!\n"
                                                  "Setze zurück auf keinen Bootsführer.", QMessageBox::Ok, this).exec();

                ui->boatDriveBoatman_comboBox->setCurrentIndex(-1);

                goto _SetBoatmanColorRed;
            }
        }

        ui->boatDriveBoatman_comboBox->setPalette(QApplication::style()->standardPalette());
    }
    else
    {
        _SetBoatmanColorRed:
        QPalette palette = ui->boatDriveBoatman_comboBox->palette();
        palette.setColor(QPalette::Button, Qt::red);
        ui->boatDriveBoatman_comboBox->setPalette(palette);
    }

    setUnappliedBoatDriveChanges();
}

/*!
 * \brief Set unapplied boat drive changes.
 *
 * Sets setUnappliedBoatDriveChanges(), if a boat drive selected.
 */
void ReportWindow::on_boatDriveComments_plainTextEdit_textChanged()
{
    if (ui->boatDrives_tableWidget->currentRow() == -1)
        return;

    setUnappliedBoatDriveChanges();
}

/*!
 * \brief Update selectable boat functions according to selected new crew member person's qualifications.
 *
 * \param arg1 Person label of potential new crew member.
 */
void ReportWindow::on_boatCrewMember_comboBox_currentTextChanged(const QString& arg1)
{
    ui->boatCrewMemberFunction_comboBox->clear();

    if (arg1 == "")
        return;

    /*
     * Lambda expression for adding \p pBoatFunction label to \p pFunctions,
     * if the qualifications required for the boat function are present in \p pQualis.
     * To be executed for each available 'BoatFunction'.
     */
    auto tFunc = [](Person::BoatFunction pBoatFunction, const struct Person::Qualifications& pQualis, QStringList& pFunctions) -> void
    {
        if (QualificationChecker::checkBoatFunction(pBoatFunction, pQualis))
            pFunctions.append(Person::boatFunctionToLabel(pBoatFunction));
    };

    Person tPerson = report.getPerson(personIdentFromLabel(arg1));

    Person::Qualifications tQualis = tPerson.getQualifications();

    QStringList availableFunctions;
    Person::iterateBoatFunctions(tFunc, tQualis, availableFunctions);

    ui->boatCrewMemberFunction_comboBox->insertItems(0, availableFunctions);
}

/*!
 * \brief Add crew member to displayed drive's crew member table.
 *
 * Add selected potential crew member person to the displayed crew member table
 * (with selected boat function), if person not already in this table or selected as boatman.
 *
 * Sets setUnappliedBoatDriveChanges(), if a boat drive selected and person was added.
 */
void ReportWindow::on_addBoatCrewMember_pushButton_pressed()
{
    if (ui->boatDrives_tableWidget->currentRow() == -1)
        return;

    if (ui->boatCrewMember_comboBox->currentIndex() == -1 || ui->boatCrewMemberFunction_comboBox->currentIndex() == -1)
        return;

    QString tLabel = ui->boatCrewMember_comboBox->currentText();
    QString tIdent = personIdentFromLabel(tLabel);

    //Ensure that new crew member is not already set as boatman
    if (tLabel == ui->boatDriveBoatman_comboBox->currentText())
    {
        QMessageBox(QMessageBox::Warning, "Person schon Bootsführer", "Person ist schon als Bootsführer eingetragen!",
                    QMessageBox::Ok, this).exec();
        return;
    }

    //Ensure that new crew member is not already crew member
    for (int row = 0; row < ui->boatCrew_tableWidget->rowCount(); ++row)
    {
        if (ui->boatCrew_tableWidget->item(row, 3)->text() == tIdent)
        {
            QMessageBox(QMessageBox::Warning, "Person schon Bootsgast", "Person ist schon als Bootsgast eingetragen!",
                        QMessageBox::Ok, this).exec();
            return;
        }
    }

    insertBoatCrewTableRow(report.getPerson(tIdent), Person::labelToBoatFunction(ui->boatCrewMemberFunction_comboBox->currentText()));

    setUnappliedBoatDriveChanges();
}

/*!
 * \brief Remove selected crew member from displayed drive's crew member table.
 *
 * Removes crew member selected in displayed crew member table from this table.
 *
 * Sets setUnappliedBoatDriveChanges(), if a person was removed.
 */
void ReportWindow::on_removeBoatCrewMember_pushButton_pressed()
{
    if (ui->boatDrives_tableWidget->currentRow() == -1)
        return;

    if (ui->boatCrew_tableWidget->currentRow() == -1)
        return;

    ui->boatCrew_tableWidget->removeRow(ui->boatCrew_tableWidget->currentRow());

    setUnappliedBoatDriveChanges();
}

//

/*!
 * \brief Update total boat hours display.
 *
 * See updateTotalBoatHours().
 */
void ReportWindow::on_boatHoursHours_spinBox_valueChanged(int)
{
    updateTotalBoatHours();
}

/*!
 * \brief Update total boat hours display.
 *
 * See updateTotalBoatHours().
 */
void ReportWindow::on_boatHoursMinutes_spinBox_valueChanged(int)
{
    updateTotalBoatHours();
}

/*!
 * \brief Set report boat hours carry and update total boat hours display.
 *
 * See also updateTotalBoatHours().
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New carry hours value.
 */
void ReportWindow::on_boatHoursCarryHours_spinBox_valueChanged(int arg1)
{
    boatLogPtr->setBoatMinutesCarry(arg1 * 60 + ui->boatHoursCarryMinutes_spinBox->value());
    setUnsavedChanges();

    if (arg1 == 0 && ui->boatHoursCarryMinutes_spinBox->value() == 0)
    {
        ui->boatHoursCarryHours_spinBox->setStyleSheet("QSpinBox { background-color: red; }");
        ui->boatHoursCarryMinutes_spinBox->setStyleSheet("QSpinBox { background-color: red; }");
    }
    else
    {
        ui->boatHoursCarryHours_spinBox->setStyleSheet("");
        ui->boatHoursCarryMinutes_spinBox->setStyleSheet("");
    }

    updateTotalBoatHours();
}

/*!
 * \brief Set report boat hours carry and update total boat hours display.
 *
 * See also updateTotalBoatHours().
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New carry minutes value.
 */
void ReportWindow::on_boatHoursCarryMinutes_spinBox_valueChanged(int arg1)
{
    boatLogPtr->setBoatMinutesCarry(ui->boatHoursCarryHours_spinBox->value() * 60 + arg1);
    setUnsavedChanges();

    if (ui->boatHoursCarryHours_spinBox->value() == 0 && arg1 == 0)
    {
        ui->boatHoursCarryHours_spinBox->setStyleSheet("QSpinBox { background-color: red; }");
        ui->boatHoursCarryMinutes_spinBox->setStyleSheet("QSpinBox { background-color: red; }");
    }
    else
    {
        ui->boatHoursCarryHours_spinBox->setStyleSheet("");
        ui->boatHoursCarryMinutes_spinBox->setStyleSheet("");
    }

    updateTotalBoatHours();
}

//

/*!
 * \brief Set report assignment number.
 *
 * Sets setUnsavedChanges().
 *
 * \param arg1 New assignment number.
 */
void ReportWindow::on_assignmentNumber_lineEdit_textEdited(const QString& arg1)
{
    report.setAssignmentNumber(arg1);
    setUnsavedChanges();
}
