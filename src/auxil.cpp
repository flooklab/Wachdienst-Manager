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

#include "auxil.h"

//Initialize static class members

const int Aux::programVersionMajor = QString(PROGRAM_VERSION_MAJOR).toInt();
const int Aux::programVersionMinor = QString(PROGRAM_VERSION_MINOR).split(QRegularExpression("[abc]")).at(0).toInt();
const int Aux::programVersionPatch = QString(PROGRAM_VERSION_PATCH).toInt();
const char Aux::programVersionType = QString(PROGRAM_VERSION_MINOR).contains(QRegularExpression("[abc]")) ?
                                         QString(PROGRAM_VERSION_MINOR).back().toLatin1() : '-';
//
const QString Aux::programVersionString = QString::number(Aux::programVersionMajor) + "." +
                                          QString::number(Aux::programVersionMinor) +
                                          (Aux::programVersionType != '-' ? QString(Aux::programVersionType) : "") + "." +
                                          QString::number(Aux::programVersionPatch);
const QString Aux::programVersionStringPretty = QString::number(Aux::programVersionMajor) + "." +
                                                QString::number(Aux::programVersionMinor) +
                                                (Aux::programVersionType == '-' ? "" : (Aux::programVersionType == 'c' ? "RC" :
                                                                                        QString(Aux::programVersionType))) +
                                                ((Aux::programVersionPatch > 0) ?
                                                     ("." + QString::number(Aux::programVersionPatch)) : "");
//
const QRegularExpressionValidator Aux::locationsValidator =
        QRegularExpressionValidator(QRegularExpression("[a-zA-ZäöüÄÖÜßæåøÆÅØ\\s\\-\\/\\(\\)]+"));
const QRegularExpressionValidator Aux::namesValidator =
        QRegularExpressionValidator(QRegularExpression("[a-zA-ZäöüÄÖÜßæåøÆÅØ\\s\\d\\-\\/\\(\\)]+"));
const QRegularExpressionValidator Aux::personNamesValidator =
        QRegularExpressionValidator(QRegularExpression("[a-zA-ZäöüÄÖÜßæåøÆÅØ\\s\\-]+"));
const QRegularExpressionValidator Aux::stationItentifiersValidator =
    QRegularExpressionValidator(QRegularExpression("[a-zA-ZäöüÄÖÜßæåøÆÅØ\\s\\-\\/\\(\\)]+%[a-zA-ZäöüÄÖÜßæåøÆÅØ\\s\\d\\-\\/\\(\\)]+"));
const QRegularExpressionValidator Aux::radioCallNamesValidator =
        QRegularExpressionValidator(QRegularExpression("[a-zA-ZäöüÄÖÜßæåøÆÅØ\\s\\d\\-\\/\\(\\)]+"));
const QRegularExpressionValidator Aux::boatAcronymsValidator =
        QRegularExpressionValidator(QRegularExpression("[A-ZÄÖÜßÆÅØ]{3}\\d \\- RTB\\d"));
const QRegularExpressionValidator Aux::fuelTypesValidator =
        QRegularExpressionValidator(QRegularExpression("[a-zA-Z\\s\\d\\-]+"));
const QRegularExpressionValidator Aux::membershipNumbersValidator =
        QRegularExpressionValidator(QRegularExpression("[\\d]+\\.?[\\d]*"));
const QRegularExpressionValidator Aux::extIdentSuffixesValidator =
        QRegularExpressionValidator(QRegularExpression("[\\d]*"));
const QRegularExpressionValidator Aux::assignmentNumbersValidator =
        QRegularExpressionValidator(QRegularExpression("[\\d]*"));
const QRegularExpressionValidator Aux::programVersionsValidator =
        QRegularExpressionValidator(QRegularExpression("[\\d]+\\.[\\d]+[abc]?\\.[\\d]+"));
//
const QStringList Aux::boatFuelTypePresets = {"Super", "Super plus", "Normalbenzin", "Diesel"};
const QStringList Aux::boatDrivePurposePresets = {"Kontrollfahrt", "Begleitung Regatta", "Begleitung Jugendtraining",
                                                  "Tonnen setzen", "Tonnen einholen", "Übung", "Einsatz"};

//Public

