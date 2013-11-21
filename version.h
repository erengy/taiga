/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

#ifndef VERSION_H
#define VERSION_H

#define XSTRINGIFY(s) STRINGIFY(s)
#define STRINGIFY(s) #s

#define VERSION_MAJOR    1
#define VERSION_MINOR    0
#define VERSION_REVISION 256
#define VERSION_BUILD    0

#define VERSION_VALUE_DIGITAL VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_BUILD
#define VERSION_VALUE_STRING XSTRINGIFY(VERSION_MAJOR) "." XSTRINGIFY(VERSION_MINOR) "." XSTRINGIFY(VERSION_REVISION) "\0"

#endif // VERSION_H
