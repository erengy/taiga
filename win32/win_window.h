/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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

#ifndef WIN_WINDOW_H
#define WIN_WINDOW_H

#include "win_main.h"

#define CONTROL_MARGIN 6
#define DEFAULT_CLASS  L"TaigaDefaultW"

enum BorderStyle {
  BORDER_NONE,
  BORDER_CLIENT,
  BORDER_STATIC
};

// =============================================================================

/* Window class */

class CWindow {
public:
  CWindow();
  CWindow(HWND hWnd);
  virtual ~CWindow();

  virtual HWND Create(HWND hWndParent = NULL);
  virtual HWND Create(DWORD dwExStyle, 
                      LPCWSTR lpClassName, 
                      LPCWSTR lpWindowName, 
                      DWORD dwStyle, 
                      int X, int Y, 
                      int nWidth, int nHeight, 
                      HWND hWndParent, 
                      HMENU hMenu, 
                      LPVOID lpParam);
  virtual void Destroy();
  virtual void PreCreate(CREATESTRUCT& cs) {};
  virtual void PreRegisterClass(WNDCLASSEX& wc) {};
  virtual BOOL PreTranslateMessage(MSG* pMsg) { return FALSE; };

  void    Attach(HWND hWindow);
  void    CenterOwner();
  HWND    Detach();
  LPCWSTR GetClassName() const { return m_WndClass.lpszClassName; };
  HMENU   GetMenuHandle() const { return m_hMenu; };
  HWND    GetParentHandle() const { return m_hParent; };
  HICON   SetIconLarge(int nIcon);
  HICON   SetIconSmall(int nIcon);
  HWND    GetWindowHandle() const { return m_hWindow; };
  void    SetWindowHandle(HWND hWnd) { m_hWindow = hWnd; };

  // Win32 API wrappers
  BOOL    BringWindowToTop() const;
  BOOL    Close() const;
  BOOL    Enable(BOOL bEnable = TRUE) const;
  BOOL    GetClientRect(LPRECT lpRect) const;
  HDC     GetDC() const;
  HFONT   GetFont();
  HMENU   GetMenu() const;
  HWND    GetParent();
  void    GetText(LPWSTR lpszString, int nMaxCount = MAX_PATH) const;
  void    GetText(std::wstring& str) const;
  INT     GetTextLength() const;
  DWORD   GetWindowLong(int nIndex = GWL_STYLE) const;
  BOOL    GetWindowRect(LPRECT lpRect) const;
  BOOL    InvalidateRect(LPCRECT lpRect = NULL, BOOL bErase = TRUE) const;
  BOOL    IsEnabled() const;
  BOOL    IsIconic() const;
  BOOL    IsVisible() const;
  BOOL    IsWindow() const;
  INT     MessageBox(LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) const;
  LRESULT PostMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL    RedrawWindow(LPCRECT lprcUpdate, HRGN hrgnUpdate, UINT flags) const;
  LRESULT SendMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL    SetBorder(int iStyle);
  HWND    SetCapture() const;
  DWORD   SetClassLong(int nIndex, LONG dwNewLong) const;
  HWND    SetFocus() const;
  void    SetFont(LPCWSTR lpFaceName, int iSize, bool bBold = false, bool bItalic = false, bool bUnderline = false);
  void    SetFont(HFONT hFont);
  BOOL    SetForegroundWindow() const;
  BOOL    SetMenu(HMENU hMenu) const;
  void    SetParent(HWND hParent);
  BOOL    SetPosition(HWND hInsertAfter, int x, int y, int w, int h, UINT uFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER) const;
  BOOL    SetPosition(HWND hInsertAfter, const RECT& rc, UINT uFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER) const;
  BOOL    SetRedraw(BOOL bRedraw) const;
  void    SetStyle(UINT style, UINT style_not, int nIndex = GWL_STYLE);
  BOOL    SetText(LPCWSTR lpszString) const;
  BOOL    SetText(const std::wstring& str) const;
  HRESULT SetTheme(LPCWSTR pszName = L"explorer") const;
  BOOL    SetTransparency(BYTE alpha, COLORREF color = 0xFF000000);
  BOOL    Show(int nCmdShow = SW_SHOWNORMAL) const;
  BOOL    Update() const;

protected:
  // Message handlers
  virtual BOOL    OnCommand(WPARAM wParam, LPARAM lParam) { return FALSE; };
  virtual BOOL    OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) { return FALSE; };
  virtual void    OnDestroy() {};
  virtual void    OnDropFiles(HDROP hDropInfo) {};
  virtual void    OnGetMinMaxInfo(LPMINMAXINFO lpMMI) {};
  virtual LRESULT OnMouseEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) { return -1; };
  virtual void    OnMove(LPPOINTS ptsPos) {};
  virtual LRESULT OnNotify(int idCtrl, LPNMHDR pnmh) { return 0; };
  virtual void    OnPaint(HDC hdc, LPPAINTSTRUCT lpps) {};
  virtual void    OnSize(UINT uMsg, UINT nType, SIZE size) {};
  virtual void    OnTaskbarCallback(UINT uMsg, LPARAM lParam) {};
  virtual void    OnTimer(UINT_PTR nIDEvent) {};
  virtual void    OnWindowPosChanging(LPWINDOWPOS lpWndPos) {};
  virtual LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual LRESULT WindowProcDefault(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  CREATESTRUCT m_CreateStruct;
  HFONT        m_hFont;
  HICON        m_hIconLarge, m_hIconSmall;
  HINSTANCE    m_hInstance;
  HMENU        m_hMenu;
  HWND         m_hParent, m_hWindow;
  WNDCLASSEX   m_WndClass;
  WNDPROC      m_PrevWindowProc;

private:
  static LRESULT CALLBACK WindowProcStatic(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  
  BOOL RegisterClass(WNDCLASSEX& wc);
  void Subclass(HWND hWnd);
  void UnSubclass();
};

#endif // WIN_WINDOW_H