/*!
 * \brief Check format of program version string and extract major/minor versions, patch and type.
 *
 * Parses a program version string of format "MAJ.MIN[abc].PATCH" provided via \p pVersion.
 * The major/minor/patch version numbers will be assigned to \p pMajor, \p pMinor and \p pPatch.
 * The release type \p pType is assigned an 'a'/'b'/'c' for an alpha/beta/release candidate
 * release and a '-' otherwise.
 *
 * Example: \p pVersion == "1.2b.3" yields \p pMajor = 3, \p pMinor = 2, \p pPatch = 3, \p pType = 'b'.
 *
 * \param pVersion The version string to be parsed.
 * \param pMajor Major program version.
 * \param pMinor Minor program version.
 * \param pPatch Version patch number.
 * \param pType Release type ('a' ^= alpha, 'b' ^= beta, 'c' ^= release candidate, '-' ^= normal).
 * \return If format is correct and parsing was successful.
 */
bool Aux::parseProgramVersion(QString pVersion, int& pMajor, int& pMinor, int& pPatch, char& pType)
{
    //Check first, if format is "MAJ.MIN[abc].PATCH"
    int tmpInt = 0;
    if (programVersionsValidator.validate(pVersion, tmpInt) != QValidator::State::Acceptable)
        return false;

    QStringList versionParts = pVersion.split('.');

    if (versionParts.size() != 3)
        return false;

    bool ok = true;

    pMajor = versionParts.at(0).toInt(&ok);
    if (!ok)
        return false;

    //Set type to '-', if normal version (no alpha, beta or release candidate)
    pType = '-';

    QString tMinor = versionParts.at(1);
    QString tMinorDigits;

    //Split type from minor version, if present
    for (int i = 0; i < tMinor.size(); ++i)
    {
        if (tMinor.at(i).isDigit())
            tMinorDigits.append(tMinor.at(i));
        else
        {
            switch (tMinor.at(i).toLatin1())
            {
                case 'a':
                    pType = 'a';
                    break;
                case 'b':
                    pType = 'b';
                    break;
                case 'c':
                    pType = 'c';
                    break;
                default:
                    return false;
            }
        }
    }

    pMinor = tMinorDigits.toInt(&ok);
    if (!ok)
        return false;

    pPatch = versionParts.at(2).toInt(&ok);
    if (!ok)
        return false;

    return true;
}

/*!
 * \brief Check, if two program versions are equal or if one version is earlier/later.
 *
 * Compares A <=> B for two program versions A and B. Patch numbers are not compared, if \p pIgnorePatch is true.
 *
 * \param pMajorA Major program version A.
 * \param pMinorA Minor program version A.
 * \param pPatchA Patch number A.
 * \param pMajorB Major program version B.
 * \param pMinorB Minor program version B.
 * \param pPatchB Patch number B.
 * \param pIgnorePatch Do not compare patch numbers?
 * \return -1 / 0 / +1, if A<B / A==B / A>B.
 */
int Aux::compareProgramVersions(int pMajorA, int pMinorA, int pPatchA, int pMajorB, int pMinorB, int pPatchB, bool pIgnorePatch)
{
    if (pMajorA < pMajorB)
        return -1;
    else if (pMajorA > pMajorB)
        return 1;
    else
    {
        if (pMinorA < pMinorB)
            return -1;
        else if (pMinorA > pMinorB)
            return 1;
        else
        {
            if (pIgnorePatch)
                return 0;
            else
            {
                if (pPatchA < pPatchB)
                    return -1;
                else if (pPatchA > pPatchB)
                    return 1;
                else
                    return 0;
            }
        }
    }
}

//

/*!
 * \brief Prompt for a password and check if hash matches reference.
 *
 * Derives a password hash from the entered password and salt \p pSalt
 * and compares it to the specified correct/reference hash \p pHash.
 *
 * \param pHash Reference hash of correct password.
 * \param pSalt Salt that was used to generate \p pHash.
 * \param pParent The parent widget for the password input dialog.
 * \return If entered password is correct.
 */
bool Aux::checkPassword(const QString& pHash, const QString& pSalt, QWidget *const pParent)
{
    QByteArray correctHash = QByteArray::fromBase64(pHash.toUtf8());
    QByteArray salt = QByteArray::fromBase64(pSalt.toUtf8());

    bool ok = false;
    QString phrase = QInputDialog::getText(pParent, "Passwort eingeben!", "Passwort:", QLineEdit::Password, "", &ok);

    if (!ok)
        return false;

    QByteArray testHash = QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Algorithm::Sha512,
                                                             phrase.toUtf8(), salt, 100000, 75);

    return (testHash == correctHash);
}

