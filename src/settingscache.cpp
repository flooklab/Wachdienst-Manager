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

#include "settingscache.h"

#include "databasecache.h"

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QStringList>

#include <stdexcept>

//Initialize static class members

bool SettingsCache::populated = false;
//
const std::map<QString, std::pair<std::function<int(bool)>, std::function<bool(int)>>> SettingsCache::availableIntSettings =
        {{"app_export_autoOnSave", {SettingsCache::getAutoExportOnSave, SettingsCache::setAutoExportOnSave}},
         {"app_export_autoOnSave_askForFileName", {SettingsCache::getAutoExportOnSaveAskFileName,
                                                   SettingsCache::setAutoExportOnSaveAskFileName}},
         {"app_export_twoSidedPrint", {SettingsCache::getTwoSidedPrint, SettingsCache::setTwoSidedPrint}},
         {"app_boatLog_disabled", {SettingsCache::getDisableBoatLog, SettingsCache::setDisableBoatLog}},
         {"app_reportWindow_autoApplyBoatDriveChanges", {SettingsCache::getAutoApplyBoatDriveChanges,
                                                         SettingsCache::setAutoApplyBoatDriveChanges}},
         {"app_singleInstance", {SettingsCache::getSingleApplicationInstance, SettingsCache::setSingleApplicationInstance}},
         {"app_default_station", {SettingsCache::getDefaultStation, SettingsCache::setDefaultStation}},
         {"app_default_boat", {SettingsCache::getDefaultBoat, SettingsCache::setDefaultBoat}}};
const std::map<QString, std::pair<std::function<double(bool)>, std::function<bool(double)>>> SettingsCache::availableDblSettings =
        {};
const std::map<QString, std::pair<std::function<QString(bool)>, std::function<bool(const QString&)>>>
        SettingsCache::availableStrSettings =
        {{"app_default_dutyTimeBegin", {SettingsCache::getDutyTimeBegin, SettingsCache::setDutyTimeBegin}},
         {"app_default_dutyTimeEnd", {SettingsCache::getDutyTimeEnd, SettingsCache::setDutyTimeEnd}},
         {"app_default_fileDialogDir", {SettingsCache::getDefaultDirectory, SettingsCache::setDefaultDirectory}},
         {"app_export_xelatexPath", {SettingsCache::getXeLaTeXPath, SettingsCache::setXeLaTeXPath}},
         {"app_export_customLogoPath", {SettingsCache::getCustomLogoPath, SettingsCache::setCustomLogoPath}},
         {"app_export_fontFamily", {SettingsCache::getPDFFont, SettingsCache::setPDFFont}},
         {"app_auth_hash", {SettingsCache::getPasswordHash, SettingsCache::setPasswordHash}},
         {"app_auth_salt", {SettingsCache::getPasswordSalt, SettingsCache::setPasswordSalt}},
         {"app_documentLinks_documentList", {SettingsCache::getDocumentLinkList, SettingsCache::setDocumentLinkList}},
         {"app_personnel_minQualis_boatman", {SettingsCache::getBoatmanRequiredLicense, SettingsCache::setBoatmanRequiredLicense}}};

//Public

/*!
 * \brief Fill settings cache with program settings from configuration database.
 *
 * If this function has not already been called or \p pForce is true, DatabaseCache::populate() will be called (forwarding
 * the \p pLockFile and \p pForce arguments) and after that any newly introduced settings will be added to the database.
 *
 * Nothing else happens since the SettingsCache is basically just a wrapper for the DatabaseCache.
 *
 * \param pLockFile Pointer to a lock file for the configuration and personnel databases.
 * \param pForce Populate cache even if already populated.
 * \return If already populated or new populate action was successful (see also return value of DatabaseCache::populate()).
 */
bool SettingsCache::populate(const std::shared_ptr<QLockFile> pLockFile, const bool pForce)
{
    if (populated && !pForce)
        return true;

    //Settings are cached in database cache, so make sure database cache is populated
    populated = DatabaseCache::populate(pLockFile, pForce);

    //Ensure that new settings are added to database by once calling getter for every setting
    for (const auto& it : availableIntSettings)
        it.second.first(false);
    for (const auto& it : availableDblSettings)
        it.second.first(false);
    for (const auto& it : availableStrSettings)
        it.second.first(false);

    return populated;
}

//

