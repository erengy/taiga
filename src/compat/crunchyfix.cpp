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

#include <windows.h>
#include <tlhelp32.h>

#include <anisthesia/src/win/util.h>
#include <windows/win/error.h>
#include <windows/win/registry.h>
#include <windows/win/task_dialog.h>

#include "base/file.h"
#include "base/log.h"
#include "base/string.h"

using anisthesia::win::Handle;

namespace crunchyfix {

const auto kRegistryValueName = L"Java";
const auto kExecutableFileName = L"svchost.exe";

bool DeleteRegistryValue() {
  win::Registry reg;

  if (reg.OpenKey(HKEY_CURRENT_USER,
                  L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                  0, KEY_SET_VALUE) != ERROR_SUCCESS) {
    LOGW(L"Could not open the registry key.");
    return false;
  }

  if (reg.DeleteValue(kRegistryValueName) != ERROR_SUCCESS) {
    LOGW(L"Could not delete the registry value.");
    return false;
  }

  LOGW(L"Successfully deleted the registry value.");
  return true;
}

std::wstring GetExecutablePath() {
  return AddTrailingSlash(GetKnownFolderPath(FOLDERID_RoamingAppData)) +
         kExecutableFileName;
}

std::wstring GetProcessPath(DWORD process_id) {
  Handle process_handle{::OpenProcess(
      PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id)};

  if (!process_handle)
    return {};

  WCHAR buffer[MAX_PATH];
  DWORD buffer_size = MAX_PATH;

  if (!::QueryFullProcessImageName(process_handle.get(), 0,
                                   buffer, &buffer_size)) {
    return std::wstring();
  }

  return std::wstring(buffer, buffer_size);
}

bool TerminateProcess() {
  DWORD process_id = 0;

  HANDLE handle = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (handle != INVALID_HANDLE_VALUE) {
    PROCESSENTRY32 process_entry;
    process_entry.dwSize = sizeof(PROCESSENTRY32);
    if (::Process32First(handle, &process_entry)) {
      do {
        if (IsEqual(process_entry.szExeFile, kExecutableFileName) &&
            IsEqual(GetProcessPath(process_entry.th32ProcessID),
                    GetExecutablePath())) {
          process_id = process_entry.th32ProcessID;
          break;
        }
      } while (Process32Next(handle, &process_entry));
    }
    ::CloseHandle(handle);
  }

  if (!process_id) {
    LOGW(L"Could not detect running process.");
    return false;
  }

  Handle process_handle{::OpenProcess(PROCESS_TERMINATE, FALSE, process_id)};
  if (!::TerminateProcess(process_handle.get(), 0)) {
    LOGW(L"Could not terminate the process. PID: {}", process_handle.get());
    return false;
  }

  LOGW(L"Successfully terminated the process. PID: {}", process_handle.get());
  return true;
}

bool DeleteExecutableFile() {
  const auto path = GetExecutablePath();

  if (!FileExists(path)) {
    LOGW(L"Could not locate the executable file: " + path);
    return false;
  }

  if (!::DeleteFile(path.c_str())) {
    auto error_message = win::FormatError(::GetLastError());
    TrimRight(error_message, L"\r\n");
    LOGW(L"Could not delete the executable file. ({})", error_message);
    return false;
  }

  LOGW(L"Successfully deleted the executable file: " + path);
  return true;
}

void ApplyFix() {
  DeleteRegistryValue();
  if (TerminateProcess())
    ::Sleep(1000);
  DeleteExecutableFile();

  win::TaskDialog dlg;
  dlg.SetWindowTitle(L"CrunchyViewer Fix");
  dlg.SetMainIcon(TD_ICON_INFORMATION);
  dlg.SetMainInstruction(L"The fix was applied. Please check the log file for details.");
  dlg.AddButton(L"OK", IDOK);
  dlg.Show(nullptr);
}

}  // namespace crunchyfix
