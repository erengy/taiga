/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

#include "win_main.h"
#include "win_taskbar.h"

class win::Taskbar Taskbar;
class win::TaskbarList TaskbarList;

static const DWORD WM_TASKBARCALLBACK = WM_APP + 0x15;
static const DWORD WM_TASKBARCREATED = ::RegisterWindowMessage(L"TaskbarCreated");
static const DWORD WM_TASKBARBUTTONCREATED = ::RegisterWindowMessage(L"TaskbarButtonCreated");

namespace win {

#define APP_SYSTRAY_ID 74164 // TAIGA ^_^

// =============================================================================

Taskbar::Taskbar() :
  m_hApp(NULL)
{
  WinVersion win_version = GetWinVersion();
  if (win_version >= VERSION_VISTA) {
    m_NID.cbSize = sizeof(NOTIFYICONDATA);
  } else if (win_version >= VERSION_XP) {
    m_NID.cbSize = NOTIFYICONDATA_V3_SIZE;
  } else {
    m_NID.cbSize = NOTIFYICONDATA_V2_SIZE;
  }
}

Taskbar::~Taskbar() {
  Destroy();
}

BOOL Taskbar::Create(HWND hwnd, HICON hIcon, LPCWSTR lpTip) {
  Destroy();
  m_hApp = hwnd;

  m_NID.hIcon = hIcon;
  m_NID.hWnd = hwnd;
  m_NID.szTip[0] = (WCHAR)'\0';
  m_NID.uCallbackMessage = WM_TASKBARCALLBACK;
  m_NID.uID = APP_SYSTRAY_ID;
  m_NID.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;

  if (hIcon == NULL) {
    m_NID.hIcon = reinterpret_cast<HICON>(LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(101), 
      IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
  }
  if (lpTip) {
    wcscpy_s(m_NID.szTip, lpTip);
  }

  return ::Shell_NotifyIcon(NIM_ADD, &m_NID);
}

BOOL Taskbar::Destroy() {
  if (!m_hApp) return FALSE;
  return ::Shell_NotifyIcon(NIM_DELETE, &m_NID);
}

BOOL Taskbar::Modify(LPCWSTR lpTip) {
  if (!m_hApp) return FALSE;

  m_NID.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  wcsncpy_s(m_NID.szTip, 128, lpTip, _TRUNCATE);

  return ::Shell_NotifyIcon(NIM_MODIFY, &m_NID);
}

BOOL Taskbar::Tip(LPCWSTR lpText, LPCWSTR lpTitle, int iIconIndex) {
  if (!m_hApp) return FALSE;

  m_NID.uFlags = NIF_INFO;
  m_NID.dwInfoFlags = iIconIndex;
  wcsncpy_s(m_NID.szInfo, 256, lpText, _TRUNCATE);
  wcsncpy_s(m_NID.szInfoTitle, 64, lpTitle, _TRUNCATE);

  return ::Shell_NotifyIcon(NIM_MODIFY, &m_NID);
}

// =============================================================================

TaskbarList::TaskbarList() : 
  m_hWnd(NULL), m_pTaskbarList(NULL)
{
}

TaskbarList::~TaskbarList() {
  Release();
}

void TaskbarList::Initialize(HWND hwnd) {
  Release();
  m_hWnd = hwnd;
  ::CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, 
    __uuidof(ITaskbarList3), (void**)&m_pTaskbarList);
}

void TaskbarList::Release() {
  if (m_pTaskbarList) {
    m_pTaskbarList->Release();
    m_pTaskbarList = NULL;
    m_hWnd = NULL;
  }
}

void TaskbarList::SetProgressState(TBPFLAG flag) {
  if (m_pTaskbarList) {
    m_pTaskbarList->SetProgressState(m_hWnd, flag);
  }
}

void TaskbarList::SetProgressValue(ULONGLONG ullValue, ULONGLONG ullTotal) {
  if (m_pTaskbarList) {
    m_pTaskbarList->SetProgressValue(m_hWnd, ullValue, ullTotal);
  }
}

}  // namespace win