/*!
 * \brief Generate new salt and hash based on given passphrase.
 *
 * Randomly generates new salt \p pSalt and then derives new \p pHash from this and password \p pPhrase.
 *
 * \param pPhrase New password.
 * \param pNewHash New hash.
 * \param pNewSalt New salt.
 */
void Aux::generatePasswordHash(const QString& pPhrase, QString& pNewHash, QString& pNewSalt)
{
    QByteArray newSalt;

    QDataStream dataStream(&newSalt, QIODevice::WriteOnly);

    for (int i = 0; i < (16 / 8); ++i)
    {
        quint64 rnd = QRandomGenerator::global()->generate64();
        dataStream<<static_cast<quint8>(rnd >>  0);
        dataStream<<static_cast<quint8>(rnd >>  8);
        dataStream<<static_cast<quint8>(rnd >> 16);
        dataStream<<static_cast<quint8>(rnd >> 24);
        dataStream<<static_cast<quint8>(rnd >> 32);
        dataStream<<static_cast<quint8>(rnd >> 40);
        dataStream<<static_cast<quint8>(rnd >> 48);
        dataStream<<static_cast<quint8>(rnd >> 56);
    }

    QByteArray newHash = QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Algorithm::Sha512,
                                                            pPhrase.toUtf8(), newSalt, 100000, 75);

    pNewSalt = QString::fromUtf8(newSalt.toBase64());
    pNewHash = QString::fromUtf8(newHash.toBase64());
}

//

/*!
 * \brief Round a time to the nearest quarter.
 *
 * If the rounded time exceeds 23:59, the hour will be reset from 24 to 0.
 *
 * \param pTime Original time.
 * \return Rounded time.
 */
QTime Aux::roundQuarterHour(QTime pTime)
{
    int hours = pTime.hour();
    int minutes = pTime.minute();

    if (minutes <= 7)
        minutes = 0;
    else if (minutes <= 22)
        minutes = 15;
    else if (minutes <= 37)
        minutes = 30;
    else if (minutes <= 52)
        minutes = 45;
    else
    {
        minutes = 0;
        ++hours;
        if (hours == 24)
            hours = 0;
    }

    return QTime(hours, minutes);
}

//

/*!
 * \brief Escape special LaTeX characters.
 *
 * Allows safe insertion of normal text \p pString in a LaTeX document.
 *
 * \param pString Text to be modified.
 */
void Aux::latexEscapeSpecialChars(QString& pString)
{
    pString.replace("\\", "\\textbackslash{}"); //Need to replace backslashes first
    pString.replace("{", "\\{");
    pString.replace("}", "\\}");
    pString.replace("\\{\\}", "{}");            //Fix wrongly replaced "{}" after "textbackslash"
    pString.replace("#", "\\#");
    pString.replace("$", "\\$");
    pString.replace("%", "\\%");
    pString.replace("^", "\\^{}");
    pString.replace("&", "\\&");
    pString.replace("_", "\\_");
    pString.replace("~", "\\~{}");
}

/*!
 * \brief Convert line breaks into double line breaks.
 *
 * Allows to preserve display of line breaks from a text edit in LaTeX output document by
 * inserting double line breaks to obtain new paragraphs instead of ignored single line breaks.
 *
 * \param pString Text to be modified.
 */
void Aux::latexFixLineBreaks(QString& pString)
{
    pString.replace("\n", "\n\n");
}

/*!
 * \brief Add "\hfill" before line breaks to expand "\ulem" underline.
 *
 * Allows to preserve display of line breaks from a text edit in LaTeX output document
 * together with the expanding underline of the "\uline" command from the "ulem" package
 * when using \p pString within this command.
 *
 * \param pString Text to be modified.
 */
void Aux::latexFixLineBreaksUline(QString& pString)
{
    pString.replace("\n", "\\hfill{}\\mbox{}\\newline\n\\mbox{}");
}

/*!
 * \brief Remove all line breaks.
 *
 * Removes any line breaks from a text edit text in order to use \p pString
 * in a LaTeX command that does not allow line breaks.
 *
 * \param pString Text to be modified.
 */