/*!
 * \brief Get an integer type setting.
 *
 * Gets the integer value stored for setting \p pSetting from the database cache.
 * If no value is set, a pre-defined default value is first written to the database and then this value is returned.
 *
 * Available integer settings are:
 * - app_export_autoOnSave
 * - app_export_autoOnSave_askForFileName
 * - app_export_twoSidedPrint
 * - app_boatLog_disabled
 * - app_reportWindow_autoApplyBoatDriveChanges
 * - app_singleInstance
 * - app_default_station
 * - app_default_boat
 *
 * \param pSetting Name of the setting.
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 *
 * \throws std::invalid_argument Integer type setting \p pSetting does not exist.
 */
int SettingsCache::getIntSetting(const QString& pSetting, const bool pNoMsgBox)
{
    if (availableIntSettings.find(pSetting) != availableIntSettings.end())
        return availableIntSettings.at(pSetting).first(pNoMsgBox);
    else
        throw std::invalid_argument("Invalid integer type setting \"" + pSetting.toStdString() + "\"");
}

/*!
 * \brief Set an integer type setting.
 *
 * Sets the integer value stored for setting \p pSetting in the database cache,
 * which also writes the value to the configuration database.
 *
 * If writing to the database fails, the cached value will not be changed.
 *
 * For available integer settings, see getIntSetting().
 *
 * \param pSetting Name of the setting.
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 *
 * \throws std::invalid_argument Integer type setting \p pSetting does not exist.
 */
bool SettingsCache::setIntSetting(const QString& pSetting, const int pValue)
{
    if (availableIntSettings.find(pSetting) != availableIntSettings.end())
        return availableIntSettings.at(pSetting).second(pValue);
    else
        throw std::invalid_argument("Invalid integer type setting \"" + pSetting.toStdString() + "\"");
}

/*!
 * \brief Get a floating-point type setting.
 *
 * Gets the floating-point value stored for setting \p pSetting from the database cache.
 * If no value is set, a pre-defined default value is first written to the database and then this value is returned.
 *
 * Available floating-point settings are:
 * NONE
 *
 * \param pSetting Name of the setting.
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 *
 * \throws std::invalid_argument Floating-point type setting \p pSetting does not exist.
 */
double SettingsCache::getDblSetting(const QString& pSetting, const bool pNoMsgBox)
{
    if (availableDblSettings.find(pSetting) != availableDblSettings.end())
        return availableDblSettings.at(pSetting).first(pNoMsgBox);
    else
        throw std::invalid_argument("Invalid floating-point type setting \"" + pSetting.toStdString() + "\"");
}

/*!
 * \brief Set a floating-point type setting.
 *
 * Sets the floating-point value stored for setting \p pSetting in the database cache,
 * which also writes the value to the configuration database.
 *
 * If writing to the database fails, the cached value will not be changed.
 *
 * For available floating-point settings, see getDblSetting().
 *
 * \param pSetting Name of the setting.
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 *
 * \throws std::invalid_argument Floating-point type setting \p pSetting does not exist.
 */
bool SettingsCache::setDblSetting(const QString& pSetting, const double pValue)
{
    if (availableDblSettings.find(pSetting) != availableDblSettings.end())
        return availableDblSettings.at(pSetting).second(pValue);
    else
        throw std::invalid_argument("Invalid floating-point type setting \"" + pSetting.toStdString() + "\"");
}

/*!
 * \brief Get a string type setting.
 *
 * Gets the string value stored for setting \p pSetting from the database cache.
 * If no value is set, a pre-defined default value is first written to the database and then this value is returned.
 *
 * Available string settings are:
 * - app_default_dutyTimeBegin
 * - app_default_dutyTimeEnd
 * - app_default_fileDialogDir
 * - app_export_xelatexPath
 * - app_export_customLogoPath
 * - app_export_fontFamily
 * - app_auth_hash
 * - app_auth_salt
 * - app_documentLinks_documentList
 * - app_personnel_minQualis_boatman
 *
 * \param pSetting Name of the setting.
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 *
 * \throws std::invalid_argument String type setting \p pSetting does not exist.
 */
QString SettingsCache::getStrSetting(const QString& pSetting, const bool pNoMsgBox)
{
    if (availableStrSettings.find(pSetting) != availableStrSettings.end())
        return availableStrSettings.at(pSetting).first(pNoMsgBox);
    else
        throw std::invalid_argument("Invalid string type setting \"" + pSetting.toStdString() + "\"");
}

