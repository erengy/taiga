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

#include <cstdint>
#include <functional>
#include <string>

#include <windows.h>

namespace base {

struct FileSearchOptions {
  bool log_errors = true;
  bool skip_directories = false;
  bool skip_files = false;
  bool skip_subdirectories = false;
  uint64_t min_file_size = 0;
};

struct FileSearchResult {
  std::wstring root;
  std::wstring name;
  WIN32_FIND_DATA data;
};

class FileSearch {
public:
  using callback_function_t = std::function<bool(const FileSearchResult&)>;

  bool Search(const std::wstring& root,
              callback_function_t on_directory,
              callback_function_t on_file) const;

  FileSearchOptions options;
};

}  // namespace base