void Aux::latexFixLineBreaksNoLineBreaks(QString& pString)
{
    pString.replace("\n", "");
}

//

/*!
 * \brief Get the label for a precipitation type.
 *
 * Get a (unique) nicely formatted label for \p pPrecip to e.g. show it in the application as a combo box item.
 * Converting back is possible using labelToPrecipitation().
 *
 * \param pPrecip The precipitation type to get a label for.
 * \return The corresponding label for \p pPrecip.
 */
QString Aux::precipitationToLabel(Precipitation pPrecip)
{
    switch (pPrecip)
    {
        case Precipitation::_NONE:
            return "Kein";
        case Precipitation::_FOG:
            return "Nebel";
        case Precipitation::_DEW:
            return "Tau";
        case Precipitation::_HOAR_FROST:
            return "Reif";
        case Precipitation::_RIME_ICE:
            return "Raureif";
        case Precipitation::_CLEAR_ICE:
            return "Klareis";
        case Precipitation::_DRIZZLE:
            return "Nieselregen";
        case Precipitation::_LIGHT_RAIN:
            return "Leichter Regen";
        case Precipitation::_MEDIUM_RAIN:
            return "Mittlerer Regen";
        case Precipitation::_HEAVY_RAIN:
            return "Starker Regen";
        case Precipitation::_FREEZING_RAIN:
            return "Gefrierender Regen";
        case Precipitation::_ICE_PELLETS:
            return "Eiskörner";
        case Precipitation::_HAIL:
            return "Hagel";
        case Precipitation::_SOFT_HAIL:
            return "Graupel";
        case Precipitation::_SNOW:
            return "Schnee";
        case Precipitation::_SLEET:
            return "Schneeregen";
        case Precipitation::_DIAMOND_DUST:
            return "Polarschnee";
        default:
            return "Kein";
    }
}

/*!
 * \brief Get the precipitation type from its label.
 *
 * Get the precipitation type corresponding to a (unique) label \p pPrecip,
 * which can in turn be obtained from precipitationToLabel().
 *
 * \param pPrecip The label representing the requested precipitation type.
 * \return The precipitation type corresponding to label \p pPrecip.
 */
Aux::Precipitation Aux::labelToPrecipitation(const QString& pPrecip)
{
    if (pPrecip == "Kein")
        return Precipitation::_NONE;
    else if (pPrecip == "Nebel")
        return Precipitation::_FOG;
    else if (pPrecip == "Tau")
        return Precipitation::_DEW;
    else if (pPrecip == "Reif")
        return Precipitation::_HOAR_FROST;
    else if (pPrecip == "Raureif")
        return Precipitation::_RIME_ICE;
    else if (pPrecip == "Klareis")
        return Precipitation::_CLEAR_ICE;
    else if (pPrecip == "Nieselregen")
        return Precipitation::_DRIZZLE;
    else if (pPrecip == "Leichter Regen")
        return Precipitation::_LIGHT_RAIN;
    else if (pPrecip == "Mittlerer Regen")
        return Precipitation::_MEDIUM_RAIN;
    else if (pPrecip == "Starker Regen")
        return Precipitation::_HEAVY_RAIN;
    else if (pPrecip == "Gefrierender Regen")
        return Precipitation::_FREEZING_RAIN;
    else if (pPrecip == "Eiskörner")
        return Precipitation::_ICE_PELLETS;
    else if (pPrecip == "Hagel")
        return Precipitation::_HAIL;
    else if (pPrecip == "Graupel")
        return Precipitation::_SOFT_HAIL;
    else if (pPrecip == "Schnee")
        return Precipitation::_SNOW;
    else if (pPrecip == "Schneeregen")
        return Precipitation::_SLEET;
    else if (pPrecip == "Polarschnee")
        return Precipitation::_DIAMOND_DUST;
    else
        return Precipitation::_NONE;
}

/*!
 * \brief Get the label for a cloudiness level.
 *
 * Get a (unique) nicely formatted label for \p pClouds to e.g. show it in the application as a combo box item.
 * Converting back is possible using labelToCloudiness().
 *
 * \param pClouds The cloudiness level to get a label for.
 * \return The corresponding label for \p pClouds.
 */
