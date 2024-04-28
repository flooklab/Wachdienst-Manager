/*
/////////////////////////////////////////////////////////////////////////////////////////
//
//  This file is part of Wachdienst-Manager, a program to manage DLRG watch duty reports.
//  Copyright (C) 2021–2024 M. Frohne
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

#include "qualificationchecker.h"

#include "settingscache.h"

//Public

/*!
 * \brief Check if a person is qualified for a certain personnel function.
 *
 * \param pFunction Requested personnel function.
 * \param pQualifications Qualifications of concerned person.
 * \return If \p pFunction is allowed according to \p pQualifications.
 */
bool QualificationChecker::checkPersonnelFunction(const Person::Function pFunction, const Person::Qualifications& pQualifications)
{
    switch (pFunction)
    {
        case Person::Function::_WF:
            if (pQualifications.wf)
                return true;
            return false;
        case Person::Function::_SL:
            if (checkBoatman(pQualifications))
                return true;
            return false;
        case Person::Function::_BF:
            if (checkBoatman(pQualifications))
                return true;
            return false;
        case Person::Function::_WR:
            if (pQualifications.faWrd)
                return true;
            return false;
        case Person::Function::_RS:
            if (pQualifications.drsaS)
                return true;
            return false;
        case Person::Function::_PR:
            return true;
        case Person::Function::_SAN:
            if (pQualifications.sanA)
                return true;
            return false;
        case Person::Function::_FU:
            if (pQualifications.bos)
                return true;
            return false;
        case Person::Function::_SR:
            if (pQualifications.sr1)
                return true;
            return false;
        case Person::Function::_ET:
            if (pQualifications.et)
                return true;
            return false;
        case Person::Function::_FUD:
            if (pQualifications.zf)
                return true;
            return false;
        case Person::Function::_OTHER:
            return false;
        default:
            return false;
    }
}

/*!
 * \brief Check if a person is qualified for a certain boat function.
 *
 * \param pFunction Requested personnel function.
 * \param pQualifications Qualifications of concerned person.
 * \return If \p pFunction is allowed according to \p pQualifications.
 */
bool QualificationChecker::checkBoatFunction(const Person::BoatFunction pFunction, const Person::Qualifications& pQualifications)
{
    switch (pFunction)
    {
        case Person::BoatFunction::_BG:
            if (pQualifications.faWrd)
                return true;
            return false;
        case Person::BoatFunction::_RS:
            if (pQualifications.drsaS)
                return true;
            return false;
        case Person::BoatFunction::_PR:
            return true;
        case Person::BoatFunction::_SAN:
            if (pQualifications.sanA)
                return true;
            return false;
        case Person::BoatFunction::_SR:
            if (pQualifications.sr1)
                return true;
            return false;
        case Person::BoatFunction::_ET:
            if (pQualifications.et)
                return true;
            return false;
        case Person::BoatFunction::_OTHER:
            return false;
        default:
            return false;
    }
}

/*!
 * \brief Check if a person is qualified to be a boatman.
 *
 * \param pQualifications Qualifications of concerned person.
 * \return If person is allowed to be a boatman according to \p pQualifications.
 */
bool QualificationChecker::checkBoatman(const Person::Qualifications& pQualifications)
{
    QString boatmanRequiredLicense = SettingsCache::getStrSetting("app_personnel_minQualis_boatman");

    if (boatmanRequiredLicense == "A")
        return pQualifications.bfA;
    else if (boatmanRequiredLicense == "B")
        return pQualifications.bfB;
    else if (boatmanRequiredLicense == "A&B")
        return pQualifications.bfA && pQualifications.bfB;
    else if (boatmanRequiredLicense == "A|B")
        return pQualifications.bfA || pQualifications.bfB;

    return false;
}
