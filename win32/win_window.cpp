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
#include "win_window.h"

static LPCWSTR PROP_CWINDOW = L"PROP_CWINDOW";

// =============================================================================

CWindow::CWindow() : 
  m_hFont(NULL), m_hIconLarge(NULL), m_hIconSmall(NULL),
  m_hMenu(NULL), m_hParent(NULL), m_hWindow(NULL), 
  m_hInstance(::GetModuleHandle(NULL))
{
  ::ZeroMemory(&m_CreateStruct, sizeof(CREATESTRUCT));
  ::ZeroMemory(&m_WndClass, sizeof(WNDCLASSEX));

  // Create default window class
  WNDCLASSEX wc = {0};
  if (!::GetClassInfoEx(m_hInstance, WIN_DEFAULT_CLASS, &wc)) {
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_DBLCLKS;
    wc.lpfnWndProc   = WindowProcStatic;
    wc.hInstance     = m_hInstance;
    wc.hCursor       = ::LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)::GetStockObject(WHITE_BRUSH);
    wc.lpszClassName = WIN_DEFAULT_CLASS;
    ::RegisterClassEx(&wc);
  }
}

CWindow::CWindow(HWND hWnd) : 
  m_hFont(NULL), m_hIconLarge(NULL), m_hIconSmall(NULL),
  m_hMenu(NULL), m_hParent(NULL), m_hWindow(NULL), 
  m_hInstance(::GetModuleHandle(NULL))
{
  m_hWindow = hWnd;
}

CWindow::~CWindow() {
  Destroy();
}

// =============================================================================

HWND CWindow::Create(HWND hWndParent) {
  PreRegisterClass(m_WndClass);
  if (m_WndClass.lpszClassName) {
    RegisterClass(m_WndClass);
    m_CreateStruct.lpszClass = m_WndClass.lpszClassName;
  }
  
  PreCreate(m_CreateStruct);
  if (!m_CreateStruct.lpszClass) {
    m_CreateStruct.lpszClass = WIN_DEFAULT_CLASS;
  }
  if (!hWndParent && m_CreateStruct.hwndParent) {
    hWndParent = m_CreateStruct.hwndParent;
  }
  DWORD dwStyle;
  if (m_CreateStruct.style) {
    dwStyle = m_CreateStruct.style;
  } else {
    dwStyle = WS_VISIBLE | (hWndParent ? WS_CHILD : WS_OVERLAPPEDWINDOW);
  }

  int x  = (m_CreateStruct.cx || m_CreateStruct.cy) ? m_CreateStruct.x  : CW_USEDEFAULT;
  int cx = (m_CreateStruct.cx || m_CreateStruct.cy) ? m_CreateStruct.cx : CW_USEDEFAULT;
  int y  = (m_CreateStruct.cx || m_CreateStruct.cy) ? m_CreateStruct.y  : CW_USEDEFAULT;
  int cy = (m_CreateStruct.cx || m_CreateStruct.cy) ? m_CreateStruct.cy : CW_USEDEFAULT;

  return Create(m_CreateStruct.dwExStyle, 
    m_CreateStruct.lpszClass, 
    m_CreateStruct.lpszName, 
    dwStyle, x, y, cx, cy, hWndParent, 
    m_CreateStruct.hMenu, 
    m_CreateStruct.lpCreateParams);
}

HWND CWindow::Create(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, 
                     int X, int Y, int nWidth, int nHeight, 
                     HWND hWndParent, HMENU hMenu, LPVOID lpParam) {
  Destroy();
  
  m_hMenu = hMenu;
  m_hParent = hWndParent;
  m_hWindow = ::CreateWindowEx(dwExStyle, lpClassName, lpWindowName, dwStyle, 
    X, Y, nWidth, nHeight, 
    hWndParent, hMenu, m_hInstance, lpParam);

  WNDCLASSEX wc = {0};
  ::GetClassInfoEx(m_hInstance, lpClassName, &wc);
  if (wc.lpfnWndProc != reinterpret_cast<WNDPROC>(WindowProcStatic)) {
    Subclass(m_hWindow);
    OnCreate(m_hWindow, &m_CreateStruct);
  }

  return m_hWindow;
}

