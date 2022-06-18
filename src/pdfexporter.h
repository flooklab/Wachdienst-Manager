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

#ifndef PDFEXPORTER_H
#define PDFEXPORTER_H

#include "auxil.h"
#include "report.h"
#include "person.h"
#include "boatlog.h"
#include "boatdrive.h"
#include "databasecache.h"
#include "settingscache.h"

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <functional>

#include <QString>
#include <QTime>
#include <QTemporaryDir>
#include <QFileInfo>
#include <QFile>
#include <QIODevice>
#include <QProcess>

/*!
 * \brief Export a Report as a PDF file using LaTeX.
 *
 * A Report can be converted/saved to a PDF file by calling exportPDF().
 * In order to do this, the report information is arranged in a LaTeX document
 * and then compiled using XeLaTeX. For this the "app_export_xelatexPath" setting
 * must be set and contain a valid path to a XeLaTeX executable.
 */
class PDFExporter
{
public:
    PDFExporter() = delete;                                                 ///< Deleted constructor.
    //
    static bool exportPDF(const Report& pReport, const QString& pFileName,
                          int pPersonnelTableMaxLength = 15);               ///< Export report as PDF file.

private:
    static void reportToLaTeX(const Report& pReport, QString& pTeXString,
                              int pPersonnelTableMaxLength);                ///< Generate LaTeX document from report.
};

#endif // PDFEXPORTER_H
