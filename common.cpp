/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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

#include "std.h"
#include "animelist.h"
#include "third_party/base64/base64.h"
#include "common.h"
#include "myanimelist.h"
#include "settings.h"
#include "string.h"
#include "theme.h"

// =============================================================================

wstring Base64Decode(const wstring& str) {
  Base64Coder coder;
  string buff = ToANSI(str);
  coder.Decode((BYTE*)buff.c_str(), buff.length());
  return ToUTF8(coder.DecodedMessage());
}

wstring Base64Encode(const wstring& str) {
  Base64Coder coder;
  string buff = ToANSI(str);
  coder.Encode((BYTE*)buff.c_str(), buff.length());
  return ToUTF8(coder.EncodedMessage());
}

// =============================================================================

int StatusToIcon(int status) {  
  switch (status) {
    case MAL_AIRING:
      return Icon16_Green;
    case MAL_FINISHED:
      return Icon16_Blue;
    case MAL_NOTYETAIRED:
      return Icon16_Red;
    default:
      return Icon16_Gray;
  }
}

// =============================================================================

wstring FormatError(DWORD dwError, LPCWSTR lpSource) {
  DWORD dwFlags = FORMAT_MESSAGE_IGNORE_INSERTS;
  HMODULE hInstance = NULL;
  const DWORD size = 101;
  WCHAR buffer[size];

  if (lpSource) {
    dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    hInstance = LoadLibrary(lpSource);
    if (!hInstance) return L"";
  } else {
    dwFlags |= FORMAT_MESSAGE_FROM_SYSTEM;
  }

  if (FormatMessage(dwFlags, hInstance, dwError, 
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, size, NULL)) {
      if (hInstance) FreeLibrary(hInstance);
      return buffer;
  } else {
    if (hInstance) FreeLibrary(hInstance);
    return ToWSTR(dwError);
  }
}

wstring GetDate(LPCWSTR lpFormat) {
  WCHAR buff[32];
  GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, NULL, lpFormat, buff, 32);
  return buff;
}

wstring GetTime(LPCWSTR lpFormat) {
  WCHAR buff[32];
  GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, NULL, lpFormat, buff, 32);
  return buff;
}

wstring GetLastEpisode(const wstring& episode) {
  int value = 1, pos = InStrRev(episode, L"-", episode.length());
  if (pos == episode.length() - 1) {
    value = ToINT(episode.substr(0, pos));
  } else if (pos > -1) {
    value = ToINT(episode.substr(pos + 1));
  } else {
    value = ToINT(episode);
  }
  if (value == 0) value = 1;
  return ToWSTR(value);
}

wstring ToTimeString(int seconds) {
  int hours = seconds / 3600;
  seconds = seconds % 3600;
  int minutes = seconds / 60;
  seconds = seconds % 60;
  #define TWO_DIGIT(x) (x >= 10 ? ToWSTR(x) : L"0" + ToWSTR(x))
  return (hours > 0 ? TWO_DIGIT(hours) + L":" : L"") + 
    TWO_DIGIT(minutes) + L":" + TWO_DIGIT(seconds);
  #undef TWO_DIGIT
}

// =============================================================================

bool Execute(const wstring& path, const wstring& parameters) {
  if (path.empty()) return false;
  HINSTANCE value = ShellExecute(NULL, L"open", path.c_str(), parameters.c_str(), NULL, SW_SHOWNORMAL);
  return reinterpret_cast<int>(value) > 32;
}

BOOL ExecuteEx(const wstring& path, const wstring& parameters) {
  SHELLEXECUTEINFO si = {0};
  si.cbSize = sizeof(SHELLEXECUTEINFO);
  si.fMask = SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI | SEE_MASK_UNICODE;
  si.lpVerb = L"open";
  si.lpFile = path.c_str();
  si.lpParameters = parameters.c_str();
  si.nShow = SW_SHOWNORMAL;
  return ShellExecuteEx(&si);
}

