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

#include <fstream>
#include <shlobj.h>

#include "file.h"
#include "string.h"
#include "win/win_registry.h"

#define MAKEQWORD(a, b) ((QWORD)(((QWORD)((DWORD)(a))) << 32 | ((DWORD)(b))))

////////////////////////////////////////////////////////////////////////////////

HANDLE OpenFileForGenericRead(const std::wstring& path) {
  return ::CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
}

HANDLE OpenFileForGenericWrite(const std::wstring& path) {
  return ::CreateFile(path.c_str(), GENERIC_WRITE, 0, nullptr,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
}

////////////////////////////////////////////////////////////////////////////////

unsigned long GetFileAge(const std::wstring& path) {
  HANDLE file_handle = OpenFileForGenericRead(path);

  if (file_handle == INVALID_HANDLE_VALUE)
    return 0;

  // Get the time the file was last modified
  FILETIME ft_file;
  BOOL result = GetFileTime(file_handle, nullptr, nullptr, &ft_file);
  CloseHandle(file_handle);

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

QWORD GetFileSize(const std::wstring& path) {
  QWORD file_size = 0;

  HANDLE file_handle = OpenFileForGenericRead(path);

  if (file_handle != INVALID_HANDLE_VALUE) {
    DWORD size_high = 0;
    DWORD size_low = ::GetFileSize(file_handle, &size_high);
    if (size_low != INVALID_FILE_SIZE)
      file_size = MAKEQWORD(size_high, size_low);
    CloseHandle(file_handle);
  }

  return file_size;
}

QWORD GetFolderSize(const std::wstring& path, bool recursive) {
  QWORD folder_size = 0;

  WIN32_FIND_DATA find_data;
  std::wstring file_name = path + L"*.*";
  HANDLE file_handle = FindFirstFile(file_name.c_str(), &find_data);

  if (file_handle == INVALID_HANDLE_VALUE)
    return 0;

  QWORD max_dword = static_cast<QWORD>(MAXDWORD) + 1;

  do {
    if (IsDirectory(find_data)) {
      if (recursive && IsValidDirectory(find_data)) {
        file_name = path + find_data.cFileName + L"\\";
        folder_size += GetFolderSize(file_name, recursive);
      }
    } else {
      folder_size += static_cast<QWORD>(find_data.nFileSizeHigh) * max_dword +
                     static_cast<QWORD>(find_data.nFileSizeLow);
    }
  } while (FindNextFile(file_handle, &find_data));

  FindClose(file_handle);

  return folder_size;
}

////////////////////////////////////////////////////////////////////////////////

bool Execute(const std::wstring& path, const std::wstring& parameters) {
  if (path.empty())
    return false;

  HINSTANCE value = ShellExecute(nullptr, L"open", path.c_str(),
                                 parameters.c_str(), nullptr, SW_SHOWNORMAL);

  return reinterpret_cast<int>(value) > 32;
}

BOOL ExecuteEx(const std::wstring& path, const std::wstring& parameters) {
  SHELLEXECUTEINFO si = {0};
  si.cbSize = sizeof(SHELLEXECUTEINFO);
  si.fMask = SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI | SEE_MASK_UNICODE;
  si.lpVerb = L"open";
  si.lpFile = path.c_str();
  si.lpParameters = parameters.c_str();
  si.nShow = SW_SHOWNORMAL;

  return ShellExecuteEx(&si);
}

void ExecuteLink(const std::wstring& link) {
  ShellExecute(nullptr, nullptr, link.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

////////////////////////////////////////////////////////////////////////////////

bool CreateFolder(const std::wstring& path) {
  return SHCreateDirectoryEx(nullptr, path.c_str(), nullptr) == ERROR_SUCCESS;
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
std::wstring GetExtendedLengthPath(const std::wstring& path) {
  if (!StartsWith(path, L"\\\\?\\"))
    return L"\\\\?\\" + path;
  return path;
}

bool IsDirectory(const WIN32_FIND_DATA& find_data) {
  return (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool IsValidDirectory(const WIN32_FIND_DATA& find_data) {
  return wcscmp(find_data.cFileName, L".") != 0 &&
         wcscmp(find_data.cFileName, L"..") != 0;
}

////////////////////////////////////////////////////////////////////////////////

bool FileExists(const std::wstring& file) {
  if (file.empty())
    return false;

  HANDLE file_handle = OpenFileForGenericRead(file);

  if (file_handle != INVALID_HANDLE_VALUE) {
    CloseHandle(file_handle);
    return true;
  }

  return false;
}

bool FolderExists(const std::wstring& path) {
  DWORD file_attr = GetFileAttributes(path.c_str());

  return (file_attr != INVALID_FILE_ATTRIBUTES) &&
         (file_attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool PathExists(const std::wstring& path) {
  return GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

void ValidateFileName(std::wstring& file) {
  EraseChars(file, L"\\/:*?<>|");
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
  win::Registry reg;
  reg.OpenKey(HKEY_CLASSES_ROOT, extension, 0, KEY_QUERY_VALUE);

  std::wstring path = reg.QueryValue(L"");

  if (!path.empty()) {
    path += L"\\shell\\open\\command";
    reg.OpenKey(HKEY_CLASSES_ROOT, path, 0, KEY_QUERY_VALUE);

    path = reg.QueryValue(L"");
    Replace(path, L"\"", L"");
    Trim(path, L" %1");
  }

  reg.CloseKey();

  return path.empty() ? default_value : path;
}

////////////////////////////////////////////////////////////////////////////////

unsigned int PopulateFiles(std::vector<std::wstring>& file_list,
                           const std::wstring& path,
                           const std::wstring& extension,
                           bool recursive, bool trim_extension) {
  if (path.empty())
    return 0;

  WIN32_FIND_DATA find_data;
  std::wstring file_name = path + L"*.*";
  HANDLE file_handle = FindFirstFile(file_name.c_str(), &find_data);

  if (file_handle == INVALID_HANDLE_VALUE)
    return 0;

  unsigned int file_count = 0;

  do {
    if (IsDirectory(find_data)) {
      if (recursive && IsValidDirectory(find_data)) {
        file_name = path + find_data.cFileName + L"\\";
        file_count += PopulateFiles(file_list, file_name, extension, recursive,
                                    trim_extension);
      }
    } else {
      if (extension.empty() ||
          IsEqual(GetFileExtension(find_data.cFileName), extension)) {
        if (trim_extension) {
          file_list.push_back(GetFileWithoutExtension(find_data.cFileName));
        } else {
          file_list.push_back(find_data.cFileName);
        }
        file_count++;
      }
    }
  } while (FindNextFile(file_handle, &find_data));

  FindClose(file_handle);

  return file_count;
}

int PopulateFolders(std::vector<std::wstring>& folder_list,
                    const std::wstring& path) {
  if (path.empty())
    return 0;

  WIN32_FIND_DATA find_data;
  std::wstring file_name = path + L"*.*";
  HANDLE file_handle = FindFirstFile(file_name.c_str(), &find_data);

  if (file_handle == INVALID_HANDLE_VALUE)
    return 0;

  int folder_count = 0;

  do {
    if (IsDirectory(find_data) && IsValidDirectory(find_data)) {
      folder_count++;
      folder_list.push_back(find_data.cFileName);
    }
  } while (FindNextFile(file_handle, &find_data));

  FindClose(file_handle);

  return folder_count;
}

////////////////////////////////////////////////////////////////////////////////

bool ReadFromFile(const std::wstring& path, std::string& output) {
  std::ifstream is;
  is.open(WstrToStr(path).c_str(), std::ios::binary);

  is.seekg(0, std::ios::end);
  size_t len = static_cast<size_t>(is.tellg());

  if (len != -1) {
    output.resize(len);
    is.seekg(0, std::ios::beg);
    is.read((char*)output.data(), output.size());
  }

  is.close();
  return len != -1;
}

bool SaveToFile(LPCVOID data, DWORD length, const string_t& path,
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
  HANDLE file_handle = OpenFileForGenericWrite(path);
  if (file_handle != INVALID_HANDLE_VALUE) {
    DWORD bytes_written = 0;
    result = ::WriteFile(file_handle, data, length, &bytes_written, nullptr);
    ::CloseHandle(file_handle);
  }

  return result != FALSE;
}

////////////////////////////////////////////////////////////////////////////////

std::wstring ToSizeString(QWORD qwSize) {
  std::wstring size, unit;

  if (qwSize > 1073741824) {      // 2^30
    size = ToWstr(static_cast<double>(qwSize) / 1073741824, 2);
    unit = L" GB";
  } else if (qwSize > 1048576) {  // 2^20
    size = ToWstr(static_cast<double>(qwSize) / 1048576, 2);
    unit = L" MB";
  } else if (qwSize > 1024) {     // 2^10
    size = ToWstr(static_cast<double>(qwSize) / 1024, 2);
    unit = L" KB";
  } else {
    size = ToWstr(qwSize);
    unit = L" bytes";
  }

  return size + unit;
}