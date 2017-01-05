/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#include <windows/win/error.h>

#include "file.h"
#include "log.h"
#include "string.h"

FileSearchHelper::FileSearchHelper()
    : minimum_file_size_(0),
      skip_directories_(false),
      skip_files_(false),
      skip_subdirectories_(false) {
}

bool FileSearchHelper::Search(const std::wstring& root) {
  using namespace std::placeholders;

  return Search(root,
      std::bind(&FileSearchHelper::OnDirectory, this, _1, _2, _3),
      std::bind(&FileSearchHelper::OnFile, this, _1, _2, _3));
}

bool FileSearchHelper::Search(const std::wstring& root,
                              callback_function_t OnDirectoryFunc,
                              callback_function_t OnFileFunc) {
  if (root.empty())
    return false;
  if (skip_directories_ && skip_files_)
    return false;

  std::wstring path = AddTrailingSlash(GetExtendedLengthPath(root)) + L"*";
  bool result = false;

  WIN32_FIND_DATA data;
  HANDLE handle = FindFirstFile(path.c_str(), &data);

  do {
    if (handle == INVALID_HANDLE_VALUE) {
      LOGE(win::FormatError(GetLastError()) + L"\nPath: " + path);
      SetLastError(ERROR_SUCCESS);
      continue;
    }

    if (IsSystemFile(data) || IsHiddenFile(data))
      continue;

    // Directory
    if (IsDirectory(data)) {
      if (!IsValidDirectory(data))
        continue;
      if (!skip_directories_ && OnDirectoryFunc)
        result = OnDirectoryFunc(root, data.cFileName, data);
      if (!skip_subdirectories_ && !result)
        result = Search(AddTrailingSlash(root) + data.cFileName,
                        OnDirectoryFunc, OnFileFunc);

    // File
    } else {
      if (skip_files_)
        continue;
      if (data.nFileSizeLow < minimum_file_size_)
        continue;
      if (OnFileFunc)
        result = OnFileFunc(root, std::wstring(data.cFileName), data);
    }

  } while (!result && FindNextFile(handle, &data));

  FindClose(handle);
  return result;
}

bool FileSearchHelper::OnDirectory(const std::wstring& root,
                                   const std::wstring& name,
                                   const WIN32_FIND_DATA& data) {
  return false;
}

bool FileSearchHelper::OnFile(const std::wstring& root,
                              const std::wstring& name,
                              const WIN32_FIND_DATA& data) {
  return false;
}

void FileSearchHelper::set_minimum_file_size(ULONGLONG minimum_file_size) {
  minimum_file_size_ = minimum_file_size;
}

void FileSearchHelper::set_skip_directories(bool skip_directories) {
  skip_directories_ = skip_directories;
}

void FileSearchHelper::set_skip_files(bool skip_files) {
  skip_files_ = skip_files;
}

void FileSearchHelper::set_skip_subdirectories(bool skip_subdirectories) {
  skip_subdirectories_ = skip_subdirectories;
}