QString Aux::cloudinessToLabel(Cloudiness pClouds)
{
    switch (pClouds)
    {
        case Cloudiness::_CLOUDLESS:
            return "Wolkenlos";
        case Cloudiness::_SUNNY:
            return "Sonnig";
        case Cloudiness::_FAIR:
            return "Heiter";
        case Cloudiness::_SLIGHTLY_CLOUDY:
            return "Leicht bewölkt";
        case Cloudiness::_MODERATELY_CLOUDY:
            return "Wolkig";
        case Cloudiness::_CONSIDERABLY_CLOUDY:
            return "Bewölkt";
        case Cloudiness::_MOSTLY_CLOUDY:
            return "Stark bewölkt";
        case Cloudiness::_NEARLY_OVERCAST:
            return "Fast bedeckt";
        case Cloudiness::_FULLY_OVERCAST:
            return "Bedeckt";
        case Cloudiness::_THUNDERCLOUDS:
            return "Gewitterwolken";
        case Cloudiness::_VARIABLE:
            return "Wechselnd bewölkt";
        default:
            return "Wolkenlos";
    }
}

/*!
 * \brief Get the clodiness level from its label.
 *
 * Get the cloudiness level corresponding to a (unique) label \p pClouds,
 * which can in turn be obtained from cloudinessToLabel().
 *
 * \param pClouds The label representing the requested cloudiness level.
 * \return The cloudiness level corresponding to label \p pClouds.
 */
Aux::Cloudiness Aux::labelToCloudiness(const QString& pClouds)
{
    if (pClouds == "Wolkenlos")
        return Cloudiness::_CLOUDLESS;
    else if (pClouds == "Sonnig")
        return Cloudiness::_SUNNY;
    else if (pClouds == "Heiter")
        return Cloudiness::_FAIR;
    else if (pClouds == "Leicht bewölkt")
        return Cloudiness::_SLIGHTLY_CLOUDY;
    else if (pClouds == "Wolkig")
        return Cloudiness::_MODERATELY_CLOUDY;
    else if (pClouds == "Bewölkt")
        return Cloudiness::_CONSIDERABLY_CLOUDY;
    else if (pClouds == "Stark bewölkt")
        return Cloudiness::_MOSTLY_CLOUDY;
    else if (pClouds == "Fast bedeckt")
        return Cloudiness::_NEARLY_OVERCAST;
    else if (pClouds == "Bedeckt")
        return Cloudiness::_FULLY_OVERCAST;
    else if (pClouds == "Gewitterwolken")
        return Cloudiness::_THUNDERCLOUDS;
    else if (pClouds == "Wechselnd bewölkt")
        return Cloudiness::_VARIABLE;
    else
        return Cloudiness::_CLOUDLESS;
}

/*!
 * \brief Get the label for a wind strength.
 *
 * Get a (unique) nicely formatted label for \p pWind to e.g. show it in the application as a combo box item.
 * Converting back is possible using labelToWindStrength().
 *
 * \param pWind The wind strength to get a label for.
 * \return The corresponding label for \p pWind.
 */
QString Aux::windStrengthToLabel(WindStrength pWind)
{
    switch (pWind)
    {
        case WindStrength::_CALM:
            return "0 Bft (Windstille)";
        case WindStrength::_LIGHT_AIR:
            return "1 Bft (Leiser Zug)";
        case WindStrength::_LIGHT_BREEZE:
            return "2 Bft (Leichte Brise)";
        case WindStrength::_GENTLE_BREEZE:
            return "3 Bft (Schwache Brise)";
        case WindStrength::_MODERATE_BREEZE:
            return "4 Bft (Mäßige Brise)";
        case WindStrength::_FRESH_BREEZE:
            return "5 Bft (Frische Brise)";
        case WindStrength::_STRONG_BREEZE:
            return "6 Bft (Starker Wind)";
        case WindStrength::_MODERATE_GALE:
            return "7 Bft (Steifer Wind)";
        case WindStrength::_FRESH_GALE:
            return "8 Bft (Stürmischer Wind)";
        case WindStrength::_STRONG_GALE:
            return "9 Bft (Sturm)";
        case WindStrength::_WHOLE_GALE:
            return "10 Bft (Schwerer Sturm)";
        case WindStrength::_STORM:
            return "11 Bft (Orkanartiger Sturm)";
        case WindStrength::_HURRICANE:
            return "12 Bft (Orkan)";
        default:
            return "0 Bft (Windstille)";
    }
}

