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

#include <limits>
#include <set>

#include <windows/win/error.h>

#include "base/file_search.h"

#include "base/file.h"
#include "base/log.h"
#include "base/string.h"

namespace base {

struct FileSearchHandleDeleter {
  using pointer = HANDLE;
  void operator()(pointer p) const { ::FindClose(p); }
};

using FileSearchHandle = std::unique_ptr<HANDLE, FileSearchHandleDeleter>;

constexpr uint64_t GetFileSize(const WIN32_FIND_DATA& data) {
  constexpr auto m = std::numeric_limits<decltype(data.nFileSizeLow)>::max();
  return (data.nFileSizeHigh * (m + 1ull)) + data.nFileSizeLow;
}

////////////////////////////////////////////////////////////////////////////////
// @TODO: Use std::filesystem?

bool FileSearch::Search(const std::wstring& root,
                        callback_function_t on_directory,
                        callback_function_t on_file) const {
  if (root.empty())
    return false;
  if (options.skip_directories && options.skip_files)
    return false;

  const auto path = AddTrailingSlash(GetExtendedLengthPath(root)) + L"*";
  std::set<std::wstring> subdirectories;

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
      if (!options.skip_directories)
        if (on_directory && on_directory({root, data.cFileName, data}))
          return true;
      if (!options.skip_subdirectories)
        subdirectories.insert(AddTrailingSlash(root) + data.cFileName);

    // File
    } else {
      if (options.skip_files)
        continue;
      if (GetFileSize(data) < options.min_file_size)
        continue;
      if (on_file && on_file({root, data.cFileName, data}))
        return true;
    }

  } while (::FindNextFile(handle.get(), &data));

  for (const auto& subdirectory : subdirectories) {
    if (Search(subdirectory, on_directory, on_file))
      return true;
  }

  return false;
}

}  // namespace base
