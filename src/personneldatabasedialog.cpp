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

#include "personneldatabasedialog.h"
#include "ui_personneldatabasedialog.h"

/*!
 * \brief Constructor.
 *
 * Creates the dialog.
 *
 * Loads the personnel data from the database cache
 * and displays it in the table widget.
 *
 * Asks for password (if set) and checks if the database is writeable.
 * Disables editing of the personnel, if password wrong or database read-only.
 *
 * \param pParent The parent widget.
 */
PersonnelDatabaseDialog::PersonnelDatabaseDialog(QWidget *const pParent) :
    QDialog(pParent, Qt::WindowTitleHint |
                     Qt::WindowSystemMenuHint |
                     Qt::WindowMinimizeButtonHint |
                     Qt::WindowMaximizeButtonHint |
                     Qt::WindowCloseButtonHint),
    ui(new Ui::PersonnelDatabaseDialog),
    editDisabled(false)
{
    ui->setupUi(this);

    //Format table header and configure selection mode
    ui->personnel_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->personnel_tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->personnel_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->personnel_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->personnel_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->personnel_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->personnel_tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    ui->personnel_tableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    //Ask for password

    QString hash = SettingsCache::getStrSetting("app_auth_hash");
    QString salt = SettingsCache::getStrSetting("app_auth_salt");

    //Note: this is not intended to be secure...
    if (hash != "" && salt != "")
    {
        _CheckPassword:
        if (!Aux::checkPassword(hash, salt, pParent))
        {
            QMessageBox msgBox(QMessageBox::Critical, "Fehler", "Falsches Passwort!", QMessageBox::Abort | QMessageBox::Retry, pParent);
            msgBox.setDefaultButton(QMessageBox::Retry);

            if (msgBox.exec() == QMessageBox::Retry)
            {
                //Retry after a second
                std::this_thread::sleep_for(std::chrono::seconds(1));
                goto _CheckPassword;
            }

            //No writing to database
            editDisabled = true;
        }
    }

    //Check if database is writeable
    if (DatabaseCache::isReadOnly())
    {
        editDisabled = true;

        QMessageBox(QMessageBox::Warning, "Warnung", "Datenbank ist nur lesbar,\nda das Programm mehrfach geöffnet ist!",
                    QMessageBox::Ok, pParent).exec();
    }

    //Disable add/edit/remove buttons, if read-only or wrong password
    if (editDisabled)
    {
        ui->add_pushButton->setEnabled(false);
        ui->edit_pushButton->setEnabled(false);
        ui->remove_pushButton->setEnabled(false);
    }

    //Load personnel records into the table widget
    updatePersonnelTable();
}

/*!
 * \brief Destructor.
 */
PersonnelDatabaseDialog::~PersonnelDatabaseDialog()
{
    delete ui;
}

//Private

/*!
 * \brief Show an up to date personnel list from the database cache.
 *
 * Loads an up to date personnel list from the database cache
 * and updates the displayed personnel data in the table widget.
 *
 * The entries are sorted by last name, first name and then identifier.
 */
void PersonnelDatabaseDialog::updatePersonnelTable()
{
    //Define lambda to compare/sort persons by name, then identifier
    auto cmp = [](const Person& pPersonA, const Person& pPersonB) -> bool
    {
        if (QString::localeAwareCompare(pPersonA.getLastName(), pPersonB.getLastName()) < 0)
            return true;
        else if (QString::localeAwareCompare(pPersonA.getLastName(), pPersonB.getLastName()) > 0)
            return false;
        else
        {
            if (QString::localeAwareCompare(pPersonA.getFirstName(), pPersonB.getFirstName()) < 0)
                return true;
            else if (QString::localeAwareCompare(pPersonA.getFirstName(), pPersonB.getFirstName()) > 0)
                return false;
            else
            {
                if (QString::localeAwareCompare(pPersonA.getIdent(), pPersonB.getIdent()) < 0)
                    return true;
                else
                    return false;
            }
        }
    };

    //Get available personnel from database cache
    std::vector<Person> tPersonnel;
    DatabaseCache::getPersonnel(tPersonnel);

    //Use temporary set to sort persons using above custom sort lambda

    std::set<std::reference_wrapper<const Person>, decltype(cmp)> tPersonnelSorted(cmp);

    for (const Person& tPerson : tPersonnel)
        tPersonnelSorted.insert(std::cref(tPerson));

    //Clear
    ui->personnel_tableWidget->setRowCount(0);

    for (const Person& tPerson : tPersonnelSorted)
    {
        QString tIdent = tPerson.getIdent();

        int tOldRowCount = ui->personnel_tableWidget->rowCount();

        ui->personnel_tableWidget->insertRow(tOldRowCount);

        ui->personnel_tableWidget->setItem(tOldRowCount, 0, new QTableWidgetItem(tIdent));
        ui->personnel_tableWidget->setItem(tOldRowCount, 1, new QTableWidgetItem(tPerson.getLastName()));
        ui->personnel_tableWidget->setItem(tOldRowCount, 2, new QTableWidgetItem(tPerson.getFirstName()));
        ui->personnel_tableWidget->setItem(tOldRowCount, 3, new QTableWidgetItem(tPerson.getQualifications().toString()));
        ui->personnel_tableWidget->setItem(tOldRowCount, 4, new QTableWidgetItem(!tPerson.getActive() ? "Deaktiviert" : ""));
    }
}

