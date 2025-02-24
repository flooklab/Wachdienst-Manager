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

#include "newreportdialog.h"
#include "ui_newreportdialog.h"

#include "boatlog.h"
#include "databasecache.h"
#include "settingscache.h"

#include <QCalendarWidget>
#include <QComboBox>
#include <QDate>
#include <QFileDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QShortcut>
#include <QStackedWidget>
#include <Qt>
#include <QTime>
#include <QTimeEdit>

#include <utility>

/*!
 * \brief Constructor.
 *
 * Creates the dialog.
 *
 * \param pParent The parent widget.
 */
NewReportDialog::NewReportDialog(QWidget *const pParent) :
    QDialog(pParent, Qt::WindowTitleHint |
                     Qt::WindowSystemMenuHint |
                     Qt::WindowCloseButtonHint),
    ui(new Ui::NewReportDialog),
    report()
{
    ui->setupUi(this);

    connectSlots();

    ui->stackedWidget->setCurrentIndex(0);

    //Set progress bar text
    at_stackedWidget_currentChanged(0);

    //Avoid automatic focusing of "previous" button in order that return key triggers "next" button
    ui->previous_pushButton->setFocusPolicy(Qt::NoFocus);

    /*
     * Lambda expression for adding a combo box item for \p pPurpose to \p pComboBox.
     * To be executed for each available 'DutyPurpose'.
     */
    auto tFunc = [](Report::DutyPurpose pPurpose, QComboBox *const pComboBox) -> void {
        pComboBox->insertItem(pComboBox->count(), Report::dutyPurposeToLabel(pPurpose));
    };

    //Add a combo box item for each duty purpose
    Report::iterateDutyPurposes(tFunc, ui->dutyPurpose_comboBox);

    //Load available stations and boats from database cache

    int defaultStationRowId = SettingsCache::getIntSetting("app_default_station");
    int defaultBoatRowId = SettingsCache::getIntSetting("app_default_boat");

    //Use station identifier instead of 'rowid' as key
    QString tDefaultStationIdent;
    for (auto it : DatabaseCache::stations())
    {
        QString tStationIdent;
        Aux::stationIdentFromNameLocation(it.second.name, it.second.location, tStationIdent);

        //Default station exists?
        if (it.first == defaultStationRowId)
            tDefaultStationIdent = tStationIdent;

        stations.insert({std::move(tStationIdent), std::move(it.second)});
    }

    //Use boat name instead of 'rowid' as key

    QString tDefaultBoatName;

    if (!SettingsCache::getBoolSetting("app_boatLog_disabled"))
    {
        for (const auto& it : DatabaseCache::boats())
        {
            //Default boat exists?
            if (it.first == defaultBoatRowId)
                tDefaultBoatName = it.second.name;

            boats.insert({it.second.name, it.second});
        }
    }

    //Add stations and boats to to combo boxes

    for (const auto& it : stations)
        ui->station_comboBox->insertItem(ui->station_comboBox->count(), Aux::stationLabelFromIdent(it.first));

    for (const auto& it : boats)
        ui->boat_comboBox->insertItem(ui->boat_comboBox->count(), it.first);

    //Set default values

    ui->dutyTimesBegin_timeEdit->setTime(QTime::fromString(SettingsCache::getStrSetting("app_default_dutyTimeBegin"), "hh:mm"));
    ui->dutyTimesEnd_timeEdit->setTime(QTime::fromString(SettingsCache::getStrSetting("app_default_dutyTimeEnd"), "hh:mm"));

    if (SettingsCache::getBoolSetting("app_newReport_forceSelectDutyPurpose"))
        ui->dutyPurpose_comboBox->setCurrentIndex(-1);
    else
    {
        ui->dutyPurpose_comboBox->setCurrentIndex(ui->dutyPurpose_comboBox->findText(
                                                      Report::dutyPurposeToLabel(Report::DutyPurpose::Watchkeeping)));
    }

    if (tDefaultStationIdent == "")
        ui->station_comboBox->setCurrentIndex(ui->station_comboBox->count() > 0 ? 0 : -1);
    else
        ui->station_comboBox->setCurrentIndex(ui->station_comboBox->findText(Aux::stationLabelFromIdent(tDefaultStationIdent)));

    if (tDefaultBoatName == "")
    {
        ui->boat_comboBox->setCurrentIndex(ui->boat_comboBox->count() > 0 ? 0 : -1);

        //Try to select the boat by matching the boats' home stations
        if (ui->station_comboBox->currentText() != "")
        {
            QString tSelectedStationIdent = Aux::stationIdentFromLabel(ui->station_comboBox->currentText());

            for (const auto& it : boats)
            {
                if (it.second.homeStation == tSelectedStationIdent)
                {
                    ui->boat_comboBox->setCurrentIndex(ui->boat_comboBox->findText(it.second.name));
                    break;
                }
            }
        }
    }
    else
        ui->boat_comboBox->setCurrentIndex(ui->boat_comboBox->findText(tDefaultBoatName));

    //Update enabled state of navigation buttons
    updatePreviousNextEnabled();

    //Add navigation shortcuts

    QShortcut* previousShortcut = new QShortcut(QKeySequence("Alt+Left"), this);
    connect(previousShortcut, &QShortcut::activated, this, &NewReportDialog::at_previous_pushButton_released);

    QShortcut* nextShortcut = new QShortcut(QKeySequence("Alt+Right"), this);
    connect(nextShortcut, &QShortcut::activated, this, &NewReportDialog::at_next_pushButton_released);
}