/*!
 * \brief Get the wind strength from its label.
 *
 * Get the wind strength corresponding to a (unique) label \p pWind,
 * which can in turn be obtained from windStrengthToLabel().
 *
 * \param pWind The label representing the requested wind strength.
 * \return The wind strength corresponding to label \p pWind.
 */
Aux::WindStrength Aux::labelToWindStrength(const QString& pWind)
{
    if (pWind == "0 Bft (Windstille)")
        return WindStrength::_CALM;
    else if (pWind == "1 Bft (Leiser Zug)")
        return WindStrength::_LIGHT_AIR;
    else if (pWind == "2 Bft (Leichte Brise)")
        return WindStrength::_LIGHT_BREEZE;
    else if (pWind == "3 Bft (Schwache Brise)")
        return WindStrength::_GENTLE_BREEZE;
    else if (pWind == "4 Bft (Mäßige Brise)")
        return WindStrength::_MODERATE_BREEZE;
    else if (pWind == "5 Bft (Frische Brise)")
        return WindStrength::_FRESH_BREEZE;
    else if (pWind == "6 Bft (Starker Wind)")
        return WindStrength::_STRONG_BREEZE;
    else if (pWind == "7 Bft (Steifer Wind)")
        return WindStrength::_MODERATE_GALE;
    else if (pWind == "8 Bft (Stürmischer Wind)")
        return WindStrength::_FRESH_GALE;
    else if (pWind == "9 Bft (Sturm)")
        return WindStrength::_STRONG_GALE;
    else if (pWind == "10 Bft (Schwerer Sturm)")
        return WindStrength::_WHOLE_GALE;
    else if (pWind == "11 Bft (Orkanartiger Sturm)")
        return WindStrength::_STORM;
    else if (pWind == "12 Bft (Orkan)")
        return WindStrength::_HURRICANE;
    else
        return WindStrength::_CALM;
}

//

/*!
 * \brief Get a station identifier composed of its name and location.
 *
 * Trimmed \p pLocation and trimmed \p pName will be concatenated using the separator '%%' and assigned to \p pIdent.
 *
 * \param pName Name of the station (should not contain '%%').
 * \param pLocation Location of the station (should not contain '%%').
 * \param pIdent Destination for the station identifier.
 */
void Aux::stationIdentFromNameLocation(const QString& pName, const QString& pLocation, QString& pIdent)
{
    pIdent = pLocation.trimmed() + "%" + pName.trimmed();
}

/*!
 * \brief Get the station name and location from its identifier.
 *
 * Performs the inverse operation of stationIdentFromNameLocation().
 * \p pIdent is split at '%%' and location and name assigned to \p pLocation and \p pName.
 * If this fails or the resulting (trimmed) location or name are empty, the function returns false.
 *
 * \param pIdent Station identifier (see stationIdentFromNameLocation()).
 * \param pName Extracted name of the station.
 * \param pLocation Extracted location of the station.
 * \return If conversion successful and location/name not empty.
 */
bool Aux::stationNameLocationFromIdent(const QString& pIdent, QString& pName, QString& pLocation)
{
    QStringList tSplit = pIdent.split('%');

    if (tSplit.size() != 2 || tSplit.at(0).trimmed() == "" || tSplit.at(1).trimmed() == "")
        return false;

    pLocation = tSplit.at(0).trimmed();
    pName = tSplit.at(1).trimmed();

    return true;
}

//

/*!
 * \brief Get a station (combo box) label composed of its name and location.
 *
 * Trimmed \p pLocation and trimmed \p pName will be combined with format "Location [Name]" and this is assigned to \p pLabel.
 *
 * \param pName Name of the station (should not contain '%%').
 * \param pLocation Location of the station (should not contain '%%').
 * \param pLabel Destination for the station label.
 */
void Aux::stationLabelFromNameLocation(const QString& pName, const QString& pLocation, QString& pLabel)
{
    pLabel = pLocation.trimmed() + " [" + pName.trimmed() + "]";
}

/*!
 * \brief Get the station name and location from its (combo box) label.
 *
 * Performs the inverse operation of stationLabelFromNameLocation().
 * Location and name are extracted and assigned to \p pLocation and \p pName.
 * If this fails, the function returns false.
 *
 * \param pLabel Station label (see stationLabelFromNameLocation()).
 * \param pName Extracted name of the station.
 * \param pLocation Extracted location of the station.
 * \return If conversion successful.
 */