void ExecuteLink(const wstring& link) {
  ShellExecute(NULL, NULL, link.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

wstring ExpandEnvironmentStrings(const wstring& path) {
  WCHAR buff[MAX_PATH];
  if (::ExpandEnvironmentStrings(path.c_str(), buff, MAX_PATH)) {
    return buff;
  } else {
    return path;
  }
}

wstring BrowseForFile(HWND hwndOwner, LPCWSTR lpstrTitle, LPCWSTR lpstrFilter) {
  WCHAR szFile[MAX_PATH] = {'\0'};

  if (!lpstrFilter) {
    lpstrFilter = L"All files (*.*)\0*.*\0";
  }

  OPENFILENAME ofn = {0};
  ofn.lStructSize  = sizeof(OPENFILENAME);
  ofn.hwndOwner    = hwndOwner;
  ofn.lpstrFile    = szFile;
  ofn.lpstrFilter  = lpstrFilter;
  ofn.lpstrTitle   = lpstrTitle;
  ofn.nMaxFile     = sizeof(szFile);
  ofn.Flags        = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
  
  if (GetOpenFileName(&ofn)) {
    return szFile;
  } else {
    return L"";
  }
}

BOOL BrowseForFolder(HWND hwndOwner, LPCWSTR lpszTitle, UINT ulFlags, wstring& output) {
  BROWSEINFO bi = {0};
  bi.hwndOwner  = hwndOwner;
  bi.lpszTitle  = lpszTitle;
  bi.ulFlags    = ulFlags;
  
  PIDLIST_ABSOLUTE pidl = SHBrowseForFolder(&bi);
  if (pidl == NULL) return FALSE;
  
  WCHAR path[MAX_PATH];
  SHGetPathFromIDList(pidl, path);
  output = path;
  
  if (output.empty()) return FALSE;
  return TRUE;
}

BOOL FileExists(const wstring& file) {
  if (file.empty()) return FALSE;
  HANDLE hFile = CreateFile(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, 
    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile != INVALID_HANDLE_VALUE) {
    CloseHandle(hFile);
    return TRUE;
  }
  return FALSE;
}

BOOL FolderExists(const wstring& path) {
  DWORD fileattr = GetFileAttributes(path.c_str());
  if (fileattr != INVALID_FILE_ATTRIBUTES) {
    return (fileattr & FILE_ATTRIBUTE_DIRECTORY) != 0;
  }
  return false;
}

BOOL PathExists(const wstring& path) {
  return GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

void ValidateFileName(wstring& file) {
  ReplaceChars(file, L"\\/:*?<>|", L"");
}

wstring GetDefaultAppPath(const wstring& extension, const wstring& default_value) {
  wstring path = Registry_ReadValue(HKEY_CLASSES_ROOT, extension.c_str(), NULL);
  if (!path.empty()) {
    path += L"\\shell\\open\\command";
    path = Registry_ReadValue(HKEY_CLASSES_ROOT, path.c_str(), NULL);
    Replace(path, L"\"", L"");
    Trim(path, L" %1");
  }
  return path.empty() ? default_value : path;
}

int PopulateFiles(vector<wstring>& file_list, wstring path, wstring extension) {
  if (path.empty()) return 0;
  path += extension;
  int found = 0;

  WIN32_FIND_DATA wfd;
  HANDLE hFind = FindFirstFile(path.c_str(), &wfd);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (wfd.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
        found++;
        file_list.push_back(wfd.cFileName);
      }
    } while (FindNextFile(hFind, &wfd));
    FindClose(hFind);
  }

  return found;
}

int PopulateFolders(vector<wstring>& folder_list, wstring path) {
  if (path.empty()) return 0;
  path += L"*.*";
  int found = 0;

  WIN32_FIND_DATA wfd;
  HANDLE hFind = FindFirstFile(path.c_str(), &wfd);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (!IsEqual(wfd.cFileName, L".") && !IsEqual(wfd.cFileName, L"..")) {
          found++;
          folder_list.push_back(wfd.cFileName);
        }
      }
    } while (FindNextFile(hFind, &wfd));
    FindClose(hFind);
  }

  return found;
}

wstring ToSizeString(QWORD qwSize) {
  wstring size, unit;

  if (qwSize > 1073741824) {      // 2^30
    size = ToWSTR(static_cast<double>(qwSize) / 1073741824, 2);
    unit = L" GB";
  } else if (qwSize > 1048576) {  // 2^20
    size = ToWSTR(static_cast<double>(qwSize) / 1048576, 2);
    unit = L" MB";
  } else if (qwSize > 1024) {     // 2^10
    size = ToWSTR(static_cast<double>(qwSize) / 1024, 2);
    unit = L" KB";
  } else {
    size = ToWSTR(qwSize);
    unit = L" bytes";
  }

  return size + unit;
}

// =============================================================================

void Registry_DeleteValue(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValueName) {
  HKEY hResult = NULL;
  if (RegOpenKeyEx(hKey, lpSubKey, 0, KEY_SET_VALUE, &hResult) == ERROR_SUCCESS) {
    RegDeleteValue(hResult, lpValueName);
    RegCloseKey(hResult);
  }
}

wstring Registry_ReadValue(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValueName) {
  HKEY hResult = NULL;
  if (RegOpenKeyEx(hKey, lpSubKey, 0, KEY_QUERY_VALUE, &hResult) == ERROR_SUCCESS) {
    DWORD dwType = 0;
    WCHAR szBuffer[MAX_PATH];
    DWORD dwBufferSize = sizeof(szBuffer);
    if (RegQueryValueEx(hResult, lpValueName, NULL, &dwType, (LPBYTE)&szBuffer, &dwBufferSize) == ERROR_SUCCESS) {
      if (dwType == REG_SZ) {
        RegCloseKey(hResult);
        return szBuffer;
      }
    }
    RegCloseKey(hResult);
  }
  return L"";
}

void Registry_SetValue(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPCWSTR lpData, DWORD cbData) {
  HKEY hResult = NULL;
  if (RegCreateKeyEx(hKey, lpSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hResult, NULL) == ERROR_SUCCESS) {
    RegSetValueEx(hResult, lpValueName, 0, REG_SZ, (LPBYTE)lpData, (cbData + 1) * sizeof(WCHAR));
    RegCloseKey(hResult);
  }
}