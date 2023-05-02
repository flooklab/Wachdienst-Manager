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

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include "databasecache.h"
#include "settingscache.h"

#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QHeaderView>
#include <QLayout>
#include <QLineEdit>
#include <QList>
#include <QMargins>
#include <QMessageBox>
#include <QRadioButton>
#include <QRect>
#include <QRegularExpressionValidator>
#include <QSpinBox>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QTimeEdit>

#include <chrono>
#include <iostream>
#include <thread>

/*!
 * \brief Constructor.
 *
 * Creates the dialog.
 *
 * Loads the settings database values.
 * Sets input validators, formats table headers.
 *
 * Asks for password (if set) and checks if the database is writeable.
 * Disables the "Ok" button, if password wrong or database read-only.
 *
 * \param pParent The parent widget.
 */
SettingsDialog::SettingsDialog(QWidget *const pParent) :
    QDialog(pParent, Qt::WindowTitleHint |
                     Qt::WindowSystemMenuHint |
                     Qt::WindowCloseButtonHint),
    ui(new Ui::SettingsDialog),
    acceptDisabled(false),
    passwordEdited(false)
{
    ui->setupUi(this);

    //Add example fuel types
    ui->boatFuelType_comboBox->addItems(Aux::boatFuelTypePresets);

    //Format documents table header
    ui->documents_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->documents_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    //Set validators

    ui->stationLocation_lineEdit->setValidator(new QRegularExpressionValidator(Aux::locationsValidator.regularExpression(),
                                                                               ui->stationLocation_lineEdit));

    ui->stationName_lineEdit->setValidator(new QRegularExpressionValidator(Aux::namesValidator.regularExpression(),
                                                                           ui->stationName_lineEdit));

    ui->localGroup_lineEdit->setValidator(new QRegularExpressionValidator(Aux::namesValidator.regularExpression(),
                                                                          ui->localGroup_lineEdit));

    ui->districtAssociation_lineEdit->setValidator(new QRegularExpressionValidator(Aux::namesValidator.regularExpression(),
                                                                                   ui->districtAssociation_lineEdit));

    ui->stationRadioCallName_lineEdit->setValidator(new QRegularExpressionValidator(Aux::radioCallNamesValidator.regularExpression(),
                                                                                    ui->stationRadioCallName_lineEdit));

    ui->stationRadioCallNameAlt_lineEdit->setValidator(new QRegularExpressionValidator(Aux::radioCallNamesValidator.regularExpression()
                                                                                       , ui->stationRadioCallNameAlt_lineEdit));

    ui->boatName_lineEdit->setValidator(new QRegularExpressionValidator(Aux::namesValidator.regularExpression(),
                                                                        ui->boatName_lineEdit));

    ui->boatAcronym_lineEdit->setValidator(new QRegularExpressionValidator(Aux::boatAcronymsValidator.regularExpression(),
                                                                           ui->boatAcronym_lineEdit));

    ui->boatType_lineEdit->setValidator(new QRegularExpressionValidator(Aux::namesValidator.regularExpression(),
                                                                        ui->boatType_lineEdit));

    ui->boatFuelType_comboBox->setValidator(new QRegularExpressionValidator(Aux::fuelTypesValidator.regularExpression(),
                                                                            ui->boatFuelType_comboBox));

    ui->boatRadioCallName_lineEdit->setValidator(new QRegularExpressionValidator(Aux::radioCallNamesValidator.regularExpression(),
                                                                                 ui->boatRadioCallName_lineEdit));

    ui->boatRadioCallNameAlt_lineEdit->setValidator(new QRegularExpressionValidator(Aux::radioCallNamesValidator.regularExpression(),
                                                                                    ui->boatRadioCallNameAlt_lineEdit));

    //Ask for password

    QString hash = SettingsCache::getStrSetting("app_auth_hash");
    QString salt = SettingsCache::getStrSetting("app_auth_salt");

    //Note: this is not intended to be secure...
    if (hash != "" && salt != "")
    {
        _CheckPassword:
        if (!Aux::checkPassword(hash, salt, pParent))
        {
            QMessageBox msgBox(QMessageBox::Critical, "Fehler", "Falsches Passwort!", QMessageBox::Abort | QMessageBox::Retry, pParent);
            msgBox.setDefaultButton(QMessageBox::Retry);

            if (msgBox.exec() == QMessageBox::Retry)
            {
                //Retry after a second
                std::this_thread::sleep_for(std::chrono::seconds(1));
                goto _CheckPassword;
            }

            //No writing to database
            acceptDisabled = true;
        }
    }

    //Check if database is writeable
    if (DatabaseCache::isReadOnly())
    {
        acceptDisabled = true;

        QMessageBox(QMessageBox::Warning, "Warnung", "Datenbank ist nur lesbar,\nda das Programm mehrfach geöffnet ist!",
                    QMessageBox::Ok, pParent).exec();
    }

    //Disable "Ok" button, if read-only or wrong password
    if (acceptDisabled)
    {
        QList<QAbstractButton*> tButtons = ui->buttonBox->buttons();
        for (int i = 0; i < tButtons.length(); ++i)
            tButtons.at(i)->setEnabled(false);
        ui->buttonBox->button(QDialogButtonBox::StandardButton::Cancel)->setEnabled(true);
    }

    //Load settings from database and fill input widgets
    readDatabase();
}

/*!
 * \brief Destructor.
 */
SettingsDialog::~SettingsDialog()
{
    delete ui;
}

//Private

/*!
 * \brief Read the settings from database.
 *
 * Reads all settings from the database cache and fills the dialog widgets.
 */
