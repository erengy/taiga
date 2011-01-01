/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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

static const DWORD WM_TASKBARCALLBACK = WM_APP + 0x15;
static const DWORD WM_TASKBARCREATED = ::RegisterWindowMessage(L"TaskbarCreated");
static const DWORD WM_TASKBARBUTTONCREATED = ::RegisterWindowMessage(L"TaskbarButtonCreated");

#define APP_SYSTRAY_ID 74164 // TAIGA ^_^

CTaskbar Taskbar;
CTaskbarList TaskbarList;

// =============================================================================

CTaskbar::CTaskbar() :
  m_hApp(NULL)
{
  WinVersion win_version = GetWinVersion();
  if (win_version >= WINVERSION_VISTA) {
    m_NID.cbSize = sizeof(NOTIFYICONDATA);
  } else if (win_version >= WINVERSION_XP) {
    m_NID.cbSize = NOTIFYICONDATA_V3_SIZE;
  } else {
    m_NID.cbSize = NOTIFYICONDATA_V2_SIZE;
  }
}

CTaskbar::~CTaskbar() {
  Destroy();
}

BOOL CTaskbar::Create(HWND hwnd, HICON hIcon, LPCWSTR lpTip) {
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

BOOL CTaskbar::Destroy() {
  if (!m_hApp) return FALSE;
  return ::Shell_NotifyIcon(NIM_DELETE, &m_NID);
}

BOOL CTaskbar::Modify(LPCWSTR lpTip) {
  if (!m_hApp) return FALSE;

  m_NID.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  wcscpy_s(m_NID.szTip, lpTip);

  return ::Shell_NotifyIcon(NIM_MODIFY, &m_NID);
}

BOOL CTaskbar::Tip(LPCWSTR lpText, LPCWSTR lpTitle, int iIconIndex) {
  if (!m_hApp) return FALSE;

  m_NID.uFlags = NIF_INFO;
  m_NID.dwInfoFlags = iIconIndex;
  wcsncpy_s(m_NID.szInfo, 256, lpText, _TRUNCATE);
  wcsncpy_s(m_NID.szInfoTitle, 64, lpTitle, _TRUNCATE);

  return ::Shell_NotifyIcon(NIM_MODIFY, &m_NID);
}

// =============================================================================

CTaskbarList::CTaskbarList() : 
  m_hWnd(NULL), m_pTaskbarList(NULL)
{
}

CTaskbarList::~CTaskbarList() {
  Release();
}

void CTaskbarList::Initialize(HWND hwnd) {
  Release();
  m_hWnd = hwnd;
  ::CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, 
    __uuidof(ITaskbarList3), (void**)&m_pTaskbarList);
}

void CTaskbarList::Release() {
  if (m_pTaskbarList) {
    m_pTaskbarList->Release();
    m_pTaskbarList = NULL;
    m_hWnd = NULL;
  }
}

void CTaskbarList::SetProgressState(TBPFLAG flag) {
  if (m_pTaskbarList) {
    m_pTaskbarList->SetProgressState(m_hWnd, flag);
  }
}

void CTaskbarList::SetProgressValue(ULONGLONG ullValue, ULONGLONG ullTotal) {
  if (m_pTaskbarList) {
    m_pTaskbarList->SetProgressValue(m_hWnd, ullValue, ullTotal);
  }
}