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
#include <algorithm>
#include "dlg_feed_condition.h"
#include "../feed.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../string.h"

CFeedConditionWindow FeedConditionWindow;

// =============================================================================

CFeedConditionWindow::CFeedConditionWindow() {
  RegisterDlgClass(L"TaigaFeedConditionW");
}

CFeedConditionWindow::~CFeedConditionWindow() {
}

// =============================================================================

BOOL CFeedConditionWindow::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

void CFeedConditionWindow::OnCancel() {
  // Clear data
  m_Condition.Element = -1;
  m_Condition.Operator = -1;
  m_Condition.Value = L"";

  // Exit
  EndDialog(IDOK);
}

BOOL CFeedConditionWindow::OnInitDialog() {
  // Initialize
  m_Element.SetWindowHandle(GetDlgItem(IDC_COMBO_FEED_ELEMENT));
  m_Operator.SetWindowHandle(GetDlgItem(IDC_COMBO_FEED_OPERATOR));
  m_Value.SetWindowHandle(GetDlgItem(IDC_COMBO_FEED_VALUE));

  // Add elements
  for (int i = 0; i < FEED_FILTER_ELEMENT_COUNT; i++) {
    m_Element.AddString(Aggregator.FilterManager.TranslateElement(i).c_str());
  }
  m_Element.SetCurSel(0);
  ChooseElement(0);

  return TRUE;
}

void CFeedConditionWindow::OnOK() {
  // Set values
  m_Condition.Element = m_Element.GetCurSel();
  m_Condition.Operator = m_Operator.GetCurSel();
  switch (m_Condition.Element) {
    case FEED_FILTER_ELEMENT_ANIME_MYSTATUS: {
      int value = m_Value.GetCurSel();
      if (value == 5) value++;
      m_Condition.Value = ToWSTR(value);
      break;
    }
    case FEED_FILTER_ELEMENT_ANIME_SERIESSTATUS:
      m_Condition.Value = ToWSTR(m_Value.GetCurSel() + 1);
      break;
    default:
      m_Value.GetText(m_Condition.Value);
  }

  // Exit
  EndDialog(IDOK);
}

BOOL CFeedConditionWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (HIWORD(wParam)) {
    case CBN_SELCHANGE: {
      if (LOWORD(wParam) == IDC_COMBO_FEED_ELEMENT) {
        ChooseElement(m_Element.GetCurSel());
      }
      break;
    }
  }

  return FALSE;
}

// =============================================================================

void CFeedConditionWindow::ChooseElement(int element) {
  // Operator
  int op_index = m_Operator.GetCurSel();
  m_Operator.ResetContent();

  #define ADD_OPERATOR(op) \
    m_Operator.AddString(Aggregator.FilterManager.TranslateOperator(op).c_str())

  switch (element) {
    case FEED_FILTER_ELEMENT_ANIME_EPISODEAVAILABLE:
    case FEED_FILTER_ELEMENT_ANIME_MYSTATUS:
    case FEED_FILTER_ELEMENT_ANIME_SERIESSTATUS:
      ADD_OPERATOR(FEED_FILTER_OPERATOR_IS);
      ADD_OPERATOR(FEED_FILTER_OPERATOR_ISNOT);
      break;
    default:
      for (int i = 0; i < FEED_FILTER_OPERATOR_COUNT; i++) {
        ADD_OPERATOR(i);
      }
  }

  #undef ADD_OPERATOR
  m_Operator.SetCurSel(op_index < 0 || op_index > m_Operator.GetCount() - 1 ? 0 : op_index);
  
  // ===========================================================================
  
  // Value
  m_Value.ResetContent();

  RECT rect;
  m_Value.GetWindowRect(&rect);
  int width = rect.right - rect.left;
  int height = rect.bottom - rect.top;
  ::ScreenToClient(m_hWindow, reinterpret_cast<LPPOINT>(&rect));

  #define RECREATE_COMBO(style) \
    m_Value.Create(0, WC_COMBOBOX, NULL, \
      style | CBS_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_VSCROLL, \
      rect.left, rect.top, width, height, \
      m_hWindow, NULL, NULL);

  switch (element) {
    case FEED_FILTER_ELEMENT_CATEGORY:
      RECREATE_COMBO(CBS_DROPDOWN);
      m_Value.AddString(L"Anime");
      m_Value.AddString(L"Batch");
      m_Value.AddString(L"Hentai");
      break;
    case FEED_FILTER_ELEMENT_ANIME_TITLE: {
      RECREATE_COMBO(CBS_DROPDOWN);
      vector<wstring> title_list;
      for (auto it = AnimeList.Item.begin() + 1; it != AnimeList.Item.end(); ++it) {
        switch (it->GetStatus()) {
          case MAL_COMPLETED:
          case MAL_DROPPED:
            continue;
          default:
            title_list.push_back(it->Series_Title);
        }
      }
      std::sort(title_list.begin(), title_list.end(), 
        [](const wstring& str1, const wstring& str2) {
          return CompareStrings(str1, str2) < 0;
        });
      for (auto it = title_list.begin(); it != title_list.end(); ++it) {
        m_Value.AddString(it->c_str());
      }
      break;
    }
    case FEED_FILTER_ELEMENT_ANIME_EPISODE:
      RECREATE_COMBO(CBS_DROPDOWN);
      m_Value.AddString(L"%watched%");
      m_Value.AddString(L"%total%");
      break;
    case FEED_FILTER_ELEMENT_ANIME_EPISODEAVAILABLE:
      RECREATE_COMBO(CBS_DROPDOWNLIST);
      m_Value.AddString(L"False");
      m_Value.AddString(L"True");
      break;
    case FEED_FILTER_ELEMENT_ANIME_RESOLUTION:
      RECREATE_COMBO(CBS_DROPDOWN);
      m_Value.AddString(L"1080p");
      m_Value.AddString(L"720p");
      m_Value.AddString(L"480p");
      m_Value.AddString(L"360p");
      break;
    case FEED_FILTER_ELEMENT_ANIME_MYSTATUS:
      RECREATE_COMBO(CBS_DROPDOWNLIST);
      m_Value.AddString(MAL.TranslateMyStatus(MAL_NOTINLIST, false).c_str());
      m_Value.AddString(MAL.TranslateMyStatus(MAL_WATCHING, false).c_str());
      m_Value.AddString(MAL.TranslateMyStatus(MAL_COMPLETED, false).c_str());
      m_Value.AddString(MAL.TranslateMyStatus(MAL_ONHOLD, false).c_str());
      m_Value.AddString(MAL.TranslateMyStatus(MAL_DROPPED, false).c_str());
      m_Value.AddString(MAL.TranslateMyStatus(MAL_PLANTOWATCH, false).c_str());
      break;
    case FEED_FILTER_ELEMENT_ANIME_SERIESSTATUS:
      RECREATE_COMBO(CBS_DROPDOWNLIST);
      m_Value.AddString(MAL.TranslateStatus(MAL_AIRING).c_str());
      m_Value.AddString(MAL.TranslateStatus(MAL_FINISHED).c_str());
      m_Value.AddString(MAL.TranslateStatus(MAL_NOTYETAIRED).c_str());
      break;
    default:
      RECREATE_COMBO(CBS_DROPDOWN);
      break;
  }

  #undef RECREATE_COMBO
  m_Value.SetCurSel(0);
}