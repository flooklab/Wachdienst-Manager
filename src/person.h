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

#ifndef PERSON_H
#define PERSON_H

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QCryptographicHash>

/*!
 * \brief Information about a person of (internal or external) personnel.
 *
 * Defines a person by its full name, its (unique) identifier and its Qualifications.
 * Additionally an 'active' state is defined to distinguish active members from not active members.
 *
 * Person identifiers are distinguished between internal (from personnel database)
 * identifiers and external (guest person) identifiers. Internal identifiers are based on
 * the person's association membership number and can be generated by createInternalIdent().
 * External identifiers should not rely on a membership number. They are thus based on a hash
 * generated from name and qualifications and can be generated by createExternalIdent().
 *
 * A Person should always be well defined but in some cases it can be useful/necessary to use dummyPerson()
 * to create a temporary Person with empty name, empty identifier and empty qualifications.
 *
 * In the context of the duty a person can fulfill different functions, which are defined in the class
 * by the Person::Function (general function) and Person::BoatFunction (special function on a boat drive) enum classes.
 * See also iterateFunctions() and iterateBoatFunctions().
 */
class Person
{
public:
    struct Qualifications;
    enum class Function : int8_t;
    enum class BoatFunction : int8_t;

public:
    Person(QString pLastName, QString pFirstName, QString pIdent,
           struct Qualifications pQualifications, bool pActive);    ///< Constructor.
    //
    static Person dummyPerson();                                    ///< Create a dummy person with empty properties.
    //
    static QString createInternalIdent(const QString& pLastName, const QString& pFirstName,
                                       const QString& pMembershipNumber);                       ///< \brief Create person identifier
                                                                                                ///  for internal personnel.
    static QString createExternalIdent(const QString& pLastName, const QString& pFirstName,
                                       const Qualifications& pQualifications, QString pSuffix); ///< \brief Create person identifier
                                                                                                ///  for internal personnel.
    static QString extractMembershipNumber(const QString& pInternalIdent);                      ///< \brief Extract membership number
                                                                                                ///  from internal person identifier.
    static QString extractExtSuffix(const QString& pExternalIdent);                             ///< \brief Extract suffix from
                                                                                                ///  external person identifier.
    //
    QString getLastName() const;                        ///< Get the person's last name.
    QString getFirstName() const;                       ///< Get the person's first name.
    QString getIdent() const;                           ///< Get the person's identifier.
    struct Qualifications getQualifications() const;    ///< Get the person's qualifications.
    bool getActive() const;                             ///< Check, if the person is set active.
    //
    static QString functionToLabel(Function pFunction);             ///< Get the label for a personnel function.
    static Function labelToFunction(const QString& pFunction);      ///< Get the personnel function from its label.
    static int functionOrder(Function pFirst, Function pSecond);    ///< Determine the order of two personnel functions.
    //
    static QString boatFunctionToLabel(BoatFunction pFunction);                 ///< Get the label for a boat function.
    static BoatFunction labelToBoatFunction(const QString& pFunction);          ///< Get the boat function from its label.
    static int boatFunctionOrder(BoatFunction pFirst, BoatFunction pSecond);    ///< Determine the order of two boat functions.

public:
    /*!
     * \brief %Qualifications of a person.
     *
     * Defines, which relevant qualifications are possessed by a person.
     * The struct can be converted to a comma-separated string listing
     * all possessed qualifications and can also be constructed from
     * such a string (or an already split list of qualification strings).
     */
    struct Qualifications
    {
        Qualifications(const QStringList& pQualifications); ///< Constructor.
        Qualifications(const QString& pQualifications);     ///< Constructor.
        //
        static QStringList listAllQualifications();         ///< List all qualifications in principle available.
        //
        QString toString() const;                           ///< Get a comma-separated list of possessed qualifications.
        //
        bool eh,    ///< "Erste-Hilfe Kurs"
             rsa,   ///< "Rettungsschwimmabzeichen Silber"
             faWrd, ///< "Fachausbildung Wasserrettungsdienst"
             sanA,  ///< "SAN-A Kurs"
             bfA,   ///< "Bootsführerschein A (Binnen)"
             sr1,   ///< "Strömungsretter"
             et,    ///< "Einsatztaucher"
             bos,   ///< "BOS Sprechfunker"
             wf,    ///< "Wachführer"
             zf;    ///< "Zugführer"
    };
    //
    /*!
     * \brief Possible personnel functions.
     *
     * Each available value describes a function primarily fulfilled by a person of duty personnel during the duty.
     *
     * See also QualificationChecker, which defines qualifications required for a specific personnel function.
     */
    enum class Function : int8_t
    {
        _WF = 0,        ///< "Wachführer".
        _SL = 1,        ///< "Stationsleiter".
        _BF = 2,        ///< "Bootsführer".
        _WR = 3,        ///< "Wasserretter".
        _RS = 4,        ///< "Rettungsschwimmer".
        _PR = 5,        ///< "Praktikant".
        _SAN = 6,       ///< "Sanitäter/Sanitätshelfer".
        _FU = 7,        ///< "Funker".
        _SR = 8,        ///< "Strömungsretter".
        _ET = 9,        ///< "Einsatztaucher".
        _FUD = 10,      ///< "Führungsdienst".
        _OTHER = 127    ///< Reserved.
    };
    /*!
     * \brief Possible boat functions.
     *
     * Each available value describes a function primarily fulfilled by a boat crew member during a boat drive.
     *
     * See also QualificationChecker, which defines qualifications required for a specific boat function.
     */
    enum class BoatFunction : int8_t
    {
        _BG = 1,        ///< "Bootsgast".
        _RS = 2,        ///< "Rettungsschwimmer".
        _PR = 3,        ///< "Praktikant".
        _SAN = 4,       ///< "Sanitäter/Sanitätshelfer".
        _SR = 5,        ///< "Strömungsretter".
        _ET = 6,        ///< "Einsatztaucher".
        _OTHER = 127    ///< Reserved.
    };

public:
    /*!
     * \brief Loop over personnel 'Function's and execute a specified function for each 'Function'.
     *
     * For each 'Function' the \p pFunction is called with first parameter being 'Function'
     * and perhaps further parameters \p pArgs, i.e. \p pFunction(function, pArgs...).
     * Non-void return values will be discarded. You may use use \p pArgs
     * to communicate results of the function calls.
     *
     * \tparam FuncT Specific type of a FunctionObject (\p pFunction) that shall be called for each personnel 'Function'.
     * \tparam ...Args Types of all additional parameters for \p pFunction if there are any.
     *
     * \param pFunction Any FunctionObject that accepts at least enum class values of 'Function' as the first
     *                  parameter and optionally also further parameters \p pArgs. A lambda expression may be used.
     *                  The function's return value will be discarded.
     *                  Note: (some of) \p pArgs can be passed by reference in order to communicate the results.
     * \param pArgs Optional arguments that will be passed to \p pFunction in addition to the 'Function's.
     */
    template <typename FuncT, typename ... Args>
    static void iterateFunctions(FuncT pFunction, Args& ... pArgs)
    {
        for (Function func : {Function::_WF,
                              Function::_SL,
                              Function::_BF,
                              Function::_WR,
                              Function::_RS,
                              Function::_PR,
                              Function::_SAN,
                              Function::_FU,
                              Function::_SR,
                              Function::_ET,
                              Function::_FUD})
        {
            pFunction(func, pArgs ...);
        }
    }
    /*!
     * \brief Loop over boat 'Function's and execute a specified function for each 'BoatFunction'.
     *
     * For each 'BoatFunction' the \p pFunction is called with first parameter being 'BoatFunction'
     * and perhaps further parameters \p pArgs, i.e. \p pFunction(boatFunction, pArgs...).
     * Non-void return values will be discarded. You may use use \p pArgs
     * to communicate results of the function calls.
     *
     * \tparam FuncT Specific type of a FunctionObject (\p pFunction) that shall be called for each 'BoatFunction'.
     * \tparam ...Args Types of all additional parameters for \p pFunction if there are any.
     *
     * \param pFunction Any FunctionObject that accepts at least enum class values of 'BoatFunction' as the first
     *                  parameter and optionally also further parameters \p pArgs. A lambda expression may be used.
     *                  The function's return value will be discarded.
     *                  Note: (some of) \p pArgs can be passed by reference in order to communicate the results.
     * \param pArgs Optional arguments that will be passed to \p pFunction in addition to the 'BoatFunction's.
     */
    template <typename FuncT, typename ... Args>
    static void iterateBoatFunctions(FuncT pFunction, Args& ... pArgs)
    {
        for (BoatFunction func : {BoatFunction::_BG,
                                  BoatFunction::_RS,
                                  BoatFunction::_PR,
                                  BoatFunction::_SAN,
                                  BoatFunction::_SR,
                                  BoatFunction::_ET})
        {
            pFunction(func, pArgs ...);
        }
    }

private:
    QString lastName, firstName;            //First and last names of the person
    QString identifier;                     //Unique identifier for this person
    struct Qualifications qualifications;   //Qualifications of the person
    bool active;                            //Is person active/enabled?
};

#endif // PERSON_H