/*!
 * \brief Set a string type setting.
 *
 * Sets the string value stored for setting \p pSetting in the database cache,
 * which also writes the value to the configuration database.
 *
 * If writing to the database fails, the cached value will not be changed.
 *
 * For available string settings, see getStrSetting().
 *
 * \param pSetting Name of the setting.
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 *
 * \throws std::invalid_argument String type setting \p pSetting does not exist.
 */
bool SettingsCache::setStrSetting(const QString& pSetting, const QString& pValue)
{
    if (availableStrSettings.find(pSetting) != availableStrSettings.end())
        return availableStrSettings.at(pSetting).second(pValue);
    else
        throw std::invalid_argument("Invalid string type setting \"" + pSetting.toStdString() + "\"");
}

//

/*!
 * \brief Get an integer-valued setting as boolean.
 *
 * Gets the integer (sic!) value for setting \p pSetting via
 * getIntSetting() and returns false if the value is 0 and true else.
 *
 * (If no value is set, the default value provided by getIntSetting() is used for the boolean conversion.)
 *
 * For available integer settings, see getIntSetting().
 *
 * \param pSetting Name of the setting.
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Boolean equivalent of the integer value of the setting.
 *
 * \throws std::invalid_argument Integer type setting \p pSetting does not exist.
 */
bool SettingsCache::getBoolSetting(const QString& pSetting, const bool pNoMsgBox)
{
    return (getIntSetting(pSetting, pNoMsgBox) == 0 ? false : true);
}

/*!
 * \brief Set an integer-valued setting as boolean.
 *
 * Sets the integer (sic!) value stored for setting \p pSetting in the database cache using getIntSetting().
 * It is set to 1 if \p pValue is true and 0 else. This also writes the value to the configuration database.
 *
 * If writing to the database fails, the cached value will not be changed.
 *
 * For available integer settings, see getIntSetting().
 *
 * \param pSetting Name of the setting.
 * \param pValue New boolean(!) value for the integer(!) setting.
 * \return If writing to database was successful.
 *
 * \throws std::invalid_argument Integer type setting \p pSetting does not exist.
 */
bool SettingsCache::setBoolSetting(const QString& pSetting, const bool pValue)
{
    return setIntSetting(pSetting, pValue ? 1 : 0);
}

//Private

/*!
 * \brief Read "app_export_autoOnSave" setting from database cache (defines default value).
 *
 * Sets (and returns) default value of 0, if setting is not set.
 *
 * Shows a warning message box, if writing not set setting to database fails.
 *
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 */
int SettingsCache::getAutoExportOnSave(const bool pNoMsgBox)
{
    int tValue = 0;
    if (!DatabaseCache::getSetting("app_export_autoOnSave", tValue, 0, true))   //Default: disabled
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Konfigurations-Datenbank!", QMessageBox::Ok).exec();
        }
    }
    return tValue;
}

/*!
 * \brief Write "app_export_autoOnSave" setting to database cache.
 *
 * Sets the cached value and also writes it to the configuration database.
 * If writing to the database fails, the cached value will not be changed.
 *
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 */
bool SettingsCache::setAutoExportOnSave(const int pValue)
{
    return DatabaseCache::setSetting("app_export_autoOnSave", pValue);
}

/*!
 * \brief Read "app_export_autoOnSave_askForFileName" setting from database cache (defines default value).
 *
 * Sets (and returns) default value of 0, if setting is not set.
 *
 * Shows a warning message box, if writing not set setting to database fails.
 *
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 */
int SettingsCache::getAutoExportOnSaveAskFileName(const bool pNoMsgBox)
{
    int tValue = 0;
    if (!DatabaseCache::getSetting("app_export_autoOnSave_askForFileName", tValue, 0, true))    //Default: disabled
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Konfigurations-Datenbank!", QMessageBox::Ok).exec();
        }
    }
    return tValue;
}

/*!
 * \brief Write "app_export_autoOnSave_askForFileName" setting to database cache.
 *
 * Sets the cached value and also writes it to the configuration database.
 * If writing to the database fails, the cached value will not be changed.
 *
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 */
bool SettingsCache::setAutoExportOnSaveAskFileName(const int pValue)
{
    return DatabaseCache::setSetting("app_export_autoOnSave_askForFileName", pValue);
}

