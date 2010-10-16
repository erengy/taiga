// ui_taskbar.h

#ifndef UI_TASKBAR_H
#define UI_TASKBAR_H

#include <shobjidl.h>

// =============================================================================

/* Taskbar */

class CTaskbar {
public:
  CTaskbar();
  virtual ~CTaskbar();

  BOOL Create(HWND hwnd, HICON hIcon, LPCWSTR lpTooltip);
  BOOL Destroy();
  BOOL Modify(LPCWSTR lpTip);
  BOOL Tip(LPCWSTR lpText, LPCWSTR lpTitle, int iIconIndex);

private:
  HWND           m_hApp;
  NOTIFYICONDATA m_NID;
};

/* Taskbar list */

class CTaskbarList {
public:
  CTaskbarList();
  virtual ~CTaskbarList();

  void Initialize(HWND hwnd);
  void Release();
  void SetProgressState(TBPFLAG flag);
  void SetProgressValue(ULONGLONG ullValue, ULONGLONG ullTotal);

private:
  HWND           m_hWnd;
  ITaskbarList3* m_pTaskbarList;
};

// =============================================================================

extern CTaskbar Taskbar;
extern CTaskbarList TaskbarList;

extern const DWORD WM_TASKBARCALLBACK;
extern const DWORD WM_TASKBARCREATED;
extern const DWORD WM_TASKBARBUTTONCREATED;

#endif // UI_TASKBAR_H