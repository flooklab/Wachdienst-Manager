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

#include "personneleditordialog.h"
#include "ui_personneleditordialog.h"

#include "auxil.h"

#include <QAbstractButton>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QSize>
#include <QStringList>

#include <set>

/*!
 * \brief Constructor.
 *
 * Creates the dialog.
 *
 * Loads name, qualifications and status of \p pPerson into the input widgets.
 * The membership number field will be set to the membership number of \p pPerson,
 * if \p pType is PersonType::_INTERNAL, or set to its identifier else. Note that
 * the actual value of the identifier will be ignored. The person returned by
 * getPerson() will only depend on \p pType and the (edited) input fields.
 *
 * The membership number and status fields only will be made editable,
 * if \p pType is PersonType::_INTERNAL, i.e. for an internal person.
 *
 * The qualifications selection field only will be made editable, if \p pType is *not* PersonType::_OTHER,
 * i.e. only for internal or external persons, which can be part of the actual duty personnel.
 *
 * If \p pEditExtQualisOnly is true then a restricted mode will be enabled, in which no fields are
 * editable, except for an editable qualifications selection if \p pType is PersonType::_EXTERNAL.
 *
 * Note that wrongly formatted \p pPerson properties will be simply reset to an empty string before inserting.
 *
 * \param pPerson The person to use for initially setting the input fields.
 * \param pType Category of personnel that the edited person belongs to.
 * \param pEditExtQualisOnly Only allow to change qualifications of \e external persons and disallow to change anything else.
 * \param pParent The parent widget.
 */
PersonnelEditorDialog::PersonnelEditorDialog(const Person& pPerson, const PersonType pType, const bool pEditExtQualisOnly,
                                             QWidget *const pParent) :
    QDialog(pParent, Qt::WindowTitleHint |
                     Qt::WindowSystemMenuHint |
                     Qt::WindowCloseButtonHint),
    ui(new Ui::PersonnelEditorDialog),
    acceptPermanentlyDisabled(pEditExtQualisOnly && pType != PersonType::_EXTERNAL),
    acceptDisabled(acceptPermanentlyDisabled),
    personType(pType),
    extIdentSuffix("")
{
    ui->setupUi(this);

    //Set validators

    ui->lastName_lineEdit->setValidator(new QRegularExpressionValidator(Aux::personNamesValidator.regularExpression(),
                                                                        ui->lastName_lineEdit));

    ui->firstName_lineEdit->setValidator(new QRegularExpressionValidator(Aux::personNamesValidator.regularExpression(),
                                                                         ui->firstName_lineEdit));

    ui->membershipNumber_lineEdit->setValidator(new QRegularExpressionValidator(Aux::membershipNumbersValidator.regularExpression(),
                                                                                ui->membershipNumber_lineEdit));

    //Validate provided person's properties and reset each, if not valid

    QString tLastName = pPerson.getLastName();
    QString tFirstName = pPerson.getFirstName();
    QString tIdent = pPerson.getIdent();
    Person::Qualifications tQualis = pPerson.getQualifications();
    bool tActive = pPerson.getActive();

    int tmpInt = 0;
    if (Aux::personNamesValidator.validate(tLastName, tmpInt) != QValidator::State::Acceptable)
        tLastName = "";
    if (Aux::personNamesValidator.validate(tFirstName, tmpInt) != QValidator::State::Acceptable)
        tFirstName = "";

    QString tMembershipNumber;

    //Adjust input widget contents and enabled states depending on person type

    if (personType == PersonType::_INTERNAL)
    {
        tMembershipNumber = Person::extractMembershipNumber(tIdent);

        if (Aux::membershipNumbersValidator.validate(tMembershipNumber, tmpInt) != QValidator::State::Acceptable)
            tMembershipNumber = "";
    }
    else if (personType == PersonType::_EXTERNAL || personType == PersonType::_OTHER)
    {
        //Disable membership number input and status checkbox, if creating or editing external/"other" person
        ui->membershipNumber_label->setEnabled(false);
        ui->membershipNumber_lineEdit->setEnabled(false);
        ui->status_label->setEnabled(false);
        ui->status_checkBox->setEnabled(false);

        //Also disable selection of qualifications in case of "other" person
        if (personType == PersonType::_OTHER)
        {
            ui->qualifications_label->setEnabled(false);
            ui->qualifications_listWidget->setEnabled(false);
        }

        //For an external/"other" person simply use the identifier for the membership number input
        //No checks here, since this is only displayed for informational purposes
        tMembershipNumber = tIdent;

        //Remember suffix to use it later in getPerson()
        extIdentSuffix = Person::extractExtSuffix(tIdent);

        if (Aux::extIdentSuffixesValidator.validate(extIdentSuffix, tmpInt) != QValidator::State::Acceptable)
            extIdentSuffix = "";
    }

    //Set input widget contents
    ui->lastName_lineEdit->setText(tLastName);
    ui->firstName_lineEdit->setText(tFirstName);
    ui->membershipNumber_lineEdit->setText(tMembershipNumber);
    ui->status_checkBox->setChecked(!tActive);

    //Insert possible qualifications into list widget and check the provided person's qualifications
    if (personType != PersonType::_OTHER)
        insertQualifications(tQualis);

    //In restricted editing mode disable editing of all input widgets but allow to edit qualifications in case of an external person
    if (pEditExtQualisOnly)
    {
        ui->lastName_lineEdit->setReadOnly(true);
        ui->firstName_lineEdit->setReadOnly(true);
        ui->membershipNumber_lineEdit->setReadOnly(true);

        ui->lastName_lineEdit->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        ui->lastName_lineEdit->setFocusPolicy(Qt::NoFocus);
        ui->firstName_lineEdit->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        ui->firstName_lineEdit->setFocusPolicy(Qt::NoFocus);
        ui->membershipNumber_lineEdit->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        ui->membershipNumber_lineEdit->setFocusPolicy(Qt::NoFocus);

        ui->status_checkBox->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        ui->status_checkBox->setFocusPolicy(Qt::NoFocus);

        if (personType != PersonType::_EXTERNAL)
        {
            ui->qualifications_listWidget->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            ui->qualifications_listWidget->setFocusPolicy(Qt::NoFocus);
        }
    }

    //Disable "Save" button if a required property is empty
    checkEmptyTexts();

    //Disable "Save" button if accepting permanently disabled
    if (acceptPermanentlyDisabled)
        disableAccept();
}

