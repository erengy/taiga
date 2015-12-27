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

#include "win_main.h"
#include "win_taskbar.h"
#include "win_window.h"

namespace win {

const int kControlMargin = 6;
const std::wstring kDefaultClassName = L"TaigaDefaultW";

Window* Window::current_window_ = nullptr;

Window::Window()
    : instance_(::GetModuleHandle(nullptr)),
      font_(nullptr), icon_large_(nullptr), icon_small_(nullptr),
      menu_(nullptr), parent_(nullptr), window_(nullptr) {
  current_window_ = nullptr;

  ::ZeroMemory(&create_struct_, sizeof(CREATESTRUCT));
  ::ZeroMemory(&window_class_, sizeof(WNDCLASSEX));

  // Create default window class
  WNDCLASSEX wc = {0};
  if (!::GetClassInfoEx(instance_, kDefaultClassName.c_str(), &wc)) {
    wc.cbSize = sizeof(wc);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = WindowProcStatic;
    wc.hInstance = instance_;
    wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH));
    wc.lpszClassName = kDefaultClassName.c_str();
    ::RegisterClassEx(&wc);
  }
}

Window::Window(HWND hwnd)
    : instance_(::GetModuleHandle(nullptr)),
      font_(nullptr), icon_large_(nullptr), icon_small_(nullptr),
      menu_(nullptr), parent_(nullptr), window_(nullptr) {
  current_window_ = nullptr;
  window_ = hwnd;
}

Window::~Window() {
  Destroy();
}

////////////////////////////////////////////////////////////////////////////////

HWND Window::Create(HWND parent) {
  PreRegisterClass(window_class_);
  if (window_class_.lpszClassName) {
    RegisterClass(window_class_);
    create_struct_.lpszClass = window_class_.lpszClassName;
  }

  PreCreate(create_struct_);
  if (!create_struct_.lpszClass) {
    create_struct_.lpszClass = kDefaultClassName.c_str();
  }
  if (!parent && create_struct_.hwndParent) {
    parent = create_struct_.hwndParent;
  }
  DWORD style;
  if (create_struct_.style) {
    style = create_struct_.style;
  } else {
    style = WS_VISIBLE | (parent ? WS_CHILD : WS_OVERLAPPEDWINDOW);
  }

  bool cx_or_cy = create_struct_.cx || create_struct_.cy;
  int x = cx_or_cy ? create_struct_.x : CW_USEDEFAULT;
  int y = cx_or_cy ? create_struct_.y : CW_USEDEFAULT;
  int cx = cx_or_cy ? create_struct_.cx : CW_USEDEFAULT;
  int cy = cx_or_cy ? create_struct_.cy : CW_USEDEFAULT;

  return Create(create_struct_.dwExStyle,
                create_struct_.lpszClass,
                create_struct_.lpszName,
                style,
                x, y, cx, cy,
                parent,
                create_struct_.hMenu,
                create_struct_.lpCreateParams);
}

HWND Window::Create(DWORD ex_style, LPCWSTR class_name, LPCWSTR window_name,
                    DWORD style, int x, int y, int width, int height,
                    HWND parent, HMENU menu, LPVOID param) {
  Destroy();

  current_window_ = this;

  menu_ = menu;
  parent_ = parent;

  window_ = ::CreateWindowEx(ex_style, class_name, window_name,
                             style, x, y, width, height,
                             parent, menu, instance_, param);

  WNDCLASSEX wc = {0};
  ::GetClassInfoEx(instance_, class_name, &wc);
  if (wc.lpfnWndProc != reinterpret_cast<WNDPROC>(WindowProcStatic)) {
    Subclass(window_);
    OnCreate(window_, &create_struct_);
  }

  current_window_ = nullptr;

  return window_;
}

void Window::Destroy() {
  if (::IsWindow(window_))
    ::DestroyWindow(window_);

  if (font_ && parent_) {
    ::DeleteObject(font_);
    font_ = nullptr;
  }
  if (icon_large_) {
    ::DestroyIcon(icon_large_);
    icon_large_ = nullptr;
  }
  if (icon_small_) {
    ::DestroyIcon(icon_small_);
    icon_small_ = nullptr;
  }

  if (prev_window_proc_) {
    UnSubclass();
    prev_window_proc_ = nullptr;
  }

  WindowMap.Remove(this);
  window_ = nullptr;
}

