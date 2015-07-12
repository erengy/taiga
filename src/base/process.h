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

#ifndef TAIGA_BASE_PROCESS_H
#define TAIGA_BASE_PROCESS_H

#include <string>
#include <vector>
#include <windows.h>

BOOL GetProcessFiles(ULONG process_id, std::vector<std::wstring>& files_vector);

bool CheckInstance(LPCWSTR mutex_name, LPCWSTR class_name);

void ActivateWindow(HWND hwnd);
std::wstring GetWindowClass(HWND hwnd);
std::wstring GetWindowPath(HWND hwnd);
std::wstring GetWindowTitle(HWND hwnd);
bool IsFullscreen(HWND hwnd);

PVOID GetLibraryProcAddress(PSTR dll_module, PSTR proc_name);

#endif  // TAIGA_BASE_PROCESS_H