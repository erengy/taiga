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

#include "win_dialog.h"
#include "win_taskbar.h"

// =============================================================================

CDialog::CDialog() :
  m_bModal(true), m_iSnapGap(0)
{
  m_SizeLast.cx = 0; m_SizeLast.cy = 0;
  m_SizeMax.cx  = 0; m_SizeMax.cy  = 0;
  m_SizeMin.cx  = 0; m_SizeMin.cy  = 0;
  m_PosLast.x   = 0; m_PosLast.y   = 0;
}

CDialog::~CDialog() {
  EndDialog(0);
}

INT_PTR CDialog::Create(UINT uResourceID, HWND hParent, bool bModal) {
  m_bModal = bModal;
  if (bModal) {
    INT_PTR nResult = ::DialogBoxParam(m_hInstance, MAKEINTRESOURCE(uResourceID), 
      hParent, DialogProcStatic, reinterpret_cast<LPARAM>(this));
    m_hWindow = NULL;
    return nResult;
  } else {
    m_hWindow = ::CreateDialogParam(m_hInstance, MAKEINTRESOURCE(uResourceID), 
      hParent, DialogProcStatic, reinterpret_cast<LPARAM>(this));
    return reinterpret_cast<INT_PTR>(m_hWindow);
  }
}

void CDialog::EndDialog(INT_PTR nResult) {
  // Remember last position and size
  RECT rect; GetWindowRect(&rect);
  m_PosLast.x = rect.left;
  m_PosLast.y = rect.top;
  m_SizeLast.cx = rect.right - rect.left;
  m_SizeLast.cy = rect.bottom - rect.top;

  // Destroy window
  if (::IsWindow(m_hWindow)) {
    if (m_bModal) {
      ::EndDialog(m_hWindow, nResult);
    } else {
      Destroy();
    }
  }
  m_hWindow = NULL;
}

void CDialog::SetSizeMax(LONG cx, LONG cy) {
  m_SizeMax.cx = cx; m_SizeMax.cy = cy;
}

void CDialog::SetSizeMin(LONG cx, LONG cy) {
  m_SizeMin.cx = cx; m_SizeMin.cy = cy;
}

void CDialog::SetSnapGap(int iSnapGap) {
  m_iSnapGap = iSnapGap;
}

// =============================================================================

#include "string.h"

void CDialog::SetMinMaxInfo(LPMINMAXINFO lpMMI) {
  if (m_SizeMax.cx > 0) lpMMI->ptMaxTrackSize.x = m_SizeMax.cx;
  if (m_SizeMax.cy > 0) lpMMI->ptMaxTrackSize.y = m_SizeMax.cy;
  if (m_SizeMin.cx > 0) lpMMI->ptMinTrackSize.x = m_SizeMin.cx;
  if (m_SizeMin.cy > 0) lpMMI->ptMinTrackSize.y = m_SizeMin.cy;
}

void CDialog::SnapToEdges(LPWINDOWPOS lpWndPos) {
  if (m_iSnapGap == 0) return;

  HMONITOR hMonitor;
  MONITORINFO mi;
  RECT mon;

  // Get monitor info
  SystemParametersInfo(SPI_GETWORKAREA, NULL, &mon, NULL);
  if (GetSystemMetrics(SM_CMONITORS) > 1) {
    hMonitor = MonitorFromWindow(m_hWindow, MONITOR_DEFAULTTONEAREST);
    if (hMonitor) {
      mi.cbSize = sizeof(mi);
      GetMonitorInfo(hMonitor, &mi);
      mon = mi.rcWork;
    }
  }
  // Snap X axis
  if (abs(lpWndPos->x - mon.left) <= m_iSnapGap) {
    lpWndPos->x = mon.left;
  } else if (abs(lpWndPos->x + lpWndPos->cx - mon.right) <= m_iSnapGap) {
    lpWndPos->x = mon.right - lpWndPos->cx;
  }
  // Snap Y axis
  if (abs(lpWndPos->y - mon.top) <= m_iSnapGap) {
    lpWndPos->y = mon.top;
  } else if (abs(lpWndPos->y + lpWndPos->cy - mon.bottom) <= m_iSnapGap) {
    lpWndPos->y = mon.bottom - lpWndPos->cy;
  }
}

