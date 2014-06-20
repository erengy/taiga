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

#include <shlobj.h>

#include "win/win_commondialog.h"

#ifdef _MSC_VER
#pragma warning (disable: 4996)
#endif

namespace win {

std::wstring BrowseForFile(HWND hwnd_owner,
                           const std::wstring& title,
                           std::wstring filter) {
  if (filter.empty())
    filter = L"All files (*.*)\0*.*\0";

  WCHAR file[MAX_PATH] = {'\0'};

  OPENFILENAME ofn = {0};
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hwnd_owner;
  ofn.lpstrFile = file;
  ofn.lpstrFilter = filter.c_str();
  ofn.lpstrTitle = title.c_str();
  ofn.nMaxFile = sizeof(file);
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;

  if (GetOpenFileName(&ofn)) {
    return std::wstring(file);
  } else {
    return std::wstring();
  }
}

bool BrowseForFolderVista(HWND hwnd_owner,
                          const std::wstring& title,
                          const std::wstring& default_folder,
                          std::wstring& output) {
  IFileDialog* file_dialog;
  bool result = false;

  HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog,
                                nullptr,
                                CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&file_dialog));

  if (SUCCEEDED(hr)) {
    FILEOPENDIALOGOPTIONS options;
    file_dialog->GetOptions(&options);
    options |= FOS_PICKFOLDERS;
    file_dialog->SetOptions(options);

    if (!title.empty())
      file_dialog->SetTitle(title.c_str());

    if (!default_folder.empty()) {
      IShellItem* shell_item;
      HRESULT hr = 0;

      typedef HRESULT (WINAPI* _SHCreateItemFromParsingName)(
          PCWSTR pszPath, IBindCtx* pbc, REFIID riid, void** ppv);
      HMODULE module = LoadLibrary(L"shell32.dll");
      if (module != nullptr) {
        auto proc = reinterpret_cast<_SHCreateItemFromParsingName>(
            GetProcAddress(module, "SHCreateItemFromParsingName"));
        if (proc != nullptr) {
          hr = (proc)(default_folder.c_str(), nullptr, IID_IShellItem,
                      reinterpret_cast<void**>(&shell_item));
        }
        FreeLibrary(module);
      }

      if (SUCCEEDED(hr)) {
        file_dialog->SetDefaultFolder(shell_item);
        shell_item->Release();
      }
    }

    hr = file_dialog->Show(hwnd_owner);
    if (SUCCEEDED(hr)) {
      IShellItem* shell_item;
      hr = file_dialog->GetFolder(&shell_item);
      if (SUCCEEDED(hr)) {
        LPWSTR path = nullptr;
        hr = shell_item->GetDisplayName(SIGDN_FILESYSPATH, &path);
        if (SUCCEEDED(hr)) {
          output.assign(path);
          CoTaskMemFree(path);
          result = true;
        }
        shell_item->Release();
      }
    }

    file_dialog->Release();
  }

  return result;
}

static int CALLBACK BrowseForFolderXpProc(
    HWND hwnd_owner, UINT uMsg, LPARAM lParam, LPARAM lpData) {
  switch (uMsg) {
    case BFFM_INITIALIZED:
      if (lpData != 0)
        SendMessage(hwnd_owner, BFFM_SETSELECTION, TRUE, lpData);
      break;
  }

  return 0;
}

bool BrowseForFolderXp(HWND hwnd_owner,
                       const std::wstring& title,
                       const std::wstring& default_path,
                       std::wstring& output) {
  BROWSEINFO bi = {0};
  bi.hwndOwner = hwnd_owner;
  bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON;

  if (!title.empty())
    bi.lpszTitle = title.c_str();

  if (!default_path.empty()) {
    WCHAR path[MAX_PATH] = {'\0'};
    default_path.copy(path, MAX_PATH);
    bi.lParam = reinterpret_cast<LPARAM>(path);
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

bool BrowseForFolder(HWND hwnd_owner,
                     const std::wstring& title,
                     const std::wstring& default_path,
                     std::wstring& output) {
  if (win::GetVersion() >= win::kVersionVista) {
    return BrowseForFolderVista(hwnd_owner, title, default_path, output);
  } else {
    return BrowseForFolderXp(hwnd_owner, title, default_path, output);
  }
}

}  // namespace win