void CWindow::Destroy() {
  if (::IsWindow(m_hWindow)) {
    ::DestroyWindow(m_hWindow);
  }
  if (m_hFont && m_hParent) {
    ::DeleteObject(m_hFont);
    m_hFont = NULL;
  }
  if (m_hIconLarge) {
    ::DestroyIcon(m_hIconLarge);
    m_hIconLarge = NULL;
  }
  if (m_hIconSmall) {
    ::DestroyIcon(m_hIconSmall);
    m_hIconSmall = NULL;
  }
  if (m_PrevWindowProc) {
    UnSubclass();
    m_PrevWindowProc = NULL;
  }
  WindowMap.Remove(this);
  m_hWindow = NULL;
}

void CWindow::PreCreate(CREATESTRUCT& cs) {
  m_CreateStruct.cx             = cs.cx;
  m_CreateStruct.cy             = cs.cy;
  m_CreateStruct.dwExStyle      = cs.dwExStyle;
  m_CreateStruct.hInstance      = m_hInstance;
  m_CreateStruct.hMenu          = cs.hMenu;
  m_CreateStruct.hwndParent     = cs.hwndParent;
  m_CreateStruct.lpCreateParams = cs.lpCreateParams;
  m_CreateStruct.lpszClass      = cs.lpszClass;
  m_CreateStruct.lpszName       = cs.lpszName;
  m_CreateStruct.style          = cs.style;
  m_CreateStruct.x              = cs.x;
  m_CreateStruct.y              = cs.y;
}

void CWindow::PreRegisterClass(WNDCLASSEX& wc) {
  m_WndClass.style         = wc.style;
  m_WndClass.lpfnWndProc   = WindowProcStatic;
  m_WndClass.cbClsExtra    = wc.cbClsExtra;
  m_WndClass.cbWndExtra    = wc.cbWndExtra;
  m_WndClass.hInstance     = m_hInstance;
  m_WndClass.hIcon         = wc.hIcon;
  m_WndClass.hCursor       = wc.hCursor;
  m_WndClass.hbrBackground = wc.hbrBackground;
  m_WndClass.lpszMenuName  = wc.lpszMenuName;
  m_WndClass.lpszClassName = wc.lpszClassName;
}

BOOL CWindow::RegisterClass(WNDCLASSEX& wc) {
  WNDCLASSEX wc_old = {0};
  if (::GetClassInfoEx(m_hInstance, wc.lpszClassName, &wc_old)) {
    wc = wc_old;
    return TRUE;
  }

  wc.cbSize = sizeof(wc);
  wc.hInstance = m_hInstance;
  wc.lpfnWndProc = WindowProcStatic;

  return ::RegisterClassEx(&wc);
}

// =============================================================================

void CWindow::Attach(HWND hWindow) {
  Detach();
  if (::IsWindow(hWindow)) {
    if (WindowMap.GetWindow(hWindow) == NULL) {
      WindowMap.Add(hWindow, this);
      Subclass(hWindow);
    }
  }
}

void CWindow::CenterOwner() {
  GetParent();
  RECT rcDesktop, rcParent, rcWindow;
  ::GetWindowRect(m_hWindow, &rcWindow);
  ::SystemParametersInfo(SPI_GETWORKAREA, 0, &rcDesktop, 0);
  if (m_hParent != NULL) {
    ::GetWindowRect(m_hParent, &rcParent);
  } else {
    ::CopyRect(&rcParent, &rcDesktop);
  }

  HMONITOR hMonitor = MonitorFromWindow(m_hWindow, MONITOR_DEFAULTTONEAREST);
  MONITORINFO mi = { sizeof(mi), 0 };
  if (GetMonitorInfo(hMonitor, &mi)) {
    rcDesktop = mi.rcWork;
    if (m_hParent == NULL) rcParent = mi.rcWork;
  }

  ::IntersectRect(&rcParent, &rcParent, &rcDesktop);
  ::SetWindowPos(m_hWindow, HWND_TOP, 
    rcParent.left + ((rcParent.right - rcParent.left) - (rcWindow.right - rcWindow.left)) / 2, 
    rcParent.top + ((rcParent.bottom - rcParent.top) - (rcWindow.bottom - rcWindow.top)) / 2, 
    0, 0, SWP_NOSIZE);
}

HWND CWindow::Detach() {
  HWND hWnd = m_hWindow;
  if (m_PrevWindowProc) UnSubclass();
  WindowMap.Remove(this);
  m_hWindow = NULL;
  return hWnd;
}

HICON CWindow::SetIconLarge(HICON hIcon) {
  if (m_hIconLarge) ::DestroyIcon(m_hIconLarge);
  m_hIconLarge = hIcon;
  if (!m_hIconLarge) return NULL;
  return (HICON)::SendMessage(m_hWindow, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)m_hIconLarge);
}

