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

#include "pdfexporter.h"

//Public

/*!
 * \brief Export report as PDF file.
 *
 * Calls reportToLaTeX() to generate LaTeX code from \p pReport, which is then compiled using
 * configured XeLaTeX executable in a temporary directory and copied over to \p pFileName.
 * \p pPersonnelTableMaxLength and \p pBoatDrivesTableMaxLength are passed to reportToLaTeX().
 *
 * Images required for the LaTeX document are copied from compiled resources to temporary directory before compilation.
 *
 * For compilation of the document a separate process is started, which is killed after 30s (assume compilation error).
 *
 * \param pReport The report to use to create the PDF.
 * \param pFileName Location / file name for the PDF file.
 * \param pPersonnelTableMaxLength See reportToLaTeX().
 * \param pBoatDrivesTableMaxLength See reportToLaTeX().
 * \return If compilation and copying was successful.
 */
bool PDFExporter::exportPDF(const Report& pReport, const QString& pFileName,
                            int pPersonnelTableMaxLength, int pBoatDrivesTableMaxLength)
{
    //Generate content of LaTeX document
    QString texString;
    reportToLaTeX(pReport, texString, pPersonnelTableMaxLength, pBoatDrivesTableMaxLength);

    //XeLaTeX application path
    //Note: Suppressing potential message boxes since this function likely executed in different thread (not possible with QMessageBox)
    QString texProg = SettingsCache::getStrSetting("app_export_xelatexPath", true);

    if (!QFileInfo::exists(texProg))
    {
        std::cerr<<"ERROR: XeLaTeX path does not exist!"<<std::endl;
        return false;
    }

    //Create temporary compilation directory
    QTemporaryDir tmpDir;
    if (!tmpDir.isValid())
    {
        std::cerr<<"ERROR: Could not create temporary directory!"<<std::endl;
        return false;
    }

    //Write LaTeX document to temporary directory

    QString texFileBaseName = "report";
    QString texFileName = texFileBaseName + ".tex";
    QString texPDFFileName = texFileBaseName + ".pdf";

    QFile texFile(tmpDir.filePath(texFileName));

    if (!texFile.open(QIODevice::WriteOnly))
    {
        texFile.close();
        std::cerr<<"ERROR: Could not create .tex file!"<<std::endl;
        return false;
    }
    if (texFile.write(texString.toUtf8()) == -1)
    {
        texFile.close();
        std::cerr<<"ERROR: Could not write to .tex file!"<<std::endl;
        return false;
    }
    texFile.close();

    //Write association logo to temporary directory

    QString logoFileName = "logo.png";
    QFile logoFile(":/resources/images/dlrg-logo.png");

    if (!logoFile.copy(tmpDir.filePath(logoFileName)))
    {
        std::cerr<<"ERROR: Could not create association logo file!"<<std::endl;
        return false;
    }

    //Compile the document

    QProcess process;
    process.setWorkingDirectory(tmpDir.path());
    process.start(texProg, {"-no-shell-escape", "-output-directory", tmpDir.path(), tmpDir.filePath(texFileName)});

    //Assume compilation/syntax error after 30s
    process.waitForFinished(30000);

    if (process.error() == QProcess::Timedout)
    {
        process.kill();
        process.close();
        std::cerr<<"ERROR: XeLaTeX process timed out after 30s! Syntax error?"<<std::endl;
        return false;
    }
    if (process.exitStatus() != QProcess::NormalExit)
    {
        process.close();
        std::cerr<<"ERROR: XeLaTeX process stopped with non-zero exit code!"<<std::endl;
        return false;
    }
    process.close();

    //Copy PDF from temporary directory to requested path
    if ((QFileInfo::exists(pFileName) && !QFile::remove(pFileName)) || !QFile::copy(tmpDir.filePath(texPDFFileName), pFileName))
    {
        std::cerr<<"ERROR: Could not copy compiled PDF file!"<<std::endl;
        return false;
    }

    return true;
}

//Private

/*!
 * \brief Generate LaTeX document from report.
 *
 * Insert \p pReport contents into a report LaTeX code template to form a report document.
 * Generated LaTeX code is assigned to \p pTeXString.
 *
 * \param pReport The report to use to create the PDF.
 * \param pTeXString Destination for the generated LaTeX code.
 * \param pPersonnelTableMaxLength Row count limit for personnel table. Inserts additional page to continue table if this exceeded.
 * \param pBoatDrivesTableMaxLength Row count limit for boat drives table. Inserts additional page to continue table if this exceeded.
 */
