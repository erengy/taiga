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

#ifndef COMMON_H
#define COMMON_H

#include "std.h"

typedef unsigned __int64 QWORD, *LPQWORD;

namespace anime {
class Episode;
class Item;
}

// =============================================================================

// action.cpp
void ExecuteAction(wstring action, WPARAM wParam = 0, LPARAM lParam = 0);

// common.cpp
wstring CalculateCRC(const wstring& file);
int GetEpisodeHigh(const wstring& episode_number);
int GetEpisodeLow(const wstring& episode_number);
void SplitEpisodeNumbers(const wstring& input, vector<int>& output);
wstring JoinEpisodeNumbers(const vector<int>& input);
int TranslateResolution(const wstring& str, bool return_validity = false);
int StatusToIcon(int status);
wstring FormatError(DWORD dwError, LPCWSTR lpSource = NULL);
void SetSharedCursor(LPCWSTR name);
unsigned long GetFileAge(const wstring& path);
QWORD GetFileSize(const wstring& path);
QWORD GetFolderSize(const wstring& path, bool recursive);
bool Execute(const wstring& path, const wstring& parameters = L"");
BOOL ExecuteEx(const wstring& path, const wstring& parameters = L"");
void ExecuteLink(const wstring& link);
wstring ExpandEnvironmentStrings(const wstring& path);
wstring BrowseForFile(HWND hwndOwner, LPCWSTR lpstrTitle, LPCWSTR lpstrFilter = NULL);
BOOL BrowseForFolder(HWND hwnd, const wstring& title, const wstring& default_path, wstring& output);
bool CreateFolder(const wstring& path);
int DeleteFolder(wstring path);
bool FileExists(const wstring& file);
bool FolderExists(const wstring& folder);
bool PathExists(const wstring& path);
void ValidateFileName(wstring& path);
wstring GetDefaultAppPath(const wstring& extension, const wstring& default_value);
int PopulateFiles(vector<wstring>& file_list, wstring path, wstring extension = L"", bool recursive = false, bool trim_extension = false);
int PopulateFolders(vector<wstring>& folder_list, wstring path);
bool SaveToFile(LPCVOID data, DWORD length, const wstring& path, bool take_backup = false);
wstring ToSizeString(QWORD qwSize);

// script.cpp
wstring EvaluateFunction(const wstring& func_name, const wstring& func_body);
bool IsScriptFunction(const wstring& str);
bool IsScriptVariable(const wstring& str);
wstring ReplaceVariables(wstring str, const anime::Episode& episode, 
  bool url_encode = false, bool is_manual = false, bool is_preview = false);
wstring EscapeScriptEntities(const wstring& str);
wstring UnescapeScriptEntities(const wstring& str);

// search.cpp
void ScanAvailableEpisodes(int anime_id, bool check_folder, bool silent);
wstring SearchFileFolder(anime::Item& anime_item, const wstring& root, int episode_number, bool search_folder);

#endif // COMMON_H