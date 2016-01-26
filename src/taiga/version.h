/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#ifndef TAIGA_TAIGA_VERSION_H
#define TAIGA_TAIGA_VERSION_H

#define XSTRINGIFY(s) STRINGIFY(s)
#define STRINGIFY(s) #s

#define TAIGA_VERSION_MAJOR 1
#define TAIGA_VERSION_MINOR 2
#define TAIGA_VERSION_PATCH 3
#define TAIGA_VERSION_PRE   L""
#define TAIGA_VERSION_BUILD 0

#define VERSION_VALUE_DIGITAL TAIGA_VERSION_MAJOR, TAIGA_VERSION_MINOR, TAIGA_VERSION_PATCH, TAIGA_VERSION_BUILD
#define VERSION_VALUE_STRING XSTRINGIFY(TAIGA_VERSION_MAJOR) "." XSTRINGIFY(TAIGA_VERSION_MINOR) "." XSTRINGIFY(TAIGA_VERSION_PATCH) "\0"

#endif  // TAIGA_TAIGA_VERSION_H
