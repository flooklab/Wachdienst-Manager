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

#include "updatereportpersonentrydialog.h"
#include "ui_updatereportpersonentrydialog.h"

/*!
 * \brief Constructor.
 *
 * Creates the dialog.
 *
 * Fills the input widgets with the properties of \p pPerson
 * and the provided times \p pBeginTime and \p pEndTime.
 *
 * The selectable functions will be according to the qualifications of \p pPerson.
 * The specified \p pFunction will be selected, if possible.
 *
 * \param pPerson The person whose function/times shall be edited.
 * \param pFunction The initial function.
 * \param pBeginTime The initial begin time.
 * \param pEndTime The initial end time.
 * \param pParent The parent widget.
 */
UpdateReportPersonEntryDialog::UpdateReportPersonEntryDialog(const Person& pPerson, Person::Function pFunction,
                                                             QTime pBeginTime, QTime pEndTime, QWidget *const pParent) :
    QDialog(pParent, Qt::WindowTitleHint |
                     Qt::WindowSystemMenuHint |
                     Qt::WindowCloseButtonHint),
    ui(new Ui::UpdateReportPersonEntryDialog)
{
    ui->setupUi(this);

    ui->lastName_lineEdit->setText(pPerson.getLastName());
    ui->firstName_lineEdit->setText(pPerson.getFirstName());

    ui->ident_comboBox->insertItem(0, pPerson.getIdent());
    ui->ident_comboBox->setCurrentIndex(0);

    ui->timeBegin_timeEdit->setTime(pBeginTime);
    ui->timeEnd_timeEdit->setTime(pEndTime);

    /*
     * Lambda expression for adding \p pFunction label to \p pFunctions,
     * if the qualifications required for the function are present in \p pQualis.
     * To be executed for each available 'Function'.
     */
    auto tFunc = [](Person::Function pFunction, const struct Person::Qualifications& pQualis, QStringList& pFunctions) -> void
    {
        if (QualificationChecker::checkPersonnelFunction(pFunction, pQualis))
            pFunctions.append(Person::functionToLabel(pFunction));
    };

    //Insert every function the person is qualified for into the functions combo box

    Person::Qualifications tQualis = pPerson.getQualifications();

    QStringList availableFunctions;
    Person::iterateFunctions(tFunc, tQualis, availableFunctions);

    ui->function_comboBox->insertItems(ui->function_comboBox->count(), availableFunctions);

    //Select the specified function
    ui->function_comboBox->setCurrentIndex(ui->function_comboBox->findText(Person::functionToLabel(pFunction)));
}

/*!
 * \brief Destructor.
 */
UpdateReportPersonEntryDialog::~UpdateReportPersonEntryDialog()
{
    delete ui;
}

//Public

/*!
 * \brief Get the currently selected function.
 *
 * \return The personnel function.
 */
Person::Function UpdateReportPersonEntryDialog::getFunction() const
{
    return Person::labelToFunction(ui->function_comboBox->currentText());
}

/*!
 * \brief Get the currently set begin time.
 *
 * \return The person's begin time.
 */
QTime UpdateReportPersonEntryDialog::getBeginTime() const
{
    return ui->timeBegin_timeEdit->time();
}

/*!
 * \brief Get the currently set end time.
 *
 * \return The person's end time.
 */
QTime UpdateReportPersonEntryDialog::getEndTime() const
{
    return ui->timeEnd_timeEdit->time();
}