void SettingsDialog::readDatabase()
{
    //General settings

    int defaultStationRowId = SettingsCache::getIntSetting("app_default_station");  //Checks further below
    int defaultBoatRowId = SettingsCache::getIntSetting("app_default_boat");        //Checks further below

    ui->defaultDutyTimesBegin_timeEdit->setTime(QTime::fromString(SettingsCache::getStrSetting("app_default_dutyTimeBegin"), "hh:mm"));
    ui->defaultDutyTimesEnd_timeEdit->setTime(QTime::fromString(SettingsCache::getStrSetting("app_default_dutyTimeEnd"), "hh:mm"));

    ui->defaultFilePath_lineEdit->setText(SettingsCache::getStrSetting("app_default_fileDialogDir"));

    ui->xelatexPath_lineEdit->setText(SettingsCache::getStrSetting("app_export_xelatexPath"));
    ui->logoPath_lineEdit->setText(SettingsCache::getStrSetting("app_export_customLogoPath"));
    ui->font_lineEdit->setText(SettingsCache::getStrSetting("app_export_fontFamily"));

    ui->autoExport_checkBox->setChecked(SettingsCache::getBoolSetting("app_export_autoOnSave"));
    ui->autoExportAskFilename_checkBox->setChecked(SettingsCache::getBoolSetting("app_export_autoOnSave_askForFileName"));
    ui->twoSidedPrint_checkBox->setChecked(SettingsCache::getBoolSetting("app_export_twoSidedPrint"));

    //Extended settings

    ui->disableBoatLog_checkBox->setChecked(SettingsCache::getBoolSetting("app_boatLog_disabled"));
    ui->boatDriveAutoApplyChanges_checkBox->setChecked(SettingsCache::getBoolSetting("app_reportWindow_autoApplyBoatDriveChanges"));

    QString boatmanRequiredLicense = SettingsCache::getStrSetting("app_personnel_minQualis_boatman");

    ui->boatingLicenseA_radioButton->setChecked(true);
    if (boatmanRequiredLicense == "B")
        ui->boatingLicenseB_radioButton->setChecked(true);
    else if (boatmanRequiredLicense == "A&B")
        ui->boatingLicenseAB_radioButton->setChecked(true);
    else if (boatmanRequiredLicense == "A|B")
        ui->boatingLicenseAny_radioButton->setChecked(true);

    ui->singleInstance_checkBox->setChecked(SettingsCache::getBoolSetting("app_singleInstance"));

    //Password

    QString hash = SettingsCache::getStrSetting("app_auth_hash");
    QString salt = SettingsCache::getStrSetting("app_auth_salt");

    //Set some string (EchoMode::Password) to indicate that password is set
    if (hash != "" && salt != "")
        ui->password_lineEdit->setText("password");

    //Stations and boats

    stations.clear();
    boats.clear();

    //Use station identifier instead of 'rowid' as key
    bool tDefaultStationOk = false;
    for (auto it : DatabaseCache::stations())
    {
        QString tStationIdent;
        Aux::stationIdentFromNameLocation(it.second.name, it.second.location, tStationIdent);

        stations.insert({std::move(tStationIdent), std::move(it.second)});

        //Default station exists?
        if (it.first == defaultStationRowId)
            tDefaultStationOk = true;
    }

    //Use boat name instead of 'rowid' as key
    bool tDefaultBoatOk = false;
    for (const auto& it : DatabaseCache::boats())
    {
        boats.insert({it.second.name, std::move(it.second)});

        //Default boat exists?
        if (it.first == defaultBoatRowId)
            tDefaultBoatOk = true;
    }

    //Reset default station/boat, if does not exist
    if (!tDefaultStationOk)
        defaultStationRowId = -1;
    if (!tDefaultBoatOk)
        defaultBoatRowId = -1;

    //Load stations and boats into combo boxes
    updateStationsBoatsComboBoxes();

    //Add stations to default station selection
    ui->defaultStation_comboBox->clear();
    for (const auto& it : stations)
    {
        QString label;
        Aux::stationLabelFromNameLocation(it.second.name, it.second.location, label);
        ui->defaultStation_comboBox->insertItem(ui->defaultStation_comboBox->count(), label);
    }
    ui->defaultStation_comboBox->setCurrentIndex(-1);

    //Add boats to default boat selection
    ui->defaultBoat_comboBox->clear();
    for (const auto& it : boats)
        ui->defaultBoat_comboBox->insertItem(ui->defaultBoat_comboBox->count(), it.second.name);
    ui->defaultBoat_comboBox->setCurrentIndex(-1);

    //Select default station
    if (defaultStationRowId >= 0)
    {
        QString tStName, tStLoc, tComboLabel;
        DatabaseCache::stationNameLocationFromRowId(defaultStationRowId, tStName, tStLoc);
        Aux::stationLabelFromNameLocation(tStName, tStLoc, tComboLabel);

        ui->defaultStation_comboBox->setCurrentIndex(ui->defaultStation_comboBox->findText(tComboLabel));
    }

    //Select default boat
    if (defaultBoatRowId >= 0)
    {
        QString tBtName;
        DatabaseCache::boatNameFromRowId(defaultBoatRowId, tBtName);

        ui->defaultBoat_comboBox->setCurrentIndex(ui->defaultBoat_comboBox->findText(tBtName));
    }

    //Force update of station/boat properties widgets, if stations/boats are empty
    if (stations.empty())
        updateStationsInputs();
    if (boats.empty())
        updateBoatsInputs();

    //Important document shortcuts

    std::vector<std::pair<QString, QString>> tDocs = Aux::parseDocumentListString(
                                                         SettingsCache::getStrSetting("app_documentLinks_documentList"));

    ui->documents_tableWidget->setRowCount(0);
    ui->documents_tableWidget->setRowCount(tDocs.size());

    //Add documents to table widget
    for (int row = 0; row < static_cast<int>(tDocs.size()); ++row)
    {
        const QString& tDocName = tDocs.at(row).first;
        const QString& tDocFile = tDocs.at(row).second;

        ui->documents_tableWidget->setItem(row, 0, new QTableWidgetItem(tDocName));
        ui->documents_tableWidget->setItem(row, 1, new QTableWidgetItem(tDocFile));
    }

    ui->numDocuments_spinBox->setValue(tDocs.size());
}

/*!
 * \brief Write the settings to database.
 *
 * Writes all settings to the database (cache).
 *
 * Returns immediately, if database read-only.
 *
 * \return If all write operations were successful.
 */
