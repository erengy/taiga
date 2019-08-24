/*
** Taiga
** Copyright (C) 2010-2019, Eren Okka
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

#include "file_search.h"

#include <windows/win/error.h>

#include "file.h"
#include "log.h"
#include "string.h"

namespace base {

struct FileSearchHandleDeleter {
  using pointer = HANDLE;
  void operator()(pointer p) const { ::FindClose(p); }
};

using FileSearchHandle = std::unique_ptr<HANDLE, FileSearchHandleDeleter>;

////////////////////////////////////////////////////////////////////////////////

// @TODO: Search breadth-first instead of depth-first
// @TODO: Use std::filesystem?
bool FileSearch::Search(const std::wstring& root,
                        callback_function_t on_directory,
                        callback_function_t on_file) const {
  if (root.empty())
    return false;
  if (options.skip_directories && options.skip_files)
    return false;

  bool result = false;
  const auto path = AddTrailingSlash(GetExtendedLengthPath(root)) + L"*";

  WIN32_FIND_DATA data;
  FileSearchHandle handle(::FindFirstFile(path.c_str(), &data));

  do {
    if (handle.get() == INVALID_HANDLE_VALUE) {
      if (options.log_errors) {
        auto error_message = win::FormatError(::GetLastError());
        TrimRight(error_message, L"\r\n");
        LOGE(L"{}\nPath: {}", error_message, path);
      }
      ::SetLastError(ERROR_SUCCESS);
      continue;
    }

    if (IsSystemFile(data) || IsHiddenFile(data))
      continue;

    // Directory
    if (IsDirectory(data)) {
      if (!IsValidDirectory(data))
        continue;
      if (!options.skip_directories && on_directory)
        result = on_directory({root, data.cFileName, data});
      if (!options.skip_subdirectories && !result)
        result = Search(AddTrailingSlash(root) + data.cFileName,
                        on_directory, on_file);
    // File
    } else {
      if (options.skip_files)
        continue;
      if (data.nFileSizeLow < options.min_file_size)
        continue;
      if (on_file)
        result = on_file({root, data.cFileName, data});
    }

  } while (!result && ::FindNextFile(handle.get(), &data));

  return result;
}

}  // namespace base
