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

#ifndef SETTINGSCACHE_H
#define SETTINGSCACHE_H

#include <QLockFile>
#include <QString>

#include <functional>
#include <map>
#include <memory>
#include <utility>

/*!
 * \brief Wrapper class to access settings from DatabaseCache.
 *
 * This class provides a wrapper interface to the DatabaseCache
 * in order to read/write settings from/to the configuration database.
 * In addition to the functions provided by DatabaseCache, this class
 * also checks, if a setting with the specified name actually exists,
 * and provides default values in case a setting has not yet been set
 * (a new configuration database will not contain any settings entries).
 *
 * Before using the SettingsCache, populate() should be called.
 * This in turn calls DatabaseCache::populate(), which loads the
 * settings values from the database into the DatabaseCache.
 */
class SettingsCache
{
public:
    SettingsCache() = delete;   ///< Deleted constructor.
    //
    static bool populate(std::shared_ptr<QLockFile> pLockFile, bool pForce = false);    ///< \brief Fill settings cache with program
                                                                                        ///  settings from configuration database.
    //
    static int getIntSetting(const QString& pSetting, bool pNoMsgBox = false);      ///< Get an integer type setting.
    static bool setIntSetting(const QString& pSetting, int pValue);                 ///< Set an integer type setting.
    static double getDblSetting(const QString& pSetting, bool pNoMsgBox = false);   ///< Get a floating-point type setting.
    static bool setDblSetting(const QString& pSetting, double pValue);              ///< Set a floating-point type setting.
    static QString getStrSetting(const QString& pSetting, bool pNoMsgBox = false);  ///< Get a string type setting.
    static bool setStrSetting(const QString& pSetting, const QString& pValue);      ///< Set a string type setting.
    //Boolean aliases
    static bool getBoolSetting(const QString& pSetting, bool pNoMsgBox = false);    ///< Get an integer-valued setting as boolean.
    static bool setBoolSetting(const QString& pSetting, bool pValue);               ///< Set an integer-valued setting as boolean.

private:
    static int getAutoExportOnSave(bool pNoMsgBox = false);             ///< \brief Read "app_export_autoOnSave" setting
                                                                        ///  from database cache (defines default value).
    static bool setAutoExportOnSave(int pValue);                        ///< Write "app_export_autoOnSave" setting to database cache.
    static int getAutoExportOnSaveAskFileName(bool pNoMsgBox = false);  ///< \brief Read "app_export_autoOnSave_askForFileName"
                                                                        ///  setting from database cache (defines default value).
    static bool setAutoExportOnSaveAskFileName(int pValue);             ///< \brief Write "app_export_autoOnSave_askForFileName"
                                                                        ///  setting to database cache.
    static int getTwoSidedPrint(bool pNoMsgBox = false);    ///< \brief Read "app_export_twoSidedPrint" setting from database cache
                                                            ///  (defines default value).
    static bool setTwoSidedPrint(int pValue);               ///< Write "app_export_twoSidedPrint" setting to database cache.
    //
    static int getDisableBoatLog(bool pNoMsgBox = false);               ///< \brief Read "app_boatLog_disabled" setting
                                                                        ///  from database cache (defines default value).
    static bool setDisableBoatLog(int pValue);                          ///< Write "app_boatLog_disabled" setting to database cache.
    static int getAutoApplyBoatDriveChanges(bool pNoMsgBox = false);    ///< \brief Read "app_reportWindow_autoApplyBoatDriveChanges"
                                                                        ///  setting from database cache (defines default value).
    static bool setAutoApplyBoatDriveChanges(int pValue);               ///< Write "app_reportWindow_autoApplyBoatDriveChanges"
                                                                        ///  setting to database cache.
    //
    static int getSingleApplicationInstance(bool pNoMsgBox = false);    ///< \brief Read "app_singleInstance" setting
                                                                        ///  from database cache (defines default value).
    static bool setSingleApplicationInstance(int pValue);               ///< Write "app_singleInstance" setting to database cache.
    //
    static int getDefaultStation(bool pNoMsgBox = false);   ///< \brief Read "app_default_station" setting from database cache
                                                            ///  (defines default value).
    static bool setDefaultStation(int pValue);              ///< Write "app_default_station" setting to database cache.
    static int getDefaultBoat(bool pNoMsgBox = false);  ///< Read "app_default_boat" setting from database cache (defines default value).
    static bool setDefaultBoat(int pValue);             ///< Write "app_default_boat" setting to database cache.
    //
    static QString getDutyTimeBegin(bool pNoMsgBox = false);    ///< \brief Read "app_default_dutyTimeBegin" setting
                                                                ///  from database cache (defines default value).
    static bool setDutyTimeBegin(const QString& pValue);        ///< Write "app_default_dutyTimeBegin" setting to database cache.
    static QString getDutyTimeEnd(bool pNoMsgBox = false);      ///< \brief Read "app_default_dutyTimeEnd" setting
                                                                ///  from database cache (defines default value).
    static bool setDutyTimeEnd(const QString& pValue);          ///< Write "app_default_dutyTimeEnd" setting to database cache.
    //
    static QString getDefaultDirectory(bool pNoMsgBox = false); ///< \brief Read "app_default_fileDialogDir" setting
                                                                ///  from database cache (defines default value).
    static bool setDefaultDirectory(const QString& pValue);     ///< Write "app_default_fileDialogDir" setting to database cache.
    //
    static QString getXeLaTeXPath(bool pNoMsgBox = false);      ///< \brief Read "app_export_xelatexPath" setting
                                                                ///  from database cache (defines default value).
    static bool setXeLaTeXPath(const QString& pValue);          ///< Write "app_export_xelatexPath" setting to database cache.
    static QString getCustomLogoPath(bool pNoMsgBox = false);   ///< \brief Read "app_export_customLogoPath" setting
                                                                ///  from database cache (defines default value).
    static bool setCustomLogoPath(const QString& pValue);       ///< Write "app_export_customLogoPath" setting to database cache.
    static QString getPDFFont(bool pNoMsgBox = false);          ///< \brief Read "app_export_fontFamily" setting
                                                                ///  from database cache (defines default value).
    static bool setPDFFont(const QString& pValue);              ///< Write "app_export_fontFamily" setting to database cache.
    //
    static QString getPasswordHash(bool pNoMsgBox = false);     ///< \brief Read "app_auth_hash" setting from database cache
                                                                ///  (defines default value).
    static bool setPasswordHash(const QString& pValue);         ///< Write "app_auth_hash" setting to database cache.
    static QString getPasswordSalt(bool pNoMsgBox = false);     ///< \brief Read "app_auth_salt" setting from database cache
                                                                ///  (defines default value).
    static bool setPasswordSalt(const QString& pValue);         ///< Write "app_auth_salt" setting to database cache.
    //
    static QString getDocumentLinkList(bool pNoMsgBox = false); ///< \brief Read "app_documentLinks_documentList"
                                                                ///  setting from database cache (defines default value).
    static bool setDocumentLinkList(const QString& pValue);     ///< Write "app_documentLinks_documentList" setting to database cache.
    //
    static QString getBoatmanRequiredLicense(bool pNoMsgBox = false);   ///< \brief Read "app_personnel_minQualis_boatman"
                                                                        ///  setting from database cache (defines default value).
    static bool setBoatmanRequiredLicense(const QString& pValue);       ///< \brief Write "app_personnel_minQualis_boatman"
                                                                        ///  setting to database cache.

private:
    static bool populated;  //Program settings loaded into cache from database by populate()?
    //
    static const std::map<QString,
                    std::pair<std::function<int(bool)>,
                              std::function<bool(int)>>> availableIntSettings;      //Integer settings with getters and setters
    static const std::map<QString,
                    std::pair<std::function<double(bool)>,
                              std::function<bool(double)>>> availableDblSettings;   //Floating-point settings with getters and setters
    static const std::map<QString,
                    std::pair<std::function<QString(bool)>,
                              std::function<bool(const QString&)>>> availableStrSettings;   //String settings with getters and setters
};

#endif // SETTINGSCACHE_H