HICON CWindow::SetIconLarge(int nIcon) {
  return SetIconLarge(reinterpret_cast<HICON>(LoadImage(m_hInstance, MAKEINTRESOURCE(nIcon), IMAGE_ICON, 
    ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR)));
}

HICON CWindow::SetIconSmall(HICON hIcon) {
  if (m_hIconSmall) ::DestroyIcon(m_hIconSmall);
  m_hIconSmall = hIcon;
  if (!m_hIconSmall) return NULL;
  return (HICON)::SendMessage(m_hWindow, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)m_hIconSmall);
}

HICON CWindow::SetIconSmall(int nIcon) {
  return SetIconSmall(reinterpret_cast<HICON>(LoadImage(m_hInstance, MAKEINTRESOURCE(nIcon), IMAGE_ICON, 
    ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR)));
}

// =============================================================================

// Win32 API wrappers

BOOL CWindow::BringWindowToTop() const {
  return ::BringWindowToTop(m_hWindow);
}

BOOL CWindow::Close() const {
  return ::CloseWindow(m_hWindow);
}

BOOL CWindow::Enable(BOOL bEnable) const {
  return ::EnableWindow(m_hWindow, bEnable);
}

BOOL CWindow::GetClientRect(LPRECT lpRect) const {
  return ::GetClientRect(m_hWindow, lpRect);
}

HDC CWindow::GetDC() const {
  return ::GetDC(m_hWindow);
}

HFONT CWindow::GetFont() {
  return reinterpret_cast<HFONT>(::SendMessage(m_hWindow, WM_GETFONT, 0, 0));
}

HMENU CWindow::GetMenu() const {
  return ::GetMenu(m_hWindow);
}

HWND CWindow::GetParent() {
  m_hParent = ::GetParent(m_hWindow);
  if (!m_hParent) m_hParent = ::GetDesktopWindow();
  return m_hParent;
}

void CWindow::GetText(LPWSTR lpszString, int nMaxCount) const {
  ::GetWindowText(m_hWindow, lpszString, nMaxCount);
}

void CWindow::GetText(wstring& str) const {
  int len = ::GetWindowTextLength(m_hWindow) + 1;
  vector<wchar_t> buffer(len);
  ::GetWindowText(m_hWindow, &buffer[0], len);
  str.assign(&buffer[0]);
}

INT CWindow::GetTextLength() const {
  return ::GetWindowTextLength(m_hWindow);
}

DWORD CWindow::GetWindowLong(int nIndex) const {
  return ::GetWindowLong(m_hWindow, nIndex);
}

BOOL CWindow::GetWindowRect(LPRECT lpRect) const {
  return ::GetWindowRect(m_hWindow, lpRect);
}

BOOL CWindow::Hide() const {
  return ::ShowWindow(m_hWindow, SW_HIDE);
}

BOOL CWindow::InvalidateRect(LPCRECT lpRect, BOOL bErase) const {
  return ::InvalidateRect(m_hWindow, lpRect, bErase);
}

BOOL CWindow::IsEnabled() const {
  return ::IsWindowEnabled(m_hWindow);
}

BOOL CWindow::IsIconic() const {
  return ::IsIconic(m_hWindow);
}

BOOL CWindow::IsVisible() const {
  return ::IsWindowVisible(m_hWindow);
}

BOOL CWindow::IsWindow() const {
  return ::IsWindow(m_hWindow);
}

INT CWindow::MessageBox(LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) const {
  return ::MessageBox(m_hWindow, lpText, lpCaption, uType);
}

LRESULT CWindow::PostMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return ::PostMessage(m_hWindow, uMsg, wParam, lParam);
}

BOOL CWindow::RedrawWindow(LPCRECT lprcUpdate, HRGN hrgnUpdate, UINT flags) const {
  return ::RedrawWindow(m_hWindow, lprcUpdate, hrgnUpdate, flags);
}

LRESULT CWindow::SendMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return ::SendMessage(m_hWindow, uMsg, wParam, lParam);
}

