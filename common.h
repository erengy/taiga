/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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

#ifndef COMMON_H
#define COMMON_H

#include "std.h"

// Timer IDs
enum TimerID {
  TIMER_MAIN = 1337,
  TIMER_TAIGA = 74164
};

// ListView sort types
enum ListSortType {
  LISTSORTTYPE_DEFAULT,
  LISTSORTTYPE_NUMBER,
  LISTSORTTYPE_SEASON,
  LISTSORTTYPE_FILESIZE
};

typedef unsigned __int64 QWORD, *LPQWORD;

class CEpisode;

// =============================================================================

// action.cpp
void ExecuteAction(wstring action, WPARAM wParam = 0, LPARAM lParam = 0);

// common.cpp
wstring Base64Decode(const wstring& str);
wstring Base64Encode(const wstring& str);
wstring CalculateCRC(const wstring& file);
int GetEpisodeHigh(const wstring& episode_number);
int GetEpisodeLow(const wstring& episode_number);
int StatusToIcon(int status);
wstring FormatError(DWORD dwError, LPCWSTR lpSource = NULL);
void GetSystemTime(SYSTEMTIME& st, int utc_offset = 0);
wstring GetDate(LPCWSTR lpFormat = L"yyyy'-'MM'-'dd");
wstring GetTime(LPCWSTR lpFormat = L"HH':'mm':'ss");
wstring GetDateJapan(LPCWSTR lpFormat = L"yyyy'-'MM'-'dd");
wstring GetTimeJapan(LPCWSTR lpFormat = L"HH':'mm':'ss");
wstring ToTimeString(int seconds);
bool Execute(const wstring& path, const wstring& parameters = L"");
BOOL ExecuteEx(const wstring& path, const wstring& parameters = L"");
void ExecuteLink(const wstring& link);
wstring ExpandEnvironmentStrings(const wstring& path);
wstring BrowseForFile(HWND hwndOwner, LPCWSTR lpstrTitle, LPCWSTR lpstrFilter = NULL);
BOOL BrowseForFolder(HWND hwndOwner, LPCWSTR lpszTitle, UINT ulFlags, wstring& output);
bool CreateFolder(const wstring& path);
int DeleteFolder(wstring path);
bool FileExists(const wstring& file);
bool FolderExists(const wstring& folder);
bool PathExists(const wstring& path);
void ValidateFileName(wstring& path);
wstring GetDefaultAppPath(const wstring& extension, const wstring& default_value);
int PopulateFiles(vector<wstring>& file_list, wstring path, wstring extension = L"*.*");
int PopulateFolders(vector<wstring>& folder_list, wstring path);
wstring ToSizeString(QWORD qwSize);

// debug.cpp
void DebugTest();

// encryption.cpp
wstring SimpleEncrypt(wstring str);
wstring SimpleDecrypt(wstring str);

// gzip.cpp
bool UncompressGzippedFile(const string& file, string& output);
bool UncompressGzippedString(const string& input, string& output);

// list_sort.cpp
int CALLBACK ListViewCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

// menu.cpp
void UpdateAllMenus(int anime_index);
void UpdateAccountMenu();
void UpdateAnimeMenu(int anime_index);
void UpdateAnnounceMenu(int anime_index);
void UpdateFilterMenu();
void UpdateFoldersMenu();
void UpdateSearchMenu();
void UpdateSearchListMenu(bool enabled = false);
void UpdateTrayMenu();

// script.cpp
wstring EvaluateFunction(const wstring& func_name, const wstring& func_body);
bool IsScriptFunction(const wstring& str);
bool IsScriptVariable(const wstring& str);
wstring ReplaceVariables(wstring str, const CEpisode& episode, bool url_encode = false);

// search.cpp
wstring SearchFileFolder(int anime_index, wstring root, int episode_number, bool search_folder);

#endif // COMMON_H