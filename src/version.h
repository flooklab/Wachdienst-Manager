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

#ifndef VERSION_H
#define VERSION_H

#if defined(PROGRAM_VERSION)
    #error macro conflict
#elif defined(PROGRAM_VERSION_MAJOR)
    #error macro conflict
#elif defined(PROGRAM_VERSION_MINOR)
    #error macro conflict
#elif defined(PROGRAM_VERSION_PATCH)
    #error macro conflict
#elif defined(CONFIG_DATABASE_USER_VERSION)
    #error macro conflict
#elif defined(PERSONNEL_DATABASE_USER_VERSION)
    #error macro conflict
#endif

#define PROGRAM_VERSION_MAJOR "1"
#define PROGRAM_VERSION_MINOR "1"
#define PROGRAM_VERSION_PATCH "0"

#define CONFIG_DATABASE_USER_VERSION 1l
#define PERSONNEL_DATABASE_USER_VERSION 1l

#endif // VERSION_H
