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

#include "../std.h"
#include "dlg_feed_condition.h"
#include "dlg_feed_filter.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../string.h"

CFeedFilterWindow FeedFilterWindow;

// =============================================================================

CFeedFilterWindow::CFeedFilterWindow() {
  RegisterDlgClass(L"TaigaFeedFilterW");
}

CFeedFilterWindow::~CFeedFilterWindow() {
}

// =============================================================================

BOOL CFeedFilterWindow::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

void CFeedFilterWindow::OnCancel() {
  m_Filter.Conditions.clear();

  // Exit
  EndDialog(IDOK);
}

BOOL CFeedFilterWindow::OnInitDialog() {
  // Initialize
  SetDlgItemText(IDC_EDIT_FEED_NAME, m_Filter.Name.c_str());
  CheckDlgButton(IDC_RADIO_FEED_MATCH1 + m_Filter.Match, TRUE);
  CheckDlgButton(IDC_RADIO_FEED_ACTION1 + m_Filter.Action, TRUE);

  // Initialize list
  m_List.Attach(GetDlgItem(IDC_LIST_FEED_CONDITIONS));
  m_List.InsertColumn(0, 150, 150, 0, L"Element");
  m_List.InsertColumn(1, 150, 150, 0, L"Operator");
  m_List.InsertColumn(2, 150, 150, 0, L"Value");
  m_List.SetExtendedStyle(LVS_EX_DOUBLEBUFFER);

  // Add conditions to list
  for (auto it = m_Filter.Conditions.begin(); it != m_Filter.Conditions.end(); ++it) {
    AddConditionToList(*it);
  }

  return TRUE;
}

void CFeedFilterWindow::OnOK() {
  // Set values
  GetDlgItemText(IDC_EDIT_FEED_NAME, m_Filter.Name);
  m_Filter.Match = GetCheckedRadioButton(IDC_RADIO_FEED_MATCH1, IDC_RADIO_FEED_MATCH2);
  m_Filter.Action = GetCheckedRadioButton(IDC_RADIO_FEED_ACTION1, IDC_RADIO_FEED_ACTION2);

  // Exit
  EndDialog(IDOK);
}

BOOL CFeedFilterWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
  if (HIWORD(wParam) == BN_CLICKED) {
    switch (LOWORD(wParam)) {
      // Add new condition
      case IDC_BUTTON_FEED_ADDCONDITION:
        FeedConditionWindow.Create(IDD_FEED_CONDITION, GetWindowHandle());
        if (FeedConditionWindow.m_Condition.Element > -1) {
          m_Filter.AddCondition(
            FeedConditionWindow.m_Condition.Element,
            FeedConditionWindow.m_Condition.Operator,
            FeedConditionWindow.m_Condition.Value);
          AddConditionToList(m_Filter.Conditions.back());
        }
        return TRUE;
      // Check radio buttons
      case IDC_RADIO_FEED_MATCH1:
      case IDC_RADIO_FEED_MATCH2:
        CheckRadioButton(IDC_RADIO_FEED_MATCH1, IDC_RADIO_FEED_MATCH2, LOWORD(wParam));
        return TRUE;
      case IDC_RADIO_FEED_ACTION1:
      case IDC_RADIO_FEED_ACTION2:
        CheckRadioButton(IDC_RADIO_FEED_ACTION1, IDC_RADIO_FEED_ACTION2, LOWORD(wParam));
        return TRUE;
    }
  }

  return FALSE;
}

// =============================================================================

void CFeedFilterWindow::AddConditionToList(const CFeedFilterCondition& condition) {
  int i = m_List.GetItemCount();
  m_List.InsertItem(i, -1, -1, 0, 0, Aggregator.FilterManager.TranslateElement(condition.Element).c_str(), NULL);
  m_List.SetItem(i, 1, Aggregator.FilterManager.TranslateOperator(condition.Operator).c_str());
  m_List.SetItem(i, 2, condition.Value.c_str());
  m_List.SetCheckState(i, TRUE);
}