/*!
 * \brief Destructor.
 */
NewReportDialog::~NewReportDialog()
{
    delete ui;
}

//Public

/*!
 * \brief Get the new report.
 *
 * Moves the internal new report instance to \p pReport.
 *
 * \param pReport The report to be overwritten.
 */
void NewReportDialog::getReport(Report& pReport) const
{
    pReport = std::move(report);
}

//Private

/*!
 * \brief Connect UI signals to their slots.
 *
 * Connects required signals from the ".ui" file widgets to their corresponding slots.
 */
void NewReportDialog::connectSlots() const
{
    connect(ui->stackedWidget, &QStackedWidget::currentChanged, this, &NewReportDialog::at_stackedWidget_currentChanged);
    connect(ui->previous_pushButton, &QPushButton::released, this, &NewReportDialog::at_previous_pushButton_released);
    connect(ui->next_pushButton, &QPushButton::released, this, &NewReportDialog::at_next_pushButton_released);
    connect(ui->dutyPurpose_comboBox, &QComboBox::currentTextChanged,
            this, &NewReportDialog::at_dutyPurpose_comboBox_currentTextChanged);
    connect(ui->station_comboBox, &QComboBox::currentTextChanged, this, &NewReportDialog::at_station_comboBox_currentTextChanged);
    connect(ui->boat_comboBox, &QComboBox::currentTextChanged, this, &NewReportDialog::at_boat_comboBox_currentTextChanged);
    connect(ui->clearStation_radioButton, &QRadioButton::toggled, this, &NewReportDialog::at_clearStation_radioButton_toggled);
    connect(ui->clearBoat_radioButton, &QRadioButton::toggled, this, &NewReportDialog::at_clearBoat_radioButton_toggled);
    connect(ui->noLoadLastReportCarries_radioButton, &QRadioButton::toggled,
            this, &NewReportDialog::at_noLoadLastReportCarries_radioButton_toggled);
    connect(ui->loadLastReportCarries_radioButton, &QRadioButton::toggled,
            this, &NewReportDialog::at_loadLastReportCarries_radioButton_toggled);
    connect(ui->loadLastReportCarries_radioButton, &QRadioButton::pressed,
            this, &NewReportDialog::at_loadLastReportCarries_radioButton_pressed);
}

//

/*!
 * \brief Update enabled state of the navigation buttons.
 *
 * If the page for setting the duty purpose (etc.) is selected:
 * - If \e no duty purpose is currently selected the navigation button for the \e next page gets disabled.
 * - If a duty purpose \e is selected the button gets enabled.
 *
 * Both navigation buttons (previous/next) get enabled for any other page.
 */
void NewReportDialog::updatePreviousNextEnabled()
{
    if (ui->stackedWidget->currentIndex() == 1)
    {
        if (ui->dutyPurpose_comboBox->currentIndex() == -1)
            ui->next_pushButton->setEnabled(false);
        else
            ui->next_pushButton->setEnabled(true);
    }
    else
    {
        ui->previous_pushButton->setEnabled(true);
        ui->next_pushButton->setEnabled(true);
    }
}

//Private slots

/*!
 * \brief Reimplementation of QDialog::accept().
 *
 * Reimplements QDialog::accept().
 *
 * Applies the adjusted configuration to the internal new report instance
 * and also loads the carryovers from the user specified last report.
 *
 * Returns without accepting, if loading the last report fails.
 */
