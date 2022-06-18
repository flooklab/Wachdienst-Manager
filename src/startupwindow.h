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

#ifndef STARTUPWINDOW_H
#define STARTUPWINDOW_H

#include "auxil.h"
#include "report.h"
#include "reportwindow.h"
#include "newreportdialog.h"
#include "personneldatabasedialog.h"
#include "settingsdialog.h"
#include "aboutdialog.h"

#include <memory>

#include <QString>
#include <QShortcut>
#include <QKeySequence>

#include <QMainWindow>
#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>

QT_BEGIN_NAMESPACE
namespace Ui { class StartupWindow; }
QT_END_NAMESPACE

/*!
 * \brief The main control window.
 *
 * All other parts of the program to create and open reports and maintain the
 * program settings and the personnel database are controlled/started from here.
 */
class StartupWindow : public QMainWindow
{
    Q_OBJECT

public:
    StartupWindow(QWidget *const pParent = nullptr);    ///< Constructor.
    ~StartupWindow();                                   ///< Destructor.
    //
    void newReport();                                   ///< Create a new report using assistant dialog and open report window.
    void openReport(const QString& pFileName);          ///< Open report from file and show it in report window.

private:
    void showReportWindow(Report&& pReport);            ///< Hide this window and create and show the report window.

private slots:
    void on_reportWindowClosed();                       ///< Destroy the report window and show this window again.
    //
    void on_newReport_pushButton_pressed();             ///< Create (and show) a new report.
    void on_loadReport_pushButton_pressed();            ///< Open a report from file,
    void on_personnel_pushButton_pressed();             ///< Maintain the personnel database.
    void on_settings_pushButton_pressed();              ///< Change the program settings.
    void on_about_pushButton_pressed();                 ///< Show program information.
    void on_quit_pushButton_pressed();                  ///< Close the program.

private:
    Ui::StartupWindow *ui;                          //UI
    //
    std::unique_ptr<ReportWindow> reportWindowPtr;  //The report window
};
#endif // STARTUPWINDOW_H
