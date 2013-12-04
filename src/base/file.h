/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
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

#ifndef TAIGA_BASE_FILE_H
#define TAIGA_BASE_FILE_H

#include "std.h"
#include "types.h"

unsigned long GetFileAge(const wstring& path);
QWORD GetFileSize(const wstring& path);
QWORD GetFolderSize(const wstring& path, bool recursive);

bool Execute(const wstring& path, const wstring& parameters = L"");
BOOL ExecuteEx(const wstring& path, const wstring& parameters = L"");
void ExecuteLink(const wstring& link);

wstring BrowseForFile(HWND hwndOwner, LPCWSTR lpstrTitle, LPCWSTR lpstrFilter = NULL);
BOOL BrowseForFolder(HWND hwnd, const wstring& title, const wstring& default_path, wstring& output);

bool CreateFolder(const wstring& path);
int DeleteFolder(wstring path);

bool FileExists(const wstring& file);
bool FolderExists(const wstring& folder);
bool PathExists(const wstring& path);
void ValidateFileName(wstring& path);

wstring ExpandEnvironmentStrings(const wstring& path);
wstring GetDefaultAppPath(const wstring& extension, const wstring& default_value);

int PopulateFiles(vector<wstring>& file_list, wstring path, wstring extension = L"", bool recursive = false, bool trim_extension = false);
int PopulateFolders(vector<wstring>& folder_list, wstring path);
bool SaveToFile(LPCVOID data, DWORD length, const wstring& path, bool take_backup = false);

wstring ToSizeString(QWORD qwSize);

#endif  // TAIGA_BASE_FILE_H