void NewReportDialog::accept()
{
    //Apply widget contents to report before accepting

    report.setBeginTime(ui->dutyTimesBegin_timeEdit->time());
    report.setEndTime(ui->dutyTimesEnd_timeEdit->time());

    report.setDate(ui->reportDate_calendarWidget->selectedDate());

    report.setDutyPurpose(Report::labelToDutyPurpose(ui->dutyPurpose_comboBox->currentText()));
    report.setDutyPurposeComment(ui->dutyPurposeComment_lineEdit->text());

    if (ui->station_comboBox->currentIndex() != -1)
        report.setStation(Aux::stationIdentFromLabel(ui->station_comboBox->currentText()));

    report.setRadioCallName(ui->stationRadioCallName_comboBox->currentText());

    report.boatLog()->setBoat(ui->boat_comboBox->currentText());
    report.boatLog()->setRadioCallName(ui->boatRadioCallName_comboBox->currentText());

    if (ui->loadLastReportCarries_radioButton->isChecked())
    {
        Report oldReport;
        if (!oldReport.open(ui->lastReportFilename_label->text()))
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Laden des letzten Wachberichts!", QMessageBox::Ok, this).exec();
            return;
        }
        report.loadCarryovers(oldReport);
    }
    else
    {
        QMessageBox(QMessageBox::Warning, "Warnung", "Kein letzter Wachbericht angegeben! Es wurden noch keine Überträge geladen.",
                    QMessageBox::Ok, this).exec();
    }

    QDialog::accept();
}

//

/*!
 * \brief Update progress bar value/label and navigation button labels.
 *
 * Sets progress bar percentage and label according to selected page (\p arg1).
 * Changes 'previous button' label to "Abort", iff on first page, and 'next button' label to "Finish", iff on last page.
 * Calls updatePreviousNextEnabled() to updates enabled state of 'previous/next' buttons according to filled values on current page.
 *
 * When the page for setting the duty purpose (etc.) was selected and no duty purpose is currently selected the selection popup
 * for the duty purpose combo box gets opened automatically (the default selection is normally Report::DutyPurpose::Watchkeeping
 * but is initially cleared if manual selection is enforced by the "app_newReport_forceSelectDutyPurpose" setting).
 *
 * When the page for loading the last report's carryovers was selected and no such report is currently
 * selected (which is initially the case) the corresponding radio button for selection of the last
 * report gets triggered automatically (see at_loadLastReportCarries_radioButton_pressed()).
 *
 * \param arg1 New page index.
 */
void NewReportDialog::at_stackedWidget_currentChanged(const int arg1)
{
    //Update progress
    ui->progressBar->setValue(static_cast<int>(100 * static_cast<double>(arg1) / ui->stackedWidget->count()));

    //Update page label/header for progress bar
    if (ui->stackedWidget->currentIndex() == 0)
        ui->progressBar->setFormat("%p% - Zeiten und Datum");
    else if (ui->stackedWidget->currentIndex() == 1)
        ui->progressBar->setFormat("%p% - Weitere Eckdaten");
    else
        ui->progressBar->setFormat("%p% - Letzter Wachbericht");

    //Update navigation button labels

    if (ui->stackedWidget->currentIndex() == ui->stackedWidget->count()-1)
        ui->next_pushButton->setText("Fertig!");
    else
        ui->next_pushButton->setText("Weiter");

    if (ui->stackedWidget->currentIndex() == 0)
        ui->previous_pushButton->setText("Abbrechen");
    else
        ui->previous_pushButton->setText("Zurück");

    //Update enabled state of navigation buttons
    updatePreviousNextEnabled();

    //Automatically open the popup for the duty purpose combo box whenever the
    //corresponding page gets selected but no duty purpose has been selected yet
    if (ui->stackedWidget->currentIndex() == 1 && ui->dutyPurpose_comboBox->currentIndex() == -1)
        ui->dutyPurpose_comboBox->showPopup();

    //Automatically trigger the selection of a file name to load last report's carryovers from
    //whenever the corresponding page gets selected and no such report has been selected yet
    if (ui->stackedWidget->currentIndex() == ui->stackedWidget->count()-1 && !ui->loadLastReportCarries_radioButton->isChecked())
        ui->loadLastReportCarries_radioButton->click();
}

/*!
 * \brief Go to previous page or reject the dialog, if on first page.
 */
void NewReportDialog::at_previous_pushButton_released()
{
    if (ui->stackedWidget->currentIndex() == 0)
        reject();
    else
        ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex()-1);
}

/*!
 * \brief Go to next page or accept the dialog, if on last page.
 */
void NewReportDialog::at_next_pushButton_released()
{
    if (ui->stackedWidget->currentIndex() == ui->stackedWidget->count()-1)
        accept();
    else
        ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex()+1);
}

//

/*!
 * \brief Enable navigation buttons if a duty purpose selected.
 *
 * See updatePreviousNextEnabled().
 */