void Window::PreCreate(CREATESTRUCT& cs) {
  create_struct_.cx = cs.cx;
  create_struct_.cy = cs.cy;
  create_struct_.dwExStyle = cs.dwExStyle;
  create_struct_.hInstance = instance_;
  create_struct_.hMenu = cs.hMenu;
  create_struct_.hwndParent = cs.hwndParent;
  create_struct_.lpCreateParams = cs.lpCreateParams;
  create_struct_.lpszClass = cs.lpszClass;
  create_struct_.lpszName = cs.lpszName;
  create_struct_.style = cs.style;
  create_struct_.x = cs.x;
  create_struct_.y = cs.y;
}

void Window::PreRegisterClass(WNDCLASSEX& wc) {
  window_class_.style = wc.style;
  window_class_.lpfnWndProc = WindowProcStatic;
  window_class_.cbClsExtra = wc.cbClsExtra;
  window_class_.cbWndExtra = wc.cbWndExtra;
  window_class_.hInstance = instance_;
  window_class_.hIcon = wc.hIcon;
  window_class_.hCursor = wc.hCursor;
  window_class_.hbrBackground = wc.hbrBackground;
  window_class_.lpszMenuName = wc.lpszMenuName;
  window_class_.lpszClassName = wc.lpszClassName;
}

BOOL Window::PreTranslateMessage(MSG* msg) {
  return FALSE;
}

BOOL Window::RegisterClass(WNDCLASSEX& wc) const {
  WNDCLASSEX wc_existing = {0};
  if (::GetClassInfoEx(instance_, wc.lpszClassName, &wc_existing)) {
    wc = wc_existing;
    return TRUE;
  }

  wc.cbSize = sizeof(wc);
  wc.hInstance = instance_;
  wc.lpfnWndProc = WindowProcStatic;

  return ::RegisterClassEx(&wc);
}

////////////////////////////////////////////////////////////////////////////////

void Window::Attach(HWND hwnd) {
  Detach();

  if (::IsWindow(hwnd)) {
    if (!WindowMap.GetWindow(hwnd)) {
      WindowMap.Add(hwnd, this);
      Subclass(hwnd);
    }
  }
}

void Window::CenterOwner() {
  GetParent();

  RECT rc_desktop, rc_parent, rc_window;
  ::GetWindowRect(window_, &rc_window);
  ::SystemParametersInfo(SPI_GETWORKAREA, 0, &rc_desktop, 0);
  if (parent_) {
    ::GetWindowRect(parent_, &rc_parent);
  } else {
    ::CopyRect(&rc_parent, &rc_desktop);
  }

  HMONITOR monitor = ::MonitorFromWindow(window_, MONITOR_DEFAULTTONEAREST);
  MONITORINFO mi = { sizeof(mi), 0 };
  if (::GetMonitorInfo(monitor, &mi)) {
    rc_desktop = mi.rcWork;
    if (!parent_)
      rc_parent = mi.rcWork;
  }

  ::IntersectRect(&rc_parent, &rc_parent, &rc_desktop);

  int parent_width = rc_parent.right - rc_parent.left;
  int parent_height = rc_parent.bottom - rc_parent.top;
  int window_width = rc_window.right - rc_window.left;
  int window_height = rc_window.bottom - rc_window.top;

  ::SetWindowPos(window_, HWND_TOP,
                 rc_parent.left + ((parent_width - window_width) / 2),
                 rc_parent.top + ((parent_height - window_height) / 2),
                 0, 0, SWP_NOSIZE);
}

HWND Window::Detach() {
  HWND hwnd = window_;

  if (prev_window_proc_)
    UnSubclass();

  WindowMap.Remove(this);

  window_ = nullptr;

  return hwnd;
}

LPCWSTR Window::GetClassName() const {
  return window_class_.lpszClassName;
}

HMENU Window::GetMenuHandle() const {
  return menu_;
}

HWND Window::GetParentHandle() const {
  return parent_;
}