//Private slots

/*!
 * \brief Add a new person to personnel.
 *
 * Adds a new person to the personnel database
 * using the PersonnelEditorDialog dialog window.
 *
 * Updates the displayed personnel table afterwards.
 *
 * Returns immediately, if editing was disabled due to wrong password or if database is read-only.
 */
void PersonnelDatabaseDialog::on_add_pushButton_pressed()
{
    if (editDisabled || DatabaseCache::isReadOnly())
        return;

    PersonnelEditorDialog dialog(Person::dummyPerson(), false, this);

    if (dialog.exec() != QDialog::Accepted)
        return;

    Person newPerson = dialog.getPerson();

    //Check that person does not exist yet
    if (DatabaseCache::personExists(newPerson.getIdent()))
    {
        QMessageBox(QMessageBox::Critical, "Fehler", "Person existiert bereits in Datenbank!", QMessageBox::Ok, this).exec();
        return;
    }
    else if (DatabaseCache::memberNumExists(Person::extractMembershipNumber(newPerson.getIdent())))
    {
        QMessageBox(QMessageBox::Critical, "Fehler", "Person mit dieser Mitgliedsnummer existiert bereits in Datenbank!",
                    QMessageBox::Ok, this).exec();
        return;
    }

    if (!DatabaseCache::addPerson(newPerson))
    {
        QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Datenbank!", QMessageBox::Ok, this).exec();
    }

    updatePersonnelTable();
}

/*!
 * \brief Edit the selected persons.
 *
 * Successively edits all persons selected in the personnel table widget
 * using the PersonnelEditorDialog dialog window.
 *
 * Updates the displayed personnel table afterwards.
 *
 * Returns immediately, if editing was disabled due to wrong password or if database is read-only.
 */
void PersonnelDatabaseDialog::on_edit_pushButton_pressed()
{
    if (editDisabled || DatabaseCache::isReadOnly())
        return;

    //Get selected persons' identifiers
    std::vector<QString> tIdents;
    QModelIndexList idxList = ui->personnel_tableWidget->selectionModel()->selectedRows();
    for (auto it = idxList.begin(); it != idxList.end(); ++it)
        tIdents.push_back(ui->personnel_tableWidget->item((*it).row(), 0)->text());

    for (const QString& tIdent : tIdents)
    {
        QString tMmbNr = Person::extractMembershipNumber(tIdent);

        Person tPerson = Person::dummyPerson();
        DatabaseCache::getPerson(tPerson, tIdent);

        PersonnelEditorDialog dialog(tPerson, false, this);

        if (dialog.exec() != QDialog::Accepted)
            continue;

        tPerson = dialog.getPerson();

        QString tNewMmbNr = Person::extractMembershipNumber(tPerson.getIdent());

        //Check that (changed!) person does not exist yet
        if (tPerson.getIdent() != tIdent && DatabaseCache::personExists(tPerson.getIdent()))
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Person existiert bereits in Datenbank!", QMessageBox::Ok, this).exec();
            continue;
        }
        else if (tNewMmbNr != tMmbNr && DatabaseCache::memberNumExists(tNewMmbNr))
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Person mit dieser Mitgliedsnummer existiert bereits in Datenbank!",
                        QMessageBox::Ok, this).exec();
            continue;
        }

        if (!DatabaseCache::updatePerson(tIdent, tPerson))
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Datenbank!", QMessageBox::Ok, this).exec();
        }
    }

    updatePersonnelTable();
}

/*!
 * \brief Remove a person from personnel.
 *
 * Removes all persons selected in the personnel table widget from the database.
 *
 * Updates the displayed personnel table afterwards.
 *
 * Returns immediately, if editing was disabled due to wrong password or if database is read-only.
 */
void PersonnelDatabaseDialog::on_remove_pushButton_pressed()
{
    if (editDisabled || DatabaseCache::isReadOnly())
        return;

    //Get selected persons' identifiers
    std::vector<QString> tIdents;
    QModelIndexList idxList = ui->personnel_tableWidget->selectionModel()->selectedRows();
    for (auto it = idxList.begin(); it != idxList.end(); ++it)
        tIdents.push_back(ui->personnel_tableWidget->item((*it).row(), 0)->text());

    for (const QString& tIdent : tIdents)
    {
        if (!DatabaseCache::removePerson(tIdent))
        {
            QMessageBox(QMessageBox::Critical, "Fehler", "Fehler beim Schreiben der Datenbank!", QMessageBox::Ok, this).exec();
        }
    }

    updatePersonnelTable();
}

/*!
 * \brief Edit the selected persons.
 *
 * Edits the selected persons. See on_edit_pushButton_pressed().
 *
 * Returns immediately, if editing was disabled due to wrong password or if database is read-only.
 */
void PersonnelDatabaseDialog::on_personnel_tableWidget_cellDoubleClicked(int, int)
{
    if (editDisabled || DatabaseCache::isReadOnly())
        return;

    on_edit_pushButton_pressed();
}
