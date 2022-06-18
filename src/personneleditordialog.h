/*
/////////////////////////////////////////////////////////////////////////////////////////
//
//  This file is part of Wachdienst-Manager, a program to manage DLRG watch duty reports.
//  Copyright (C) 2021–2022 M. Frohne
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

#include "auxil.h"
#include "person.h"

#include <set>
#include <map>

#include <QList>
#include <QString>
#include <QStringList>
#include <QRegularExpressionValidator>
#include <QSize>

#include <QDialog>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QAbstractButton>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QListWidget>
#include <QListWidgetItem>

namespace Ui {
class PersonnelEditorDialog;
}

/*!
 * \brief Edit or create an internal or external Person.
 *
 * Edit the properties of the person passed to the dialog's constructor.
 * New persons can also be created by passing e.g. Person::dummyPerson() as person.
 * Internal and external persons can be created/edited, depending on a constructor argument.
 *
 * The created/edited person can be obtained by calling getPerson() after accepting the dialog.
 */
class PersonnelEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PersonnelEditorDialog(const Person& pPerson, bool pExternal, QWidget *const pParent = nullptr);    ///< Constructor.
    ~PersonnelEditorDialog();                                                                                   ///< Destructor.
    //
    Person getPerson() const;   ///< Create a person from the current content of the input widgets.

private:
    void checkEmptyTexts();     ///< Disable accepting the dialog, if a required line edit is empty, enable otherwise.
    void disableAccept();       ///< Prevent accepting the dialog.
    void enableAccept();        ///< Allow accepting the dialog.
    //
    void insertQualifications(Person::Qualifications pQualis);  ///< Fill the list widget with a checkable item for each qualification.
    Person::Qualifications compileQualifications() const;       ///< Get qualifications based on the check state of list widget's items.

private slots:
    virtual void accept();                                                  ///< Reimplementation of QDialog::accept().
    //
    void on_firstName_lineEdit_textEdited(const QString&);                  ///< Disable accepting, if a required line edit is empty.
    void on_lastName_lineEdit_textEdited(const QString&);                   ///< Disable accepting, if a required line edit is empty.
    void on_membershipNumber_lineEdit_textEdited(const QString&);           ///< Disable accepting, if a required line edit is empty.
    void on_qualifications_listWidget_itemDoubleClicked(QListWidgetItem *item);     ///< Toggle the list widget item's check state.

private:
    Ui::PersonnelEditorDialog *ui;                          //UI
    //
    std::map<QString, QListWidgetItem*> qualificationItems; //Pointers to checkable qualifications list widget items
    //
    bool acceptDisabled;                                    //Accepting the dialog is disabled?
    //
    bool editExternalPerson;                                //Calling getPerson() after editing shall generate an external person
    QString extIdentSuffix;                                 //In the case of an external person, keep the ident suffix from constructor
};

#endif // PERSONNELEDITORDIALOG_H