HICON Window::SetIconLarge(HICON icon) {
  if (icon_large_)
    ::DestroyIcon(icon_large_);

  icon_large_ = icon;

  if (!icon_large_)
    return nullptr;

  return reinterpret_cast<HICON>(SendMessage(
      WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon_large_)));
}

HICON Window::SetIconLarge(int icon) {
  return SetIconLarge(reinterpret_cast<HICON>(LoadImage(
      instance_, MAKEINTRESOURCE(icon), IMAGE_ICON,
      ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON),
      LR_DEFAULTCOLOR)));
}

HICON Window::SetIconSmall(HICON icon) {
  if (icon_small_)
    ::DestroyIcon(icon_small_);

  icon_small_ = icon;

  if (!icon_small_)
    return nullptr;

  return reinterpret_cast<HICON>(SendMessage(
      WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon_small_)));
}

HICON Window::SetIconSmall(int icon) {
  return SetIconSmall(reinterpret_cast<HICON>(LoadImage(
      instance_, MAKEINTRESOURCE(icon), IMAGE_ICON,
      ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON),
      LR_DEFAULTCOLOR)));
}

HWND Window::GetWindowHandle() const {
  return window_;
}

void Window::SetWindowHandle(HWND hwnd) {
  window_ = hwnd;
}

////////////////////////////////////////////////////////////////////////////////
// Win32 API wrappers

BOOL Window::BringWindowToTop() const {
  return ::BringWindowToTop(window_);
}

BOOL Window::Close() const {
  return ::CloseWindow(window_);
}

BOOL Window::Enable(BOOL enable) const {
  return ::EnableWindow(window_, enable);
}

BOOL Window::GetClientRect(LPRECT rect) const {
  return ::GetClientRect(window_, rect);
}

HDC Window::GetDC() const {
  return ::GetDC(window_);
}

HFONT Window::GetFont() const {
  return reinterpret_cast<HFONT>(SendMessage(WM_GETFONT, 0, 0));
}

HMENU Window::GetMenu() const {
  return ::GetMenu(window_);
}

HWND Window::GetParent() {
  parent_ = ::GetParent(window_);
  if (!parent_)
    parent_ = ::GetDesktopWindow();
  return parent_;
}

BOOL Window::GetPlacement(WINDOWPLACEMENT& wp) const {
  return ::GetWindowPlacement(window_, &wp);
}

void Window::GetText(LPWSTR output, int max_count) const {
  ::GetWindowText(window_, output, max_count);
}

void Window::GetText(std::wstring& output) const {
  int len = ::GetWindowTextLength(window_) + 1;
  std::vector<wchar_t> buffer(len);
  ::GetWindowText(window_, &buffer[0], len);
  output.assign(&buffer[0]);
}

std::wstring Window::GetText() const {
  int len = ::GetWindowTextLength(window_) + 1;
  std::vector<wchar_t> buffer(len);
  ::GetWindowText(window_, &buffer[0], len);
  return std::wstring(&buffer[0]);
}

INT Window::GetTextLength() const {
  return ::GetWindowTextLength(window_);
}

DWORD Window::GetWindowLong(int index) const {
  return ::GetWindowLong(window_, index);
}

BOOL Window::GetWindowRect(LPRECT rect) const {
  return ::GetWindowRect(window_, rect);
}

void Window::GetWindowRect(HWND hwnd_to, LPRECT rect) const {
  ::GetClientRect(window_, rect);
  int width = rect->right;
  int height = rect->bottom;

  ::GetWindowRect(window_, rect);
  ::ScreenToClient(hwnd_to, reinterpret_cast<LPPOINT>(rect));

  rect->right = rect->left + width;
  rect->bottom = rect->top + height;
}

BOOL Window::Hide() const {
  return ::ShowWindow(window_, SW_HIDE);
}

BOOL Window::HideCaret() const {
  return ::HideCaret(window_);
}

BOOL Window::InvalidateRect(LPCRECT rect, BOOL erase) const {
  return ::InvalidateRect(window_, rect, erase);
}

BOOL Window::IsEnabled() const {
  return ::IsWindowEnabled(window_);
}

BOOL Window::IsIconic() const {
  return ::IsIconic(window_);
}