/*!
 * \brief Read "app_export_twoSidedPrint" setting from database cache (defines default value).
 *
 * Sets (and returns) default value of 0, if setting is not set.
 *
 * Shows a warning message box, if writing not set setting to database fails.
 *
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 */
int SettingsCache::getTwoSidedPrint(const bool pNoMsgBox)
{
    int tValue = 0;
    if (!DatabaseCache::getSetting("app_export_twoSidedPrint", tValue, 0, true))    //Default: one-sided
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Konfigurations-Datenbank!", QMessageBox::Ok).exec();
        }
    }
    return tValue;
}

/*!
 * \brief Write "app_export_twoSidedPrint" setting to database cache.
 *
 * Sets the cached value and also writes it to the configuration database.
 * If writing to the database fails, the cached value will not be changed.
 *
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 */
bool SettingsCache::setTwoSidedPrint(const int pValue)
{
    return DatabaseCache::setSetting("app_export_twoSidedPrint", pValue);
}

//

/*!
 * \brief Read "app_boatLog_disabled" setting from database cache (defines default value).
 *
 * Sets (and returns) default value of 0, if setting is not set.
 *
 * Shows a warning message box, if writing not set setting to database fails.
 *
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 */
int SettingsCache::getDisableBoatLog(const bool pNoMsgBox)
{
    int tValue = 0;
    if (!DatabaseCache::getSetting("app_boatLog_disabled", tValue, 0, true))    //Default: enabled boat log
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Konfigurations-Datenbank!", QMessageBox::Ok).exec();
        }
    }
    return tValue;
}

/*!
 * \brief Write "app_boatLog_disabled" setting to database cache.
 *
 * Sets the cached value and also writes it to the configuration database.
 * If writing to the database fails, the cached value will not be changed.
 *
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 */
bool SettingsCache::setDisableBoatLog(const int pValue)
{
    return DatabaseCache::setSetting("app_boatLog_disabled", pValue);
}

/*!
 * \brief Read "app_reportWindow_autoApplyBoatDriveChanges" setting from database cache (defines default value).
 *
 * Sets (and returns) default value of 1, if setting is not set.
 *
 * Shows a warning message box, if writing not set setting to database fails.
 *
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 */
int SettingsCache::getAutoApplyBoatDriveChanges(const bool pNoMsgBox)
{
    int tValue = 0;
    if (!DatabaseCache::getSetting("app_reportWindow_autoApplyBoatDriveChanges", tValue, 1, true))  //Default: auto-apply changes
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Konfigurations-Datenbank!", QMessageBox::Ok).exec();
        }
    }
    return tValue;
}

/*!
 * \brief Write "app_reportWindow_autoApplyBoatDriveChanges" setting to database cache.
 *
 * Sets the cached value and also writes it to the configuration database.
 * If writing to the database fails, the cached value will not be changed.
 *
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 */
bool SettingsCache::setAutoApplyBoatDriveChanges(const int pValue)
{
    return DatabaseCache::setSetting("app_reportWindow_autoApplyBoatDriveChanges", pValue);
}

//

/*!
 * \brief Read "app_singleInstance" setting from database cache (defines default value).
 *
 * Sets (and returns) default value of 0, if setting is not set.
 *
 * Shows a warning message box, if writing not set setting to database fails.
 *
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 */
int SettingsCache::getSingleApplicationInstance(const bool pNoMsgBox)
{
    int tValue = 0;
    if (!DatabaseCache::getSetting("app_singleInstance", tValue, 0, true))  //Default: allow multiple instances
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Konfigurations-Datenbank!", QMessageBox::Ok).exec();
        }
    }
    return tValue;
}

/*!
 * \brief Write "app_singleInstance" setting to database cache.
 *
 * Sets the cached value and also writes it to the configuration database.
 * If writing to the database fails, the cached value will not be changed.
 *
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 */
bool SettingsCache::setSingleApplicationInstance(const int pValue)
{
    return DatabaseCache::setSetting("app_singleInstance", pValue);
}

//

/*!
 * \brief Read "app_default_station" setting from database cache (defines default value).
 *
 * Sets (and returns) default value of -1, if setting is not set.
 *
 * Shows a warning message box, if writing not set setting to database fails.
 *
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 */
int SettingsCache::getDefaultStation(const bool pNoMsgBox)
{
    int tValue = 0;
    if (!DatabaseCache::getSetting("app_default_station", tValue, -1, true))    //Default: no default station
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Konfigurations-Datenbank!", QMessageBox::Ok).exec();
        }
    }
    return tValue;
}