bool SettingsDialog::writeDatabase() const
{
    if (DatabaseCache::isReadOnly())
        return false;

    //General settings

    //Write default station, if stations map keys have not changed
    if (ui->defaultStation_comboBox->isEnabled())
    {
        int tDefaultStRowId = -1;
        if (ui->defaultStation_comboBox->currentIndex() != -1)
        {
            const Aux::Station& tDefaultStation = stations.at(Aux::stationIdentFromLabel(ui->defaultStation_comboBox->currentText()));
            DatabaseCache::stationRowIdFromNameLocation(tDefaultStation.name, tDefaultStation.location, tDefaultStRowId);
        }

        if (!SettingsCache::setIntSetting("app_default_station", tDefaultStRowId))
            return false;
    }

    //Write default boat, if boats map keys have not changed
    if (ui->defaultBoat_comboBox->isEnabled())
    {
        int tDefaultBtRowId = -1;
        if (ui->defaultBoat_comboBox->currentIndex() != -1)
        {
            const Aux::Boat& tDefaultBoat = boats.at(ui->defaultBoat_comboBox->currentText());
            DatabaseCache::boatRowIdFromName(tDefaultBoat.name, tDefaultBtRowId);
        }

        if (!SettingsCache::setIntSetting("app_default_boat", tDefaultBtRowId))
            return false;
    }

    if (!SettingsCache::setStrSetting("app_default_dutyTimeBegin", ui->defaultDutyTimesBegin_timeEdit->time().toString("hh:mm")))
        return false;

    if (!SettingsCache::setStrSetting("app_default_dutyTimeEnd", ui->defaultDutyTimesEnd_timeEdit->time().toString("hh:mm")))
        return false;

    if (!SettingsCache::setStrSetting("app_default_fileDialogDir", ui->defaultFilePath_lineEdit->text()))
        return false;

    if (!SettingsCache::setStrSetting("app_export_xelatexPath", ui->xelatexPath_lineEdit->text()))
        return false;

    if (!SettingsCache::setStrSetting("app_export_customLogoPath", ui->logoPath_lineEdit->text()))
        return false;

    if (!SettingsCache::setStrSetting("app_export_fontFamily", ui->font_lineEdit->text()))
        return false;

    if (!SettingsCache::setBoolSetting("app_export_autoOnSave", ui->autoExport_checkBox->isChecked()))
        return false;

    if (!SettingsCache::setBoolSetting("app_export_autoOnSave_askForFileName", ui->autoExportAskFilename_checkBox->isChecked()))
        return false;

    if (!SettingsCache::setBoolSetting("app_export_twoSidedPrint", ui->twoSidedPrint_checkBox->isChecked()))
        return false;

    //Extended settings

    if (!SettingsCache::setBoolSetting("app_boatLog_disabled", ui->disableBoatLog_checkBox->isChecked()))
        return false;

    if (!SettingsCache::setBoolSetting("app_reportWindow_autoApplyBoatDriveChanges",
                                       ui->boatDriveAutoApplyChanges_checkBox->isChecked()))
    {
        return false;
    }

    if (ui->boatingLicenseA_radioButton->isChecked())
        SettingsCache::setStrSetting("app_personnel_minQualis_boatman", "A");
    else if (ui->boatingLicenseB_radioButton->isChecked())
        SettingsCache::setStrSetting("app_personnel_minQualis_boatman", "B");
    else if (ui->boatingLicenseAB_radioButton->isChecked())
        SettingsCache::setStrSetting("app_personnel_minQualis_boatman", "A&B");
    else if (ui->boatingLicenseAny_radioButton->isChecked())
        SettingsCache::setStrSetting("app_personnel_minQualis_boatman", "A|B");

    if (!SettingsCache::setBoolSetting("app_singleInstance", ui->singleInstance_checkBox->isChecked()))
        return false;

    //Password

    if (passwordEdited)
    {
        QString phrase = ui->password_lineEdit->text();

        if (phrase == "")
        {
            //Reset password
            if (!SettingsCache::setStrSetting("app_auth_hash", "") || !SettingsCache::setStrSetting("app_auth_salt", ""))
                return false;
        }
        else
        {
            //Create new hash
            QString newHash, newSalt;
            Aux::generatePasswordHash(phrase, newHash, newSalt);

            if (!SettingsCache::setStrSetting("app_auth_hash", newHash) || !SettingsCache::setStrSetting("app_auth_salt", newSalt))
                return false;
        }
    }

    //Stations and boats

    std::vector<Aux::Station> tStations;
    for (const auto& it : stations)
        tStations.push_back(it.second);

    DatabaseCache::updateStations(tStations);

    std::vector<Aux::Boat> tBoats;
    for (const auto& it : boats)
        tBoats.push_back(it.second);

    DatabaseCache::updateBoats(tBoats);

    //Important document shortcuts

    std::vector<std::pair<QString, QString>> tDocs;

    //Gather documents information from table widget
    for (int row = 0; row < static_cast<int>(ui->documents_tableWidget->rowCount()); ++row)
    {
        QString tDocName = ui->documents_tableWidget->item(row, 0)->text();
        QString tDocFile = ui->documents_tableWidget->item(row, 1)->text();

        tDocs.push_back({std::move(tDocName), std::move(tDocFile)});
    }

    if (!SettingsCache::setStrSetting("app_documentLinks_documentList", Aux::createDocumentListString(tDocs)))
        return false;

    return true;
}

//

/*!
 * \brief Update the entries of station/boat combo boxes.
 *
 * Clears and refills all station and boat combo box items in the "stations and boats" tab
 * from the stations and boats listed in the dialog-internal stations and boats maps.
 */
void SettingsDialog::updateStationsBoatsComboBoxes()
{
    //Remember selections
    int currSelStations = ui->stations_comboBox->currentIndex();
    int currSelBoats = ui->boats_comboBox->currentIndex();

    ui->boats_comboBox->clear();
    ui->stations_comboBox->clear();
    ui->boatHomeStation_comboBox->clear();

    for (const auto& it : stations)
    {
        QString label;
        Aux::stationLabelFromNameLocation(it.second.name, it.second.location, label);

        ui->stations_comboBox->insertItem(ui->stations_comboBox->count(), label);
        ui->boatHomeStation_comboBox->insertItem(ui->boatHomeStation_comboBox->count(), label);
    }

    for (const auto& it : boats)
        ui->boats_comboBox->insertItem(ui->boats_comboBox->count(), it.second.name);

    //Try to restore selections

    if (currSelStations >= ui->stations_comboBox->count())
        currSelStations = ui->stations_comboBox->count()-1;
    if (currSelBoats >= ui->boats_comboBox->count())
        currSelBoats = ui->boats_comboBox->count()-1;

    //Show first item by default
    if (currSelStations == -1 && ui->stations_comboBox->count() > 0)
        currSelStations = 0;
    if (currSelBoats == -1 && ui->boats_comboBox->count() > 0)
        currSelBoats = 0;

    ui->stations_comboBox->setCurrentIndex(currSelStations);
    ui->boats_comboBox->setCurrentIndex(currSelBoats);
}

