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

#ifndef TAIGA_BASE_FILE_H
#define TAIGA_BASE_FILE_H

#include <functional>
#include <string>
#include <vector>
#include <windows.h>

#include "types.h"

unsigned long GetFileAge(const std::wstring& path);
std::wstring GetFileLastModifiedDate(const std::wstring& path);
QWORD GetFileSize(const std::wstring& path);
QWORD GetFolderSize(const std::wstring& path, bool recursive);

bool Execute(const std::wstring& path, const std::wstring& parameters = L"", int show_command = SW_SHOWNORMAL);
bool ExecuteFile(const std::wstring& path, std::wstring parameters = L"");
void ExecuteLink(const std::wstring& link);

bool CreateFolder(const std::wstring& path);
int DeleteFolder(std::wstring path);

void ConvertToLongPath(std::wstring& path);
std::wstring GetExtendedLengthPath(const std::wstring& path);
bool IsDirectory(const WIN32_FIND_DATA& find_data);
bool IsHiddenFile(const WIN32_FIND_DATA& find_data);
bool IsSystemFile(const WIN32_FIND_DATA& find_data);
bool IsValidDirectory(const WIN32_FIND_DATA& find_data);
bool TranslateDeviceName(std::wstring& path);

bool FileExists(const std::wstring& file);
bool FolderExists(const std::wstring& folder);
bool PathExists(const std::wstring& path);
void ValidateFileName(std::wstring& path);

std::wstring ExpandEnvironmentStrings(const std::wstring& path);
std::wstring GetDefaultAppPath(const std::wstring& extension, const std::wstring& default_value);

unsigned int PopulateFiles(std::vector<std::wstring>& file_list, const std::wstring& path, const std::wstring& extension = L"", bool recursive = false, bool trim_extension = false);
int PopulateFolders(std::vector<std::wstring>& folder_list, const std::wstring& path);

bool ReadFromFile(const std::wstring& path, std::string& output);
bool SaveToFile(LPCVOID data, DWORD length, const std::wstring& path, bool take_backup = false);
bool SaveToFile(const std::string& data, const std::wstring& path, bool take_backup = false);

std::wstring ToSizeString(QWORD qwSize);

class FileSearchHelper {
public:
  typedef std::function<bool(const std::wstring& root, const std::wstring& name, const WIN32_FIND_DATA& data)> callback_function_t;

  FileSearchHelper();
  virtual ~FileSearchHelper() {}

  bool Search(const std::wstring& root);
  bool Search(const std::wstring& root, callback_function_t OnDirectoryFunc, callback_function_t OnFileFunc);

  virtual bool OnDirectory(const std::wstring& root, const std::wstring& name, const WIN32_FIND_DATA& data);
  virtual bool OnFile(const std::wstring& root, const std::wstring& name, const WIN32_FIND_DATA& data);

  void set_minimum_file_size(ULONGLONG minimum_file_size);
  void set_skip_directories(bool skip_directories);
  void set_skip_files(bool skip_files);
  void set_skip_subdirectories(bool skip_subdirectories);

protected:
  ULONGLONG minimum_file_size_;
  bool skip_directories_;
  bool skip_files_;
  bool skip_subdirectories_;
};

#endif  // TAIGA_BASE_FILE_H