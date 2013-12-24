/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
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

#include <shlobj.h>

#include "file.h"
#include "string.h"
#include "win/win_main.h"
#include "win/win_registry.h"

#ifdef _MSC_VER
#pragma warning (disable: 4996)
#endif

#define MAKEQWORD(a, b)	((QWORD)(((QWORD)((DWORD)(a))) << 32 | ((DWORD)(b))))

////////////////////////////////////////////////////////////////////////////////

HANDLE OpenFileForGenericRead(const wstring& path) {
  return ::CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
}

HANDLE OpenFileForGenericWrite(const wstring& path) {
  return ::CreateFile(path.c_str(), GENERIC_WRITE, 0, nullptr,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
}

////////////////////////////////////////////////////////////////////////////////

unsigned long GetFileAge(const wstring& path) {
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

QWORD GetFileSize(const wstring& path) {
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

QWORD GetFolderSize(const wstring& path, bool recursive) {
  QWORD folder_size = 0;
  
  WIN32_FIND_DATA wfd;
  wstring file_name = path + L"*.*";
  HANDLE file_handle = FindFirstFile(file_name.c_str(), &wfd);

  if (file_handle == INVALID_HANDLE_VALUE)
    return 0;

  QWORD max_dword = static_cast<QWORD>(MAXDWORD) + 1;

  do {
    if (recursive && wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if (wcscmp(wfd.cFileName, L".") != 0 &&
          wcscmp(wfd.cFileName, L"..") != 0) {
        file_name = path + wfd.cFileName + L"\\";
        folder_size += GetFolderSize(file_name, recursive);
      }
    }
    folder_size += static_cast<QWORD>(wfd.nFileSizeHigh) * max_dword +
                   static_cast<QWORD>(wfd.nFileSizeLow);
  } while (FindNextFile(file_handle, &wfd));
	
  FindClose(file_handle);

  return folder_size;
}

////////////////////////////////////////////////////////////////////////////////

bool Execute(const wstring& path, const wstring& parameters) {
  if (path.empty())
    return false;

  HINSTANCE value = ShellExecute(nullptr, L"open", path.c_str(),
                                 parameters.c_str(), nullptr, SW_SHOWNORMAL);

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
  ShellExecute(nullptr, nullptr, link.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

////////////////////////////////////////////////////////////////////////////////

wstring BrowseForFile(HWND hwndOwner, LPCWSTR lpstrTitle, LPCWSTR lpstrFilter) {
  if (!lpstrFilter)
    lpstrFilter = L"All files (*.*)\0*.*\0";

  WCHAR szFile[MAX_PATH] = {'\0'};

  OPENFILENAME ofn = {0};
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hwndOwner;
  ofn.lpstrFile = szFile;
  ofn.lpstrFilter = lpstrFilter;
  ofn.lpstrTitle = lpstrTitle;
  ofn.nMaxFile = sizeof(szFile);
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
  
  if (GetOpenFileName(&ofn)) {
    return wstring(szFile);
  } else {
    return wstring();
  }
}

bool BrowseForFolderVista(HWND hwnd, const wstring& title,
                          const wstring& default_folder, wstring& output) {
  IFileDialog* pFileDialog;
  bool result = false;

  HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog,
                                nullptr,
                                CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&pFileDialog));

  if (SUCCEEDED(hr)) {
    FILEOPENDIALOGOPTIONS fos;
    pFileDialog->GetOptions(&fos);
    fos |= FOS_PICKFOLDERS;
    pFileDialog->SetOptions(fos);

    if (!title.empty())
      pFileDialog->SetTitle(title.c_str());

    if (!default_folder.empty()) {
      IShellItem* pShellItem;
      HRESULT hr = 0;

      typedef HRESULT (WINAPI *_SHCreateItemFromParsingName)(
          PCWSTR pszPath, IBindCtx *pbc, REFIID riid, void **ppv);
      HMODULE hShell32 = LoadLibrary(L"shell32.dll");
      if (hShell32 != nullptr) {
        auto proc = reinterpret_cast<_SHCreateItemFromParsingName>(
            GetProcAddress(hShell32, "SHCreateItemFromParsingName"));
        if (proc != nullptr) {
          hr = (proc)(default_folder.c_str(), nullptr, IID_IShellItem,
                      reinterpret_cast<void**>(&pShellItem));
        }
        FreeLibrary(hShell32);
      }

      if (SUCCEEDED(hr)) {
        pFileDialog->SetDefaultFolder(pShellItem);
        pShellItem->Release();
      }
    }

    hr = pFileDialog->Show(hwnd);
    if (SUCCEEDED(hr)) {
      IShellItem* pShellItem;
      hr = pFileDialog->GetFolder(&pShellItem);
      if (SUCCEEDED(hr)) {
        LPWSTR path = nullptr;
        hr = pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &path);
        if (SUCCEEDED(hr)) {
          output.assign(path);
          CoTaskMemFree(path);
          result = true;
        }
        pShellItem->Release();
      }
    }

    pFileDialog->Release();
  }

  return result;
}

static int CALLBACK BrowseForFolderXpProc(HWND hwnd, UINT uMsg, LPARAM lParam,
                                          LPARAM lpData) {
  switch (uMsg) {
    case BFFM_INITIALIZED:
      if (lpData != 0)
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
      break;
  }

  return 0;
}

bool BrowseForFolderXp(HWND hwnd, const wstring& title,
                       const wstring& default_path, wstring& output) {
  BROWSEINFO bi = {0};
  bi.hwndOwner = hwnd;
  bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON;

  if (!title.empty())
    bi.lpszTitle = title.c_str();

  if (!default_path.empty()) {
    WCHAR lpszDefault[MAX_PATH] = {'\0'};
    default_path.copy(lpszDefault, MAX_PATH);
    bi.lParam = reinterpret_cast<LPARAM>(lpszDefault);
    bi.lpfn = BrowseForFolderXpProc;
  }

  PIDLIST_ABSOLUTE pidl = SHBrowseForFolder(&bi);
  if (pidl == nullptr)
    return false;

  WCHAR path[MAX_PATH];
  SHGetPathFromIDList(pidl, path);
  output = path;

  return !output.empty();
}

BOOL BrowseForFolder(HWND hwnd, const wstring& title,
                     const wstring& default_path, wstring& output) {
  if (win::GetVersion() >= win::kVersionVista) {
    return BrowseForFolderVista(hwnd, title, default_path, output);
  } else {
    return BrowseForFolderXp(hwnd, title, default_path, output);
  }
}

////////////////////////////////////////////////////////////////////////////////

bool CreateFolder(const wstring& path) {
  return SHCreateDirectoryEx(nullptr, path.c_str(), nullptr) == ERROR_SUCCESS;
}

int DeleteFolder(wstring path) {
  if (path.back() == '\\')
    path.pop_back();

  path.push_back('\0');

  SHFILEOPSTRUCT fos = {0};
  fos.wFunc = FO_DELETE;
  fos.pFrom = path.c_str();
  fos.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

  return SHFileOperation(&fos);
}

////////////////////////////////////////////////////////////////////////////////

bool FileExists(const wstring& file) {
  if (file.empty())
    return false;

  HANDLE file_handle = OpenFileForGenericRead(file);

  if (file_handle != INVALID_HANDLE_VALUE) {
    CloseHandle(file_handle);
    return true;
  }

  return false;
}

bool FolderExists(const wstring& path) {
  DWORD file_attr = GetFileAttributes(path.c_str());

  return (file_attr != INVALID_FILE_ATTRIBUTES) &&
         (file_attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool PathExists(const wstring& path) {
  return GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

void ValidateFileName(wstring& file) {
  EraseChars(file, L"\\/:*?<>|");
}

////////////////////////////////////////////////////////////////////////////////

wstring ExpandEnvironmentStrings(const wstring& path) {
  WCHAR buff[MAX_PATH];

  if (::ExpandEnvironmentStrings(path.c_str(), buff, MAX_PATH))
    return buff;

  return path;
}

wstring GetDefaultAppPath(const wstring& extension,
                          const wstring& default_value) {
  win::Registry reg;
  reg.OpenKey(HKEY_CLASSES_ROOT, extension, 0, KEY_QUERY_VALUE);

  wstring path = reg.QueryValue(L"");

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

unsigned int PopulateFiles(vector<wstring>& file_list,
                           const wstring& path,
                           const wstring& extension,
                           bool recursive, bool trim_extension) {
  if (path.empty())
    return 0;

  WIN32_FIND_DATA wfd;
  wstring file_name = path + L"*.*";
  HANDLE file_handle = FindFirstFile(file_name.c_str(), &wfd);

  if (file_handle == INVALID_HANDLE_VALUE)
    return 0;

  unsigned int file_count = 0;

  do {
    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if (recursive &&
          wcscmp(wfd.cFileName, L".") != 0 &&
          wcscmp(wfd.cFileName, L"..") != 0) {
        file_name = path + wfd.cFileName + L"\\";
        file_count += PopulateFiles(file_list, file_name, extension, recursive,
                                    trim_extension);
      }
    } else if (wfd.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
      if (extension.empty() ||
          IsEqual(GetFileExtension(wfd.cFileName), extension)) {
        if (trim_extension) {
          file_list.push_back(GetFileWithoutExtension(wfd.cFileName));
        } else {
          file_list.push_back(wfd.cFileName);
        }
        file_count++;
      }
    }
  } while (FindNextFile(file_handle, &wfd));

  FindClose(file_handle);

  return file_count;
}

int PopulateFolders(vector<wstring>& folder_list, const wstring& path) {
  if (path.empty())
    return 0;

  WIN32_FIND_DATA wfd;
  wstring file_name = path + L"*.*";
  HANDLE file_handle = FindFirstFile(path.c_str(), &wfd);

  if (file_handle == INVALID_HANDLE_VALUE)
    return 0;

  int folder_count = 0;

  do {
    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if (wcscmp(wfd.cFileName, L".") != 0 &&
          wcscmp(wfd.cFileName, L"..") != 0) {
        folder_count++;
        folder_list.push_back(wfd.cFileName);
      }
    }
  } while (FindNextFile(file_handle, &wfd));

  FindClose(file_handle);

  return folder_count;
}

////////////////////////////////////////////////////////////////////////////////

bool SaveToFile(LPCVOID data, DWORD length, const string_t& path,
                bool take_backup) {
  // Make sure the path is available
  CreateFolder(GetPathOnly(path));

  // Take a backup if needed
  if (take_backup) {
    wstring new_path = path + L".bak";
    MoveFileEx(path.c_str(), new_path.c_str(),
               MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
  }

  // Save the data
  BOOL result = FALSE;
  HANDLE file_handle = OpenFileForGenericWrite(path);
  if (file_handle != INVALID_HANDLE_VALUE) {
    DWORD bytes_written = 0;
    result = ::WriteFile(file_handle, (LPCVOID)data, length, &bytes_written,
                         nullptr);
    ::CloseHandle(file_handle);
  }

  return result != FALSE;
}

////////////////////////////////////////////////////////////////////////////////

wstring ToSizeString(QWORD qwSize) {
  wstring size, unit;

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