/*!
 * \brief Update the station inputs according to the selected station combo box entry.
 *
 * Fills the station input widgets with the properties of the currently selected station.
 */
void SettingsDialog::updateStationsInputs()
{
    if (ui->stations_comboBox->currentText() != "")
    {
        const Aux::Station& tStation = stations.at(Aux::stationIdentFromLabel(ui->stations_comboBox->currentText()));

        ui->stationLocation_lineEdit->setText(tStation.location);
        ui->stationName_lineEdit->setText(tStation.name);
        ui->localGroup_lineEdit->setText(tStation.localGroup);
        ui->districtAssociation_lineEdit->setText(tStation.districtAssociation);
        ui->stationRadioCallName_lineEdit->setText(tStation.radioCallName);
        ui->stationRadioCallNameAlt_lineEdit->setText(tStation.radioCallNameAlt);
    }
    else
    {
        ui->stationLocation_lineEdit->setText("");
        ui->stationName_lineEdit->setText("");
        ui->localGroup_lineEdit->setText("");
        ui->districtAssociation_lineEdit->setText("");
        ui->stationRadioCallName_lineEdit->setText("");
        ui->stationRadioCallNameAlt_lineEdit->setText("");
    }
}

/*!
 * \brief Update the boat inputs according to the selected boat combo box entry.
 *
 * Fills the boat input widgets with the properties of the currently selected boat.
 */
void SettingsDialog::updateBoatsInputs()
{
    if (ui->boats_comboBox->currentText() != "")
    {
        const Aux::Boat& tBoat = boats.at(ui->boats_comboBox->currentText());

        ui->boatName_lineEdit->setText(tBoat.name);
        ui->boatAcronym_lineEdit->setText(tBoat.acronym);
        ui->boatType_lineEdit->setText(tBoat.type);
        if (ui->boatFuelType_comboBox->findText(tBoat.fuelType) != -1)
            ui->boatFuelType_comboBox->setCurrentIndex(ui->boatFuelType_comboBox->findText(tBoat.fuelType));
        else
            ui->boatFuelType_comboBox->setCurrentText(tBoat.fuelType);
        ui->boatRadioCallName_lineEdit->setText(tBoat.radioCallName);
        ui->boatRadioCallNameAlt_lineEdit->setText(tBoat.radioCallNameAlt);

        ui->boatHomeStation_comboBox->setCurrentIndex(ui->boatHomeStation_comboBox->findText(
                                                          Aux::stationLabelFromIdent(tBoat.homeStation)));
        ui->boatHomeStation_comboBox->setEnabled(true);
    }
    else
    {
        ui->boatName_lineEdit->setText("");
        ui->boatAcronym_lineEdit->setText("");
        ui->boatType_lineEdit->setText("");
        ui->boatFuelType_comboBox->setCurrentText("");
        ui->boatRadioCallName_lineEdit->setText("");
        ui->boatRadioCallNameAlt_lineEdit->setText("");

        ui->boatHomeStation_comboBox->setCurrentIndex(-1);
        ui->boatHomeStation_comboBox->setEnabled(false);
    }
}

/*!
 * \brief Fit the documents table height to contents.
 *
 * Shrinks the documents table height to the minimum height required to display all rows.
 * If this height exceeds the available space, the height is adjusted to the surrounding group box height.
 */
void SettingsDialog::resizeDocumentsTable()
{
    int maxHeight = ui->documents_tableWidget->contentsMargins().top() + ui->documents_tableWidget->contentsMargins().bottom() +
                    ui->documents_tableWidget->horizontalHeader()->height();

    for (int i = 0; i < ui->documents_tableWidget->rowCount(); ++i)
        maxHeight += ui->documents_tableWidget->rowHeight(i);

    int maxMaxHeight = ui->documents_groupBox->contentsRect().height() -
                       (ui->documentsControls_horizontalLayout->sizeHint().height() +
                        ui->documentsGroupBox_line->sizeHint().height() +
                        3*ui->documents_groupBox->layout()->spacing() + 12);

    if (maxMaxHeight < 0)
        maxMaxHeight = 0;

    if (maxHeight > maxMaxHeight)
        maxHeight = maxMaxHeight;

    ui->documents_tableWidget->setMinimumHeight(maxHeight);
    ui->documents_tableWidget->setMaximumHeight(maxHeight);
}

//

/*!
 * \brief Replace a station's changed identifier (key).
 *
 * Removes the station with identifier (map key) \p pOldIdent and adds \p pNewStation
 * (same station with changed name or location i.e. changed identifier).
 *
 * Replaces the home station of all boats with home station identifier \p pOldIdent with the \p pNewStation.
 *
 * Calls updateStationsBoatsComboBoxes() to update the stations combo box items.
 *
 * Changes stations map keys and hence also calls disableDefaultStationSelection().
 *
 * \param pOldIdent Identifier of station to be removed.
 * \param pNewStation Station to be added.
 */
void SettingsDialog::changeStationIdent(const QString& pOldIdent, Aux::Station&& pNewStation)
{
    //Replace station
    QString ident;
    Aux::stationIdentFromNameLocation(pNewStation.name, pNewStation.location, ident);
    stations.erase(pOldIdent);
    stations[ident] = std::move(pNewStation);

    //Search for boats with home station equal to 'pOldIdent' and replace the home station
    for (auto& it : boats)
        if (it.second.homeStation == pOldIdent)
            it.second.homeStation = ident;

    updateStationsBoatsComboBoxes();

    disableDefaultStationSelection();
}

/*!
 * \brief Replace a boat's changed name (key).
 *
 * Removes the boat with name (map key) \p pOldName and adds \p pNewBoat (same boat with changed name).
 *
 * Calls updateStationsBoatsComboBoxes() to update the boats combo box items.
 *
 * Changes boat map keys and hence also calls disableDefaultBoatSelection().
 *
 * \param pOldName Name of boat to be removed.
 * \param pNewBoat Boat to be added.
 */
