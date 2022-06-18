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

#include "aboutdialog.h"
#include "ui_aboutdialog.h"

/*!
 * \brief Constructor.
 *
 * Constructs the dialog.
 *
 * Inserts the program and Qt versions into the corresponding info label.
 *
 * \param pParent The parent widget.
 */
AboutDialog::AboutDialog(QWidget *const pParent) :
    QDialog(pParent, Qt::WindowTitleHint |
                     Qt::WindowSystemMenuHint |
                     Qt::WindowCloseButtonHint),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    //Show current program version and Qt version used for compilation
    ui->info_label->setText("Wachdienst-Manager " + Aux::programVersionStringPretty + ".\n\n" +
                            "Verwendet Qt. Erstellt mit Version " + QT_VERSION_STR + ".");
}

/*!
 * \brief Destructor.
 */
AboutDialog::~AboutDialog()
{
    delete ui;
}
