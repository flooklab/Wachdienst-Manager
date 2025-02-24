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

#ifndef WDMGR_AUXIL_H
#define WDMGR_AUXIL_H

#include <QRegularExpressionValidator>
#include <QString>
#include <QStringList>
#include <Qt>
#include <QTime>
#include <QWidget>

#include <utility>
#include <vector>

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
    enum class WindDirection : int8_t;

public:
    Aux() = delete; ///< Deleted constructor.
    //
    static bool parseProgramVersion(QString pVersion, int& pMajor, int& pMinor,
                                    int& pPatch, char& pType);                  ///< \brief Check format of program version string and
                                                                                ///  extract major/minor versions, patch and type.
    static int compareProgramVersions(int pMajorA, int pMinorA, int pPatchA,
                                      int pMajorB, int pMinorB, int pPatchB, bool pIgnorePatch = false);    ///< \brief Check, if two
                                                                                                    /// program versions are equal or
                                                                                                    /// if one version is earlier/later.
    //
    static bool checkPassword(const QString& pHash, const QString& pSalt,
                              QWidget* pParent);                    ///< Prompt for a password and check if hash matches reference.
    static void generatePasswordHash(const QString& pPhrase,
                                     QString& pNewHash, QString& pNewSalt); ///< Generate new salt and hash based on given passphrase.
    //
    static QTime roundQuarterHour(QTime pTime);                     ///< Round a time to the nearest quarter.
    //
    static Qt::CheckState checkStateFromChecked(bool pChecked);     ///< Convert "checkedness" of check box to Qt::CheckState.
    static bool checkedFromCheckState(Qt::CheckState pCheckState);  ///< Convert Qt::CheckState to "checkedness" of check box.
    //
    static void latexEscapeSpecialChars(QString& pString);          ///< Escape special LaTeX characters.
    static void latexFixLineBreaks(QString& pString);               ///< Convert line breaks into double line breaks.
    static void latexFixLineBreaksUline(QString& pString);          ///< Add "\hfill" before line breaks to expand "\ulem" underline.
    static void latexFixLineBreaksNoLineBreaks(QString& pString);   ///< Remove all line breaks.
    static void latexUseHyphdash(QString& pString);                 ///< Replace every dash with the "\Hyphdash" command.
    //
    static QString precipitationToLabel(Precipitation pPrecip);             ///< Get the label for a precipitation type.
    static Precipitation labelToPrecipitation(const QString& pPrecip);      ///< Get the precipitation type from its label.
    static QString cloudinessToLabel(Cloudiness pClouds);                   ///< Get the label for a cloudiness level.
    static Cloudiness labelToCloudiness(const QString& pClouds);            ///< Get the clodiness level from its label.
    static QString windStrengthToLabel(WindStrength pWind);                 ///< Get the label for a wind strength.
    static WindStrength labelToWindStrength(const QString& pWind);          ///< Get the wind strength from its label.
    static QString windDirectionToLabel(WindDirection pDirection);          ///< Get the label for a wind direction.
    static WindDirection labelToWindDirection(const QString& pDirection);   ///< Get the wind direction from its label.
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
    static const QString programVersionStringPretty;    ///< \brief Program version formatted as "MAJ.MIN (alpha|beta|RC).PATCH",
                                                        ///  leaving out the ".PATCH" part if PATCH is zero.
    static const QString fileFormatVersionString;       ///< Version of used report file format as "MAJ.MIN[abc].PATCH".
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
    static const QStringList reportFileNamePresets;                         ///< Example file name presets to use as combo box presets.
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
    enum class Precipitation : int8_t
    {
        None = 0,           ///< None.
        Fog = 1,            ///< "Nebel".
        Dew = 2,            ///< "Tau".
        HoarFrost = 3,      ///< "Reif".
        RimeIce = 4,        ///< "Raureif".
        ClearIce = 5,       ///< "Klareis".
        Drizzle = 6,        ///< "Nieselregen".
        LightRain = 7,      ///< "Leichter Regen".
        MediumRain = 8,     ///< "Mittlerer Regen".
        HeavyRain = 9,      ///< "Starker Regen".
        FreezingRain = 10,  ///< "Gefrierender Regen".
        IcePellets = 11,    ///< "Eiskörner".
        Hail = 12,          ///< "Hagel".
        SoftHail = 13,      ///< "Graupel".
        Snow = 14,          ///< "Schnee".
        Sleet = 15,         ///< "Schneeregen".
        DiamondDust = 16,   ///< "Polarschnee".
    };
    /*!
     * \brief A number of cloudiness levels.
     *
     * The available values describe different levels of cloudiness (plus some special conditions like thunderclouds etc.).
     * The enum class can be used to describe weather information for the Report.
     */
    enum class Cloudiness : int8_t
    {
        Cloudless = 0,          ///< "Wolkenlos".
        Sunny = 1,              ///< "Sonnig".
        Fair = 2,               ///< "Heiter".
        SlightlyCloudy = 3,     ///< "Leicht bewölkt".
        ModeratelyCloudy = 4,   ///< "Wolkig".
        ConsiderablyCloudy = 5, ///< "Bewölkt".
        MostlyCloudy = 6,       ///< "Stark bewölkt".
        NearlyOvercast = 7,     ///< "Fast bedeckt".
        FullyOvercast = 8,      ///< "Bedeckt".
        //
        Thunderclouds = 50,     ///< "Gewitterwolken".
        Variable = 100          ///< "Wechselnd bewölkt".
    };
    /*!
     * \brief A number of wind strengths.
     *
     * The available values describe different levels of wind strength.
     * The enum class can be used to describe weather information for the Report.
     */
    enum class WindStrength : int8_t
    {
        Calm = 0,              ///< "Windstille".
        LightAir = 1,         ///< "Leiser Zug".
        LightBreeze = 2,      ///< "Leichte Brise".
        GentleBreeze = 3,     ///< "Schwache Brise".
        ModerateBreeze = 4,   ///< "Mäßige Brise".
        FreshBreeze = 5,      ///< "Frische Brise".
        StrongBreeze = 6,     ///< "Starker Wind".
        NearGale = 7,         ///< "Steifer Wind".
        Gale = 8,              ///< "Stürmischer Wind".
        StrongGale = 9,       ///< "Sturm".
        Storm = 10,            ///< "Schwerer Sturm".
        ViolentStorm = 11,    ///< "Orkanartiger Sturm".
        Hurricane = 12         ///< "Orkan".
    };
    /*!
     * \brief A number of wind directions.
     *
     * The available values describe wind directions in the usual steps of 22.5 degrees.
     * The enum class can be used to describe weather information for the Report.
     */
    enum class WindDirection : int8_t
    {
        S = 0,          ///< "Süd".
        Sse = 1,        ///< "Südsüdost".
        Se = 2,         ///< "Südost".
        Ese = 3,        ///< "Ostsüdost".
        E = 4,          ///< "Ost".
        Ene = 5,        ///< "Ostnordost".
        Ne = 6,         ///< "Nordost".
        Nne = 7,        ///< "Nordnordost".
        N = 8,          ///< "Nord".
        Nnw = 9,        ///< "Nordnordwest".
        Nw = 10,        ///< "Nordwest".
        Wnw = 11,       ///< "Westnordwest".
        W = 12,         ///< "West".
        Wsw = 13,       ///< "Westsüdwest".
        Sw = 14,        ///< "Südwest".
        Ssw = 15,       ///< "Südsüdwest".
        //
        Variable = 100, ///< "Wechselnde Richtungen".
        //
        Unknown = 110   ///< Unknown.
    };

