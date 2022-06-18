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

#ifndef PERSONNELDATABASEDIALOG_H
#define PERSONNELDATABASEDIALOG_H

#include "auxil.h"
#include "person.h"
#include "databasecache.h"
#include "settingscache.h"
#include "personneleditordialog.h"

#include <thread>

#include <QString>

#include <QDialog>
#include <QMessageBox>
#include <QTableWidget>
#include <QAbstractItemView>
#include <QItemSelectionModel>
#include <QModelIndexList>
#include <QHeaderView>
#include <QPushButton>

namespace Ui {
class PersonnelDatabaseDialog;
}

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
    explicit PersonnelDatabaseDialog(QWidget *const pParent = nullptr); ///< Constructor.
    ~PersonnelDatabaseDialog();                                         ///< Destructor.

private:
    void updatePersonnelTable();                                        ///< Show an up to date personnel list from the database cache.

private slots:
    void on_add_pushButton_pressed();                                   ///< Add a new person to personnel.
    void on_edit_pushButton_pressed();                                  ///< Edit the selected persons.
    void on_remove_pushButton_pressed();                                ///< Remove a person from personnel.
    void on_personnel_tableWidget_cellDoubleClicked(int, int);          ///< Edit the selected persons.

private:
    Ui::PersonnelDatabaseDialog *ui;    //UI
    //
    bool editDisabled;                  //Disable writing to the database
};

#endif // PERSONNELDATABASEDIALOG_H