/*!
 * \brief Destructor.
 */
PersonnelEditorDialog::~PersonnelEditorDialog()
{
    delete ui;
}

//Public

/*!
 * \brief Create a person from the current content of the input widgets.
 *
 * Creates an internal or external person, depending on the \p pExternal
 * argument passed to PersonnelEditorDialog(). The contents of the
 * editable widgets will be used to construct the person.
 *
 * Note: Whether a person (identifier) already exists e.g. in the database must
 * be checked manually by the caller! Due to input widget validators, however,
 * the person's properties are checked for valid formatting and they are also
 * not empty, if the dialog was accepted (and not rejected).
 *
 * \return The new person.
 */
Person PersonnelEditorDialog::getPerson() const
{
    QString lastName = ui->lastName_lineEdit->text().trimmed();
    QString firstName = ui->firstName_lineEdit->text().trimmed();
    Person::Qualifications qualifications = compileQualifications();

    QString ident;

    if (personType == PersonType::_INTERNAL)
        ident = Person::createInternalIdent(lastName, firstName, ui->membershipNumber_lineEdit->text());
    else if (personType == PersonType::_EXTERNAL)
        ident = Person::createExternalIdent(lastName, firstName, qualifications, extIdentSuffix);
    else if (personType == PersonType::_OTHER)
        ident = Person::createOtherIdent(lastName, firstName, extIdentSuffix);

    return Person(std::move(lastName), std::move(firstName), ident, qualifications, !ui->status_checkBox->isChecked());
}

//Private

/*!
 * \brief Disable accepting the dialog, if a required line edit is empty, enable otherwise.
 *
 * Disables accepting the dialog if one of the name line edit texts is empty,
 * or if the membership number line edit text is empty in case of an internal person.
 * Accepting the dialog will be enabled otherwise.
 *
 * Also disables/enables the "Ok" button.
 */
void PersonnelEditorDialog::checkEmptyTexts()
{
    if (ui->lastName_lineEdit->text().trimmed() == "" || ui->firstName_lineEdit->text().trimmed() == "" ||
        (personType == PersonType::_INTERNAL && ui->membershipNumber_lineEdit->text().trimmed() == ""))
    {
        disableAccept();
    }
    else
        enableAccept();
}

/*!
 * \brief Prevent accepting the dialog.
 *
 * Disables accepting of the dialog and disables the "Ok" button.
 */
void PersonnelEditorDialog::disableAccept()
{
    acceptDisabled = true;

    //Disable "Save" button
    QList<QAbstractButton*> tButtons = ui->buttonBox->buttons();
    for (int i = 0; i < tButtons.length(); ++i)
        tButtons.at(i)->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::StandardButton::Cancel)->setEnabled(true);
}

/*!
 * \brief Allow accepting the dialog.
 *
 * Enables accepting of the dialog and enables the "Ok" button, except if accepting was initially permanently disabled.
 */