void PDFExporter::reportToLaTeX(const Report& pReport, QString& pTeXString, int pPersonnelTableMaxLength, int pBoatDrivesTableMaxLength)
{
    QString rawTexString0 = "\\documentclass[a4paper, notitlepage, 10pt]{scrreprt}\n"
       "\n"
       "\\usepackage[T1]{fontenc}\n"
       "\\usepackage[utf8]{inputenc}\n"
       "\\usepackage[ngerman]{babel}\n"
       "\n"
       "\\usepackage[top=0.4in, left=0.5in, bottom=0.4in, right=0.4in]{geometry}\n"
       "\\usepackage{calc}\n"
       "\n"
       "\\usepackage{amssymb}\n"
       "\n"
       "\\usepackage{siunitx}\n"
       "\n"
       "\\usepackage{ulem}\n"
       "\n"
       "\\usepackage{array}\n"
       "\\usepackage{extdash}\n"
       "\\usepackage{multirow}\n"
       "\\usepackage{makecell}\n"
       "\\usepackage{booktabs}\n"
       "\\usepackage{longtable}\n"
       "\n"
       "\\usepackage{graphicx}\n"
       "\n"
       "%1"
       "\\setlength{\\parindent}{0pt}\n"
       "\n"
       "\\begin{document}\n";

    //Document font

    //Note: Suppressing potential message boxes since this function likely executed in different thread (not possible with QMessageBox)
    QString tFontFamily = SettingsCache::getStrSetting("app_export_fontFamily", true);
    Aux::latexEscapeSpecialChars(tFontFamily);
    Aux::latexFixLineBreaksNoLineBreaks(tFontFamily);

    QString tFontsString;

    //Only set fonts (explicitly), if not set to "CMU"
    if (tFontFamily != "CMU")
    {
        QString tFontMain = tFontFamily;
        QString tFontSans = tFontMain;
        QString tFontMono = tFontMain;

        tFontsString = QString("\\usepackage{fontspec}\n"
                               "\\setmainfont{%1}\n"
                               "\\setsansfont{%2}\n"
                               "\\setmonofont{%3}\n"
                               "\n").arg(tFontMain, tFontSans, tFontMono);
    }

    QString texString0 = rawTexString0.arg(tFontsString);

    //Report header

    //Station defaults
    QString tLocalGroup = "---";
    QString tDistrictAssociation = "---";
    QString tStationLocation = "---";
    QString tStationName = "---";

    //Get station information from database
    std::map<int, Aux::Station> tStations = DatabaseCache::stations();
    int tStRowId = 0;
    QString tStName, tStLoc;
    if (Aux::stationNameLocationFromIdent(pReport.getStation(), tStName, tStLoc) &&
        DatabaseCache::stationRowIdFromNameLocation(tStName, tStLoc, tStRowId))
    {
        const Aux::Station& tStation = tStations.at(tStRowId);

        tLocalGroup = tStation.localGroup;
        tDistrictAssociation = tStation.districtAssociation;
        tStationLocation = tStation.location;
        tStationName = tStation.name;

        Aux::latexUseHyphdash(tLocalGroup);
        Aux::latexUseHyphdash(tDistrictAssociation);
        Aux::latexUseHyphdash(tStationLocation);
        Aux::latexUseHyphdash(tStationName);
    }

    //Combine duty purpose with its comment, if not empty

    QString tPurpose = "\\hspace{0pt}" + Report::dutyPurposeToLabel(pReport.getDutyPurpose());
    Aux::latexUseHyphdash(tPurpose);

    if (pReport.getDutyPurposeComment() != "")
    {
        QString tPurposeComment = pReport.getDutyPurposeComment();
        Aux::latexEscapeSpecialChars(tPurposeComment);
        Aux::latexFixLineBreaksNoLineBreaks(tPurposeComment);
        Aux::latexUseHyphdash(tPurposeComment);

        tPurpose += "\\newline{}\\hspace{0pt}(\\textit{" + std::move(tPurposeComment) + "})\\vspace{-0.25\\baselineskip}";
    }

    //Station radio call name
    QString tStationRadioCallName = pReport.getRadioCallName();
    Aux::latexUseHyphdash(tStationRadioCallName);

    QString rawTexString1 = "{\\LARGE\\textbf{Wachbericht}}\n"
       "\n"
       "\\vspace{-2pt}\\hspace{-0.5in}\n"
       "\\begin{minipage}[b][0pt][t]{\\linewidth+0.5in+0.4in-2pt-8pt}\\vspace{-51pt+8pt}\\hfill\\includegraphics[width=105pt]{logo}\\end{minipage}\n"
       "\n"
       "\\begin{minipage}{\\linewidth}\n"
       "\\renewcommand{\\arraystretch}{1.55}\n"
       "\\begin{tabular}{>{}p{0.095\\linewidth}>{}p{0.28\\linewidth}>{}p{0.10\\linewidth}>{}p{0.09\\linewidth}\n"
       "                 >{}p{0.09\\linewidth}>{}p{0.09\\linewidth}}\n"
       "\\textbf{Bezirk/OG:} & \\hspace{0pt}%2 / %10 & \\textbf{Lfd. Nr.:} & %3 && \\\\\n"
       "\\textbf{Station:} & \\hspace{0pt}%4 & \\textbf{Dienstzweck:} & \\multicolumn{3}{p{0.27\\linewidth}}{%5} \\\\\n"
       "\\textbf{Ort:} & \\hspace{0pt}%1 & \\textbf{Funkruf:} & \\multicolumn{3}{p{0.27\\linewidth}}{\\hspace{0pt}%6} \\\\\n"
       "\\textbf{Datum:} & %7 & \\textbf{Beginn:} & %8 & \\textbf{Ende:} & %9 \\\\\n"
       "\\end{tabular}\n"
       "\\end{minipage}\n"
       "\\vfill\n\n\\vspace{-7pt}";

    QString texString1 = rawTexString1.arg(tStationLocation, tDistrictAssociation, QString::number(pReport.getNumber()),
                                           tStationName, tPurpose).arg(
                                           tStationRadioCallName, pReport.getDate().toString("dd.MM.yyyy"),
                                           pReport.getBeginTime().toString("hh:mm"), pReport.getEndTime().toString("hh:mm"),
                                           tLocalGroup);

    QString texString2 = "\\subsection*{Wachmannschaft}\n"
       "\\vspace{3pt}\n"
       "\\renewcommand{\\arraystretch}{0.6}\n"
       "\\begin{tabular}{>{\\raggedleft}p{0.02\\linewidth}>{\\raggedright}p{0.26\\linewidth}>{\\raggedright}p{0.26\\linewidth}\n"
       "                 >{\\raggedright}p{0.09\\linewidth}>{\\raggedleft}p{0.07\\linewidth}>{\\raggedleft}p{0.07\\linewidth}\n"
       "                 >{\\raggedleft\\arraybackslash}p{0.07\\linewidth}}\n"
       "\\textbf{Nr.} & \\textbf{Name} & \\textbf{Vorname} & \\textbf{Funktion} & \\textbf{Beginn} & \\textbf{Ende} &\n"
       "\\textbf{Gesamt}\\\\\n"
       "\\toprule\n";

    //Row template for personnel table
    QString tRowString = "\\textbf{%1} & \\hspace{0pt}%2 & \\hspace{0pt}%3 & %4 & %5 & %6 & %7 \\\\";

    //Sorted personnel list
    std::vector<QString> tPersonnelSorted = pReport.getPersonnel(true);
    int tPersonnelSize = tPersonnelSorted.size();

    int tPersonNumber = 1;                                                  //Enumerate personnel
    int tTotalPersonnelMinutes = 0;                                         //Print total personnel hours at the end of the table

    bool splitPersonnelTable = (tPersonnelSize > pPersonnelTableMaxLength); //Continue long table on next page

    //Add a table row for each person
    for (const QString& tIdent : tPersonnelSorted)
    {        
        const Person tPerson = pReport.getPerson(tIdent);
        const Person::Function tFunction = pReport.getPersonFunction(tIdent);
        const QTime tBeginTime = pReport.getPersonBeginTime(tIdent);
        const QTime tEndTime = pReport.getPersonEndTime(tIdent);

        int dMinutes = tBeginTime.secsTo(tEndTime) / 60;

        if (dMinutes < 0)
            dMinutes += 24 * 60;

        tTotalPersonnelMinutes += dMinutes;

        QTime tDuration = QTime(0, 0, 0).addSecs(dMinutes * 60);

        QString tLastNameStr = tPerson.getLastName();
        Aux::latexUseHyphdash(tLastNameStr);

        QString tFirstNameStr = tPerson.getFirstName();
        Aux::latexUseHyphdash(tFirstNameStr);

        if (splitPersonnelTable && tPersonNumber == pPersonnelTableMaxLength)
        {
            //Hint at continuation of the table on next page, if table is too long and split here
            texString2.append(" \\midrule\n");
            texString2.append("\\textbf{\\dots} & \\multicolumn{5}{c}{\\textit{Fortsetzung auf nächster Seite}} & "
                              "\\dots\\vspace{0pt} \\\\");
        }
        else if (splitPersonnelTable && tPersonNumber > pPersonnelTableMaxLength)
        {
            //Skip remaining rows here, if table is too long and continued on next page
            ;   //(sic!)
        }
        else
        {
            if (tPersonNumber > 1)
                texString2.append(" \\midrule\n");

            texString2.append(tRowString.arg(QString::number(tPersonNumber), tLastNameStr, tFirstNameStr,
                                             Person::functionToLabel(tFunction),
                                             tBeginTime.toString("hh:mm"), tEndTime.toString("hh:mm"), tDuration.toString("hh:mm")));
        }

        ++tPersonNumber;
    }

    //Calculate personnel hours split in hours and minutes

    int tTotalPersonnelModMinutes = tTotalPersonnelMinutes % 60;
    int tTotalPersonnelHours = (tTotalPersonnelMinutes - tTotalPersonnelModMinutes) / 60;

    int tCarryPersonnelModMinutes = pReport.getPersonnelMinutesCarry() % 60;
    int tCarryPersonnelHours = (pReport.getPersonnelMinutesCarry() - tCarryPersonnelModMinutes) / 60;

    int tTotalPersonnelModMinutesWithCarry = (tTotalPersonnelMinutes + pReport.getPersonnelMinutesCarry()) % 60;
    int tTotalPersonnelHoursWithCarry = (tTotalPersonnelMinutes + pReport.getPersonnelMinutesCarry() -
                                         tTotalPersonnelModMinutesWithCarry) / 60;
    texString2.append(QString("\n"
       "\\bottomrule\\addlinespace[\\belowrulesep]\n"
       "\\multicolumn{6}{r}{Einsatzstunden} & %1\\vspace{1pt} \\\\\n"
       "\\multicolumn{6}{r}{+ Übertrag} & %2\\vspace{3pt} \\\\\n"
       "\\multicolumn{6}{r}{= Gesamt} & \\textbf{%3} \\\\\n"
       "\\end{tabular}\n"
       "\\vspace{-2pt}\n"
       "\\vfill\n\n").arg(QString::asprintf("%02u:%02u", tTotalPersonnelHours, tTotalPersonnelModMinutes),
                          QString::asprintf("%02u:%02u", tCarryPersonnelHours, tCarryPersonnelModMinutes),
                          QString::asprintf("%02u:%02u", tTotalPersonnelHoursWithCarry, tTotalPersonnelModMinutesWithCarry)));

    //Weather conditions

    QString tWeatherComments = pReport.getWeatherComments();
    Aux::latexEscapeSpecialChars(tWeatherComments);
    Aux::latexFixLineBreaks(tWeatherComments);
    Aux::latexUseHyphdash(tWeatherComments);
    if (tWeatherComments == "")
        tWeatherComments = "---";

    QString rawTexString3 = "\\begin{minipage}{\\linewidth}\n"
       "\\subsection*{Wetter}\n"
       "\\renewcommand{\\arraystretch}{1.2}\n"
       "\\begin{tabular}{>{\\raggedright}p{0.155\\linewidth}>{\\raggedright}p{0.065\\linewidth}>{\\raggedright}p{0.11\\linewidth}\n"
       "                 >{\\raggedright}p{0.18\\linewidth}>{\\raggedright}p{0.12\\linewidth}\n"
       "                 >{\\raggedright\\arraybackslash}p{0.25\\linewidth}}\n"
       "Lufttemperatur: & \\SI{%1}{\\degreeCelsius} & Bewölkung: & %3 & Wind: & %5 \\\\\n"
       "Wassertemperatur: & \\SI{%2}{\\degreeCelsius} & Niederschlag: & %4 & Bemerkungen: & \\hspace{0pt}%6 \n"
       "\\end{tabular}\n"
       "\\end{minipage}\n"
       "\\vspace{7pt}\n\\vfill\n\n";

    QString texString3 = rawTexString3.arg(QString::number(pReport.getAirTemperature()),
                                           QString::number(pReport.getWaterTemperature()),
                                           Aux::cloudinessToLabel(pReport.getCloudiness()),
                                           Aux::precipitationToLabel(pReport.getPrecipitation()),
                                           Aux::windStrengthToLabel(pReport.getWindStrength()), tWeatherComments);

    QString texString4 = "\\newsavebox{\\rescuesBox}\n"
       "\\savebox{\\rescuesBox}{\n"
       "\\begin{minipage}{0.45\\linewidth}\n"
       "\\subsection*{Hilfeleistungen}\n"
       "\\renewcommand{\\arraystretch}{0.6}\n"
       "\\begin{tabular}{>{\\raggedright}p{0.8\\linewidth}>{\\raggedleft\\arraybackslash}p{0.12\\linewidth}}\n"
       "\\textbf{Art der Hilfeleistung} & \\textbf{Anzahl} \\\\ \\toprule\n";

    //Add table row for each type of rescue operation, summarizing the numbers of carried out operations

    //Find number of different rescue operation types first, to properly format table

    //Lambda expression to count number of rescue operation types. To be executed for each available 'RescueOperation'.
    auto tCntFunc = [](Report::RescueOperation pRescue, int& pNumberLeistungen) -> void { ++pNumberLeistungen; (void)pRescue; };

    int tNumberRescueTypes = 0;
    int tCurrentRescueType = 1;

    Report::iterateRescueOperations(tCntFunc, tNumberRescueTypes);

    /*
     * Lambda expression to add a table row displaying the number of carried out rescue operations of the specified type.
     * To be executed for each available 'RescueOperation'.
     */
    auto tRescOpFunc = [](Report::RescueOperation pRescue, const std::map<Report::RescueOperation, int>& pRescuesCountMap,
                          QString& pTexString4, int pNumberRescueTypes, int& pCurrentRescueType) -> void
    {
        pTexString4.append(QString("%1 & %2 \\\\").arg(Report::rescueOperationToLabel(pRescue),
                                                       QString::number(pRescuesCountMap.at(pRescue))));

        if (pCurrentRescueType++ < pNumberRescueTypes)
            pTexString4.append(" \\midrule\n");
        else
            pTexString4.append(" \\bottomrule\n");
    };

    //Add new row for each 'RescueOperation'
    auto tRescOpCtrs = pReport.getRescueOperationCtrs();
    Report::iterateRescueOperations(tRescOpFunc, tRescOpCtrs, texString4, tNumberRescueTypes, tCurrentRescueType);

    texString4.append(QString("\\end{tabular}\n"
       "\\end{minipage}\n"
       "}\n"));

    //Vehicles

    QString texString5 = "\\newsavebox{\\vehiclesBox}\n"
       "\\savebox{\\vehiclesBox}{\n"
       "\\begin{minipage}{0.45\\linewidth}\n"
       "\\subsection*{Eingesetzte Einsatzfahrzeuge}\n"
       "\\renewcommand{\\arraystretch}{0.6}\n"
       "\\begin{tabular}{>{\\raggedright}p{0.66\\linewidth}>{\\raggedright\\arraybackslash}p{0.10\\linewidth}\n"
       "                >{\\raggedright\\arraybackslash}p{0.09\\linewidth}}\n"
       "\\textbf{Fahrzeug} & \\textbf{Von} & \\textbf{Bis} \\\\ \\toprule\n";

    auto tNumVehicles = pReport.getVehicles().size();
    decltype(tNumVehicles) tVehicleNumber = 0;

    //Add new row for each vehicle
    for (const auto& it : pReport.getVehicles(true))
    {
        QString vehicle = it.first;
        Aux::latexUseHyphdash(vehicle);

        const QTime arrivalTime = it.second.first;
        const QTime leavingTime = it.second.second;

        texString5.append(QString("\\hspace{0pt}%1 & %2 & %3 \\\\").arg(vehicle, arrivalTime.toString("hh:mm"),
                                                                                 leavingTime.toString("hh:mm")));

        if (++tVehicleNumber < tNumVehicles)
            texString5.append(" \\midrule\n");
    }
    texString5.append("\\bottomrule\n");

    texString5.append(QString("\\end{tabular}\n"
       "\\end{minipage}\n"
       "}\n"
       "\\newsavebox{\\assignmentNumberBox}\n"
       "\\savebox{\\assignmentNumberBox}{\n"
       "\\begin{minipage}{0.45\\linewidth}\\hfill\n"
       "\\begin{tabular}{>{\\raggedleft\\arraybackslash}p{0.45\\linewidth}}\n"
       "\\multicolumn{1}{c}{\\textbf{Einsatznummer LSt:}} \\\\ \\toprule\\vspace{-3pt}\n"
       "%1\n"
       "\\end{tabular}\n"
       "\\end{minipage}\n").arg(pReport.getAssignmentNumber().size() > 0 ? pReport.getAssignmentNumber() : "---"));

    texString5.append(QString("}\n"
       "\\newlength{\\rescuesBoxHeight}\n"
       "\\setlength{\\rescuesBoxHeight}{\\ht\\rescuesBox+\\dp\\rescuesBox}\n"
       "\\newlength{\\vehiclesBoxHeight}\n"
       "\\setlength{\\vehiclesBoxHeight}{\\ht\\vehiclesBox+\\dp\\vehiclesBox}\n"
       "\\newlength{\\assignmentNumberBoxHeight}\n"
       "\\setlength{\\assignmentNumberBoxHeight}{\\ht\\assignmentNumberBox+\\dp\\assignmentNumberBox}\n"
       "\\newlength{\\vehiclesBoxSepLength}\n"
       "\\setlength{\\vehiclesBoxSepLength}{5pt}\n"
       "\\newlength{\\maxMinipageColHeight}\n"
       "\\setlength{\\maxMinipageColHeight}{%\n"
       "\\maxof{\\rescuesBoxHeight}{\\vehiclesBoxHeight+\\assignmentNumberBoxHeight+\\vehiclesBoxSepLength}}\n"
       "\\begin{minipage}[c][\\maxMinipageColHeight][t]{\\linewidth}\n"
       "\\begin{minipage}[c][\\maxMinipageColHeight][t]{0.45\\linewidth}\n"
       "\\usebox{\\rescuesBox}\n"
       "\\vfill\n"
       "\\end{minipage}\n"
       "\\hfill\n"
       "\\begin{minipage}[c][\\maxMinipageColHeight][t]{0.45\\linewidth}\\raggedleft\n"
       "\\usebox{\\vehiclesBox}\n"
       "\\vspace{\\vehiclesBoxSepLength}\\vfill\n"
       "\\usebox{\\assignmentNumberBox}\n"
       "\\end{minipage}\n"
       "\\end{minipage}\n"
       "\\vspace{10pt}\n\\vfill\n\n"));

    QString rawTexString6 = "\\begin{minipage}{\\linewidth}\n"
       "\\subsection*{Bemerkungen}\n"
       "\\uline{\\mbox{}%1\\mbox{}\\hfill}\n"
       "\\end{minipage}\n"
       "\\\\\\\\\\vspace{-2pt}\n\\vfill\n\n";

    QString tComments = pReport.getComments();
    Aux::latexEscapeSpecialChars(tComments);
    Aux::latexFixLineBreaksUline(tComments);
    QString texString6 = rawTexString6.arg(tComments);

    //Enclosures

    bool tEnclosedBoatLog = true;   //Boat log always enclosed
    int tEnclosedOperationProtocols = pReport.getOperationProtocolsCtr();
    int tEnclosedPatientRecords = pReport.getPatientRecordsCtr();
    int tEnclosedRadioCallLogs = pReport.getRadioCallLogsCtr();

    QString tEnclosedOperationProtocolsStr = "($\\times$\\," + QString::number(tEnclosedOperationProtocols) + ")";
    QString tEnclosedPatientRecordsStr = "($\\times$\\," + QString::number(tEnclosedPatientRecords) + ")";
    QString tEnclosedRadioCallLogsStr = "($\\times$\\," + QString::number(tEnclosedRadioCallLogs) + ")";

    //Omit numbers, if they are zero
    if (tEnclosedOperationProtocols == 0)
        tEnclosedOperationProtocolsStr = "\\hphantom{($\\times$\\,0)}";
    if (tEnclosedPatientRecords == 0)
        tEnclosedPatientRecordsStr = "\\hphantom{($\\times$\\,0)}";
    if (tEnclosedRadioCallLogs == 0)
        tEnclosedRadioCallLogsStr = "\\hphantom{($\\times$\\,0)}";

    QString tOtherEnclosures = pReport.getOtherEnclosures();
    Aux::latexEscapeSpecialChars(tOtherEnclosures);
    Aux::latexFixLineBreaksNoLineBreaks(tOtherEnclosures);

    if (tOtherEnclosures.length() == 0)
        tOtherEnclosures = "\\mbox{\\hspace{200pt}}";   //Keep some empty underlined space

    QString rawTexString7 = "\\begin{minipage}{\\linewidth}\n"
       "Anlagen zum Wachbericht:\\vspace*{5pt}\\\\\n"
       "\\mbox{$%1$ Bootstagebuch \\qquad\\qquad $%2$ Einsatzprotokoll %6\\qquad\\qquad $%3$ Patientenprotokoll %7"
                                 "\\qquad\\qquad $%4$ Funktagebuch %8}\n"
       "\\vspace*{5pt}\\\\\n"
       "Weitere Anlagen:\\\\\\\\[-8pt]\n"
       "\\hphantom{X}\\uline{\\mbox{}\\,%5\\ \\ \\mbox{}}\n"
       "\\end{minipage}\n"
       "\\vspace{-13pt}\n"
       "\n"
       "\\hfill\\vfill\\hfill\\parbox[c][0pt][r]{150pt}{\\hrule \\vspace{3pt} Unterschrift Stationsleiter \\vspace{-2pt}}\n";

    QString texString7 = rawTexString7.arg(tEnclosedBoatLog ? "\\boxtimes" : "\\Box",
                                           (tEnclosedOperationProtocols > 0) ? "\\boxtimes" : "\\Box",
                                           (tEnclosedPatientRecords > 0) ? "\\boxtimes" : "\\Box",
                                           (tEnclosedRadioCallLogs > 0) ? "\\boxtimes" : "\\Box",
                                           tOtherEnclosures,
                                           tEnclosedOperationProtocolsStr,
                                           tEnclosedPatientRecordsStr,
                                           tEnclosedRadioCallLogsStr);

    QString texString8 = "";

    //Continue split personnel table here
    if (splitPersonnelTable)
    {
        texString8 = "\n\\clearpage\n\n"
                     "\\subsection*{Fortsetzung: Wachmannschaft}\n"
                     "\\vspace{3pt}\n"
                     "\\renewcommand{\\arraystretch}{0.6}\n"
        "\\begin{longtable}{>{\\raggedleft}p{0.02\\linewidth}>{\\raggedright}p{0.26\\linewidth}>{\\raggedright}p{0.26\\linewidth}\n"
        "                   >{\\raggedright}p{0.11\\linewidth}>{\\raggedleft}p{0.06\\linewidth}>{\\raggedleft}p{0.06\\linewidth}\n"
        "                   >{\\raggedleft\\arraybackslash}p{0.07\\linewidth}}\n"
        "\\textbf{Nr.} & \\textbf{Name} & \\textbf{Vorname} & \\textbf{Funktion} & \\textbf{Beginn} & \\textbf{Ende} &\n"
        "\\textbf{Gesamt}\\\\\n"
        "\\toprule\n"
        "\\textbf{\\dots} & \\multicolumn{5}{c}{\\textit{Fortsetzung von letzter Seite}} & \\dots\\vspace{0pt} \\\\\n"
        "\\midrule\n"
        "\\endhead\n"
        "\\textbf{\\dots} & \\multicolumn{5}{c}{\\textit{Fortsetzung auf nächster Seite}}\\strut &\n"
        "\\dots\\vspace{0pt} \\\\\n"
        "\\bottomrule\n"
        "\\endfoot\n"
        "\\bottomrule\n"
        "\\endlastfoot\n";

        //Reuse enumerator variable from above
        tPersonNumber = 0;

        //Add a table row for each person
        for (const QString& tIdent : tPersonnelSorted)
        {
            //Skip entries that are already in first part of the personnel table
            if (++tPersonNumber < pPersonnelTableMaxLength)
                continue;

            const Person tPerson = pReport.getPerson(tIdent);
            const Person::Function tFunction = pReport.getPersonFunction(tIdent);
            const QTime tBeginTime = pReport.getPersonBeginTime(tIdent);
            const QTime tEndTime = pReport.getPersonEndTime(tIdent);

            int dMinutes = tBeginTime.secsTo(tEndTime) / 60;

            if (dMinutes < 0)
                dMinutes += 24 * 60;

            QTime tDuration = QTime(0, 0, 0).addSecs(dMinutes * 60);

            QString tLastNameStr = tPerson.getLastName();
            Aux::latexUseHyphdash(tLastNameStr);

            QString tFirstNameStr = tPerson.getFirstName();
            Aux::latexUseHyphdash(tFirstNameStr);

            //Skip midrule for first line after header
            if (tPersonNumber > pPersonnelTableMaxLength)
                texString8.append(" \\midrule\n");

            texString8.append(tRowString.arg(QString::number(tPersonNumber), tLastNameStr, tFirstNameStr,
                                             Person::functionToLabel(tFunction),
                                             tBeginTime.toString("hh:mm"), tEndTime.toString("hh:mm"), tDuration.toString("hh:mm")));
        }

        texString8.append(QString("\n"
                                  "\\end{longtable}\n"
                                  "\\vspace{0pt}\n"
                                  "\\vfill\n"));
    }

    //Properly handle page break for two-sided printing setting

    QString pagebreakString = "\n\\clearpage\n";

    //Note: Suppressing potential message boxes since this function likely executed in different thread (not possible with QMessageBox)
    if (SettingsCache::getBoolSetting("app_export_twoSidedPrint", true))
    {
        pagebreakString.append("\\ifodd\\value{page}\n"
                               "\\else\n"
                               "    \\hbox{}\\clearpage\n"
                               "\\fi\n\n");
    }

    //Boat log

    //Boat defaults
    QString tBoatName = "---";
    QString tBoatAcronym = "";
    QString tBoatType = "---";
    QString tBoatFuelType = "---";

    const BoatLog& boatLog = *pReport.boatLog();

    //Get boat information from database
    std::map<int, Aux::Boat> tBoats = DatabaseCache::boats();
    int tBtRowId = 0;
    if (DatabaseCache::boatRowIdFromName(boatLog.getBoat(), tBtRowId))
    {
        const Aux::Boat& tBoat = tBoats.at(tBtRowId);

        tBoatName = tBoat.name;
        tBoatAcronym = tBoat.acronym;
        tBoatType = tBoat.type;
        tBoatFuelType = tBoat.fuelType;

        Aux::latexUseHyphdash(tBoatName);
        Aux::latexUseHyphdash(tBoatType);
        Aux::latexUseHyphdash(tBoatFuelType);
    }

    //Split boat engine hours in parts before and after decimal point

    int tEngineInitialFullHours = static_cast<int>(boatLog.getEngineHoursInitial());
    int tEngineInitialDecimalPlace = static_cast<int>(10 * boatLog.getEngineHoursInitial()) % 10;

    int tEngineFinalFullHours = static_cast<int>(boatLog.getEngineHoursFinal());
    int tEngineFinalDecimalPlace = static_cast<int>(10 * boatLog.getEngineHoursFinal()) % 10;

    //Combine boat acronym and name
    QString tBoatNameStr;
    if (tBoatAcronym == "")
        tBoatNameStr = tBoatName;
    else
        tBoatNameStr = tBoatAcronym + " " + tBoatName;

    //Boat radio call name
    QString tBoatRadioCallName = boatLog.getRadioCallName();
    Aux::latexUseHyphdash(tBoatRadioCallName);

    QString rawTexString9 = "{\\LARGE\\textbf{Bootstagebuch}}\n"
        "\n"
        "\\vspace{-2pt}\\hspace{-0.5in}\n"
        "\\begin{minipage}[b][0pt][t]{\\linewidth+0.5in+0.4in-2pt-8pt}\\vspace{-51pt+8pt}\\hfill\\includegraphics[width=105pt]{logo}\\end{minipage}\n"
        "\n"
        "\\begin{minipage}{\\linewidth}\n"
        "\\renewcommand{\\arraystretch}{1.55}\n"
        "\\begin{tabular}{>{}p{0.095\\linewidth}>{}p{0.28\\linewidth}>{}p{0.11\\linewidth}>{}p{0.075\\linewidth}\n"
        "                >{}p{0.11\\linewidth}>{}p{0.075\\linewidth}}\n"
        "\\textbf{Bezirk/OG:} & \\hspace{0pt}%2 / %1 & \\textbf{Lfd. Nr.:} & %3 && \\\\\n"
        "\\textbf{Boot:} & \\hspace{0pt}%10 & \\textbf{Typ:} & \\multicolumn{3}{p{0.26\\linewidth}}{\\hspace{0pt}%4} \\\\\n"
        "\\textbf{Ort:} & \\hspace{0pt}%5 & \\textbf{Funkruf:} & \\multicolumn{3}{p{0.26\\linewidth}}{\\hspace{0pt}%6} \\\\\n"
        "\\textbf{Datum:} & %7 & \\textbf{BSZ-Start:} & %8 & \\textbf{BSZ-Ende:} & %9 \\\\\n"
        "\\end{tabular}\n"
        "\\end{minipage}\n"
        "\\vfill\n\n";

    //Boat log header
    QString texString9 = rawTexString9.arg(tLocalGroup, tDistrictAssociation, QString::number(pReport.getNumber()), tBoatType,
                                           tStationLocation, tBoatRadioCallName, pReport.getDate().toString("dd.MM.yyyy"),
                                           QString::asprintf("%04u,%1u", tEngineInitialFullHours, tEngineInitialDecimalPlace),
                                           QString::asprintf("%04u,%1u", tEngineFinalFullHours, tEngineFinalDecimalPlace)
                                           ).arg(tBoatNameStr);

    //Define lambda to compare/sort persons by name, then identifier
    auto cmpNameIdent = [](const Person& pA, const Person& pB) -> bool
    {
        if (QString::localeAwareCompare(pA.getLastName(), pB.getLastName()) < 0)
            return true;
        else if (QString::localeAwareCompare(pA.getLastName(), pB.getLastName()) > 0)
            return false;
        else
        {
            if (QString::localeAwareCompare(pA.getFirstName(), pB.getFirstName()) < 0)
                return true;
            else if (QString::localeAwareCompare(pA.getFirstName(), pB.getFirstName()) > 0)
                return false;
            else
            {
                if (QString::localeAwareCompare(pA.getIdent(), pB.getIdent()) < 0)
                    return true;
                else
                    return false;
            }
        }
    };

    QString texString10 = "\\subsection*{Bootsfahrten}\n"
        "\\vspace{3pt}\n"
        "\\renewcommand{\\arraystretch}{0.6}\n"
        "\\begin{tabular}{>{\\raggedleft}p{0.02\\linewidth}>{\\raggedright}p{0.08\\linewidth}>{\\raggedright}p{0.14\\linewidth}\n"
        "                >{\\raggedright}p{0.13\\linewidth}>{\\raggedright}p{0.21\\linewidth}>{\\raggedright}p{0.19\\linewidth}\n"
        "                >{\\raggedleft\\arraybackslash}p{0.07\\linewidth}}\n"
        "\\textbf{Nr.} & \\textbf{Zeitraum} & \\textbf{Fahrtzweck} & \\textbf{Bootsführer} & \\textbf{Besatzung} &\n"
        "\\textbf{Bemerkungen} & \\textbf{Dauer}\\\\\n"
        "\\toprule\n";

    //Row template for boat drives table
    QString tDriveRowString = "\\textbf{%1} & \\makecell[rt]{%2\\\\--%3} & \\hspace{0pt}%4 & \\hspace{0pt}%5 & %6 & "
                              "\\hspace{0pt}%7 & %8 \\\\";

    //Boat drives list
    auto tDrives = boatLog.getDrives();
    int tDrivesSize = tDrives.size();

    int tDriveNumber = 1;       //Enumerate drives
    int tTotalBoatMinutes = 0;  //Print total boat drive hours at the end of the table
    int tTotalDrivesFuel = 0;   //Sum up amount of fuel added during/after individual drives

    bool splitDrivesTable = (tDrivesSize > pBoatDrivesTableMaxLength);  //Continue long table on next page

    //Add a table row for each drive
    for (const BoatDrive& tDrive : tDrives)
    {
        //Name of boatman

        Person tBoatman = Person::dummyPerson();
        if (tDrive.getBoatman() != "")
            tBoatman = pReport.getPerson(tDrive.getBoatman());

        QString tBoatmanStr = tBoatman.getLastName() + ", " + tBoatman.getFirstName();
        Aux::latexUseHyphdash(tBoatmanStr);

        if (tDrive.getBoatman() == "")
            tBoatmanStr = "---";

        //Crew members

        std::vector<Person> tCrew;
        for (const auto& it : tDrive.crew())
            tCrew.push_back(pReport.getPerson(it.first));

        int tCrewMemberNumber = 1;

        //Use temporary set to sort persons using above custom sort lambda

        std::set<std::reference_wrapper<const Person>, decltype(cmpNameIdent)> tCrewSorted(cmpNameIdent);

        for (const Person& tPerson : tCrew)
            tCrewSorted.insert(std::cref(tPerson));

        QString tCrewStr = "\\hspace{0pt}";

        for (const Person& tPerson : tCrewSorted)
        {
            QString tPersonStr = tPerson.getLastName() + ", " + tPerson.getFirstName();
            Aux::latexUseHyphdash(tPersonStr);

            tCrewStr.append(tPersonStr);

            if (tCrewMemberNumber++ < static_cast<int>(tCrewSorted.size()))
                tCrewStr.append("\\newline{}\\hspace{0pt}");
        }

        if (tDrive.crewSize() == 0)
            tCrewStr = "---";

        //Drive purpose

        QString tDrivePurpose = tDrive.getPurpose();
        Aux::latexEscapeSpecialChars(tDrivePurpose);
        Aux::latexFixLineBreaksNoLineBreaks(tDrivePurpose);
        Aux::latexUseHyphdash(tDrivePurpose);

        if (tDrivePurpose == "")
            tDrivePurpose = "---";

        //Drive comment
        QString tDriveComments = tDrive.getComments();
        Aux::latexEscapeSpecialChars(tDriveComments);
        Aux::latexFixLineBreaks(tDriveComments);
        Aux::latexUseHyphdash(tDriveComments);

        //Drive's timeframe
        QTime tBeginTime = tDrive.getBeginTime();
        QTime tEndTime = tDrive.getEndTime();

        int dMinutes = tBeginTime.secsTo(tEndTime) / 60;

        if (dMinutes < 0)
            dMinutes += 24 * 60;

        tTotalBoatMinutes += dMinutes;

        QTime tDuration = QTime(0, 0, 0).addSecs(dMinutes * 60);

        //Add fuel
        tTotalDrivesFuel += tDrive.getFuel();

        if (splitDrivesTable && tDriveNumber == pBoatDrivesTableMaxLength)
        {
            //Hint at continuation of the table on next page, if table is too long and split here
            texString10.append(" \\midrule\n");
            texString10.append("\\textbf{\\dots} & \\multicolumn{5}{c}{\\textit{Fortsetzung auf nächster Seite}} & "
                               "\\dots\\vspace{0pt} \\\\");
        }
        else if (splitDrivesTable && tDriveNumber > pBoatDrivesTableMaxLength)
        {
            //Skip remaining rows here, if table is too long and continued on next page
            ;   //(sic!)
        }
        else
        {
            if (tDriveNumber > 1)
                texString10.append(" \\midrule\n");

            texString10.append(tDriveRowString.arg(QString::number(tDriveNumber),
                                                   tBeginTime.toString("hh:mm"), tEndTime.toString("hh:mm"),
                                                   tDrivePurpose, tBoatmanStr, tCrewStr,
                                                   tDriveComments, tDuration.toString("hh:mm")));
        }

        ++tDriveNumber;
    }

    //Calculate boat hours split in hours and minutes

    int tTotalBoatModMinutes = tTotalBoatMinutes % 60;
    int tTotalBoatHours = (tTotalBoatMinutes - tTotalBoatModMinutes) / 60;

    int tCarryBoatModMinutes = boatLog.getBoatMinutesCarry() % 60;
    int tCarryBoatHours = (boatLog.getBoatMinutesCarry() - tCarryBoatModMinutes) / 60;

    int tTotalBoatModMinutesWithCarry = (tTotalBoatMinutes + boatLog.getBoatMinutesCarry()) % 60;
    int tTotalBoatHoursWithCarry = (tTotalBoatMinutes + boatLog.getBoatMinutesCarry() - tTotalBoatModMinutesWithCarry) / 60;

    texString10.append(QString("\n"
        "\\bottomrule\\addlinespace[\\belowrulesep]\n"
        "\\multicolumn{6}{r}{Einsatzstunden} & %1\\vspace{1pt} \\\\\n"
        "\\multicolumn{6}{r}{+ Übertrag} & %2\\vspace{3pt} \\\\\n"
        "\\multicolumn{6}{r}{= Gesamt} & \\textbf{%3} \\\\\n"
        "\\end{tabular}\n"
        "\\vspace{0pt}\n"
        "\\vfill\n\n").arg(QString::asprintf("%02u:%02u", tTotalBoatHours, tTotalBoatModMinutes),
                           QString::asprintf("%02u:%02u", tCarryBoatHours, tCarryBoatModMinutes),
                           QString::asprintf("%02u:%02u", tTotalBoatHoursWithCarry, tTotalBoatModMinutesWithCarry)));

    //Sum up fuel added at begin/end of duty and during/after individual drives
    int tFuelTotal = tTotalDrivesFuel + boatLog.getFuelInitial() + boatLog.getFuelFinal();
    QString tFuelTotalStr = QString::number(tFuelTotal);

    QString rawTexString11 = "\\begin{minipage}{\\linewidth}\n"
        "\\subsection*{Sonstiges}\n"
        "\\renewcommand{\\arraystretch}{1.2}\n"
        "\\begin{tabular}{ll}\n"
        "\\multicolumn{2}{l}{\\textbf{Boot einsatzbereit im Wasser:}} \\\\\n"
        "Von & %1\\,Uhr \\\\\n"
        "Bis & %2\\,Uhr\n"
        "\\end{tabular}\n"
        "\\hfill\n"
        "\\begin{tabular}{l}\n"
        "\\textbf{Boot geslipt:}\\\\\n"
        "$%3$ Zu Dienstanfang\\\\\n"
        "$%4$ Zu Dienstende\n"
        "\\end{tabular}\n"
        "\\hfill\n"
        "\\begin{tabular}{>{\\raggedright}p{0.10\\linewidth}>{\\raggedleft\\arraybackslash}p{0.075\\linewidth}}\n"
        "\\multicolumn{2}{l}{\\textbf{Getankt:}} \\\\\n"
        "\\hspace{0pt}%5: & %6\\,Liter \\\\ & \n"
        "\\end{tabular}\n"
        "\\end{minipage}\n"
        "\\vspace{8pt}\n"
        "\\vfill\n\n";

    QString texString11 = rawTexString11.arg(boatLog.getReadyFrom().toString("hh:mm"),
                                             boatLog.getReadyUntil().toString("hh:mm"),
                                             boatLog.getSlippedInitial() ? "\\boxtimes" : "\\Box",
                                             boatLog.getSlippedFinal() ? "\\boxtimes" : "\\Box",
                                             tBoatFuelType, tFuelTotalStr);

    QString rawTexString12 = "\\begin{minipage}{\\linewidth}\n"
        "\\subsection*{Bemerkungen}\n"
        "\\uline{\\mbox{}%1\\mbox{}\\hfill}\n"
        "\\end{minipage}\n"
        "\\\\\\\\\\vspace{2pt}\\vfill\n\n";

    QString tBoatComments = boatLog.getComments();
    Aux::latexEscapeSpecialChars(tBoatComments);
    Aux::latexFixLineBreaksUline(tBoatComments);

    QString texString12 = rawTexString12.arg(tBoatComments);

    QString texString13 = "\\hfill\\vfill\\parbox[c][0pt][r]{150pt}{\\hrule \\vspace{3pt} Unterschrift Bootsführer "
                          "\\vspace{-2pt}}\\hfill\n \\parbox[c][0pt][r]{150pt}{\\hrule \\vspace{3pt} Unterschrift Stationsleiter "
                          "\\vspace{-2pt}}\n";

    QString texString14 = "";

    //Continue split boat drives table here
    if (splitDrivesTable)
    {
        texString14 = "\n\\clearpage\n\n"
                      "\\subsection*{Fortsetzung: Bootsfahrten}\n"
                      "\\vspace{3pt}\n"
                      "\\renewcommand{\\arraystretch}{0.6}\n"
        "\\begin{longtable}{>{\\raggedleft}p{0.02\\linewidth}>{\\raggedright}p{0.08\\linewidth}>{\\raggedright}p{0.14\\linewidth}\n"
        "                   >{\\raggedright}p{0.13\\linewidth}>{\\raggedright}p{0.21\\linewidth}>{\\raggedright}p{0.19\\linewidth}\n"
        "                   >{\\raggedleft\\arraybackslash}p{0.07\\linewidth}}\n"
        "\\textbf{Nr.} & \\textbf{Zeitraum} & \\textbf{Fahrtzweck} & \\textbf{Bootsführer} & \\textbf{Besatzung} &\n"
        "\\textbf{Bemerkungen} & \\textbf{Dauer}\\\\\n"
        "\\toprule\n"
        "\\textbf{\\dots} & \\multicolumn{5}{c}{\\textit{Fortsetzung von letzter Seite}} & \\dots\\vspace{0pt} \\\\\n"
        "\\midrule\n"
        "\\endhead\n"
        "\\textbf{\\dots} & \\multicolumn{5}{c}{\\textit{Fortsetzung auf nächster Seite}}\\strut &\n"
        "\\dots\\vspace{0pt} \\\\\n"
        "\\bottomrule\n"
        "\\endfoot\n"
        "\\bottomrule\n"
        "\\endlastfoot\n";

        //Reuse enumerator variable from above
        tDriveNumber = 0;

        //Add a table row for each drive
        for (const BoatDrive& tDrive : tDrives)
        {
            //Skip entries that are already in first part of the boat drives table
            if (++tDriveNumber < pBoatDrivesTableMaxLength)
                continue;

            //Name of boatman

            Person tBoatman = Person::dummyPerson();
            if (tDrive.getBoatman() != "")
                tBoatman = pReport.getPerson(tDrive.getBoatman());

            QString tBoatmanStr = tBoatman.getLastName() + ", " + tBoatman.getFirstName();
            Aux::latexUseHyphdash(tBoatmanStr);

            if (tDrive.getBoatman() == "")
                tBoatmanStr = "---";

            //Crew members

            std::vector<Person> tCrew;
            for (const auto& it : tDrive.crew())
                tCrew.push_back(pReport.getPerson(it.first));

            int tCrewMemberNumber = 1;

            //Use temporary set to sort persons using above custom sort lambda

            std::set<std::reference_wrapper<const Person>, decltype(cmpNameIdent)> tCrewSorted(cmpNameIdent);

            for (const Person& tPerson : tCrew)
                tCrewSorted.insert(std::cref(tPerson));

            QString tCrewStr = "\\hspace{0pt}";

            for (const Person& tPerson : tCrewSorted)
            {
                QString tPersonStr = tPerson.getLastName() + ", " + tPerson.getFirstName();
                Aux::latexUseHyphdash(tPersonStr);

                tCrewStr.append(tPersonStr);

                if (tCrewMemberNumber++ < static_cast<int>(tCrewSorted.size()))
                    tCrewStr.append("\\newline{}\\hspace{0pt}");
            }

            if (tDrive.crewSize() == 0)
                tCrewStr = "---";

            //Drive purpose

            QString tDrivePurpose = tDrive.getPurpose();
            Aux::latexEscapeSpecialChars(tDrivePurpose);
            Aux::latexFixLineBreaksNoLineBreaks(tDrivePurpose);
            Aux::latexUseHyphdash(tDrivePurpose);

            if (tDrivePurpose == "")
                tDrivePurpose = "---";

            //Drive comment
            QString tDriveComments = tDrive.getComments();
            Aux::latexEscapeSpecialChars(tDriveComments);
            Aux::latexFixLineBreaks(tDriveComments);
            Aux::latexUseHyphdash(tDriveComments);

            //Drive's timeframe
            QTime tBeginTime = tDrive.getBeginTime();
            QTime tEndTime = tDrive.getEndTime();

            int dMinutes = tBeginTime.secsTo(tEndTime) / 60;

            if (dMinutes < 0)
                dMinutes += 24 * 60;

            QTime tDuration = QTime(0, 0, 0).addSecs(dMinutes * 60);

            //Skip midrule for first line after header
            if (tDriveNumber > pBoatDrivesTableMaxLength)
                texString14.append(" \\midrule\n");

            texString14.append(tDriveRowString.arg(QString::number(tDriveNumber),
                                                   tBeginTime.toString("hh:mm"), tEndTime.toString("hh:mm"),
                                                   tDrivePurpose, tBoatmanStr, tCrewStr,
                                                   tDriveComments, tDuration.toString("hh:mm")));
        }

        texString14.append(QString("\n"
                                   "\\end{longtable}\n"
                                   "\\vspace{0pt}\n"
                                   "\\vfill\n"));
    }

    QString texString15 = "\\end{document}\n";

    //Assemble all parts to one document
    QString texString = texString0 + texString1 + texString2 + texString3 + texString4 + texString5 + texString6 +
                        texString7 + texString8 + pagebreakString + texString9 + texString10 + texString11 +
                        texString12 + texString13 + texString14 + texString15;

    //Swap document to the function argument
    pTeXString.swap(texString);
}