/*!
 * \brief Write "app_default_station" setting to database cache.
 *
 * Sets the cached value and also writes it to the configuration database.
 * If writing to the database fails, the cached value will not be changed.
 *
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 */
bool SettingsCache::setDefaultStation(const int pValue)
{
    return DatabaseCache::setSetting("app_default_station", pValue);
}

/*!
 * \brief Read "app_default_boat" setting from database cache (defines default value).
 *
 * Sets (and returns) default value of -1, if setting is not set.
 *
 * Shows a warning message box, if writing not set setting to database fails.
 *
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 */
int SettingsCache::getDefaultBoat(const bool pNoMsgBox)
{
    int tValue = 0;
    if (!DatabaseCache::getSetting("app_default_boat", tValue, -1, true))   //Default: no default boat
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Konfigurations-Datenbank!", QMessageBox::Ok).exec();
        }
    }
    return tValue;
}

/*!
 * \brief Write "app_default_boat" setting to database cache.
 *
 * Sets the cached value and also writes it to the configuration database.
 * If writing to the database fails, the cached value will not be changed.
 *
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 */
bool SettingsCache::setDefaultBoat(const int pValue)
{
    return DatabaseCache::setSetting("app_default_boat", pValue);
}

//

/*!
 * \brief Read "app_default_dutyTimeBegin" setting from database cache (defines default value).
 *
 * Sets (and returns) default value of "10:00", if setting is not set.
 *
 * Shows a warning message box, if writing not set setting to database fails.
 *
 * Note: Also sets the setting to "10:00", if it is set but the value does not represent a valid time with format "hh:mm".
 *
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 */
QString SettingsCache::getDutyTimeBegin(const bool pNoMsgBox)
{
    QString tValue;
    if (!DatabaseCache::getSetting("app_default_dutyTimeBegin", tValue, "10:00", true))
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Konfigurations-Datenbank!", QMessageBox::Ok).exec();
        }
    }
    if (!QTime::fromString(tValue, "hh:mm").isValid())
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Warning, "Warnung", "Ungültige Zeitangabe! Setze auf 10:00.", QMessageBox::Ok).exec();
        }

        //Reset to default: begin at 10:00
        tValue = "10:00";
        DatabaseCache::setSetting("app_default_dutyTimeBegin", tValue);
    }
    return tValue;
}

/*!
 * \brief Write "app_default_dutyTimeBegin" setting to database cache.
 *
 * Sets the cached value and also writes it to the configuration database.
 * If writing to the database fails, the cached value will not be changed.
 *
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 */
bool SettingsCache::setDutyTimeBegin(const QString& pValue)
{
    return DatabaseCache::setSetting("app_default_dutyTimeBegin", pValue);
}

/*!
 * \brief Read "app_default_dutyTimeEnd" setting from database cache (defines default value).
 *
 * Sets (and returns) default value of "18:00", if setting is not set.
 *
 * Shows a warning message box, if writing not set setting to database fails.
 *
 * Note: Also sets the setting to "18:00", if it is set but the value does not represent a valid time with format "hh:mm".
 *
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 */
QString SettingsCache::getDutyTimeEnd(const bool pNoMsgBox)
{
    QString tValue = "";
    if (!DatabaseCache::getSetting("app_default_dutyTimeEnd", tValue, "18:00", true))   //Default: end at 18:00
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Konfigurations-Datenbank!", QMessageBox::Ok).exec();
        }
    }
    if (!QTime::fromString(tValue, "hh:mm").isValid())
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Warning, "Warnung", "Ungültige Zeitangabe! Setze auf 18:00.", QMessageBox::Ok).exec();
        }

        //Reset to default: end at 18:00
        tValue = "18:00";
        DatabaseCache::setSetting("app_default_dutyTimeEnd", tValue);
    }
    return tValue;
}

/*!
 * \brief Write "app_default_dutyTimeEnd" setting to database cache.
 *
 * Sets the cached value and also writes it to the configuration database.
 * If writing to the database fails, the cached value will not be changed.
 *
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 */
bool SettingsCache::setDutyTimeEnd(const QString& pValue)
{
    return DatabaseCache::setSetting("app_default_dutyTimeEnd", pValue);
}

