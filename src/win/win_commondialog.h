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

#ifndef TAIGA_WIN_COMMONDIALOG_H
#define TAIGA_WIN_COMMONDIALOG_H

#include "win_main.h"

namespace win {

std::wstring BrowseForFile(HWND hwnd_owner,
                           const std::wstring& title,
                           std::wstring filter);

bool BrowseForFolder(HWND hwnd_owner,
                     const std::wstring& title,
                     const std::wstring& default_path,
                     std::wstring& output);

}  // namespace win

#endif  // TAIGA_WIN_COMMONDIALOG_H