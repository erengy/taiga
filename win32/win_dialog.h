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

#ifndef WIN_DIALOG_H
#define WIN_DIALOG_H

#include "win_main.h"

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

  virtual BOOL AddComboString(int nIDDlgItem, LPCWSTR lpString);
  virtual BOOL CheckDlgButton(int nIDButton, UINT uCheck);
  virtual BOOL CheckRadioButton(int nIDFirstButton, int nIDLastButton, int nIDCheckButton);
  virtual BOOL EnableDlgItem(int nIDDlgItem, BOOL bEnable);
  virtual INT  GetCheckedRadioButton(int nIDFirstButton, int nIDLastButton);
  virtual INT  GetComboSelection(int nIDDlgItem);
  virtual HWND GetDlgItem(int nIDDlgItem);
  virtual UINT GetDlgItemInt(int nIDDlgItem);
  virtual void GetDlgItemText(int nIDDlgItem, LPWSTR lpString, int cchMax = MAX_PATH);
  virtual void GetDlgItemText(int nIDDlgItem, wstring& str);
  virtual BOOL IsDlgButtonChecked(int nIDButton);
  virtual BOOL SendDlgItemMessage(int nIDDlgItem, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual BOOL SetComboSelection(int nIDDlgItem, int iIndex);
  virtual BOOL SetDlgItemText(int nIDDlgItem, LPCWSTR lpString);

protected:
  virtual void OnCancel();
  virtual BOOL OnClose();
  virtual BOOL OnInitDialog();
  virtual void OnOK();
  virtual void RegisterDlgClass(LPCWSTR lpszClassName);

  virtual INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual INT_PTR DialogProcDefault(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
  static INT_PTR CALLBACK DialogProcStatic(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  void SetMinMaxInfo(LPMINMAXINFO lpMMI);
  void SnapToEdges(LPWINDOWPOS lpWndPos);

  bool  m_bModal;
  int   m_iSnapGap;
  SIZE  m_SizeLast, m_SizeMax, m_SizeMin;
  POINT m_PosLast;
};

#endif // WIN_DIALOG_H