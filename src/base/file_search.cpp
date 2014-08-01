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
  if (root.empty())
    return false;
  if (skip_directories_ && skip_files_)
    return false;

  std::wstring path = AddTrailingSlash(GetExtendedLengthPath(root)) + L"*";
  bool result = false;

  WIN32_FIND_DATA find_data;
  HANDLE handle = FindFirstFile(path.c_str(), &find_data);

  do {
    if (handle == INVALID_HANDLE_VALUE) {
      LOG(LevelError, Logger::FormatError(GetLastError()) + L"\nPath: " + path);
      SetLastError(ERROR_SUCCESS);
      continue;
    }

    if (IsSystemFile(find_data) || IsHiddenFile(find_data))
      continue;

    // Directory
    if (IsDirectory(find_data)) {
      if (skip_directories_)
        continue;
      if (!IsValidDirectory(find_data))
        continue;
      result = OnDirectory(root, find_data.cFileName);
      if (!result)
        result = Search(AddTrailingSlash(root) + find_data.cFileName);

    // File
    } else {
      if (skip_files_)
        continue;
      if (find_data.nFileSizeLow < minimum_file_size_)
        continue;
      result = OnFile(root, find_data.cFileName);
    }

  } while (!result && FindNextFile(handle, &find_data));

  FindClose(handle);
  return result;
}

bool FileSearchHelper::OnDirectory(const std::wstring& root,
                                   const std::wstring& name) {
  return false;
}

bool FileSearchHelper::OnFile(const std::wstring& root,
                              const std::wstring& name) {
  return false;
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