BOOL Window::IsVisible() const {
  return ::IsWindowVisible(window_);
}

BOOL Window::IsWindow() const {
  return ::IsWindow(window_);
}

INT Window::MessageBox(LPCWSTR text, LPCWSTR caption, UINT type) const {
  return ::MessageBox(window_, text, caption, type);
}

LRESULT Window::PostMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) const {
  return ::PostMessage(window_, uMsg, wParam, lParam);
}

BOOL Window::RedrawWindow(LPCRECT rect, HRGN region, UINT flags) const {
  return ::RedrawWindow(window_, rect, region, flags);
}

LRESULT Window::SendMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) const {
  return ::SendMessage(window_, uMsg, wParam, lParam);
}

BOOL Window::SetBorder(int style) const {
  switch (style) {
    case kWindowBorderNone:
      SetStyle(0, WS_EX_CLIENTEDGE | WS_EX_STATICEDGE, GWL_EXSTYLE);
      break;
    case kWindowBorderClient:
      SetStyle(WS_EX_CLIENTEDGE, WS_EX_STATICEDGE, GWL_EXSTYLE);
      break;
    case kWindowBorderStatic:
      SetStyle(WS_EX_STATICEDGE, WS_EX_CLIENTEDGE, GWL_EXSTYLE);
      break;
  }

  UINT flags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED;

  return ::SetWindowPos(window_, nullptr, 0, 0, 0, 0, flags);
}

HWND Window::SetCapture() const {
  return ::SetCapture(window_);
}

DWORD Window::SetClassLong(int index, LONG new_long) const {
  return ::SetClassLong(window_, index, new_long);
}

HWND Window::SetFocus() const {
  return ::SetFocus(window_);
}

