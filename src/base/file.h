/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <string>
#include <vector>

#include <windows.h>
#include <shlobj.h>

unsigned long GetFileAge(const std::wstring& path);
std::wstring GetFileLastModifiedDate(const std::wstring& path);
uint64_t GetFileSize(const std::wstring& path);
uint64_t GetFolderSize(const std::wstring& path, bool recursive);

bool Execute(const std::wstring& path, const std::wstring& parameters = L"",
             int show_command = SW_SHOWNORMAL);
bool ExecuteLink(const std::wstring& link);

bool OpenFolderAndSelectFile(const std::wstring& path);
bool CreateFolder(const std::wstring& path);
int DeleteFolder(std::wstring path);

std::wstring GetExtendedLengthPath(const std::wstring& path);
std::wstring GetNormalizedPath(std::wstring path);
bool IsDirectory(const WIN32_FIND_DATA& find_data);
bool IsHiddenFile(const WIN32_FIND_DATA& find_data);
bool IsSystemFile(const WIN32_FIND_DATA& find_data);
bool IsValidDirectory(const WIN32_FIND_DATA& find_data);

bool FileExists(const std::wstring& file);
bool FolderExists(const std::wstring& folder);
bool PathExists(const std::wstring& path);
void ValidateFileName(std::wstring& file);

std::wstring ExpandEnvironmentStrings(const std::wstring& path);
std::wstring GetDefaultAppPath(const std::wstring& extension,
                               const std::wstring& default_value = L"");
std::wstring GetKnownFolderPath(REFKNOWNFOLDERID rfid);
std::wstring GetFinalPath(const std::wstring& path);

unsigned int PopulateFiles(std::vector<std::wstring>& file_list,
                           const std::wstring& path,
                           const std::wstring& extension = L"",
                           bool recursive = false, bool trim_extension = false);

bool ReadFromFile(const std::wstring& path, std::string& output);
bool SaveToFile(LPCVOID data, DWORD length, const std::wstring& path,
                bool take_backup = false);
bool SaveToFile(const std::string& data, const std::wstring& path,
                bool take_backup = false);

UINT64 ParseSizeString(std::wstring value);
std::wstring ToSizeString(const UINT64 size);
