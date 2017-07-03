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

#include "process.h"

bool CheckInstance(LPCWSTR mutex_name, LPCWSTR class_name) {
  if (CreateMutex(NULL, FALSE, mutex_name) == NULL ||
      GetLastError() == ERROR_ALREADY_EXISTS ||
      GetLastError() == ERROR_ACCESS_DENIED) {
    HWND hwnd = FindWindow(class_name, NULL);

    if (IsWindow(hwnd)) {
      ActivateWindow(hwnd);
      FlashWindow(hwnd, TRUE);
    }

    return true;
  }

  return false;
}

void ActivateWindow(HWND hwnd) {
  if (IsIconic(hwnd))
    SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, NULL);

  if (!IsWindowVisible(hwnd))
    ShowWindow(hwnd, SW_SHOWNORMAL);

  SetForegroundWindow(hwnd);
  BringWindowToTop(hwnd);
}

std::wstring GetWindowClass(HWND hwnd) {
  WCHAR buff[MAX_PATH];
  GetClassName(hwnd, buff, MAX_PATH);
  return buff;
}

std::wstring GetWindowTitle(HWND hwnd) {
  WCHAR buff[MAX_PATH];
  GetWindowText(hwnd, buff, MAX_PATH);
  return buff;
}

bool IsFullscreen(HWND hwnd) {
  LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);

  if (style & WS_EX_TOPMOST) {
    RECT rect;
    GetClientRect(hwnd, &rect);

    if (rect.right >= GetSystemMetrics(SM_CXSCREEN) &&
        rect.bottom >= GetSystemMetrics(SM_CYSCREEN))
      return true;
  }

  return false;
}
