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

#ifndef QUALIFICATIONCHECKER_H
#define QUALIFICATIONCHECKER_H

#include "person.h"

/*!
 * \brief Check, if a Person has sufficient qualifications for a specific function.
 *
 * For certain personnel functions (Person::Function) or boat functions (Person::BoatFunction)
 * a person needs to have special qualifications (see Person::Qualifications).
 * Which qualifications are required for which function, is defined by this class and can be checked
 * with checkPersonnelFunction() and checkBoatFunction().
 * To check whether a person can be a boat drive's boatman the function checkBoatman() should be used.
 */
class QualificationChecker
{
public:
    QualificationChecker() = delete;    ///< Deleted constructor.
    //
    static bool checkPersonnelFunction(Person::Function pFunction,
                                       const Person::Qualifications& pQualifications);  ///< \brief Check if a person is qualified
                                                                                        ///  for a certain personnel function.
    static bool checkBoatFunction(Person::BoatFunction pFunction,
                                  const Person::Qualifications& pQualifications);       ///< \brief Check if a person is qualified
                                                                                        ///  for a certain boat function.
    static bool checkBoatman(const Person::Qualifications& pQualifications);            ///< \brief Check if a person is qualified
                                                                                        ///  to be a boatman.
};

#endif // QUALIFICATIONCHECKER_H
