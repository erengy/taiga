// ui_taskdialog.h

#ifndef UI_TASKDIALOG_H
#define UI_TASKDIALOG_H

#include "ui_main.h"

#define TD_ICON_NONE         static_cast<PCWSTR>(0)
#define TD_ICON_INFORMATION  TD_INFORMATION_ICON
#define TD_ICON_WARNING      TD_WARNING_ICON
#define TD_ICON_ERROR        TD_ERROR_ICON
#define TD_ICON_SHIELD       TD_SHIELD_ICON
#define TD_ICON_SHIELD_GREEN MAKEINTRESOURCE(-8)
#define TD_ICON_SHIELD_RED   MAKEINTRESOURCE(-7)

#define TDF_SIZE_TO_CONTENT 0x1000000

// =============================================================================

class CTaskDialog {
public:
  CTaskDialog();
  virtual ~CTaskDialog() {}
  
  void    AddButton(LPCWSTR text, int id);
  int     GetSelectedButtonID() const;
  void    SetCollapsedControlText(LPCWSTR text);
  void    SetContent(LPCWSTR text);
  void    SetExpandedControlText(LPCWSTR text);
  void    SetExpandedInformation(LPCWSTR text);
  void    SetFooter(LPCWSTR LPCWSTR);
  void    SetFooterIcon(LPWSTR icon);
  void    SetMainIcon(LPWSTR icon);
  void    SetMainInstruction(LPCWSTR text);
  void    SetWindowTitle(LPCWSTR text);
  HRESULT Show(HWND hParent);
  void    UseCommandLinks(bool use);

protected:
  static HRESULT CALLBACK Callback(HWND hwnd, UINT uNotification, 
    WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData);
  
  vector<TASKDIALOG_BUTTON> m_Buttons;
  TASKDIALOGCONFIG m_Config;
  int m_SelectedButtonID;
};

#endif // UI_TASKDIALOG_H