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

#ifndef VERSION_H
#define VERSION_H

namespace Version
{
inline constexpr char const* ProgramVersionMajor = "1";
inline constexpr char const* ProgramVersionMinor = "4";
inline constexpr char const* ProgramVersionPatch = "0";

inline constexpr char const* FileFormatVersion = "1.4.0";

inline constexpr int ConfigDatabaseUserVersion = 1;
inline constexpr int PersonnelDatabaseUserVersion = 2;
}

#endif // VERSION_H