//

/*!
 * \brief Read "app_default_fileDialogDir" setting from database cache (defines default value).
 *
 * Sets (and returns) default value of "", if setting is not set.
 *
 * Shows a warning message box, if writing not set setting to database fails.
 *
 * Note: Shows a warning message box if value set but the corresponding directory does not exist.
 *
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 */
QString SettingsCache::getDefaultDirectory(const bool pNoMsgBox)
{
    QString tValue;
    if (!DatabaseCache::getSetting("app_default_fileDialogDir", tValue, "", true))  //Default: empty path
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Konfigurations-Datenbank!", QMessageBox::Ok).exec();
        }
    }
    if (tValue != "" && !QDir(tValue).exists())
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Warning, "Warnung", "Standard-Pfad existiert nicht!", QMessageBox::Ok).exec();
        }
    }
    return tValue;
}

/*!
 * \brief Write "app_default_fileDialogDir" setting to database cache.
 *
 * Sets the cached value and also writes it to the configuration database.
 * If writing to the database fails, the cached value will not be changed.
 *
 * Note: Shows a warning message box if the directory pointed to by \p pValue does not exist.
 *
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 */
bool SettingsCache::setDefaultDirectory(const QString& pValue)
{
    if (pValue != "" && !QDir(pValue).exists())
    {
        QMessageBox(QMessageBox::Warning, "Warnung", "Standard-Pfad existiert nicht!", QMessageBox::Ok).exec();
    }
    return DatabaseCache::setSetting("app_default_fileDialogDir", pValue);
}

//

/*!
 * \brief Read "app_export_xelatexPath" setting from database cache (defines default value).
 *
 * Sets (and returns) default value of "", if setting is not set.
 *
 * Shows a warning message box, if writing not set setting to database fails.
 *
 * Note: Shows a warning message box if value set but the corresponding path does not exist.
 *
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 */
QString SettingsCache::getXeLaTeXPath(const bool pNoMsgBox)
{
    QString tValue;
    if (!DatabaseCache::getSetting("app_export_xelatexPath", tValue, "", true)) //Default: empty path
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Konfigurations-Datenbank!", QMessageBox::Ok).exec();
        }
    }
    if (tValue != "" && !QFileInfo::exists(tValue))
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Warning, "Warnung", "XeLaTeX-Pfad existiert nicht!", QMessageBox::Ok).exec();
        }
    }
    return tValue;
}

/*!
 * \brief Write "app_export_xelatexPath" setting to database cache.
 *
 * Sets the cached value and also writes it to the configuration database.
 * If writing to the database fails, the cached value will not be changed.
 *
 * Note: Shows a warning message box if value set but the corresponding path does not exist.
 *
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 */
bool SettingsCache::setXeLaTeXPath(const QString& pValue)
{
    if (pValue != "" && !QFileInfo::exists(pValue))
    {
        QMessageBox(QMessageBox::Warning, "Warnung", "XeLaTeX-Pfad existiert nicht!", QMessageBox::Ok).exec();
    }
    return DatabaseCache::setSetting("app_export_xelatexPath", pValue);
}

/*!
 * \brief Read "app_export_customLogoPath" setting from database cache (defines default value).
 *
 * Sets (and returns) default value of "", if setting is not set.
 *
 * Shows a warning message box, if writing not set setting to database fails.
 *
 * Note: Shows a warning message box if value set but the corresponding path does not exist.
 *
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 */
QString SettingsCache::getCustomLogoPath(const bool pNoMsgBox)
{
    QString tValue;
    if (!DatabaseCache::getSetting("app_export_customLogoPath", tValue, "", true))  //Default: empty path
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Konfigurations-Datenbank!", QMessageBox::Ok).exec();
        }
    }
    if (tValue != "" && !QFileInfo::exists(tValue))
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Warning, "Warnung", "Logo-Datei existiert nicht!", QMessageBox::Ok).exec();
        }
    }
    return tValue;
}

