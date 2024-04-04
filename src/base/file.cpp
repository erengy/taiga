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

#include <fstream>
#include <shlobj.h>

#include <anisthesia/win_util.hpp>
#include <windows/win/error.h>
#include <windows/win/registry.h>

#include "base/file.h"

#include "base/file_search.h"
#include "base/format.h"
#include "base/log.h"
#include "base/string.h"
#include "base/time.h"

using anisthesia::win::detail::Handle;

HANDLE OpenFileForGenericRead(const std::wstring& path) {
  return ::CreateFile(GetExtendedLengthPath(path).c_str(),
                      GENERIC_READ, FILE_SHARE_READ, nullptr,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
}

HANDLE OpenFileForGenericWrite(const std::wstring& path) {
  return ::CreateFile(GetExtendedLengthPath(path).c_str(),
                      GENERIC_WRITE, 0, nullptr,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
}

////////////////////////////////////////////////////////////////////////////////

unsigned long GetFileAge(const std::wstring& path) {
  Handle file_handle{OpenFileForGenericRead(path)};

  if (file_handle.get() == INVALID_HANDLE_VALUE)
    return 0;

  // Get the time the file was last modified
  FILETIME ft_file;
  BOOL result = GetFileTime(file_handle.get(), nullptr, nullptr, &ft_file);

  if (!result)
    return 0;

  // Get current time
  SYSTEMTIME st_now;
  GetSystemTime(&st_now);
  FILETIME ft_now;
  SystemTimeToFileTime(&st_now, &ft_now);

  // Convert to ULARGE_INTEGER
  ULARGE_INTEGER ul_file;
  ul_file.LowPart = ft_file.dwLowDateTime;
  ul_file.HighPart = ft_file.dwHighDateTime;
  ULARGE_INTEGER ul_now;
  ul_now.LowPart = ft_now.dwLowDateTime;
  ul_now.HighPart = ft_now.dwHighDateTime;

  // Return difference in seconds
  return static_cast<unsigned long>(
      (ul_now.QuadPart - ul_file.QuadPart) / 10000000);
}

std::wstring GetFileLastModifiedDate(const std::wstring& path) {
  Handle file_handle{OpenFileForGenericRead(path)};
  if (file_handle.get() != INVALID_HANDLE_VALUE) {
    FILETIME ft_file = {0};
    BOOL result = GetFileTime(file_handle.get(), nullptr, nullptr, &ft_file);
    if (result) {
      SYSTEMTIME st_file = {0};
      result = FileTimeToSystemTime(&ft_file, &st_file);
      if (result)
        return Date(st_file).to_string();
    }
  }

  return std::wstring();
}

uint64_t GetFileSize(const std::wstring& path) {
  uint64_t file_size = 0;

  Handle file_handle{OpenFileForGenericRead(path)};

  if (file_handle.get() != INVALID_HANDLE_VALUE) {
    LARGE_INTEGER size;
    if (::GetFileSizeEx(file_handle.get(), &size))
      file_size = size.QuadPart;
  }

  return file_size;
}

uint64_t GetFolderSize(const std::wstring& path, bool recursive) {
  uint64_t folder_size = 0;
  const auto max_dword = static_cast<uint64_t>(MAXDWORD) + 1;

  const auto on_file = [&](const base::FileSearchResult& result) {
    folder_size +=
        static_cast<uint64_t>(result.data.nFileSizeHigh) * max_dword +
        static_cast<uint64_t>(result.data.nFileSizeLow);
    return false;
  };

  base::FileSearch helper;
  helper.options.log_errors = false;
  helper.options.skip_subdirectories = !recursive;
  helper.Search(path, nullptr, on_file);

  return folder_size;
}

////////////////////////////////////////////////////////////////////////////////

static bool ShellExecute(const std::wstring& verb, const std::wstring& path,
                         const std::wstring& parameters, int show_command) {
  if (path.empty())
    return false;

  if (path.length() > MAX_PATH && !verb.empty())
    LOGW(L"Path is longer than MAX_PATH: {}", path);

  SHELLEXECUTEINFO info = {0};
  info.cbSize = sizeof(SHELLEXECUTEINFO);
  info.fMask = SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
  info.lpVerb = verb.empty() ? nullptr : verb.c_str();
  info.lpFile = path.c_str();
  info.lpParameters = parameters.empty() ? nullptr : parameters.c_str();
  info.nShow = show_command;

  if (::ShellExecuteEx(&info) != TRUE) {
    auto error_message = win::FormatError(::GetLastError());
    TrimRight(error_message, L"\r\n");
    LOGE(L"ShellExecuteEx failed: {}\nVerb: {}\nPath: {}\nParameters: {}",
         error_message, verb, path, parameters);
    return false;
  }

  return true;
}

bool Execute(const std::wstring& path, const std::wstring& parameters,
             int show_command) {
  return ShellExecute(L"open", path, parameters, show_command);
}

bool ExecuteLink(const std::wstring& link) {
  return ShellExecute({}, link, {}, SW_SHOWNORMAL);
}

////////////////////////////////////////////////////////////////////////////////

bool OpenFolderAndSelectFile(const std::wstring& path) {
  HRESULT result = S_FALSE;
  const auto pidl = ::ILCreateFromPath(path.c_str());
  if (pidl) {
    result = ::SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
    ::ILFree(pidl);
  }
  return result == S_OK;
}

bool CreateFolder(const std::wstring& path) {
  auto result = SHCreateDirectoryEx(nullptr, path.c_str(), nullptr);
  return result == ERROR_SUCCESS || result == ERROR_ALREADY_EXISTS;
}

int DeleteFolder(std::wstring path) {
  if (path.back() == '\\')
    path.pop_back();

  path.push_back('\0');

  SHFILEOPSTRUCT fos = {0};
  fos.wFunc = FO_DELETE;
  fos.pFrom = path.c_str();
  fos.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

  return SHFileOperation(&fos);
}

// Extends the length limit from 260 to 32767 characters
// See: http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247%28v=vs.85%29.aspx#maxpath
std::wstring GetExtendedLengthPath(const std::wstring& path) {
  const std::wstring prefix = LR"(\\?\)";

  if (StartsWith(path, prefix))
    return path;

  // "\\computer\path" -> "\\?\UNC\computer\path"
  if (StartsWith(path, LR"(\\)"))
    return prefix + LR"(UNC\)" + path.substr(2);

  // "C:\path" -> "\\?\C:\path"
  return prefix + path;
}

std::wstring GetNormalizedPath(std::wstring path) {
  constexpr auto kPrefix = LR"(\\?\)";
  constexpr auto kUnc = L"UNC";

  if (StartsWith(path, kPrefix)) {
    // "\\?\C:\path" -> "C:\path"
    // "\\?\UNC\computer\path" -> "UNC\computer\path"
    EraseLeft(path, kPrefix);

    if (StartsWith(path, kUnc)) {
      // "UNC\computer\path" -> "\\computer\path"
      EraseLeft(path, kUnc);
      path.insert(0, 1, L'\\');
    }
  }

  return path;
}

bool IsDirectory(const WIN32_FIND_DATA& find_data) {
  return (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool IsHiddenFile(const WIN32_FIND_DATA& find_data) {
  return (find_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
}

bool IsSystemFile(const WIN32_FIND_DATA& find_data) {
  return (find_data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0;
}

bool IsValidDirectory(const WIN32_FIND_DATA& find_data) {
  return wcscmp(find_data.cFileName, L".") != 0 &&
         wcscmp(find_data.cFileName, L"..") != 0;
}

////////////////////////////////////////////////////////////////////////////////

bool FileExists(const std::wstring& file) {
  if (file.empty())
    return false;

  Handle file_handle{OpenFileForGenericRead(file)};
  return file_handle.get() != INVALID_HANDLE_VALUE;
}

bool FolderExists(const std::wstring& path) {
  win::ErrorMode error_mode(SEM_FAILCRITICALERRORS);

  auto file_attr = GetFileAttributes(GetExtendedLengthPath(path).c_str());

  return (file_attr != INVALID_FILE_ATTRIBUTES) &&
         (file_attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool PathExists(const std::wstring& path) {
  win::ErrorMode error_mode(SEM_FAILCRITICALERRORS);

  auto file_attr = GetFileAttributes(GetExtendedLengthPath(path).c_str());

  return file_attr != INVALID_FILE_ATTRIBUTES;
}

void ValidateFileName(std::wstring& file) {
  // Remove reserved characters, strip whitespace and trailing period
  // See: https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file
  EraseChars(file, LR"(<>:"/\|?*)");
  TrimLeft(file);
  TrimRight(file, L" .");

  // Collapse whitespace
  // This is not really required, but looks subjectively better. Besides, while
  // the file system supports consecutive spaces, some applications might not.
  while (ReplaceString(file, L"  ", L" "));
}

////////////////////////////////////////////////////////////////////////////////

std::wstring ExpandEnvironmentStrings(const std::wstring& path) {
  WCHAR buff[MAX_PATH];

  if (::ExpandEnvironmentStrings(path.c_str(), buff, MAX_PATH))
    return buff;

  return path;
}

std::wstring GetDefaultAppPath(const std::wstring& extension,
                               const std::wstring& default_value) {
  auto query_root_value = [](const std::wstring& subkey) {
    win::Registry reg;
    reg.OpenKey(HKEY_CLASSES_ROOT, subkey, 0, KEY_QUERY_VALUE);
    return reg.QueryValue(L"");
  };

  std::wstring path = query_root_value(extension);

  if (!path.empty())
    path = query_root_value(path + L"\\shell\\open\\command");

  if (!path.empty()) {
    size_t position = 0;
    bool inside_quotes = false;
    for (; position < path.size(); ++position) {
      if (path.at(position) == ' ') {
        if (!inside_quotes)
          break;
      } else if (path.at(position) == '"') {
        inside_quotes = !inside_quotes;
      }
    }
    if (position != path.size())
      path.resize(position);
    Trim(path, L"\" ");
  }

  return path.empty() ? default_value : path;
}

std::wstring GetKnownFolderPath(REFKNOWNFOLDERID rfid) {
  std::wstring output;

  PWSTR path = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(rfid, KF_FLAG_CREATE, nullptr, &path))) {
    output = path;
  }
  CoTaskMemFree(path);

  return output;
}

std::wstring GetFinalPathNameByHandle(HANDLE handle) {
  std::wstring buffer(MAX_PATH, '\0');

  const auto get_final_path_name_by_handle = [&]() {
    return ::GetFinalPathNameByHandle(handle, &buffer.front(),
                                      static_cast<DWORD>(buffer.size()),
                                      FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
  };

  auto result = get_final_path_name_by_handle();
  if (result > buffer.size()) {
    buffer.resize(result, '\0');
    result = get_final_path_name_by_handle();
  }

  if (result < buffer.size())
    buffer.resize(result);

  return buffer;
}

std::wstring GetFinalPath(const std::wstring& path) {
  // FILE_FLAG_BACKUP_SEMANTICS flag is required for directories
  Handle handle{::CreateFile(path.c_str(), 0, 0, nullptr, OPEN_EXISTING,
                             FILE_FLAG_BACKUP_SEMANTICS, nullptr)};

  if (handle.get() == INVALID_HANDLE_VALUE)
    return path;

  return GetFinalPathNameByHandle(handle.get());
}

////////////////////////////////////////////////////////////////////////////////

unsigned int PopulateFiles(std::vector<std::wstring>& file_list,
                           const std::wstring& path,
                           const std::wstring& extension,
                           bool recursive,
                           bool trim_extension) {
  unsigned int file_count = 0;

  const auto on_file = [&](const base::FileSearchResult& result) {
    if (extension.empty() ||
        IsEqual(GetFileExtension(result.name), extension)) {
      file_list.push_back(trim_extension ? GetFileWithoutExtension(result.name)
                                         : result.name);
      file_count++;
    }
    return false;
  };

  base::FileSearch helper;
  helper.options.log_errors = false;
  helper.options.skip_subdirectories = !recursive;
  helper.Search(path, nullptr, on_file);

  return file_count;
}

////////////////////////////////////////////////////////////////////////////////

bool ReadFromFile(const std::wstring& path, std::string& output) {
  Handle file_handle{OpenFileForGenericRead(path)};

  if (file_handle.get() == INVALID_HANDLE_VALUE)
    return false;

  LARGE_INTEGER file_size{};
  if (::GetFileSizeEx(file_handle.get(), &file_size) == FALSE)
    return false;
  output.resize(static_cast<size_t>(file_size.QuadPart));

  DWORD bytes_read = 0;
  const BOOL result = ::ReadFile(file_handle.get(), output.data(),
                                 static_cast<DWORD>(output.size()),
                                 &bytes_read, nullptr);

  return result != FALSE && bytes_read == output.size();
}

bool SaveToFile(LPCVOID data, DWORD length, const std::wstring& path,
                bool take_backup) {
  // Make sure the path is available
  CreateFolder(GetPathOnly(path));

  // Take a backup if needed
  if (take_backup) {
    std::wstring new_path = path + L".bak";
    MoveFileEx(path.c_str(), new_path.c_str(),
               MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
  }

  // Save the data
  BOOL result = FALSE;
  Handle file_handle{OpenFileForGenericWrite(path)};
  if (file_handle.get() != INVALID_HANDLE_VALUE) {
    DWORD bytes_written = 0;
    result =
        ::WriteFile(file_handle.get(), data, length, &bytes_written, nullptr);
  }

  return result != FALSE;
}

bool SaveToFile(const std::string& data, const std::wstring& path,
                bool take_backup) {
  if (data.empty())
    return false;

  return SaveToFile((LPCVOID)&data.front(), static_cast<DWORD>(data.size()),
                    path, take_backup);
}

////////////////////////////////////////////////////////////////////////////////

enum Unit : UINT64 {
  kKB  = 1000,
  kKiB = 1024,
  kMB  = 1000000,
  kMiB = 1048576,
  kGB  = 1000000000,
  kGiB = 1073741824,
  kTB  = 1000000000000,
  kTiB = 1099511627776,
};

UINT64 ParseSizeString(std::wstring value) {
  std::wstring unit;
  const auto pos = value.find_first_not_of(L"0123456789.");
  if (pos != value.npos) {
    unit = value.substr(pos);
    value.resize(pos);
    Trim(unit, L"\r\n\t ");
  }

  const auto size = [&unit]() -> UINT64 {
    if (IsEqual(unit, L"KB")) return Unit::kKB;
    if (IsEqual(unit, L"KiB")) return Unit::kKiB;
    if (IsEqual(unit, L"MB")) return Unit::kMB;
    if (IsEqual(unit, L"MiB")) return Unit::kMiB;
    if (IsEqual(unit, L"GB")) return Unit::kGB;
    if (IsEqual(unit, L"GiB")) return Unit::kGiB;
    if (IsEqual(unit, L"TB")) return Unit::kTB;
    if (IsEqual(unit, L"TiB")) return Unit::kTiB;
    return 1;
  }();

  return static_cast<UINT64>(size * ToDouble(value));
}

std::wstring ToSizeString(const UINT64 size) {
  const auto to_wstr = [&size](const Unit unit) {
    return ToWstr(static_cast<double>(size) / unit, 1);
  };

  if (size >= Unit::kTiB) return to_wstr(Unit::kTiB) + L" TiB";
  if (size >= Unit::kGiB) return to_wstr(Unit::kGiB) + L" GiB";
  if (size >= Unit::kMiB) return to_wstr(Unit::kMiB) + L" MiB";
  if (size >= Unit::kKiB) return to_wstr(Unit::kKiB) + L" KiB";

  return ToWstr(size) + L" bytes";
}
