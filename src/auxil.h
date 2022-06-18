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

#ifndef AUXIL_H
#define AUXIL_H

#include "version.h"

#include <vector>

#include <QTime>
#include <QWidget>
#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QCryptographicHash>
#include <QPasswordDigestor>
#include <QRandomGenerator>
#include <QDataStream>
#include <QByteArray>
#include <QIODevice>
#include <QInputDialog>
#include <QLineEdit>

/*!
 * \brief Helper functions and public common variables.
 *
 * Provides helper functions such as for conversion between enums (or identifier strings) and labels, for password handling etc.
 * Also provides useful common data/variables such as program version, fixed regular expression validators, weather definitions etc.
 */
class Aux
{
public:
    enum class Precipitation : int8_t;
    enum class Cloudiness : int8_t;
    enum class WindStrength : int8_t;

public:
    Aux() = delete; ///< Deleted constructor.
    //
    static bool parseProgramVersion(QString pVersion, int& pMajor, int& pMinor,
                                    int& pPatch, char& pType);                  ///< \brief Check format of program version string and
                                                                                ///  extract major/minor versions, patch and type.
    //
    static bool checkPassword(const QString& pHash, const QString& pSalt,
                              QWidget *const pParent);                  ///< Prompt for a password and check if hash matches reference.
    static void generatePasswordHash(const QString& pPhrase,
                                     QString& pNewHash, QString& pNewSalt); ///< Generate new salt and hash based on given passphrase.
    //
    static QTime roundQuarterHour(QTime pTime);                     ///< Round a time to the nearest quarter.
    //
    static void latexEscapeSpecialChars(QString& pString);          ///< Escape special LaTeX characters.
    static void latexFixLineBreaks(QString& pString);               ///< Convert line breaks into double line breaks.
    static void latexFixLineBreaksUline(QString& pString);          ///< Add "\hfill" before line breaks to expand "\ulem" underline.
    static void latexFixLineBreaksNoLineBreaks(QString& pString);   ///< Remove all line breaks.
    //
    static QString precipitationToLabel(Precipitation pPrecip);         ///< Get the label for a precipitation type.
    static Precipitation labelToPrecipitation(const QString& pPrecip);  ///< Get the precipitation type from its label.
    static QString cloudinessToLabel(Cloudiness pClouds);               ///< Get the label for a cloudiness level.
    static Cloudiness labelToCloudiness(const QString& pClouds);        ///< Get the clodiness level from its label.
    static QString windStrengthToLabel(WindStrength pWind);             ///< Get the label for a wind strength.
    static WindStrength labelToWindStrength(const QString& pWind);      ///< Get the wind strength from its label.
    //
    static void stationIdentFromNameLocation(const QString& pName, const QString& pLocation,
                                             QString& pIdent);                      ///< \brief Get a station identifier
                                                                                    /// composed of its name and location.
    static bool stationNameLocationFromIdent(const QString& pIdent,
                                             QString& pName, QString& pLocation);   ///< \brief Get the station name and
                                                                                    ///  location from its identifier.
    //
    static void stationLabelFromNameLocation(const QString& pName, const QString& pLocation,
                                             QString& pLabel);                      ///< \brief Get a station (combo box) label
                                                                                    /// composed of its name and location.
    static bool stationNameLocationFromLabel(const QString& pLabel,
                                             QString& pName, QString& pLocation);   ///< \brief Get the station name and
                                                                                    ///  location from its (combo box) label.
    //
    static QString stationLabelFromIdent(const QString& pIdent);        ///< Get a formatted combo box label from a station identifier.
    static QString stationIdentFromLabel(const QString& pLabel);        ///< Get the staion identifier from a combo box label.
    //
    static QString createDocumentListString(const std::vector<std::pair<QString, QString>>& pDocs); ///< \brief Create a string
                                                                                                    ///  containing a list of document
                                                                                                    /// names and their file paths.
    static std::vector<std::pair<QString, QString>> parseDocumentListString(const QString& pDocStr);    ///< \brief Extract a list of
                                                                                                        ///  document names and their
                                                                                                        ///  file paths from a string.

public:
    static const int programVersionMajor;   ///< Major program version.
    static const int programVersionMinor;   ///< Minor program version.
    static const int programVersionPatch;   ///< Patch number.
    static const char programVersionType;   ///< Release type ('a' ^= alpha, 'b' ^= beta, 'c' ^= release candidate, '-' ^= normal).
    //
    static const QString programVersionString;          ///< Program version formatted as "MAJ.MIN[abc].PATCH".
    static const QString programVersionStringPretty;    ///< Program version formatted as "MAJ.MIN[abc]", if PATCH is zero.
    //
    static const QRegularExpressionValidator locationsValidator;            ///< Validator for station locations.
    static const QRegularExpressionValidator namesValidator;                ///< Validator for station/boat names.
    static const QRegularExpressionValidator personNamesValidator;          ///< Validator for person names.
    static const QRegularExpressionValidator stationItentifiersValidator;   ///< Validator for identifier "LOCATION + '%' + NAME".
    static const QRegularExpressionValidator radioCallNamesValidator;       ///< Validator for radio call names.
    static const QRegularExpressionValidator boatAcronymsValidator;         ///< Validator for boat name acronyms.
    static const QRegularExpressionValidator fuelTypesValidator;            ///< Validator for boat fuels.
    static const QRegularExpressionValidator membershipNumbersValidator;    ///< Validator for personnel membership numbers.
    static const QRegularExpressionValidator extIdentSuffixesValidator;     ///< Validator for external person identifier suffixes.
    static const QRegularExpressionValidator assignmentNumbersValidator;    ///< Validator for directing center assignment numbers.
    static const QRegularExpressionValidator programVersionsValidator;      ///< Validator for program version strings.
    //
    static const QStringList boatFuelTypePresets;                           ///< Example fuel types to use as combo box presets.
    static const QStringList boatDrivePurposePresets;                       ///< Example boat drive purposes to use as combo box presets.

public:
    /*!
     * \brief Properties of a station.
     *
     * Groups information about a specific station.
     * Structure is analog to the stations database records.
     */
    struct Station
    {
        QString location;               ///< Location of the station.
        QString name;                   ///< Name of the station.
        QString localGroup;             ///< Local group (user / owner of the station).
        QString districtAssociation;    ///< District association of the local group.
        QString radioCallName;          ///< %Station's radio call name.
        QString radioCallNameAlt;       ///< %Station's alternative radio call name.
    };
    /*!
     * \brief Properties of a boat.
     *
     * Groups information about a specific boat.
     * Structure is analog to the boats database records.
     */
    struct Boat
    {
        QString name;               ///< Name of the boat.
        QString acronym;            ///< Acronym for the boat.
        QString type;               ///< Type of the boat (manufacturer / model).
        QString fuelType;           ///< Required fuel for the boat.
        QString radioCallName;      ///< %Boat's radio call name.
        QString radioCallNameAlt;   ///< %Boat's alternative radio call name.
        QString homeStation;        ///< The station that the boat is associated with.
    };
    //
    /*!
     * \brief A number of precipitation types.
     *
     * The available values describe different kinds of precipitation types.
     * The enum class can be used to describe weather information for the Report.
     */
    enum class Precipitation : int8_t {
        _NONE = 0,              ///< None
        _FOG = 1,               ///< "Nebel"
        _DEW = 2,               ///< "Tau"
        _HOAR_FROST = 3,        ///< "Reif"
        _RIME_ICE = 4,          ///< "Raureif"
        _CLEAR_ICE = 5,         ///< "Klareis"
        _DRIZZLE = 6,           ///< "Nieselregen"
        _LIGHT_RAIN = 7,        ///< "Leichter Regen"
        _MEDIUM_RAIN = 8,       ///< "Mittlerer Regen"
        _HEAVY_RAIN = 9,        ///< "Starker Regen"
        _FREEZING_RAIN = 10,    ///< "Gefrierender Regen"
        _ICE_PELLETS = 11,      ///< "Eiskörner"
        _HAIL = 12,             ///< "Hagel"
        _SOFT_HAIL = 13,        ///< "Graupel"
        _SNOW = 14,             ///< "Schnee"
        _SLEET = 15,            ///< "Schneeregen"
        _DIAMOND_DUST = 16,     ///< "Polarschnee"
    };
    /*!
     * \brief A number of cloudiness levels.
     *
     * The available values describe different levels of cloudiness (plus some special conditions like thunderclouds etc.).
     * The enum class can be used to describe weather information for the Report.
     */
    enum class Cloudiness : int8_t {
        _CLOUDLESS = 0,             ///< "Wolkenlos"
        _SUNNY = 1,                 ///< "Sonnig"
        _FAIR = 2,                  ///< "Heiter"
        _SLIGHTLY_CLOUDY = 3,       ///< "Leicht bewölkt"
        _MODERATELY_CLOUDY = 4,     ///< "Wolkig"
        _CONSIDERABLY_CLOUDY = 5,   ///< "Bewölkt"
        _MOSTLY_CLOUDY = 6,         ///< "Stark bewölkt"
        _NEARLY_OVERCAST = 7,       ///< "Fast bedeckt"
        _FULLY_OVERCAST = 8,        ///< "Bedeckt"
        //
        _THUNDERCLOUDS = 50,        ///< "Gewitterwolken"
        _VARIABLE = 100             ///< "Wechselnd bewölkt"
    };
    /*!
     * \brief A number of wind strengths.
     *
     * The available values describe different levels of wind strength.
     * The enum class can be used to describe weather information for the Report.
     */
    enum class WindStrength : int8_t {
        _CALM = 0,              ///< "Windstille"
        _LIGHT_AIR = 1,         ///< "Leiser Zug"
        _LIGHT_BREEZE = 2,      ///< "Leichte Brise"
        _GENTLE_BREEZE = 3,     ///< "Schwache Brise"
        _MODERATE_BREEZE = 4,   ///< "Mäßige Brise"
        _FRESH_BREEZE = 5,      ///< "Frische Brise"
        _STRONG_BREEZE = 6,     ///< "Starker Wind"
        _MODERATE_GALE = 7,     ///< "Steifer Wind"
        _FRESH_GALE = 8,        ///< "Stürmischer Wind"
        _STRONG_GALE = 9,       ///< "Sturm"
        _WHOLE_GALE = 10,       ///< "Schwerer Sturm"
        _STORM = 11,            ///< "Orkanartiger Sturm"
        _HURRICANE = 12         ///< "Orkan"
    };

public:
    /*!
     * \brief Loop over precipitation types and execute a specified function for each type.
     *
     * For each precipitation type the \p pFunction is called with first parameter being the type
     * and perhaps further parameters \p pArgs, i.e. \p pFunction(precipType, pArgs...).
     * Non-void return values will be discarded. You may use use \p pArgs to communicate results of the function calls.
     *
     * \tparam FuncT Specific type of a FunctionObject (\p pFunction) that shall be called for each precipitation type.
     * \tparam ...Args Types of all additional parameters for \p pFunction if there are any.
     *
     * \param pFunction Any FunctionObject that accepts at least enum class values of Precipitation as the first parameter
     *                  and optionally also further parameters \p pArgs. A lambda expression may be used.
     *                  The function's return value will be discarded.
     *                  Note: (some of) \p pArgs can be passed by reference in order to communicate the results.
     * \param pArgs Optional arguments that will be passed to \p pFunction in addition to the precipitation type.
     */
    template <typename FuncT, typename ... Args>
    static void iteratePrecipitationTypes(FuncT pFunction, Args& ... pArgs)
    {
        for (Precipitation prec : {Precipitation::_NONE,
                                   Precipitation::_FOG,
                                   Precipitation::_DEW,
                                   Precipitation::_HOAR_FROST,
                                   Precipitation::_RIME_ICE,
                                   Precipitation::_CLEAR_ICE,
                                   Precipitation::_DRIZZLE,
                                   Precipitation::_LIGHT_RAIN,
                                   Precipitation::_MEDIUM_RAIN,
                                   Precipitation::_HEAVY_RAIN,
                                   Precipitation::_FREEZING_RAIN,
                                   Precipitation::_ICE_PELLETS,
                                   Precipitation::_HAIL,
                                   Precipitation::_SOFT_HAIL,
                                   Precipitation::_SNOW,
                                   Precipitation::_SLEET,
                                   Precipitation::_DIAMOND_DUST})
        {
            pFunction(prec, pArgs ...);
        }
    }
    /*!
     * \brief Loop over cloudiness levels and execute a specified function for each level.
     *
     * For each cloudiness level the \p pFunction is called with first parameter being the level
     * and perhaps further parameters \p pArgs, i.e. \p pFunction(cloudLevel, pArgs...).
     * Non-void return values will be discarded. You may use use \p pArgs to communicate results of the function calls.
     *
     * \tparam FuncT Specific type of a FunctionObject (\p pFunction) that shall be called for each cloudiness level.
     * \tparam ...Args Types of all additional parameters for \p pFunction if there are any.
     *
     * \param pFunction Any FunctionObject that accepts at least enum class values of Cloudiness as the first parameter
     *                  and optionally also further parameters \p pArgs. A lambda expression may be used.
     *                  The function's return value will be discarded.
     *                  Note: (some of) \p pArgs can be passed by reference in order to communicate the results.
     * \param pArgs Optional arguments that will be passed to \p pFunction in addition to the cloudiness level.
     */
    template <typename FuncT, typename ... Args>
    static void iterateCloudinessLevels(FuncT pFunction, Args& ... pArgs)
    {
        for (Cloudiness clouds : {Cloudiness::_CLOUDLESS,
                                  Cloudiness::_SUNNY,
                                  Cloudiness::_FAIR,
                                  Cloudiness::_SLIGHTLY_CLOUDY,
                                  Cloudiness::_MODERATELY_CLOUDY,
                                  Cloudiness::_CONSIDERABLY_CLOUDY,
                                  Cloudiness::_MOSTLY_CLOUDY,
                                  Cloudiness::_NEARLY_OVERCAST,
                                  Cloudiness::_FULLY_OVERCAST,
                                  Cloudiness::_THUNDERCLOUDS,
                                  Cloudiness::_VARIABLE})
        {
            pFunction(clouds, pArgs ...);
        }
    }
    /*!
     * \brief Loop over wind strengths and execute a specified function for each strength.
     *
     * For each wind strength the \p pFunction is called with first parameter being the strength
     * and perhaps further parameters \p pArgs, i.e. \p pFunction(windStrength, pArgs...).
     * Non-void return values will be discarded. You may use use \p pArgs to communicate results of the function calls.
     *
     * \tparam FuncT Specific type of a FunctionObject (\p pFunction) that shall be called for each wind strength.
     * \tparam ...Args Types of all additional parameters for \p pFunction if there are any.
     *
     * \param pFunction Any FunctionObject that accepts at least enum class values of WindStrength as the first parameter
     *                  and optionally also further parameters \p pArgs. A lambda expression may be used.
     *                  The function's return value will be discarded.
     *                  Note: (some of) \p pArgs can be passed by reference in order to communicate the results.
     * \param pArgs Optional arguments that will be passed to \p pFunction in addition to the wind strength.
     */
    template <typename FuncT, typename ... Args>
    static void iterateWindStrengths(FuncT pFunction, Args& ... pArgs)
    {
        for (WindStrength wind : {WindStrength::_CALM,
                                  WindStrength::_LIGHT_AIR,
                                  WindStrength::_LIGHT_BREEZE,
                                  WindStrength::_GENTLE_BREEZE,
                                  WindStrength::_MODERATE_BREEZE,
                                  WindStrength::_FRESH_BREEZE,
                                  WindStrength::_STRONG_BREEZE,
                                  WindStrength::_MODERATE_GALE,
                                  WindStrength::_FRESH_GALE,
                                  WindStrength::_STRONG_GALE,
                                  WindStrength::_WHOLE_GALE,
                                  WindStrength::_STORM,
                                  WindStrength::_HURRICANE})
        {
            pFunction(wind, pArgs ...);
        }
    }
};

#endif // AUXIL_H
