/*
/////////////////////////////////////////////////////////////////////////////////////////
//
//  This file is part of Wachdienst-Manager, a program to manage DLRG watch duty reports.
//  Copyright (C) 2021–2023 M. Frohne
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

#include "report.h"
#include "reportwindow.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMainWindow>
#include <QString>
#include <QWidget>

#include <memory>
#include <set>

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
    StartupWindow(QWidget* pParent = nullptr);              ///< Constructor.
    ~StartupWindow();                                       ///< Destructor.
    //
    void newReport();                                       ///< Create a new report using assistant dialog and open report window.
    bool openReport(const QString& pFileName);              ///< Open report from file and show it in report window.
    //
    void emitOpenAnotherReportRequested(const QString& pFileName);  ///< Emit the openAnotherReportRequested() signal.

private:
    void dragEnterEvent(QDragEnterEvent* pEvent) override;  ///< Reimplementation of QMainWindow::dragEnterEvent().
    void dropEvent(QDropEvent* pEvent) override;            ///< Reimplementation of QMainWindow::dropEvent().
    //
    void showReportWindow(Report&& pReport);                ///< Hide this window and create and show a new report window.

private slots:
    void on_reportWindowClosed(const ReportWindow* pWindow);                                ///< \brief Destroy and remove the pointer
                                                                                            ///  to the closed report window.
    void on_openAnotherReportRequested(const QString& pFileName, bool pChooseFile = false); ///< \brief Load a report from file and
                                                                                            ///  open a new report window for it.
    //
    void on_newReport_pushButton_pressed();                 ///< Create (and show) a new report.
    void on_loadReport_pushButton_pressed();                ///< Open a report from file.
    void on_personnel_pushButton_pressed();                 ///< Maintain the personnel database.
    void on_settings_pushButton_pressed();                  ///< Change the program settings.
    void on_about_pushButton_pressed();                     ///< Show program information.
    void on_quit_pushButton_pressed();                      ///< Close the program.

signals:
    void openAnotherReportRequested(const QString& pFileName);  ///< \brief Signal emitted when "master" application instance received
                                                                ///  request by a "slave" instance to open another report window.

private:
    Ui::StartupWindow* ui;                                      //UI
    //
    std::set<std::unique_ptr<ReportWindow>> reportWindowPtrs;   //All open report windows
};
#endif // STARTUPWINDOW_H