BOOL CWindow::SetBorder(int iStyle) {
  switch (iStyle) {
    case WIN_BORDER_NONE:
      SetStyle(0, WS_EX_CLIENTEDGE | WS_EX_STATICEDGE, GWL_EXSTYLE);
      break;
    case WIN_BORDER_CLIENT:
      SetStyle(WS_EX_CLIENTEDGE, WS_EX_STATICEDGE, GWL_EXSTYLE);
      break;
    case WIN_BORDER_STATIC:
      SetStyle(WS_EX_STATICEDGE, WS_EX_CLIENTEDGE, GWL_EXSTYLE);
      break;
  }
  return ::SetWindowPos(m_hWindow, NULL, 0, 0, 0, 0, 
    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

HWND CWindow::SetCapture() const {
  return ::SetCapture(m_hWindow);
}

DWORD CWindow::SetClassLong(int nIndex, LONG dwNewLong) const {
  return ::SetClassLong(m_hWindow, nIndex, dwNewLong);
}

HWND CWindow::SetFocus() const {
  return ::SetFocus(m_hWindow);
}

void CWindow::SetFont(LPCWSTR lpFaceName, int iSize, bool bBold, bool bItalic, bool bUnderline) {
  HDC hDC = ::GetDC(m_hWindow);
  HFONT hFontOld = reinterpret_cast<HFONT>(::GetCurrentObject(hDC, OBJ_FONT));
  LOGFONT lFont; ::GetObject(hFontOld, sizeof(LOGFONT), &lFont);
  
  if (lpFaceName) {
    ::lstrcpy(lFont.lfFaceName, lpFaceName);
  }
  if (iSize) {
    lFont.lfHeight = -MulDiv(iSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
    lFont.lfWidth = 0;
  }
  lFont.lfItalic = bItalic;
  lFont.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
  lFont.lfUnderline = bUnderline;

  if (m_hFont) ::DeleteObject(m_hFont);
  m_hFont = ::CreateFontIndirect(&lFont);
  ::SendMessage(m_hWindow, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFont), TRUE);
  
  ::DeleteObject(hFontOld);
  ::ReleaseDC(m_hWindow, hDC);
}

void CWindow::SetFont(HFONT hFont) {
  if (m_hFont) ::DeleteObject(m_hFont); m_hFont = hFont;
  ::SendMessage(m_hWindow, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFont), TRUE);
}

BOOL CWindow::SetForegroundWindow() const {
  return ::SetForegroundWindow(m_hWindow);
}

BOOL CWindow::SetMenu(HMENU hMenu) const {
  return ::SetMenu(m_hWindow, hMenu);
}

void CWindow::SetParent(HWND hParent) {
  if (hParent == NULL) {
    ::SetParent(m_hWindow, hParent);
    SetStyle(WS_POPUP, WS_CHILD);
  } else {
    SetStyle(WS_CHILD, WS_POPUP);
    ::SetParent(m_hWindow, hParent);
  }
}

BOOL CWindow::SetPosition(HWND hInsertAfter, int x, int y, int w, int h, UINT uFlags) const {
  return ::SetWindowPos(m_hWindow, hInsertAfter, x, y, w, h, uFlags);
}

BOOL CWindow::SetPosition(HWND hInsertAfter, const RECT& rc, UINT uFlags) const {
  return ::SetWindowPos(m_hWindow, hInsertAfter, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, uFlags);
}

BOOL CWindow::SetRedraw(BOOL bRedraw) const {
  return ::SendMessage(m_hWindow, WM_SETREDRAW, bRedraw, 0);
}

void CWindow::SetStyle(UINT style, UINT style_not, int nIndex) {
  UINT old_style = ::GetWindowLong(m_hWindow, nIndex);
  ::SetWindowLongPtr(m_hWindow, nIndex, (old_style | style) & ~style_not);
}

BOOL CWindow::SetText(LPCWSTR lpszString) const {
  return ::SendMessage(m_hWindow, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(lpszString));
}

BOOL CWindow::SetText(const std::wstring& str) const {
  return SetText(str.c_str());
}

HRESULT CWindow::SetTheme(LPCWSTR pszName) const {
  return ::SetWindowTheme(m_hWindow, pszName, NULL);
}

BOOL CWindow::SetTransparency(BYTE alpha, COLORREF color) {
  if (alpha == 255) {
    SetStyle(0, WS_EX_LAYERED, GWL_EXSTYLE);
  } else {
    SetStyle(WS_EX_LAYERED, 0, GWL_EXSTYLE);
  }
  
  DWORD flags;
  if (color & 0xFF000000) {
    color = RGB(255, 255, 255);
    flags = LWA_ALPHA;
  } else {
    flags = LWA_COLORKEY;
  }

  return ::SetLayeredWindowAttributes(m_hWindow, color, alpha, flags);
}

BOOL CWindow::Show(int nCmdShow) const {
  return ::ShowWindow(m_hWindow, nCmdShow);
}

BOOL CWindow::Update() const {
  return ::UpdateWindow(m_hWindow);
}

// =============================================================================

void CWindow::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {
  LOGFONT lFont;
  ::GetObject(::GetStockObject(DEFAULT_GUI_FONT), sizeof(lFont), &lFont);
  m_hFont = ::CreateFontIndirect(&lFont);
  ::SendMessage(m_hWindow, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFont), FALSE);
}

