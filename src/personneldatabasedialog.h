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

#ifndef WDMGR_PERSONNELDATABASEDIALOG_H
#define WDMGR_PERSONNELDATABASEDIALOG_H

#include <QDialog>
#include <QWidget>

namespace Ui { class PersonnelDatabaseDialog; }

/*!
 * \brief Show and edit the records of the personnel database.
 *
 * Displays a table containing all personnel data.
 * New persons can be added and selected existing
 * persons can be edited or removed.
 */
class PersonnelDatabaseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PersonnelDatabaseDialog(QWidget* pParent = nullptr);       ///< Constructor.
    ~PersonnelDatabaseDialog();                                         ///< Destructor.

private:
    void connectSlots() const;                                          ///< Connect UI signals to their slots.
    //
    void updatePersonnelTable();                                        ///< Show an up to date personnel list from the database cache.

private slots:
    void at_add_pushButton_pressed();                                   ///< Add a new person to personnel.
    void at_edit_pushButton_pressed();                                  ///< Edit the selected persons.
    void at_remove_pushButton_pressed();                                ///< Remove a person from personnel.
    void at_personnel_tableWidget_cellDoubleClicked(int, int);          ///< Edit the selected persons.

private:
    Ui::PersonnelDatabaseDialog* ui;    //UI
    //
    bool editDisabled;                  //Disable writing to the database
};

#endif // WDMGR_PERSONNELDATABASEDIALOG_H
