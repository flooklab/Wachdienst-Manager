/*
/////////////////////////////////////////////////////////////////////////////////////////
//
//  This file is part of Wachdienst-Manager, a program to manage DLRG watch duty reports.
//  Copyright (C) 2021–2024 M. Frohne
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

#ifndef PDFEXPORTER_H
#define PDFEXPORTER_H

#include "report.h"

#include <QString>

/*!
 * \brief Export a Report as a PDF file using LaTeX.
 *
 * A Report can be converted/saved to a PDF file by calling exportPDF(). In order to do this,
 * the report information is arranged in a LaTeX document and then compiled using XeLaTeX.
 * For this the "app_export_xelatexPath" setting must be set and contain a valid path to
 * a XeLaTeX executable. Note that no boat log page will be generated if boat log
 * keeping has been disabled via the "app_boatLog_disabled" setting.
 */
class PDFExporter
{
public:
    PDFExporter() = delete;                                                                         ///< Deleted constructor.
    //
    static bool exportPDF(Report pReport, const QString& pFileName,
                          int pPersonnelTableMaxLength = 13, int pBoatDrivesTableMaxLength = 9);    ///< Export report as PDF file.

private:
    static void reportToLaTeX(const Report& pReport, QString& pTeXString,
                              int pPersonnelTableMaxLength, int pBoatDrivesTableMaxLength);     ///< Generate LaTeX document from report.
};

#endif // PDFEXPORTER_H