bool Aux::stationNameLocationFromLabel(const QString& pLabel, QString& pName, QString& pLocation)
{
    QStringList tSplit = pLabel.split('[');

    if (tSplit.size() != 2 || !tSplit.at(1).endsWith(']'))
        return false;

    pLocation = tSplit.at(0).trimmed();
    pName = tSplit.at(1);
    pName.chop(1);
    pName = pName.trimmed();

    return true;
}

//

/*!
 * \brief Get a formatted combo box label from a station identifier.
 *
 * Wrapper for obtaining station combo box labels from identifiers using
 * Aux::stationNameLocationFromIdent() and
 * Aux::stationLabelFromNameLocation().
 *
 * Ignores their return codes (assumes correct formatting).
 *
 * \param pIdent A station identifier.
 * \return A station combo box label.
 */
QString Aux::stationLabelFromIdent(const QString& pIdent)
{
    QString tName, tLocation, tLabel;
    stationNameLocationFromIdent(pIdent, tName, tLocation);
    stationLabelFromNameLocation(tName, tLocation, tLabel);
    return tLabel;
}

/*!
 * \brief Get the staion identifier from a combo box label.
 *
 * Wrapper for obtaining station identifiers from combo box labels using
 * Aux::stationNameLocationFromLabel() and
 * Aux::stationIdentFromNameLocation().
 *
 * Ignores their return codes (assumes correct formatting).
 *
 * \param pLabel A station combo box label.
 * \return A station identifier.
 */
QString Aux::stationIdentFromLabel(const QString& pLabel)
{
    QString tName, tLocation, tIdent;
    stationNameLocationFromLabel(pLabel, tName, tLocation);
    stationIdentFromNameLocation(tName, tLocation, tIdent);
    return tIdent;
}

//

/*!
 * \brief Create a string containing a list of document names and their file paths.
 *
 * Each element of \p pDocs is a pair {name, path} of the name of a document and its file path.
 * For each pair i.e. for each document the name and path will be concatenated using the separator '%%'.
 * Then different documents will be concatenated using the separator '$'.
 * (Note: Hence '%%' and '$' should not be contained in the document names or paths.)
 *
 * Example: {{"P1_1", "P1_2"}, {"P2_1", "P2_2"}, ..., {"PN_1", "PN_2"}} yields "P1_1%P1_2$P2_1%P2_2$...$PN_1%PN_2".
 *
 * \param pDocs List of pairs of document names and paths.
 * \return All document names and paths formatted as a single string.
 */
QString Aux::createDocumentListString(const std::vector<std::pair<QString, QString>>& pDocs)
{
    QString docStr;

    for (const std::pair<QString, QString>& pair : pDocs)
        docStr += pair.first + "%" + pair.second + "$";

    if (docStr != "")
        docStr.chop(1);

    return docStr;
}

/*!
 * \brief Extract a list of document names and their file paths from a string.
 *
 * Performs the inverse operation of createDocumentListString().
 *
 * \p pDocStr will be split into a list of documents at all occurrences of '$'.
 * The name and path of each document is then obtained by splitting each of these parts at '%%'.
 * (Note: Hence correct result is only obtained, if encoded document names and paths do not contain any '%%' or '$'.)
 *
 * Example: "P1_1%P1_2$P2_1%P2_2$...$PN_1%PN_2" yields {{"P1_1", "P1_2"}, {"P2_1", "P2_2"}, ..., {"PN_1", "PN_2"}}.
 *
 * \param pDocStr String representing a list of document names and paths analog to result of createDocumentListString().
 * \return List of pairs of document names and paths analog to input of createDocumentListString().
 */
std::vector<std::pair<QString, QString>> Aux::parseDocumentListString(const QString& pDocStr)
{
    std::vector<std::pair<QString, QString>> docs;

    QStringList tDocList = pDocStr.split('$', Qt::SkipEmptyParts);

    for (int i = 0; i < tDocList.size(); ++i)
    {
        QStringList tList = tDocList.at(i).split('%', Qt::KeepEmptyParts);

        //Simply fix broken entries
        while (tList.size() > 2)
            tList.removeLast();
        while (tList.size() < 2)
            tList.append("");

        const QString& tDocName = tList.at(0);
        const QString& tDocFile = tList.at(1);

        docs.push_back({tDocName, tDocFile});
    }

    return docs;
}