void PersonnelEditorDialog::enableAccept()
{
    if (acceptPermanentlyDisabled)
        return;

    acceptDisabled = false;

    //Enable "Save" button
    QList<QAbstractButton*> tButtons = ui->buttonBox->buttons();
    for (int i = 0; i < tButtons.length(); ++i)
        tButtons.at(i)->setEnabled(true);
}

//

/*!
 * \brief Fill the list widget with a checkable item for each qualification.
 *
 * Fills the qualifications list widget with a checkable item for each possible qualification.
 * The qualifications in \p pQualis are directly added in checked state.
 *
 * \param pQualis Qualifications to use for setting initial check states.
 */
void PersonnelEditorDialog::insertQualifications(const Person::Qualifications& pQualis)
{
    std::set<QString> tQualis;
    for (QString tQuali : pQualis.toString().split(','))
        tQualis.insert(std::move(tQuali));

    ui->qualifications_listWidget->setStyleSheet("QListWidget::item { padding: -8px; }");

    for (const QString& qualiStr : Person::Qualifications::listAllQualifications())
    {
        QWidget* tWidget = new QWidget(ui->qualifications_listWidget);

        QLabel* tLabel = new QLabel(qualiStr, tWidget);
        tLabel->setFocusPolicy(Qt::FocusPolicy::NoFocus);

        QHBoxLayout* tLayout = new QHBoxLayout(ui->qualifications_listWidget);
        tLayout->addWidget(tLabel);
        tLayout->setSizeConstraint(QLayout::SetMaximumSize);

        tWidget->setLayout(tLayout);
        tWidget->resize(tWidget->size().width() - 4, tWidget->size().height() - 4);

        QListWidgetItem* tItem = new QListWidgetItem(ui->qualifications_listWidget);
        tItem->setFlags(tItem->flags() & (~Qt::ItemIsUserCheckable));
        tItem->setSizeHint(tWidget->size());

        //Check item, if person has this qualification
        if (tQualis.find(qualiStr) == tQualis.end())
            tItem->setCheckState(Qt::CheckState::Unchecked);
        else
            tItem->setCheckState(Qt::CheckState::Checked);

        ui->qualifications_listWidget->insertItem(0, tItem);
        ui->qualifications_listWidget->setItemWidget(tItem, tWidget);

        qualificationItems[qualiStr] = tItem;
    }
}

/*!
 * \brief Get qualifications based on the check state of list widget's items.
 *
 * Returns qualifications created using the current check states
 * of the items of the qualifications list widget.
 *
 * \return Currently checked qualifications.
 */
Person::Qualifications PersonnelEditorDialog::compileQualifications() const
{
    QStringList qualis;

    for (auto it : qualificationItems)
        if ((it.second)->checkState() == Qt::CheckState::Checked)
            qualis.append(std::move(it.first));

    return Person::Qualifications(qualis);
}

//Private slots

/*!
 * \brief Reimplementation of QDialog::accept().
 *
 * Reimplements QDialog::accept().
 *
 * Returns immediately, if accepting was permanently disabled or disabled due to an empty required input text.
 */
void PersonnelEditorDialog::accept()
{
    if (acceptDisabled || acceptPermanentlyDisabled)
        return;

    QDialog::accept();
}

//

/*!
 * \brief Disable accepting, if a required line edit is empty.
 *
 * Disables accepting the dialog, if any required line edit text is empty, and enables it else.
 */
void PersonnelEditorDialog::on_firstName_lineEdit_textEdited(const QString&)
{
    checkEmptyTexts();
}

/*!
 * \brief Disable accepting, if a required line edit is empty.
 *
 * Disables accepting the dialog, if any required line edit text is empty, and enables it else.
 */
void PersonnelEditorDialog::on_lastName_lineEdit_textEdited(const QString&)
{
    checkEmptyTexts();
}

/*!
 * \brief Disable accepting, if a required line edit is empty.
 *
 * Disables accepting the dialog, if any required line edit text is empty, and enables it else.
 */
void PersonnelEditorDialog::on_membershipNumber_lineEdit_textEdited(const QString&)
{
    checkEmptyTexts();
}

/*!
 * \brief Toggle the list widget item's check state.
 *
 * Toggles the check state of the qualifications list widget item \p item.
 *
 * \param item The qualification item to be toggled.
 */
void PersonnelEditorDialog::on_qualifications_listWidget_itemDoubleClicked(QListWidgetItem *const item)
{
    if (item->checkState() == Qt::Checked)
        item->setCheckState(Qt::Unchecked);
    else
        item->setCheckState(Qt::Checked);
}