void SettingsDialog::changeBoatName(const QString& pOldName, Aux::Boat&& pNewBoat)
{
    //Replace boat
    boats.erase(pOldName);
    boats[pNewBoat.name] = std::move(pNewBoat);

    updateStationsBoatsComboBoxes();

    disableDefaultBoatSelection();
}

//

/*!
 * \brief Prevent editing default station.
 *
 * Disables the corresponding combo box.
 *
 * Call this function when station map keys have changed,
 * because setting the default station requires up to date
 * rowids in order to be meaningful. The default station
 * can then be set again after reloading the settings dialog.
 */
void SettingsDialog::disableDefaultStationSelection()
{
    ui->defaultStation_comboBox->setEnabled(false);
    ui->defaultStation_comboBox->setCurrentIndex(-1);
    ui->defaultStation_comboBox->setEditable(true);
    ui->defaultStation_comboBox->setCurrentText("Zum Bearbeiten Einstellungen schließen und neu öffnen.");
}

/*!
 * \brief Prevent editing default boat.
 *
 * Disables the corresponding combo box.
 *
 * Call this function when boat map keys (boat names) have changed,
 * because setting the default boat requires up to date rowids in
 * order to be meaningful. The default boat can then be set again
 * after reloading the settings dialog.
 */
void SettingsDialog::disableDefaultBoatSelection()
{
    ui->defaultBoat_comboBox->setEnabled(false);
    ui->defaultBoat_comboBox->setCurrentIndex(-1);
    ui->defaultBoat_comboBox->setEditable(true);
    ui->defaultBoat_comboBox->setCurrentText("Zum Bearbeiten Einstellungen schließen und neu öffnen.");
}

//Private slots

/*!
 * \brief Reimplementation of QDialog::accept().
 *
 * Reimplements QDialog::accept().
 *
 * Writes the settings database before accepting/closing the dialog.
 *
 * Returns immediately, if accepting was disabled due to a wrong password or read-only database.
 */
void SettingsDialog::accept()
{
    if (acceptDisabled)
        return;

    //Do not write changes to settings database, if database not writeable
    if (DatabaseCache::isReadOnly())
    {
        QMessageBox(QMessageBox::Critical, "Fehler", "Schreiben nicht möglich! Datenbank ist nur lesbar, "
                    "da das Programm mehrfach geöffnet ist!", QMessageBox::Ok, this).exec();
        return;
    }

    //Write database
    if (!writeDatabase())
    {
        QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Datenbank!", QMessageBox::Ok, this).exec();
        return;
    }

    QDialog::accept();
}

//

/*!
 * \brief If documents tab selected, fit table to contents.
 *
 * \param index New selected tab index.
 */
void SettingsDialog::on_settings_tabWidget_currentChanged(const int index)
{
    //Need to initially resize documents table once after dialog fully constructed; do this when documents tab selected
    if (index == ui->settings_tabWidget->indexOf(ui->documents_tab))
        resizeDocumentsTable();
}

//

/*!
 * \brief Set default file path using a file dialog.
 *
 * Sets the default file path line edit text to a directory selected by a file chooser dialog.
 */
void SettingsDialog::on_chooseDefaultFilePath_pushButton_pressed()
{
    QFileDialog fileDialog(this, "Standard-Dateipfad", "", "Ordner");
    fileDialog.setFileMode(QFileDialog::FileMode::Directory);
    fileDialog.setOption(QFileDialog::Option::ShowDirsOnly, true);
    fileDialog.setAcceptMode(QFileDialog::AcceptMode::AcceptOpen);

    if (fileDialog.exec() != QDialog::DialogCode::Accepted)
        return;

    QStringList tFileNames = fileDialog.selectedFiles();

    if (tFileNames.empty() || tFileNames.at(0) == "")
    {
        QMessageBox(QMessageBox::Warning, "Kein Ordner", "Bitte Ordner auswählen!", QMessageBox::Ok, this).exec();
        return;
    }
    else if (tFileNames.size() > 1)
    {
        QMessageBox(QMessageBox::Warning, "Mehrere Ordner", "Bitte nur einen Ordner auswählen!", QMessageBox::Ok, this).exec();
        return;
    }

    QString tDir = tFileNames.at(0);

    ui->defaultFilePath_lineEdit->setText(tDir);
}

/*!
 * \brief Set XeLaTeX executable using a file dialog.
 *
 * Sets the XeLaTeX executable path line edit text to a file name selected by a file chooser dialog.
 *
 * On windows the dialog filter is set to "*.exe" files.
 * On linux the dialog filter is set to "*".
 */
void SettingsDialog::on_chooseXelatexPath_pushButton_pressed()
{
#if defined(Q_OS_LINUX)
    QString filter = "Ausführbare Dateien (*)";
#elif defined(Q_OS_WIN)
    QString filter = "Ausführbare Dateien (*.exe)";
#else
    #error Unsupported OS
#endif

    QString fileName = QFileDialog::getOpenFileName(this, "XeLaTeX-Pfad", "", filter);

    if (fileName == "")
        return;

    ui->xelatexPath_lineEdit->setText(fileName);
}

/*!
 * \brief Set custom association logo image file path.
 *
 * Sets the logo image file path line edit text to a file name selected by a file chooser dialog.
 */
void SettingsDialog::on_chooseLogoPath_pushButton_pressed()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Logo-Datei", "", "Bilddateien (*.png *.jpg *.jpeg *.bmp *.gif)");

    if (fileName == "")
        return;

    ui->logoPath_lineEdit->setText(fileName);
}

/*!
 * \brief Remember that password field was edited.
 *
 * Set/change the password only if the password field was edited by the user.
 */
void SettingsDialog::on_password_lineEdit_textEdited(const QString&)
{
    passwordEdited = true;
}

//

/*!
 * \brief Update station inputs with newly selected station.
 *
 * Insert the properties of the newly selected station into the corresponding input widgets.
 */
void SettingsDialog::on_stations_comboBox_currentIndexChanged(int)
{
    updateStationsInputs();
}

/*!
 * \brief Add a new station.
 *
 * Adds a new station and automatically selects it.
 *
 * The station name and location are set to automatically generated placeholder values.
 * All other properties are set to pre-defined placeholder values.
 */