// =============================================================================

void CDialog::OnCancel() {
  EndDialog(IDCANCEL);
}

BOOL CDialog::OnClose() {
  return FALSE;
}

BOOL CDialog::OnInitDialog() {
  return TRUE;
}

void CDialog::OnOK() {
  EndDialog(IDOK);
}

// Superclassing
void CDialog::RegisterDlgClass(LPCWSTR lpszClassName) {
  WNDCLASS wc;
  ::GetClassInfo(m_hInstance, WC_DIALOG, &wc); // WC_DIALOG is #32770
  wc.lpszClassName = lpszClassName;
  ::RegisterClass(&wc);
}

// =============================================================================

BOOL CDialog::AddComboString(int nIDDlgItem, LPCWSTR lpString) {
  return ::SendDlgItemMessage(m_hWindow, nIDDlgItem, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(lpString));
}

BOOL CDialog::CheckDlgButton(int nIDButton, UINT uCheck) {
  return ::CheckDlgButton(m_hWindow, nIDButton, uCheck);
}

BOOL CDialog::CheckRadioButton(int nIDFirstButton, int nIDLastButton, int nIDCheckButton) {
  return ::CheckRadioButton(m_hWindow, nIDFirstButton, nIDLastButton, nIDCheckButton);
}

BOOL CDialog::EnableDlgItem(int nIDDlgItem, BOOL bEnable) {
  return ::EnableWindow(::GetDlgItem(m_hWindow, nIDDlgItem), bEnable);
}

INT CDialog::GetCheckedRadioButton(int nIDFirstButton, int nIDLastButton) {
  for (int i = 0; i <= nIDLastButton - nIDFirstButton; i++) {
    if (::IsDlgButtonChecked(m_hWindow, nIDFirstButton + i)) {
      return i;
    }
  }
  return 0;
}

INT CDialog::GetComboSelection(int nIDDlgItem) {
  return ::SendDlgItemMessage(m_hWindow, nIDDlgItem, CB_GETCURSEL, 0, 0);
}

HWND CDialog::GetDlgItem(int nIDDlgItem) {
  return ::GetDlgItem(m_hWindow, nIDDlgItem);
}

UINT CDialog::GetDlgItemInt(int nIDDlgItem) {
  return ::GetDlgItemInt(m_hWindow, nIDDlgItem, NULL, TRUE);
}

void CDialog::GetDlgItemText(int nIDDlgItem, LPWSTR lpString, int cchMax) {
  ::GetDlgItemText(m_hWindow, nIDDlgItem, lpString, cchMax);
}

void CDialog::GetDlgItemText(int nIDDlgItem, wstring& str) {
  int len = ::GetWindowTextLength(::GetDlgItem(m_hWindow, nIDDlgItem)) + 1;
  vector<wchar_t> buffer(len);
  ::GetDlgItemText(m_hWindow, nIDDlgItem, &buffer[0], len);
  str.assign(&buffer[0]);
}

BOOL CDialog::HideDlgItem(int nIDDlgItem) {
  return ::ShowWindow(::GetDlgItem(m_hWindow, nIDDlgItem), SW_HIDE);
}

BOOL CDialog::IsDlgButtonChecked(int nIDButton) {
  return ::IsDlgButtonChecked(m_hWindow, nIDButton);
}

BOOL CDialog::SendDlgItemMessage(int nIDDlgItem, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return ::SendDlgItemMessage(m_hWindow, nIDDlgItem, uMsg, wParam, lParam);
}

BOOL CDialog::SetComboSelection(int nIDDlgItem, int iIndex) {
  return ::SendDlgItemMessage(m_hWindow, nIDDlgItem, CB_SETCURSEL, iIndex, 0);
}

BOOL CDialog::SetDlgItemText(int nIDDlgItem, LPCWSTR lpString) {
  return ::SetDlgItemText(m_hWindow, nIDDlgItem, lpString);
}

BOOL CDialog::ShowDlgItem(int nIDDlgItem, int nCmdShow) {
  return ::ShowWindow(::GetDlgItem(m_hWindow, nIDDlgItem), nCmdShow);
}

