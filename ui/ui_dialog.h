// ui_dialog.h

#ifndef UI_DIALOG_H
#define UI_DIALOG_H

#include "ui_main.h"

// =============================================================================

class CDialog : public CWindow {
public:
  CDialog();
  virtual ~CDialog();
  
  virtual INT_PTR Create(UINT uResourceID, HWND hParent = NULL, bool bModal = true);
  virtual void    EndDialog(INT_PTR nResult);
  virtual void    SetSizeMax(LONG cx, LONG cy);
  virtual void    SetSizeMin(LONG cx, LONG cy);
  virtual void    SetSnapGap(int iSnapGap);

  virtual BOOL    AddComboString(int nIDDlgItem, LPCWSTR lpString);
  virtual BOOL    CheckDlgButton(int nIDButton, UINT uCheck);
  virtual BOOL    CheckRadioButton(int nIDFirstButton, int nIDLastButton, int nIDCheckButton);
  virtual BOOL    EnableDlgItem(int nIDDlgItem, BOOL bEnable);
  virtual INT     GetCheckedRadioButton(int nIDFirstButton, int nIDLastButton);
  virtual INT     GetComboSelection(int nIDDlgItem);
  virtual HWND    GetDlgItem(int nIDDlgItem);
  virtual UINT    GetDlgItemInt(int nIDDlgItem);
  virtual void    GetDlgItemText(int nIDDlgItem, LPWSTR lpString, int cchMax = MAX_PATH);
  virtual void    GetDlgItemText(int nIDDlgItem, wstring& str);
  virtual BOOL    IsDlgButtonChecked(int nIDButton);
  virtual BOOL    SendDlgItemMessage(int nIDDlgItem, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual BOOL    SetComboSelection(int nIDDlgItem, int iIndex);
  virtual BOOL    SetDlgItemText(int nIDDlgItem, LPCWSTR lpString);

protected:
  virtual void OnCancel();
  virtual BOOL OnClose();
  virtual BOOL OnInitDialog();
  virtual void OnOK();
  virtual void RegisterDlgClass(LPCWSTR lpszClassName);

  virtual INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual INT_PTR DialogProcDefault(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
  void SetMinMaxInfo(LPMINMAXINFO lpMMI);
  void SnapToEdges(LPWINDOWPOS lpWndPos);
  static INT_PTR CALLBACK DialogProcStatic(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  bool  m_bModal;
  int   m_iSnapGap;
  SIZE  m_SizeLast, m_SizeMax, m_SizeMin;
  POINT m_PosLast;
};

#endif // UI_DIALOG_H