/*!
 * \brief Write "app_export_customLogoPath" setting to database cache.
 *
 * Sets the cached value and also writes it to the configuration database.
 * If writing to the database fails, the cached value will not be changed.
 *
 * Note: Shows a warning message box if value set but the corresponding path does not exist.
 *
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 */
bool SettingsCache::setCustomLogoPath(const QString& pValue)
{
    if (pValue != "" && !QFileInfo::exists(pValue))
    {
        QMessageBox(QMessageBox::Warning, "Warnung", "Logo-Datei existiert nicht!", QMessageBox::Ok).exec();
    }
    return DatabaseCache::setSetting("app_export_customLogoPath", pValue);
}

/*!
 * \brief Read "app_export_fontFamily" setting from database cache (defines default value).
 *
 * Sets (and returns) default value of "CMU", if setting is not set.
 *
 * Shows a warning message box, if writing not set setting to database fails.
 *
 * Note: Also sets the setting to "CMU", if it is set but the string is empty.
 *
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 */
QString SettingsCache::getPDFFont(const bool pNoMsgBox)
{
    QString tValue;
    if (!DatabaseCache::getSetting("app_export_fontFamily", tValue, "CMU", true))   //Default: Computer Modern font
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Konfigurations-Datenbank!", QMessageBox::Ok).exec();
        }
    }
    if (tValue == "")
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Warning, "Warnung", "Schriftart nicht gesetzt! Setze auf \"CMU\".", QMessageBox::Ok).exec();
        }

        //Reset to default: Computer Modern font
        tValue = "CMU";
        DatabaseCache::setSetting("app_export_fontFamily", tValue);
    }
    return tValue;
}

/*!
 * \brief Write "app_export_fontFamily" setting to database cache.
 *
 * Sets the cached value and also writes it to the configuration database.
 * If writing to the database fails, the cached value will not be changed.
 *
 * Note: Shows a warning message box if \p pValue is empty.
 *
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 */
bool SettingsCache::setPDFFont(const QString& pValue)
{
    if (pValue == "")
    {
        QMessageBox(QMessageBox::Warning, "Warnung", "Schriftart-Feld ist leer!", QMessageBox::Ok).exec();
    }
    return DatabaseCache::setSetting("app_export_fontFamily", pValue);
}

//

/*!
 * \brief Read "app_auth_hash" setting from database cache (defines default value).
 *
 * Sets (and returns) default value of "", if setting is not set.
 *
 * Shows a warning message box, if writing not set setting to database fails.
 *
 * Note: Checks that hash (getPasswordHash()) and salt (getPasswordSalt()) are either
 * both not empty or both empty and shows a warning message box if this is not the case.
 *
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 */
QString SettingsCache::getPasswordHash(const bool pNoMsgBox)
{
    QString tValue;
    if (!DatabaseCache::getSetting("app_auth_hash", tValue, "", false)) //Default: no password (i.e. no hash)
    {
        DatabaseCache::setSetting("app_auth_hash", tValue);

        //Check that salt not set as well
        QString tValue2;
        if (DatabaseCache::getSetting("app_auth_salt", tValue2, "", false) && tValue2 != "")
        {
            if (!pNoMsgBox)
            {
                QMessageBox(QMessageBox::Warning, "Warnung", "Passwort nicht korrekt gesetzt!", QMessageBox::Ok).exec();
            }
        }
    }
    else    //If hash set, check that salt set as well
    {
        QString tValue2;
        if (DatabaseCache::getSetting("app_auth_salt", tValue2, "", false) &&
                ((tValue != "" && tValue2 == "") || (tValue == "" && tValue2 != "")))
        {
            if (!pNoMsgBox)
            {
                QMessageBox(QMessageBox::Warning, "Warnung", "Passwort nicht korrekt gesetzt!", QMessageBox::Ok).exec();
            }
        }
    }
    return tValue;
}

/*!
 * \brief Write "app_auth_hash" setting to database cache.
 *
 * Sets the cached value and also writes it to the configuration database.
 * If writing to the database fails, the cached value will not be changed.
 *
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 */
bool SettingsCache::setPasswordHash(const QString& pValue)
{
    return DatabaseCache::setSetting("app_auth_hash", pValue);
}