void SettingsDialog::on_addStation_pushButton_pressed()
{
    QString newIdent;

    QString tLocation = "Ort ";
    QString tName;

    //Try 99 station identifiers (should be sufficient to find an unused one...)
    for (int i = 1; i < 100; ++i)
    {
        tLocation.append('-');
        tName = "Name " + QString::number(i);

        Aux::stationIdentFromNameLocation(tName, tLocation, newIdent);

        if (stations.find(newIdent) == stations.end())
            break;
    }

    //Check that station identifier does not exist yet
    if (stations.find(newIdent) != stations.end())
    {
        QMessageBox(QMessageBox::Warning, "Warnung", "Station existiert bereits! Zuerst vorhandene Stationen bearbeiten.",
                    QMessageBox::Ok, this).exec();
        return;
    }

    QString tLocalGroup = "Ortsgruppe";
    QString tDistrict = "Bezirk";
    QString tRadio = "Funk1";
    QString tRadioAlt = "Funk2";

    stations.insert({newIdent, {std::move(tLocation), std::move(tName), std::move(tLocalGroup), std::move(tDistrict),
                                std::move(tRadio), std::move(tRadioAlt)}});

    updateStationsBoatsComboBoxes();

    disableDefaultStationSelection();

    //Select the new station
    ui->stations_comboBox->setCurrentIndex(ui->stations_comboBox->findText(Aux::stationLabelFromIdent(newIdent)));
}

/*!
 * \brief Remove the selected station.
 */
void SettingsDialog::on_removeStation_pushButton_pressed()
{
    if (ui->stations_comboBox->currentIndex() == -1)
        return;

    QMessageBox msgBox(QMessageBox::Question, "Station entfernen?", "Station wird entfernt!\nFortfahren?",
                       QMessageBox::Abort | QMessageBox::Yes, this);
    msgBox.setDefaultButton(QMessageBox::Abort);

    if (msgBox.exec() != QMessageBox::Yes)
        return;

    QString tIdent = Aux::stationIdentFromLabel(ui->stations_comboBox->currentText());

    //Check that station is not set as home station for one of the boats
    for (const auto& it : boats)
    {
        if (it.second.homeStation == tIdent)
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Station ist für ein Boot als Heimatstation gesetzt!",
                        QMessageBox::Ok, this).exec();
            return;
        }
    }

    stations.erase(tIdent);

    updateStationsBoatsComboBoxes();

    disableDefaultStationSelection();
}

/*!
 * \brief Change the selected station's location.
 *
 * Simple whitespace changes and empty values are ignored.
 *
 * If a station with the resulting new identifier already exists, the change will be ignored as well.
 *
 * \param arg1 The new location.
 */
void SettingsDialog::on_stationLocation_lineEdit_textEdited(const QString& arg1)
{
    if (ui->stations_comboBox->currentIndex() == -1)
        return;

    QString newStationLocation = arg1.trimmed();

    if (newStationLocation == "")
        return;

    QString oldStationIdent = Aux::stationIdentFromLabel(ui->stations_comboBox->currentText());
    const Aux::Station& oldStation = stations.at(oldStationIdent);

    //Ignore if only whitespace added
    if (newStationLocation == oldStation.location)
        return;

    QString newStationIdent;
    Aux::stationIdentFromNameLocation(oldStation.name, newStationLocation, newStationIdent);

    if (stations.find(newStationIdent) != stations.end())
    {
        QMessageBox(QMessageBox::Warning, "Warnung", "Station existiert bereits!", QMessageBox::Ok, this).exec();
        return;
    }

    Aux::Station newStation = oldStation;
    newStation.location = newStationLocation;

    changeStationIdent(oldStationIdent, std::move(newStation));
}

/*!
 * \brief Change the selected station's name.
 *
 * Simple whitespace changes and empty values are ignored.
 *
 * If a station with the resulting new identifier already exists, the change will be ignored as well.
 *
 * \param arg1 The new name.
 */
void SettingsDialog::on_stationName_lineEdit_textEdited(const QString& arg1)
{
    if (ui->stations_comboBox->currentIndex() == -1)
        return;

    QString newStationName = arg1.trimmed();

    if (newStationName == "")
        return;

    QString oldStationIdent = Aux::stationIdentFromLabel(ui->stations_comboBox->currentText());
    const Aux::Station& oldStation = stations.at(oldStationIdent);

    //Ignore if only whitespace added
    if (newStationName == oldStation.name)
        return;

    QString newStationIdent;
    Aux::stationIdentFromNameLocation(newStationName, oldStation.location, newStationIdent);

    if (stations.find(newStationIdent) != stations.end())
    {
        QMessageBox(QMessageBox::Warning, "Warnung", "Station existiert bereits!", QMessageBox::Ok, this).exec();
        return;
    }

    Aux::Station newStation = oldStation;
    newStation.name = newStationName;

    changeStationIdent(oldStationIdent, std::move(newStation));
}

/*!
 * \brief Change the selected station's local group.
 *
 * \param arg1 The new local group.
 */
void SettingsDialog::on_localGroup_lineEdit_textEdited(const QString& arg1)
{
    if (ui->stations_comboBox->currentIndex() == -1)
        return;

    Aux::Station& tStation = stations.at(Aux::stationIdentFromLabel(ui->stations_comboBox->currentText()));

    tStation.localGroup = arg1.trimmed();
}

/*!
 * \brief Change the selected station's district association.
 *
 * \param arg1 The new district association.
 */
void SettingsDialog::on_districtAssociation_lineEdit_textEdited(const QString& arg1)
{
    if (ui->stations_comboBox->currentIndex() == -1)
        return;

    Aux::Station& tStation = stations.at(Aux::stationIdentFromLabel(ui->stations_comboBox->currentText()));

    tStation.districtAssociation = arg1.trimmed();
}

/*!
 * \brief Change the selected station's radio call name.
 *
 * \param arg1 The new radio call name.
 */
void SettingsDialog::on_stationRadioCallName_lineEdit_textEdited(const QString& arg1)
{
    if (ui->stations_comboBox->currentIndex() == -1)
        return;

    Aux::Station& tStation = stations.at(Aux::stationIdentFromLabel(ui->stations_comboBox->currentText()));

    tStation.radioCallName = arg1.trimmed();
}

