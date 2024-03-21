/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

// @NOTE: Make sure to update the following files after changing the names of
// macros defined here:
//
// - /setup/Taiga.nsi
// - /src/taiga/resource.rc

// Relative path to avoid RC1015 error with version.rc
#include "../base/preprocessor.h"

#define TAIGA_APP_NAME  L"Taiga"
#define TAIGA_APP_MUTEX L"Taiga-33d5a63c-de90-432f-9a8b-f6f733dab258"

#define TAIGA_VERSION_MAJOR 1
#define TAIGA_VERSION_MINOR 4
#define TAIGA_VERSION_PATCH 1
#define TAIGA_VERSION_PRE   ""
#define TAIGA_VERSION_BUILD 0

// Used in version.rc
#define TAIGA_VERSION_DIGITAL \
    TAIGA_VERSION_MAJOR, \
    TAIGA_VERSION_MINOR, \
    TAIGA_VERSION_PATCH, \
    TAIGA_VERSION_BUILD
#define TAIGA_VERSION_STRING \
    STRINGIZE(TAIGA_VERSION_MAJOR) "." \
    STRINGIZE(TAIGA_VERSION_MINOR) "." \
    STRINGIZE(TAIGA_VERSION_PATCH) "\0"

// Store data at the same directory as the executable
#define TAIGA_PORTABLE
