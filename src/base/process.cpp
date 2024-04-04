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

#include <windows.h>

#include "base/process.h"

bool CheckInstance(LPCWSTR mutex_name, LPCWSTR class_name) {
  if (::CreateMutexW(NULL, FALSE, mutex_name) == NULL ||
      GetLastError() == ERROR_ALREADY_EXISTS ||
      GetLastError() == ERROR_ACCESS_DENIED) {
    HWND hwnd = FindWindow(class_name, NULL);

    if (::IsWindow(hwnd)) {
      ActivateWindow(hwnd);
      ::FlashWindow(hwnd, TRUE);
    }

    return true;
  }

  return false;
}

void ActivateWindow(HWND hwnd) {
  if (::IsIconic(hwnd)) {
    ::ShowWindow(hwnd, SW_RESTORE);
  }

  if (!::IsWindowVisible(hwnd)) {
    WINDOWPLACEMENT wp = {0};
    wp.length = sizeof(WINDOWPLACEMENT);
    ::GetWindowPlacement(hwnd, &wp);
    ::ShowWindow(hwnd, wp.showCmd);
  }

  ::SetForegroundWindow(hwnd);
  ::BringWindowToTop(hwnd);
}
