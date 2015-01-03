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

#ifndef TAIGA_WIN_DIALOG_H
#define TAIGA_WIN_DIALOG_H

#include "win_main.h"
#include "win_window.h"

namespace win {

class Dialog : public Window {
public:
  Dialog();
  virtual ~Dialog();

  virtual INT_PTR Create(UINT resource_id, HWND parent = nullptr, bool modal = true);
  virtual void EndDialog(INT_PTR result);
  virtual void SetSizeMax(LONG cx, LONG cy);
  virtual void SetSizeMin(LONG cx, LONG cy);
  virtual void SetSnapGap(int snap_gap);

  virtual BOOL AddComboString(int id_combo, LPCWSTR text);
  virtual BOOL CheckDlgButton(int id_button, UINT check);
  virtual BOOL CheckRadioButton(int id_first_button, int id_last_button, int id_check_button);
  virtual BOOL EnableDlgItem(int id_item, BOOL enable);
  virtual INT  GetCheckedRadioButton(int id_first_button, int id_last_button);
  virtual INT  GetComboSelection(int id_item);
  virtual HWND GetDlgItem(int id_item);
  virtual UINT GetDlgItemInt(int id_item);
  virtual void GetDlgItemText(int id_item, LPWSTR output, int max_length = MAX_PATH);
  virtual void GetDlgItemText(int id_item, std::wstring& output);
  virtual std::wstring GetDlgItemText(int id_item);
  virtual BOOL HideDlgItem(int id_item);
  virtual BOOL IsDlgButtonChecked(int id_button);
  virtual bool IsModal();
  virtual BOOL SendDlgItemMessage(int id_item, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual BOOL SetComboSelection(int id_item, int index);
  virtual BOOL SetDlgItemText(int id_item, LPCWSTR text);
  virtual BOOL ShowDlgItem(int id_item, int cmd_show = SW_SHOWNORMAL);

protected:
  virtual void OnCancel();
  virtual BOOL OnClose();
  virtual BOOL OnInitDialog();
  virtual void OnOK();
  virtual void RegisterDlgClass(LPCWSTR class_name);

  virtual INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual INT_PTR DialogProcDefault(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
  static INT_PTR CALLBACK DialogProcStatic(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  void SetMinMaxInfo(LPMINMAXINFO mmi);
  void SnapToEdges(LPWINDOWPOS window_pos);

  bool  modal_;
  int   snap_gap_;
  SIZE  size_last_, size_max_, size_min_;
  POINT pos_last_;
};

}  // namespace win

#endif  // TAIGA_WIN_DIALOG_H