public:
    /*!
     * \brief Loop over precipitation types and execute a specified function for each type.
     *
     * For each precipitation type the \p pFunction is called with first parameter being the type
     * and perhaps further parameters \p pArgs, i.e. \p pFunction(precipType, pArgs...).
     * Non-void return values will be discarded. You may use \p pArgs to communicate results of the function calls.
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
    template <typename FuncT, typename... Args>
    static void iteratePrecipitationTypes(FuncT pFunction, Args&... pArgs)
    {
        for (Precipitation prec : {Precipitation::None,
                                   Precipitation::Fog,
                                   Precipitation::Dew,
                                   Precipitation::HoarFrost,
                                   Precipitation::RimeIce,
                                   Precipitation::ClearIce,
                                   Precipitation::Drizzle,
                                   Precipitation::LightRain,
                                   Precipitation::MediumRain,
                                   Precipitation::HeavyRain,
                                   Precipitation::FreezingRain,
                                   Precipitation::IcePellets,
                                   Precipitation::Hail,
                                   Precipitation::SoftHail,
                                   Precipitation::Snow,
                                   Precipitation::Sleet,
                                   Precipitation::DiamondDust})
        {
            pFunction(prec, pArgs...);
        }
    }
    /*!
     * \brief Loop over cloudiness levels and execute a specified function for each level.
     *
     * For each cloudiness level the \p pFunction is called with first parameter being the level
     * and perhaps further parameters \p pArgs, i.e. \p pFunction(cloudLevel, pArgs...).
     * Non-void return values will be discarded. You may use \p pArgs to communicate results of the function calls.
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
    template <typename FuncT, typename... Args>
    static void iterateCloudinessLevels(FuncT pFunction, Args&... pArgs)
    {
        for (Cloudiness clouds : {Cloudiness::Cloudless,
                                  Cloudiness::Sunny,
                                  Cloudiness::Fair,
                                  Cloudiness::SlightlyCloudy,
                                  Cloudiness::ModeratelyCloudy,
                                  Cloudiness::ConsiderablyCloudy,
                                  Cloudiness::MostlyCloudy,
                                  Cloudiness::NearlyOvercast,
                                  Cloudiness::FullyOvercast,
                                  Cloudiness::Thunderclouds,
                                  Cloudiness::Variable})
        {
            pFunction(clouds, pArgs...);
        }
    }
    /*!
     * \brief Loop over wind strengths and execute a specified function for each strength.
     *
     * For each wind strength the \p pFunction is called with first parameter being the strength
     * and perhaps further parameters \p pArgs, i.e. \p pFunction(windStrength, pArgs...).
     * Non-void return values will be discarded. You may use \p pArgs to communicate results of the function calls.
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
    template <typename FuncT, typename... Args>
    static void iterateWindStrengths(FuncT pFunction, Args&... pArgs)
    {
        for (WindStrength wind : {WindStrength::Calm,
                                  WindStrength::LightAir,
                                  WindStrength::LightBreeze,
                                  WindStrength::GentleBreeze,
                                  WindStrength::ModerateBreeze,
                                  WindStrength::FreshBreeze,
                                  WindStrength::StrongBreeze,
                                  WindStrength::NearGale,
                                  WindStrength::Gale,
                                  WindStrength::StrongGale,
                                  WindStrength::Storm,
                                  WindStrength::ViolentStorm,
                                  WindStrength::Hurricane})
        {
            pFunction(wind, pArgs...);
        }
    }
    /*!
     * \brief Loop over wind directions and execute a specified function for each direction.
     *
     * For each wind direction the \p pFunction is called with first parameter being the direction
     * and perhaps further parameters \p pArgs, i.e. \p pFunction(windDirection, pArgs...).
     * Non-void return values will be discarded. You may use \p pArgs to communicate results of the function calls.
     *
     * \tparam FuncT Specific type of a FunctionObject (\p pFunction) that shall be called for each wind direction.
     * \tparam ...Args Types of all additional parameters for \p pFunction if there are any.
     *
     * \param pFunction Any FunctionObject that accepts at least enum class values of WindDirection as the first parameter
     *                  and optionally also further parameters \p pArgs. A lambda expression may be used.
     *                  The function's return value will be discarded.
     *                  Note: (some of) \p pArgs can be passed by reference in order to communicate the results.
     * \param pArgs Optional arguments that will be passed to \p pFunction in addition to the wind direction.
     */
    template <typename FuncT, typename... Args>
    static void iterateWindDirections(FuncT pFunction, Args&... pArgs)
    {
        for (WindDirection dir : {WindDirection::Unknown,
                                  WindDirection::N,
                                  WindDirection::Nnw,
                                  WindDirection::Nw,
                                  WindDirection::Wnw,
                                  WindDirection::W,
                                  WindDirection::Wsw,
                                  WindDirection::Sw,
                                  WindDirection::Ssw,
                                  WindDirection::S,
                                  WindDirection::Sse,
                                  WindDirection::Se,
                                  WindDirection::Ese,
                                  WindDirection::E,
                                  WindDirection::Ene,
                                  WindDirection::Ne,
                                  WindDirection::Nne,
                                  WindDirection::Variable})
        {
            pFunction(dir, pArgs...);
        }
    }
};

#endif // WDMGR_AUXIL_H