void Window::SetFont(LPCWSTR face_name, int size,
                     bool bold, bool italic, bool underline) {
  HDC hdc = ::GetDC(window_);
  HFONT font_old = reinterpret_cast<HFONT>(::GetCurrentObject(hdc, OBJ_FONT));
  LOGFONT logfont;
  ::GetObject(font_old, sizeof(LOGFONT), &logfont);

  if (face_name) {
    ::lstrcpy(logfont.lfFaceName, face_name);
  }
  if (size) {
    logfont.lfHeight = -MulDiv(size, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    logfont.lfWidth = 0;
  }
  logfont.lfItalic = italic;
  logfont.lfWeight = bold ? FW_BOLD : FW_NORMAL;
  logfont.lfUnderline = underline;

  if (font_)
    ::DeleteObject(font_);
  font_ = ::CreateFontIndirect(&logfont);
  SendMessage(WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);

  ::DeleteObject(font_old);
  ::ReleaseDC(window_, hdc);
}

void Window::SetFont(HFONT font) {
  if (font_)
    ::DeleteObject(font_);
  font_ = font;
  SendMessage(WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
}

BOOL Window::SetForegroundWindow() const {
  return ::SetForegroundWindow(window_);
}

BOOL Window::SetMenu(HMENU menu) const {
  return ::SetMenu(window_, menu);
}

void Window::SetParent(HWND parent) const {
  if (!parent) {
    ::SetParent(window_, parent);
    SetStyle(WS_POPUP, WS_CHILD);
  } else {
    SetStyle(WS_CHILD, WS_POPUP);
    ::SetParent(window_, parent);
  }
}

BOOL Window::SetPlacement(const WINDOWPLACEMENT& wp) const {
  return ::SetWindowPlacement(window_, &wp);
}

BOOL Window::SetPosition(HWND hwnd_insert_after, int x, int y, int w, int h,
                         UINT flags) const {
  return ::SetWindowPos(window_, hwnd_insert_after, x, y, w, h, flags);
}

BOOL Window::SetPosition(HWND hwnd_insert_after, const RECT& rc,
                         UINT flags) const {
  return ::SetWindowPos(window_, hwnd_insert_after,
                        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                        flags);
}

BOOL Window::SetRedraw(BOOL redraw) const {
  return ::SendMessage(window_, WM_SETREDRAW, redraw, 0);
}

void Window::SetStyle(UINT style, UINT style_not, int index) const {
  UINT old_style = ::GetWindowLong(window_, index);
  ::SetWindowLongPtr(window_, index, (old_style | style) & ~style_not);
}

BOOL Window::SetText(LPCWSTR text) const {
  return SendMessage(WM_SETTEXT, 0, reinterpret_cast<LPARAM>(text));
}

BOOL Window::SetText(const std::wstring& text) const {
  return SetText(text.c_str());
}

HRESULT Window::SetTheme(LPCWSTR theme_name) const {
  return ::SetWindowTheme(window_, theme_name, nullptr);
}

BOOL Window::SetTransparency(BYTE alpha, COLORREF color) const {
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

  return ::SetLayeredWindowAttributes(window_, color, alpha, flags);
}

BOOL Window::Show(int cmd_show) const {
  return ::ShowWindow(window_, cmd_show);
}

BOOL Window::Update() const {
  return ::UpdateWindow(window_);
}

////////////////////////////////////////////////////////////////////////////////

void Window::OnCreate(HWND hwnd, LPCREATESTRUCT create_struct) {
  LOGFONT logfont;
  ::GetObject(::GetStockObject(DEFAULT_GUI_FONT), sizeof(logfont), &logfont);
  font_ = ::CreateFontIndirect(&logfont);
  ::SendMessage(window_, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
}

////////////////////////////////////////////////////////////////////////////////

void Window::Subclass(HWND hwnd) {
  WNDPROC current_proc = reinterpret_cast<WNDPROC>(
      ::GetWindowLongPtr(hwnd, GWLP_WNDPROC));
  if (current_proc != reinterpret_cast<WNDPROC>(WindowProcStatic)) {
    prev_window_proc_ = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(
        hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProcStatic)));
    window_ = hwnd;
  }
}

void Window::UnSubclass() {
  WNDPROC current_proc = reinterpret_cast<WNDPROC>(
      ::GetWindowLongPtr(window_, GWLP_WNDPROC));
  if (current_proc == reinterpret_cast<WNDPROC>(WindowProcStatic)) {
    ::SetWindowLongPtr(window_, GWLP_WNDPROC,
                       reinterpret_cast<LONG_PTR>(prev_window_proc_));
    prev_window_proc_ = nullptr;
  }
}

LRESULT CALLBACK Window::WindowProcStatic(HWND hwnd, UINT uMsg,
                                          WPARAM wParam, LPARAM lParam) {
  Window* window = WindowMap.GetWindow(hwnd);

  if (!window) {
    window = current_window_;
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

LRESULT Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return WindowProcDefault(hwnd, uMsg, wParam, lParam);
}

LRESULT Window::WindowProcDefault(HWND hwnd, UINT uMsg,
                                  WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_COMMAND: {
      if (OnCommand(wParam, lParam))
        return 0;
      break;
    }
    case WM_CONTEXTMENU: {
      POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
      OnContextMenu(reinterpret_cast<HWND>(wParam), pt);
      break;
    }
    case WM_CREATE: {
      OnCreate(hwnd, reinterpret_cast<LPCREATESTRUCT>(lParam));
      break;
    }
    case WM_DESTROY: {
      if (OnDestroy())
        return 0;
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
      if (lResult != -1)
        return lResult;
      break;
    }
    case WM_MOVE: {
      POINTS pts = MAKEPOINTS(lParam);
      OnMove(&pts);
      break;
    }
    case WM_NOTIFY: {
      LRESULT lResult = OnNotify(static_cast<int>(wParam),
                                 reinterpret_cast<LPNMHDR>(lParam));
      if (lResult)
        return lResult;
      break;
    }
    case WM_PAINT: {
      if (!prev_window_proc_) {
        if (::GetUpdateRect(hwnd, nullptr, FALSE)) {
          PAINTSTRUCT ps;
          HDC hdc = ::BeginPaint(hwnd, &ps);
          OnPaint(hdc, &ps);
          ::EndPaint(hwnd, &ps);
        } else {
          HDC hdc = ::GetDC(hwnd);
          OnPaint(hdc, nullptr);
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

  if (prev_window_proc_) {
    return ::CallWindowProc(prev_window_proc_, hwnd, uMsg, wParam, lParam);
  } else {
    return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
  }
}

}  // namespace win