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
 * \param pParent The parent widget.
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

    //Enable drag and drop in order to open reports being dropped on the window
    setAcceptDrops(true);
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
        if (reportWindowPtrs.empty())
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
 * Loads a report from \p pFileName and, if successful, shows it in a newly created report window after hiding this window.
 *
 * \param pFileName File name of the report.
 * \return If successful.
 */
bool StartupWindow::openReport(const QString& pFileName)
{
    Report report;
    if (!report.open(pFileName))
    {
        QMessageBox(QMessageBox::Warning, "Fehler", "Konnte Wachbericht nicht laden!", QMessageBox::Ok, this).exec();
        return false;
    }

    showReportWindow(std::move(report));

    return true;
}

//Private

/*!
 * \brief Reimplementation of QMainWindow::dragEnterEvent().
 *
 * Reimplements QMainWindow::dragEnterEvent().
 *
 * \p pEvent is accepted, if the window was entered by a drag and drop action that represents a single file,
 * and is ignored otherwise. Before accepting, the event's proposed action is changed to Qt::DropAction::LinkAction.
 *
 * \param pEvent The event containing information about the drag and drop action that entered the window.
 */
void StartupWindow::dragEnterEvent(QDragEnterEvent* pEvent)
{
    if (pEvent->mimeData()->hasUrls() && pEvent->mimeData()->urls().length() == 1)
    {
        pEvent->setDropAction(Qt::DropAction::LinkAction);
        pEvent->accept();
    }
    else
        pEvent->ignore();
}

/*!
 * \brief Reimplementation of QMainWindow::dropEvent().
 *
 * Reimplements QMainWindow::dropEvent().
 *
 * \p pEvent is accepted (using event action Qt::DropAction::LinkAction), if a single
 * file was dropped on the window, and is ignored otherwise. See also dragEnterEvent().
 *
 * As the file is assumed to be a saved Report, it is tried to open a report from the dropped file name (see openReport()).
 *
 * \param pEvent The event containing information about the drag and drop action that was dropped on the window.
 */
void StartupWindow::dropEvent(QDropEvent* pEvent)
{
    if (pEvent->mimeData()->hasUrls() && pEvent->mimeData()->urls().length() == 1)
    {
        pEvent->setDropAction(Qt::DropAction::LinkAction);
        pEvent->accept();

        openReport(pEvent->mimeData()->urls().at(0).toLocalFile());
    }
    else
        pEvent->ignore();
}

//

/*!
 * \brief Hide this window and create and show a new report window.
 *
 * Creates a new report window, moving \p pReport to the window instance.
 *
 * This window gets hidden and the ReportWindow::closed() signal is
 * connected to on_reportWindowClosed() in order to eventually remove
 * the window from the list of open report windows and to re-show
 * this window when no other report windows are still open.
 *
 * The ReportWindow::openAnotherReportRequested() signal is connected to on_openAnotherReportRequested()
 * in order to be able to open other report windows from within a report window.
 *
 * \param pReport The actual report to use for the report window.
 */
void StartupWindow::showReportWindow(Report&& pReport)
{
    //Create a new report window
    std::unique_ptr<ReportWindow> reportWindowPtr = std::make_unique<ReportWindow>(std::move(pReport), nullptr);

    //Always automatically delete window on close
    reportWindowPtr->setAttribute(Qt::WA_DeleteOnClose);

    //React on report window's closed() signal to release pointer and to show startup window again if no report windows open anymore
    connect(reportWindowPtr.get(), SIGNAL(closed(const ReportWindow *const)),
            this, SLOT(on_reportWindowClosed(const ReportWindow *const)));

    //React on report window's openAnotherReportRequested() signal to open an existing or a new report in another report window
    connect(reportWindowPtr.get(), SIGNAL(openAnotherReportRequested(const QString&, bool)),
            this, SLOT(on_openAnotherReportRequested(const QString&, bool)));

    //Hide startup window before showing report window
    hide();

    reportWindowPtr->show();

    //Add new window to list of open report windows
    reportWindowPtrs.insert(std::move(reportWindowPtr));
}

//Private slots

/*!
 * \brief Destroy and remove the pointer to the closed report window.
 *
 * Disconnects this very slot and on_openAnotherReportRequested() from the connected signals again,
 * destroys the report window \p pWindow, removes it from the list of open report windows and,
 * if no other report window is still open, shows this window again.
 *
 * \param pWindow Pointer to the report window that has been closed.
 */
void StartupWindow::on_reportWindowClosed(const ReportWindow *const pWindow)
{
    decltype(reportWindowPtrs)::const_iterator windowIt = reportWindowPtrs.end();

    for (decltype(windowIt) tWindowIt = reportWindowPtrs.begin(); tWindowIt != reportWindowPtrs.end(); ++tWindowIt)
    {
        if ((*tWindowIt).get() == pWindow)
        {
            windowIt = tWindowIt;
            break;
        }
    }

    if (windowIt == reportWindowPtrs.end())
        return;

    disconnect(pWindow, SIGNAL(closed(const ReportWindow *const)),
               this, SLOT(on_reportWindowClosed(const ReportWindow *const)));
    disconnect(pWindow, SIGNAL(openAnotherReportRequested(const QString&, bool)),
               this, SLOT(on_openAnotherReportRequested(const QString&, bool)));

    //Window is already deleted, see showReportWindow()
    reportWindowPtrs.extract(windowIt).value().release();

    if (reportWindowPtrs.empty())
        show();
}

/*!
 * \brief Load a report from file and open a new report window for it.
 *
 * See openReport(). If \p pFileName is empty, a new report is created and shown instead (see newReport()).
 *
 * If \p pChooseFile is true, it will be asked for a file name. Note that in this
 * case no empty report will be created and shown if the file dialog is rejected.
 *
 * \param pFileName File name of the report.
 * \param pChooseFile Ask for a file name (ignore \p pFileName).
 */
void StartupWindow::on_openAnotherReportRequested(const QString& pFileName, bool pChooseFile)
{
    if (pChooseFile)
        on_loadReport_pushButton_pressed();
    else if (pFileName == "")
        newReport();
    else
        openReport(pFileName);
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