/*!
 * \brief Read "app_auth_salt" setting from database cache (defines default value).
 *
 * Sets (and returns) default value of "", if setting is not set.
 *
 * Shows a warning message box, if writing not set setting to database fails.
 *
 * Note: Checks that hash (getPasswordHash()) and salt (getPasswordSalt()) are either
 * both not empty or both empty and shows a warning message box if this is not the case.
 *
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 */
QString SettingsCache::getPasswordSalt(const bool pNoMsgBox)
{
    QString tValue;
    if (!DatabaseCache::getSetting("app_auth_salt", tValue, "", false)) //Default: no password (i.e. no salt)
    {
        DatabaseCache::setSetting("app_auth_salt", tValue);

        //Check that hash not set as well
        QString tValue2;
        if (DatabaseCache::getSetting("app_auth_hash", tValue2, "", false) && tValue2 != "")
        {
            if (!pNoMsgBox)
            {
                QMessageBox(QMessageBox::Warning, "Warnung", "Passwort nicht korrekt gesetzt!", QMessageBox::Ok).exec();
            }
        }
    }
    else    //If salt set, check that hash set as well
    {
        QString tValue2;
        if (DatabaseCache::getSetting("app_auth_hash", tValue2, "", false) &&
                ((tValue != "" && tValue2 == "") || (tValue == "" && tValue2 != "")))
        {
            if (!pNoMsgBox)
            {
                QMessageBox(QMessageBox::Warning, "Warnung", "Passwort nicht korrekt gesetzt!", QMessageBox::Ok).exec();
            }
        }
    }
    return tValue;
}

/*!
 * \brief Write "app_auth_salt" setting to database cache.
 *
 * Sets the cached value and also writes it to the configuration database.
 * If writing to the database fails, the cached value will not be changed.
 *
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 */
bool SettingsCache::setPasswordSalt(const QString& pValue)
{
    return DatabaseCache::setSetting("app_auth_salt", pValue);
}

//

/*!
 * \brief Read "app_documentLinks_documentList" setting from database cache (defines default value).
 *
 * Sets (and returns) default value of "", if setting is not set.
 *
 * Shows a warning message box, if writing not set setting to database fails.
 *
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 */
QString SettingsCache::getDocumentLinkList(const bool pNoMsgBox)
{
    QString tValue;
    if (!DatabaseCache::getSetting("app_documentLinks_documentList", tValue, "", true)) //Default: no documents
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Konfigurations-Datenbank!", QMessageBox::Ok).exec();
        }
    }
    return tValue;
}

/*!
 * \brief Write "app_documentLinks_documentList" setting to database cache.
 *
 * Sets the cached value and also writes it to the configuration database.
 * If writing to the database fails, the cached value will not be changed.
 *
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 */
bool SettingsCache::setDocumentLinkList(const QString& pValue)
{
    return DatabaseCache::setSetting("app_documentLinks_documentList", pValue);
}

//

/*!
 * \brief Read "app_personnel_minQualis_boatman" setting from database cache (defines default value).
 *
 * Sets (and returns) default value of "A", if setting is not set.
 *
 * Shows a warning message box, if writing not set setting to database fails.
 *
 * Note: Also sets the setting to "A", if it is set but the string is empty or does not describe a valid boating license combination.
 *
 * \param pNoMsgBox Suppress warning message boxes.
 * \return Value of the setting.
 */
QString SettingsCache::getBoatmanRequiredLicense(const bool pNoMsgBox)
{
    QString tValue;
    if (!DatabaseCache::getSetting("app_personnel_minQualis_boatman", tValue, "A", true))   //Default: DLRG boating license A (inland)
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Konfigurations-Datenbank!", QMessageBox::Ok).exec();
        }
    }
    if (tValue == "" || !QStringList({"A", "B", "A&B", "A|B"}).contains(tValue))
    {
        if (!pNoMsgBox)
        {
            QMessageBox(QMessageBox::Warning, "Warnung", "Benötigter Bootsführerschein nicht gesetzt! Setze auf \"A (Binnen)\".",
                        QMessageBox::Ok).exec();
        }

        //Reset to default: DLRG boating license A (inland)
        tValue = "A";
        DatabaseCache::setSetting("app_personnel_minQualis_boatman", tValue);
    }
    return tValue;
}

/*!
 * \brief Write "app_personnel_minQualis_boatman" setting to database cache.
 *
 * Sets the cached value and also writes it to the configuration database.
 * If writing to the database fails, the cached value will not be changed.
 *
 * \param pValue New value for the setting.
 * \return If writing to database was successful.
 */
bool SettingsCache::setBoatmanRequiredLicense(const QString& pValue)
{
    return DatabaseCache::setSetting("app_personnel_minQualis_boatman", pValue);
}
