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

#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include "auxil.h"

#include <QString>

#include <QDialog>
#include <QLabel>

namespace Ui {
class AboutDialog;
}

/*!
 * \brief Show version, contributors and license information.
 *
 * - Shows the program version and the Qt version used to compile the program.
 * - Shows the contributors and image/logo copyright information.
 * - Shows the program's AGPL license preamble.
 */
class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *const pParent = nullptr); ///< Constructor.
    ~AboutDialog();                                         ///< Destructor.

private:
    Ui::AboutDialog *ui;    //UI
};

#endif // ABOUTDIALOG_H
