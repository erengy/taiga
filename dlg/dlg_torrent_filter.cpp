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
#include "dlg_torrent_filter.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../string.h"

CTorrentFilterWindow TorrentFilterWindow;

// =============================================================================

CTorrentFilterWindow::CTorrentFilterWindow() {
  RegisterDlgClass(L"TaigaTorrentFilterW");
}

CTorrentFilterWindow::~CTorrentFilterWindow() {
}

// =============================================================================

BOOL CTorrentFilterWindow::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    // Set text color
    case WM_CTLCOLORSTATIC: {
      HDC hdc = reinterpret_cast<HDC>(wParam);
      HWND hStatic = reinterpret_cast<HWND>(lParam);
      if (hStatic == GetDlgItem(IDC_STATIC_DETAILS1) || 
        hStatic == GetDlgItem(IDC_STATIC_DETAILS2) || 
        hStatic == GetDlgItem(IDC_STATIC_DETAILS3)) {
          SetBkMode(hdc, TRANSPARENT);
          SetTextColor(hdc, ::GetSysColor(COLOR_GRAYTEXT));
          return (BOOL)::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
      }
      break;
    }
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

void CTorrentFilterWindow::OnCancel() {
  // Set values
  m_Filter.Option = 0;
  m_Filter.Type = 0;
  m_Filter.Value.clear();

  // Exit
  EndDialog(IDOK);
}

BOOL CTorrentFilterWindow::OnInitDialog() {
  // Initialize
  OnCommand(MAKEWPARAM(IDC_RADIO_FILTER_OPTION1 + m_Filter.Option, STN_CLICKED), 0);
  CheckDlgButton(IDC_RADIO_FILTER_TYPE1 + m_Filter.Type, TRUE);
  m_Combo.Attach(GetDlgItem(IDC_COMBO_FILTER_VALUE));
  RefreshComboBox(m_Filter.Type);
  switch (m_Filter.Type) {
    case RSS_FILTER_KEYWORD:
      m_Combo.SetText(m_Filter.Value.c_str());
      m_Combo.SetEditSel(0, -1);
      break;
    case RSS_FILTER_AIRINGSTATUS:
    case RSS_FILTER_WATCHINGSTATUS:
      int index = ToINT(m_Filter.Value);
      if (index == MAL_PLANTOWATCH) index--;
      m_Combo.SetCurSel(index - 1);
      break;
  }

  return TRUE;
}

void CTorrentFilterWindow::OnOK() {
  // Set values
  m_Filter.Option = GetCheckedRadioButton(IDC_RADIO_FILTER_OPTION1, IDC_RADIO_FILTER_OPTION3);
  m_Filter.Type = GetCheckedRadioButton(IDC_RADIO_FILTER_TYPE1, IDC_RADIO_FILTER_TYPE3);
  switch (m_Filter.Type) {
    case RSS_FILTER_KEYWORD:
      m_Combo.GetText(m_Filter.Value);
      break;
    case RSS_FILTER_AIRINGSTATUS:
    case RSS_FILTER_WATCHINGSTATUS:
      int index = m_Combo.GetCurSel() + 1;
      if (index == MAL_UNKNOWN) index++;
      m_Filter.Value = ToWSTR(index);
      break;
  }

  // Exit
  EndDialog(IDOK);
}

BOOL CTorrentFilterWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
  if (HIWORD(wParam) == BN_CLICKED) {
    switch (LOWORD(wParam)) {
      // Check radio buttons
      case IDC_RADIO_FILTER_OPTION1:
      case IDC_RADIO_FILTER_OPTION2:
      case IDC_RADIO_FILTER_OPTION3: {
        CheckRadioButton(IDC_RADIO_FILTER_OPTION1, IDC_RADIO_FILTER_OPTION3, LOWORD(wParam));
        EnableDlgItem(IDC_RADIO_FILTER_TYPE2, LOWORD(wParam) == IDC_RADIO_FILTER_OPTION1);
        EnableDlgItem(IDC_RADIO_FILTER_TYPE3, LOWORD(wParam) == IDC_RADIO_FILTER_OPTION1);
        if (LOWORD(wParam) != IDC_RADIO_FILTER_OPTION1) {
          OnCommand(MAKEWPARAM(IDC_RADIO_FILTER_TYPE1, BN_CLICKED), 0);
        }
        return TRUE;
      }
      case IDC_STATIC_DETAILS1:
      case IDC_STATIC_DETAILS2:
      case IDC_STATIC_DETAILS3: {
        OnCommand(MAKEWPARAM(IDC_RADIO_FILTER_OPTION1 + (LOWORD(wParam) - IDC_STATIC_DETAILS1), STN_CLICKED), 0);
        return TRUE;
      }
      case IDC_RADIO_FILTER_TYPE1:
      case IDC_RADIO_FILTER_TYPE2:
      case IDC_RADIO_FILTER_TYPE3: {
        CheckRadioButton(IDC_RADIO_FILTER_TYPE1, IDC_RADIO_FILTER_TYPE3, LOWORD(wParam));
        RefreshComboBox(LOWORD(wParam) - IDC_RADIO_FILTER_TYPE1);
        return TRUE;
      }
    }
  }

  return FALSE;
}

// =============================================================================

void CTorrentFilterWindow::RefreshComboBox(int type) {
  // Save position
  RECT rect;
  m_Combo.GetWindowRect(&rect);
  int width = rect.right - rect.left;
  int height = rect.bottom - rect.top;
  ::ScreenToClient(m_hWindow, (LPPOINT)&rect);

  // Re-create window
  DWORD old_style = m_Combo.GetWindowLong();
  DWORD style = type == 0 ? CBS_DROPDOWN : CBS_DROPDOWNLIST;
  DWORD new_style = old_style | style;
  DEBUG_PRINT(L"Cmb style: " + WSTR(style) + L" | ");
  DEBUG_PRINT(L"Old style: " + WSTR(old_style) + L" | ");
  DEBUG_PRINT(L"New style: " + WSTR(new_style) + L"\n");
  /*if (new_style != old_style)*/ {
    m_Combo.Create(0, WC_COMBOBOX, NULL, 
      style | CBS_HASSTRINGS | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 
      rect.left, rect.top, width, height, 
      m_hWindow, NULL, NULL);
  }

  // Add items
  switch (type) {
    case RSS_FILTER_AIRINGSTATUS:
      m_Combo.ResetContent();
      for (int i = 0; i < 3; i ++) {
        m_Combo.AddString(MAL.TranslateStatus(i + 1).c_str());
      }
      m_Combo.SetCurSel(0);
      break;
    case RSS_FILTER_WATCHINGSTATUS:
      m_Combo.ResetContent();
      for (int i = 0; i < 6; i ++) {
        if (i != 4) {
          m_Combo.AddString(MAL.TranslateMyStatus(i + 1, false).c_str());
        }
      }
      m_Combo.SetCurSel(0);
      break;
  }
}

void CTorrentFilterWindow::SetValues(int option, int type, wstring value) {
  m_Filter.Option = option;
  m_Filter.Type = type;
  m_Filter.Value = value;
}