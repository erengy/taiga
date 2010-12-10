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

#include "../std.h"
#include "dlg_input.h"
#include "../resource.h"
#include "../win32/win_control.h"

// =============================================================================

CInputDialog::CInputDialog() {
  Info = L"Set new value:";
  m_NumbersOnly = false;
  m_CurrentValue = 0;
  m_MinValue = 0;
  m_MaxValue = 0;
  Result = 0;
  Title = L"Input";
};

void CInputDialog::SetNumbers(bool enabled, int min_value, int max_value, int current_value) {
  m_NumbersOnly = enabled;
  m_MinValue = min_value;
  m_MaxValue = max_value;
  m_CurrentValue = current_value;
}

void CInputDialog::Show(HWND hParent) {
  Result = Create(IDD_INPUT, hParent, true);
}

// =============================================================================

BOOL CInputDialog::OnInitDialog() {
  // Set dialog title
  SetWindowText(m_hWindow, Title.c_str());

  // Set information text
  SetDlgItemText(IDC_STATIC_INPUTINFO, Info.c_str());

  // Set text style and properties
  CEdit m_Edit = GetDlgItem(IDC_EDIT_INPUT);
  CSpin m_Spin = GetDlgItem(IDC_SPIN_INPUT);
  m_Edit.LimitText(256);
  if (m_NumbersOnly) {
    m_Edit.SetStyle(ES_NUMBER, 0);
    m_Spin.SetBuddy(m_Edit.GetWindowHandle());
    m_Spin.SetRange32(m_MinValue, m_MaxValue > 0 ? m_MaxValue : 999);
    m_Spin.SetPos32(m_CurrentValue);
  } else {
    m_Edit.SetStyle(0, ES_NUMBER);
    m_Edit.SetText(Text.c_str());
    m_Spin.Enable(FALSE);
    m_Spin.Show(SW_HIDE);
  }
  m_Edit.SetSel(0, -1);
  m_Edit.SetWindowHandle(NULL);
  m_Spin.SetWindowHandle(NULL);

  return TRUE;
}

void CInputDialog::OnCancel() {
  EndDialog(IDCANCEL);
}

void CInputDialog::OnOK() {
  GetDlgItemText(IDC_EDIT_INPUT, Text);
  EndDialog(IDOK);
}