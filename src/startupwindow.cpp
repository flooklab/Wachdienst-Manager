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

#include "startupwindow.h"
#include "ui_startupwindow.h"

/*!
 * \brief Constructor.
 *
 * Constructs the window.
 *
 * Inserts the program version into the window title.
 *
 * Adds shortcuts for all push buttons:
 * - New report: Ctrl+N
 * - Load report: Ctrl+O
 * - Personnel: Ctrl+P
 * - Settings: Ctrl+E
 * - About: Ctrl+A
 * - Quit: Ctrl+Q
 *
 * \param pParent
 */
StartupWindow::StartupWindow(QWidget *const pParent) :
    QMainWindow(pParent),
    ui(new Ui::StartupWindow)
{
    ui->setupUi(this);

    //Show current program version in window title
    setWindowTitle("DLRG Wachdienst-Manager " + Aux::programVersionStringPretty);

    //Add button shortcuts

    QShortcut* newReportShortcut = new QShortcut(QKeySequence("Ctrl+N"), this);
    connect(newReportShortcut, SIGNAL(activated()), this, SLOT(on_newReport_pushButton_pressed()));

    QShortcut* loadReportShortcut = new QShortcut(QKeySequence("Ctrl+O"), this);
    connect(loadReportShortcut, SIGNAL(activated()), this, SLOT(on_loadReport_pushButton_pressed()));

    QShortcut* personnelShortcut = new QShortcut(QKeySequence("Ctrl+P"), this);
    connect(personnelShortcut, SIGNAL(activated()), this, SLOT(on_personnel_pushButton_pressed()));

    QShortcut* settingsShortcut = new QShortcut(QKeySequence("Ctrl+E"), this);
    connect(settingsShortcut, SIGNAL(activated()), this, SLOT(on_settings_pushButton_pressed()));

    QShortcut* aboutShortcut = new QShortcut(QKeySequence("Ctrl+A"), this);
    connect(aboutShortcut, SIGNAL(activated()), this, SLOT(on_about_pushButton_pressed()));

    QShortcut* quitShortcut = new QShortcut(QKeySequence("Ctrl+Q"), this);
    connect(quitShortcut, SIGNAL(activated()), this, SLOT(on_quit_pushButton_pressed()));
}

/*!
 * \brief Destructor.
 */
StartupWindow::~StartupWindow()
{
    delete ui;
}

//Public

/*!
 * \brief Create a new report using assistant dialog and open report window.
 *
 * Hides this window and shows the new report assistant dialog to create a new report.
 * The report is then shown in a newly created report window.
 */
void StartupWindow::newReport()
{
    //Hide startup window before showing new report assistant dialog (clean startup with "-n" command line argument)
    hide();

    NewReportDialog newReportDialog(nullptr);

    if (newReportDialog.exec() != QDialog::Accepted)
    {
        show();
        return;
    }

    Report report;
    newReportDialog.getReport(report);

    showReportWindow(std::move(report));
}

/*!
 * \brief Open report from file and show it in report window.
 *
 * Loads a report from \p pFileName and shows it in a newly created report window after hiding this window.
 *
 * \param pFileName
 */
void StartupWindow::openReport(const QString& pFileName)
{
    Report report;
    if (!report.open(pFileName))
    {
        QMessageBox(QMessageBox::Warning, "Fehler", "Konnte Wachericht nicht laden!", QMessageBox::Ok, this).exec();
        return;
    }

    showReportWindow(std::move(report));
}

//Private

/*!
 * \brief Hide this window and create and show the report window.
 *
 * Creates a new report window, moving \p pReport to the window instance.
 * This window is hidden and the 'closed()' signal from the report window
 * is connected to on_reportWindowClosed() in order to re-show
 * this window when the report window has closed.
 *
 * \param pReport
 */
void StartupWindow::showReportWindow(Report&& pReport)
{
    //Create report window
    reportWindowPtr = std::make_unique<ReportWindow>(std::move(pReport), nullptr);

    //Connect report window's closed signal to a slot here to show startup window again and to release the pointer
    reportWindowPtr->setAttribute(Qt::WA_DeleteOnClose);
    connect(reportWindowPtr.get(), SIGNAL(closed()), this, SLOT(on_reportWindowClosed()));

    //Hide startup window before showing report window
    hide();

    reportWindowPtr->show();
}

//Private slots

/*!
 * \brief Destroy the report window and show this window again.
 *
 * Disconnects this slot from the report window 'closed()' signal again,
 * destroys the report window and shows this window again.
 */
void StartupWindow::on_reportWindowClosed()
{
    disconnect(reportWindowPtr.get(), SIGNAL(closed()), this, SLOT(on_reportWindowClosed()));
    reportWindowPtr.release();
    show();
}

//

/*!
 * \brief Create (and show) a new report.
 *
 * Create a new report using the assistant dialog and show it in the report window.
 */
void StartupWindow::on_newReport_pushButton_pressed()
{
    newReport();
}

/*!
 * \brief Open a report from file,
 *
 * Ask for a file name, load a report from this file and show it in the report window.
 */
void StartupWindow::on_loadReport_pushButton_pressed()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Wachbericht öffnen", "", "Wachberichte (*.wbr)");

    if (fileName == "")
        return;

    openReport(fileName);
}

/*!
 * \brief Maintain the personnel database.
 *
 * Open a dialog to maintain the personnel database.
 */
void StartupWindow::on_personnel_pushButton_pressed()
{
    PersonnelDatabaseDialog personnelDialog(this);
    personnelDialog.exec();
}

/*!
 * \brief Change the program settings.
 *
 * Open a dialog to change program settings.
 */
void StartupWindow::on_settings_pushButton_pressed()
{
    SettingsDialog settingsDialog(this);
    settingsDialog.exec();
}

/*!
 * \brief Show program information.
 *
 * Open a dialog to show program version information, license etc.
 */
void StartupWindow::on_about_pushButton_pressed()
{
    AboutDialog aboutDialog(this);
    aboutDialog.exec();
}

/*!
 * \brief Close the program.
 */
void StartupWindow::on_quit_pushButton_pressed()
{
    close();
}