// =============================================================================

INT_PTR CALLBACK CDialog::DialogProcStatic(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  CDialog* window = reinterpret_cast<CDialog*>(WindowMap.GetWindow(hwnd));
  if (!window && uMsg == WM_INITDIALOG) {
    window = reinterpret_cast<CDialog*>(lParam);
    if (window) {
      window->SetWindowHandle(hwnd);
      WindowMap.Add(hwnd, window);
    }
  }
  if (window) {
    return window->DialogProc(hwnd, uMsg, wParam, lParam);
  } else {
    return FALSE;
  }
}

INT_PTR CDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

INT_PTR CDialog::DialogProcDefault(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_CLOSE: {
      return OnClose();
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDOK:
          OnOK();
          return TRUE;
        case IDCANCEL:
          OnCancel();
          return TRUE;
        default:
          return OnCommand(wParam, lParam);
      }
      break;
    }
    case WM_DESTROY: {
      OnDestroy();
      break;
    }
    case WM_INITDIALOG: {
      /*if (m_PosLast.x && m_PosLast.y) {
        SetPosition(NULL, m_PosLast.x, m_PosLast.y, 0, 0, SWP_NOSIZE);
      }
      if (m_SizeLast.cx && m_SizeLast.cy) {
        SetPosition(NULL, 0, 0, m_SizeLast.cx, m_SizeLast.cy, SWP_NOMOVE);
      }*/
      return OnInitDialog();
    }
    case WM_DROPFILES: {
      OnDropFiles(reinterpret_cast<HDROP>(wParam));
      break;
    }
    case WM_ENTERSIZEMOVE:
    case WM_EXITSIZEMOVE: {
      SIZE size = {0};
      OnSize(uMsg, 0, size);
      break;
    }
    case WM_GETMINMAXINFO: {
      LPMINMAXINFO lpMMI = reinterpret_cast<LPMINMAXINFO>(lParam);
      SetMinMaxInfo(lpMMI);
      OnGetMinMaxInfo(lpMMI);
      break;
    }
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MOUSEACTIVATE:
    case WM_MOUSEHOVER:
    case WM_MOUSEHWHEEL:
    case WM_MOUSELEAVE:
    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL: {
      LRESULT lResult = OnMouseEvent(uMsg, wParam, lParam);
      if (lResult != -1) {
        ::SetWindowLongPtrW(hwnd, DWL_MSGRESULT, lResult);
        return TRUE;
      }
      break;
    }
    case WM_MOVE: {
      POINTS pts = MAKEPOINTS(lParam);
      OnMove(&pts);
      break;
    }
    case WM_NOTIFY: {
      LRESULT lResult = OnNotify(wParam, reinterpret_cast<LPNMHDR>(lParam));
      if (lResult) {
        ::SetWindowLongPtr(hwnd, DWL_MSGRESULT, lResult);
        return TRUE;
      }
      break;
    }
    case WM_PAINT: {
      HDC hdc;
      PAINTSTRUCT ps;
      hdc = ::BeginPaint(hwnd, &ps);
      OnPaint(hdc, &ps);
      ::EndPaint(hwnd, &ps);
      break;
    }
    case WM_SIZE: {
      SIZE size = {LOWORD(lParam), HIWORD(lParam)};
      OnSize(uMsg, static_cast<UINT>(wParam), size);
      break;
    }
    case WM_TIMER: {
      OnTimer(static_cast<UINT_PTR>(wParam));
      break;
    }
    case WM_WINDOWPOSCHANGING: {
      LPWINDOWPOS lpWndPos = reinterpret_cast<LPWINDOWPOS>(lParam);
      SnapToEdges(lpWndPos);
      OnWindowPosChanging(lpWndPos);
      break;
    }
    default: {
      if (uMsg == WM_TASKBARCREATED || 
          uMsg == WM_TASKBARBUTTONCREATED || 
          uMsg == WM_TASKBARCALLBACK) {
            OnTaskbarCallback(uMsg, lParam);
            return FALSE;
      }
      break;
    }
  }

  return FALSE;
}