/*!
 * \brief Change the selected station's alt. radio call name.
 *
 * \param arg1 The new alternative radio call name.
 */
void SettingsDialog::on_stationRadioCallNameAlt_lineEdit_textEdited(const QString& arg1)
{
    if (ui->stations_comboBox->currentIndex() == -1)
        return;

    Aux::Station& tStation = stations.at(Aux::stationIdentFromLabel(ui->stations_comboBox->currentText()));

    tStation.radioCallNameAlt = arg1.trimmed();
}

/*!
 * \brief Copy the radio call name to the alternative one.
 *
 * Do not need an alternative radio call name, simply insert the normal radio call name.
 *
 * Only performs the copy action, if \p checked is true, i.e. if the button was clicked by the user.
 *
 * \param checked Button checked (i.e. pressed)?
 */
void SettingsDialog::on_copyStationRadioCallNameAlt_radioButton_toggled(const bool checked)
{
    if (checked)
    {
        ui->copyStationRadioCallNameAlt_radioButton->setChecked(false);
        ui->stationRadioCallNameAlt_lineEdit->setText(ui->stationRadioCallName_lineEdit->text());

        //QLineEdit::setText() does not trigger textEdited() signal; call slot directly
        on_stationRadioCallNameAlt_lineEdit_textEdited(ui->stationRadioCallNameAlt_lineEdit->text());
    }
}

//

/*!
 * \brief Update boat inputs with newly selected boat.
 *
 * Insert the properties of the newly selected boat into the corresponding input widgets.
 */
void SettingsDialog::on_boats_comboBox_currentIndexChanged(int)
{
    updateBoatsInputs();
}

/*!
 * \brief Add a new boat.
 *
 * Adds a new boat and automatically selects it.
 *
 * The boat name is set to an automatically generated placeholder value.
 * All other properties are set to pre-defined placeholder values.
 */
void SettingsDialog::on_addBoat_pushButton_pressed()
{
    QString newName;

    //Try 99 boat names (should be sufficient to find an unused one...)
    for (int i = 1; i < 100; ++i)
    {
        newName = "Name " + QString::number(i);

        if (boats.find(newName) == boats.end())
            break;
    }

    //Check that boat identifier does not exist yet
    if (boats.find(newName) != boats.end())
    {
        QMessageBox(QMessageBox::Warning, "Warnung", "Boot existiert bereits! Zuerst vorhandene Boote bearbeiten.",
                    QMessageBox::Ok, this).exec();
        return;
    }

    QString tAcronym = "";
    QString tType = "Typ";
    QString tFuel = "Treibstoff";
    QString tRadio = "Funk1";
    QString tRadioAlt = "Funk2";
    QString tHomeStation = "";

    //Set home station to currently selected station
    if (ui->stations_comboBox->currentIndex() != -1)
        tHomeStation = Aux::stationIdentFromLabel(ui->stations_comboBox->currentText());

    boats.insert({newName, {newName, std::move(tAcronym), std::move(tType), std::move(tFuel),
                            std::move(tRadio), std::move(tRadioAlt), std::move(tHomeStation)}});

    updateStationsBoatsComboBoxes();

    disableDefaultBoatSelection();

    //Select the new boat
    ui->boats_comboBox->setCurrentIndex(ui->boats_comboBox->findText(newName));
}

/*!
 * \brief Remove the selected boat.
 */
void SettingsDialog::on_removeBoat_pushButton_pressed()
{
    if (ui->boats_comboBox->currentIndex() == -1)
        return;

    QMessageBox msgBox(QMessageBox::Question, "Boot entfernen?", "Boot wird entfernt!\nFortfahren?",
                       QMessageBox::Abort | QMessageBox::Yes, this);
    msgBox.setDefaultButton(QMessageBox::Abort);

    if (msgBox.exec() != QMessageBox::Yes)
        return;

    boats.erase(ui->boats_comboBox->currentText());

    updateStationsBoatsComboBoxes();

    disableDefaultBoatSelection();
}

/*!
 * \brief Change the selected boat's name.
 *
 * Simple whitespace changes and empty values are ignored.
 *
 * If a boat with the resulting new identifier already exists, the change will be ignored as well.
 *
 * \param arg1 The new name.
 */
void SettingsDialog::on_boatName_lineEdit_textEdited(const QString& arg1)
{
    if (ui->boats_comboBox->currentIndex() == -1)
        return;

    if (arg1.trimmed() == "")
        return;

    QString oldBoatName = ui->boats_comboBox->currentText();
    QString newBoatName = arg1.trimmed();

    //Ignore if only whitespace added
    if (newBoatName == oldBoatName)
        return;

    if (boats.find(newBoatName) != boats.end())
    {
        QMessageBox(QMessageBox::Warning, "Warnung", "Boot existiert bereits!", QMessageBox::Ok, this).exec();
        return;
    }

    const Aux::Boat& oldBoat = boats.at(oldBoatName);

    Aux::Boat newBoat = oldBoat;
    newBoat.name = newBoatName;

    changeBoatName(oldBoat.name, std::move(newBoat));
}

/*!
 * \brief Change the selected boat's acronym.
 *
 * \param arg1 The new acronym.
 */
void SettingsDialog::on_boatAcronym_lineEdit_textEdited(const QString& arg1)
{
    if (ui->boats_comboBox->currentIndex() == -1)
        return;

    Aux::Boat& tBoat = boats.at(ui->boats_comboBox->currentText());

    tBoat.acronym = arg1.trimmed();
}

/*!
 * \brief Change the selected boat's type.
 *
 * \param arg1 The new type.
 */
void SettingsDialog::on_boatType_lineEdit_textEdited(const QString& arg1)
{
    if (ui->boats_comboBox->currentIndex() == -1)
        return;

    Aux::Boat& tBoat = boats.at(ui->boats_comboBox->currentText());

    tBoat.type = arg1.trimmed();
}

/*!
 * \brief Change the selected boat's fuel type.
 *
 * \param arg1 The new fuel type.
 */
void SettingsDialog::on_boatFuelType_comboBox_currentTextChanged(const QString& arg1)
{
    if (ui->boats_comboBox->currentIndex() == -1)
        return;

    Aux::Boat& tBoat = boats.at(ui->boats_comboBox->currentText());

    tBoat.fuelType = arg1.trimmed();
}

