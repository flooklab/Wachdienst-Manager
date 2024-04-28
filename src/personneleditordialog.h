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

#ifndef PERSONNELEDITORDIALOG_H
#define PERSONNELEDITORDIALOG_H

#include "person.h"

#include <QDialog>
#include <QListWidgetItem>
#include <QString>
#include <QWidget>

#include <map>

namespace Ui {
class PersonnelEditorDialog;
}

/*!
 * \brief Edit or create an internal or external Person.
 *
 * Edit the properties of the person passed to the dialog's constructor.
 * New persons can also be created by passing e.g. Person::dummyPerson() as person.
 * Internal, external and "other" persons (see PersonType) can be created/edited, depending on a constructor argument.
 *
 * The created/edited person can be obtained by calling getPerson() after accepting the dialog.
 */
class PersonnelEditorDialog : public QDialog
{
    Q_OBJECT

public:
    enum class PersonType : int8_t;

public:
    explicit PersonnelEditorDialog(const Person& pPerson, PersonType pType,
                                   bool pEditExtQualisOnly = false, QWidget* pParent = nullptr);    ///< Constructor.
    ~PersonnelEditorDialog();                                                                       ///< Destructor.
    //
    Person getPerson() const;   ///< Create a person from the current content of the input widgets.

private:
    void checkEmptyTexts();     ///< Disable accepting the dialog, if a required line edit is empty, enable otherwise.
    void disableAccept();       ///< Prevent accepting the dialog.
    void enableAccept();        ///< Allow accepting the dialog.
    //
    void insertQualifications(const Person::Qualifications& pQualis);       ///< \brief Fill the list widget with a
                                                                            ///  checkable item for each qualification.
    Person::Qualifications compileQualifications() const;                   ///< \brief Get qualifications based on the
                                                                            ///  check state of list widget's items.

private slots:
    void accept() override;                                                 ///< Reimplementation of QDialog::accept().
    //
    void on_firstName_lineEdit_textEdited(const QString&);                  ///< Disable accepting, if a required line edit is empty.
    void on_lastName_lineEdit_textEdited(const QString&);                   ///< Disable accepting, if a required line edit is empty.
    void on_membershipNumber_lineEdit_textEdited(const QString&);           ///< Disable accepting, if a required line edit is empty.
    void on_qualifications_listWidget_itemDoubleClicked(QListWidgetItem* item);     ///< Toggle the list widget item's check state.

public:
    /*!
     * \brief Category of personnel that the edited person belongs to.
     *
     * Categories _INTERNAL / _EXTERNAL / _OTHER correspond to
     * Person identifiers starting with 'i'/'e'/'o', respectively.
     */
    enum class PersonType : int8_t
    {
        _INTERNAL = 0,  ///< Internal personnel (part of duty personnel; to be found in the database).
        _EXTERNAL = 1,  ///< External personnel (also part of duty personnel but from other local group; not in the database).
        _OTHER =    2,  ///< Other people not part of duty personnel (e.g., temporarily present as part of a boat drive's crew).
    };

private:
    Ui::PersonnelEditorDialog* ui;                          //UI
    //
    std::map<QString, QListWidgetItem*> qualificationItems; //Pointers to checkable qualifications list widget items
    //
    const bool acceptPermanentlyDisabled;   //Accepting the dialog shall never be enabled during dialogs lifetime
    bool acceptDisabled;                    //Accepting the dialog is currently disabled
    //
    PersonType personType;                  //Type of person to be created/edited (internal personnel, external personnel, other person)
    QString extIdentSuffix;                 //In the case of an external person, keep the ident suffix from constructor
};

#endif // PERSONNELEDITORDIALOG_H