void NewReportDialog::at_dutyPurpose_comboBox_currentTextChanged(const QString&)
{
    //Update enabled state of navigation buttons
    updatePreviousNextEnabled();
}

/*!
 * \brief Update selectable radio call names from selected station.
 *
 * Changes the selectable station radio call name combo box items to those radio call names defined for the selected station.
 * All items will be cleared, if no station is selected.
 *
 * \param arg1 Selected combo box label representing a station.
 */
void NewReportDialog::at_station_comboBox_currentTextChanged(const QString& arg1)
{
    ui->stationRadioCallName_comboBox->clear();

    if (arg1 == "")
        return;

    const Aux::Station& tStation = stations.at(Aux::stationIdentFromLabel(arg1));

    ui->stationRadioCallName_comboBox->insertItem(ui->stationRadioCallName_comboBox->count(), tStation.radioCallName);
    ui->stationRadioCallName_comboBox->insertItem(ui->stationRadioCallName_comboBox->count(), tStation.radioCallNameAlt);
    ui->stationRadioCallName_comboBox->setCurrentIndex(0);
}

/*!
 * \brief Update selectable radio call names from selected boat.
 *
 * Changes the selectable boat radio call name combo box items to those radio call names defined for the selected boat.
 * All items will be cleared, if no boat is selected.
 *
 * \param arg1 Selected combo box label representing a boat.
 */
void NewReportDialog::at_boat_comboBox_currentTextChanged(const QString& arg1)
{
    ui->boatRadioCallName_comboBox->clear();

    if (arg1 == "")
        return;

    const Aux::Boat& tBoat = boats.at(arg1);

    ui->boatRadioCallName_comboBox->insertItem(ui->boatRadioCallName_comboBox->count(), tBoat.radioCallName);
    ui->boatRadioCallName_comboBox->insertItem(ui->boatRadioCallName_comboBox->count(), tBoat.radioCallNameAlt);
    ui->boatRadioCallName_comboBox->setCurrentIndex(0);
}

/*!
 * \brief Clear station selection (and reset the radio button).
 *
 * If \p checked, then select no station and reset radio button check state (button not auto-exclusive).
 *
 * \param checked Button checked (i.e. pressed)?
 */
void NewReportDialog::at_clearStation_radioButton_toggled(const bool checked)
{
    if (!checked)
        return;

    ui->station_comboBox->setCurrentIndex(-1);
    ui->clearStation_radioButton->setChecked(false);
}

/*!
 * \brief Clear boat selection (and reset the radio button).
 *
 * If \p checked, then select no boat and reset radio button check state (button not auto-exclusive).
 *
 * \param checked Button checked (i.e. pressed)?
 */
void NewReportDialog::at_clearBoat_radioButton_toggled(const bool checked)
{
    if (!checked)
        return;

    ui->boat_comboBox->setCurrentIndex(-1);
    ui->clearBoat_radioButton->setChecked(false);
}

//

/*!
 * \brief Reset file name selected/displayed for last report.
 *
 * If \p checked, reset displayed last report file name, i.e. do not load last report on dialog accept.
 *
 * \param checked Button checked (i.e. pressed)?
 */
void NewReportDialog::at_noLoadLastReportCarries_radioButton_toggled(const bool checked)
{
    if (!checked)
        return;

    ui->lastReportFilename_label->setText("");
}

/*!
 * \brief Select a file name to load last report's carryovers from.
 *
 * If \p checked, show file dialog to select last report and set the displayed last report file name,
 * i.e. load last report on dialog accept with displayed file name.
 *
 * \param checked Button checked (i.e. pressed)?
 */
void NewReportDialog::at_loadLastReportCarries_radioButton_toggled(const bool checked)
{
    if (!checked)
        return;

    QString tFileName = QFileDialog::getOpenFileName(this, "[Überträge laden] Letzten Wachbericht öffnen", "", "Wachberichte (*.wbr)");

    if (tFileName == "")
    {
        //If no file selected but also no previously selected file set, reset this radio button
        if (ui->lastReportFilename_label->text() == "")
            ui->noLoadLastReportCarries_radioButton->toggle();

        return;
    }

    ui->lastReportFilename_label->setText(tFileName);
}

/*!
 * \brief See at_loadLastReportCarries_radioButton_toggled().
 *
 * Calls at_loadLastReportCarries_radioButton_toggled() with current check state as argument.
 */
void NewReportDialog::at_loadLastReportCarries_radioButton_pressed()
{
    //Also trigger the slot, when pressed while already checked
    at_loadLastReportCarries_radioButton_toggled(ui->loadLastReportCarries_radioButton->isChecked());
}