/*!
 * \brief Change the selected boat's radio call name.
 *
 * \param arg1 The new radio call name.
 */
void SettingsDialog::on_boatRadioCallName_lineEdit_textEdited(const QString& arg1)
{
    if (ui->boats_comboBox->currentIndex() == -1)
        return;

    Aux::Boat& tBoat = boats.at(ui->boats_comboBox->currentText());

    tBoat.radioCallName = arg1.trimmed();
}

/*!
 * \brief Change the selected boat's alt. radio call name.
 *
 * \param arg1 The new alternative radio call name.
 */
void SettingsDialog::on_boatRadioCallNameAlt_lineEdit_textEdited(const QString& arg1)
{
    if (ui->boats_comboBox->currentIndex() == -1)
        return;

    Aux::Boat& tBoat = boats.at(ui->boats_comboBox->currentText());

    tBoat.radioCallNameAlt = arg1.trimmed();
}

/*!
 * \brief Copy the radio call name to the alternative one.
 *
 * Do not need an alternative radio call name, simply insert the normal radio call name.
 *
 * Only performs the copy action, if \p checked is true, i.e. if the button was clicked by the user.
 *
 * \param checked Button checked (i.e. pressed)?
 */
void SettingsDialog::on_copyBoatRadioCallNameAlt_radioButton_toggled(const bool checked)
{
    if (checked)
    {
        ui->copyBoatRadioCallNameAlt_radioButton->setChecked(false);
        ui->boatRadioCallNameAlt_lineEdit->setText(ui->boatRadioCallName_lineEdit->text());

        //QLineEdit::setText() does not trigger textEdited() signal; call slot directly
        on_boatRadioCallNameAlt_lineEdit_textEdited(ui->boatRadioCallNameAlt_lineEdit->text());
    }
}

/*!
 * \brief Change the selected boat's home station.
 *
 * \param index The new home station's combo box item index.
 */
void SettingsDialog::on_boatHomeStation_comboBox_currentIndexChanged(const int index)
{
    if (ui->boats_comboBox->currentIndex() == -1)
        return;

    if (index == -1)
        return;

    Aux::Boat& tBoat = boats.at(ui->boats_comboBox->currentText());

    tBoat.homeStation = Aux::stationIdentFromLabel(ui->boatHomeStation_comboBox->currentText());
}

//

/*!
 * \brief Adjust the number rows of the documents table.
 *
 * Adds/removes document table rows to/from the end of the table.
 *
 * \param arg1 The new number of documents.
 */
void SettingsDialog::on_numDocuments_spinBox_valueChanged(const int arg1)
{
    int tOldRowCount = ui->documents_tableWidget->rowCount();

    if (arg1 > tOldRowCount)
    {
        for (int i = 0; i < arg1-tOldRowCount; ++i)
        {
            ui->documents_tableWidget->insertRow(ui->documents_tableWidget->rowCount());

            ui->documents_tableWidget->setItem(ui->documents_tableWidget->rowCount()-1, 0, new QTableWidgetItem(""));
            ui->documents_tableWidget->setItem(ui->documents_tableWidget->rowCount()-1, 1, new QTableWidgetItem(""));
        }
    }
    else if (arg1 < tOldRowCount)
        ui->documents_tableWidget->setRowCount(arg1);

    resizeDocumentsTable();
}

/*!
 * \brief Set a document path using a file dialog.
 *
 * Sets the document path in the currently selected row to a file selected by a file chooser dialog.
 */
void SettingsDialog::on_chooseDocument_pushButton_pressed()
{
    if (ui->documents_tableWidget->currentRow() == -1)
        return;

    QFileDialog fileDialog(this, "Dokument wählen", "", "Alle Dateien (*.*)");
    fileDialog.setFileMode(QFileDialog::FileMode::AnyFile);
    fileDialog.setAcceptMode(QFileDialog::AcceptMode::AcceptOpen);

    if (fileDialog.exec() != QDialog::DialogCode::Accepted)
        return;

    QStringList tFileNames = fileDialog.selectedFiles();

    if (tFileNames.empty() || tFileNames.at(0) == "")
    {
        QMessageBox(QMessageBox::Warning, "Keine Datei", "Bitte Datei auswählen!", QMessageBox::Ok, this).exec();
        return;
    }
    if (tFileNames.size() > 1)
    {
        QMessageBox(QMessageBox::Warning, "Mehrere Dateien", "Bitte nur eine Datei auswählen!", QMessageBox::Ok, this).exec();
        return;
    }

    ui->documents_tableWidget->item(ui->documents_tableWidget->currentRow(), 1)->setText(tFileNames.at(0));
}

/*!
 * \brief Reset table item if it contains a character used to separate documents and names/paths.
 *
 * Forbidden characters are '$' and '%', which are used to separate different documents
 * and their names and paths in the single database field, respectively.
 *
 * \param row Row of the edited item.
 * \param column Column of the edited item.
 */
void SettingsDialog::on_documents_tableWidget_cellChanged(const int row, const int column)
{
    if (row == -1 || column == -1)
        return;

    if (ui->documents_tableWidget->item(row, column)->text().contains('%') ||
        ui->documents_tableWidget->item(row, column)->text().contains('$'))
    {
        QMessageBox(QMessageBox::Warning, "Nicht erlaubtes Zeichen", "Zeichen '%' und '$' nicht erlaubt!",
                    QMessageBox::Ok, this).exec();
        ui->documents_tableWidget->item(row, column)->setText("");
    }
}

//

/*!
 * \brief Show restart hint when newly activating single instance mode.
 *
 * Shows a message box asking the user to restart the application in order for the activated single instance mode to become active.
 *
 * \param arg1 Check box check state.
 */
void SettingsDialog::on_singleInstance_checkBox_stateChanged(const int arg1)
{
    if ((arg1 == Qt::CheckState::Checked) && !SettingsCache::getBoolSetting("app_singleInstance"))
    {
        QMessageBox(QMessageBox::Information, "Nur eine Instanz erlauben",
                    "Damit diese Änderung wirksam wird, muss das Programm neu gestartet werden!", QMessageBox::Ok, this).exec();
    }
}