// =============================================================================

void CWindow::Subclass(HWND hWnd) {
  WNDPROC current_proc = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(hWnd, GWLP_WNDPROC));
  if (current_proc != reinterpret_cast<WNDPROC>(WindowProcStatic)) {
    m_PrevWindowProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hWnd, GWLP_WNDPROC, 
      reinterpret_cast<LONG_PTR>(WindowProcStatic)));
    ::SetProp(hWnd, PROP_CWINDOW, this);
    m_hWindow = hWnd;
  }
}

void CWindow::UnSubclass() {
  WNDPROC current_proc = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(m_hWindow, GWLP_WNDPROC));
  if (current_proc == reinterpret_cast<WNDPROC>(WindowProcStatic)) {
    ::SetWindowLongPtr(m_hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_PrevWindowProc));
    ::RemoveProp(m_hWindow, PROP_CWINDOW);
    m_PrevWindowProc = NULL;
  }
}

LRESULT CALLBACK CWindow::WindowProcStatic(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  CWindow* window = WindowMap.GetWindow(hwnd);
  if (!window) {
    window = reinterpret_cast<CWindow*>(::GetProp(hwnd, PROP_CWINDOW));
    if (window) {
      window->SetWindowHandle(hwnd);
      WindowMap.Add(hwnd, window);
    }
  }
  if (window) {
    return window->WindowProc(hwnd, uMsg, wParam, lParam);
  } else {
    return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
  }
}

LRESULT CWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return WindowProcDefault(hwnd, uMsg, wParam, lParam);
}

LRESULT CWindow::WindowProcDefault(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_COMMAND: {
      if (OnCommand(wParam, lParam)) return 0;
      break;
    }
    case WM_CREATE: {
      OnCreate(hwnd, reinterpret_cast<LPCREATESTRUCT>(lParam));
      break;
    }
    case WM_DESTROY: {
      if (OnDestroy()) return 0;
      break;
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
      OnGetMinMaxInfo(reinterpret_cast<LPMINMAXINFO>(lParam));
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
      if (lResult != -1) return lResult;
      break;
    }
    case WM_MOVE: {
      POINTS pts = MAKEPOINTS(lParam);
      OnMove(&pts);
      break;
    }
    case WM_NOTIFY: {
      LRESULT lResult = OnNotify(static_cast<int>(wParam), reinterpret_cast<LPNMHDR>(lParam));
      if (lResult) return lResult;
      break;
    }
    case WM_PAINT: {
      if (!m_PrevWindowProc) {
        if (::GetUpdateRect(hwnd, NULL, FALSE)) {
          PAINTSTRUCT ps;
          HDC hdc = ::BeginPaint(hwnd, &ps);
          OnPaint(hdc, &ps);
          ::EndPaint(hwnd, &ps);
        } else {
          HDC hdc = ::GetDC(hwnd);
          OnPaint(hdc, NULL);
          ::ReleaseDC(hwnd, hdc);
        }
      }
      break;
    }
    case WM_SIZE: {
      SIZE size = {LOWORD(lParam), HIWORD(lParam)};
      OnSize(uMsg, static_cast<UINT>(wParam), size);
      break;
    }
    case WM_TIMER: {
      OnTimer(static_cast<UINT>(wParam));
      break;
    }
    case WM_WINDOWPOSCHANGING: {
      OnWindowPosChanging(reinterpret_cast<LPWINDOWPOS>(lParam));
      break;
    }
    default: {
      if (uMsg == WM_TASKBARCREATED || 
          uMsg == WM_TASKBARBUTTONCREATED || 
          uMsg == WM_TASKBARCALLBACK) {
            OnTaskbarCallback(uMsg, lParam);
            return 0;
      }
      break;
    }
  }

  if (m_PrevWindowProc) {
    return ::CallWindowProc(m_PrevWindowProc, hwnd, uMsg, wParam, lParam);
  } else {
    return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
  }
}