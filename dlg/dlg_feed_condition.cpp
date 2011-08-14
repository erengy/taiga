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
#include "../win32/win_gdi.h"

class FeedConditionDialog FeedConditionDialog;

// =============================================================================

FeedConditionDialog::FeedConditionDialog() {
  RegisterDlgClass(L"TaigaFeedConditionW");
}

FeedConditionDialog::~FeedConditionDialog() {
}

// =============================================================================

BOOL FeedConditionDialog::OnInitDialog() {
  // Set title
  if (condition.element != 0 || condition.op != 0 || !condition.value.empty()) {
    SetText(L"Edit Condition");
  }

  // Initialize
  element_combo_.Attach(GetDlgItem(IDC_COMBO_FEED_ELEMENT));
  operator_combo_.Attach(GetDlgItem(IDC_COMBO_FEED_OPERATOR));
  value_combo_.Attach(GetDlgItem(IDC_COMBO_FEED_VALUE));

  // Add elements
  for (int i = 0; i < FEED_FILTER_ELEMENT_COUNT; i++) {
    element_combo_.AddString(Aggregator.filter_manager.TranslateElement(i).c_str());
  }
  
  // Choose
  element_combo_.SetCurSel(condition.element);
  ChooseElement(condition.element);
  operator_combo_.SetCurSel(condition.op);
  switch (condition.element) {
    case FEED_FILTER_ELEMENT_ANIME_ID: {
      value_combo_.SetCurSel(0);
      for (int i = 0; i < value_combo_.GetCount(); i++) {
        Anime* anime = reinterpret_cast<Anime*>(value_combo_.GetItemData(i));
        if (anime && anime->series_id == ToINT(condition.value)) {
          value_combo_.SetCurSel(i);
          break;
        }
      }
      break;
    }
    case FEED_FILTER_ELEMENT_ANIME_MYSTATUS: {
      int value = ToINT(condition.value);
      if (value == 6) value--;
      value_combo_.SetCurSel(value);
      break;
    }
    case FEED_FILTER_ELEMENT_ANIME_SERIESSTATUS:
      value_combo_.SetCurSel(ToINT(condition.value) - 1);
      break;
    case FEED_FILTER_ELEMENT_ANIME_EPISODE_AVAILABLE:
      value_combo_.SetCurSel(condition.value == L"True" ? 1 : 0);
      break;
    default:
      value_combo_.SetText(condition.value);
  }

  return TRUE;
}

// =============================================================================

INT_PTR FeedConditionDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_CTLCOLORSTATIC:
      return reinterpret_cast<INT_PTR>(GetStockObject(WHITE_BRUSH));
  }

  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

void FeedConditionDialog::OnCancel() {
  // Clear data
  condition.element = -1;
  condition.op = -1;
  condition.value = L"";

  // Exit
  EndDialog(IDCANCEL);
}

void FeedConditionDialog::OnOK() {
  // Set values
  condition.element = element_combo_.GetCurSel();
  condition.op = operator_combo_.GetCurSel();
  switch (condition.element) {
    case FEED_FILTER_ELEMENT_ANIME_ID: {
      Anime* anime = reinterpret_cast<Anime*>(value_combo_.GetItemData(value_combo_.GetCurSel()));
      if (anime) {
        condition.value = ToWSTR(anime->series_id);
      } else {
        condition.value = L"";
      }
      break;
    }
    case FEED_FILTER_ELEMENT_ANIME_SERIESSTATUS:
    case FEED_FILTER_ELEMENT_ANIME_MYSTATUS:
      condition.value = ToWSTR(value_combo_.GetItemData(value_combo_.GetCurSel()));
      break;
    default:
      value_combo_.GetText(condition.value);
  }

  // Exit
  EndDialog(IDOK);
}

BOOL FeedConditionDialog::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (HIWORD(wParam)) {
    case CBN_SELCHANGE: {
      if (LOWORD(wParam) == IDC_COMBO_FEED_ELEMENT) {
        ChooseElement(element_combo_.GetCurSel());
      }
      break;
    }
  }

  return FALSE;
}

void FeedConditionDialog::OnPaint(HDC hdc, LPPAINTSTRUCT lpps) {
  CDC dc = hdc;
  CRect rect;

  // Paint background
  GetClientRect(&rect);
  dc.FillRect(rect, ::GetSysColor(COLOR_WINDOW));

  // Paint bottom area
  CRect rect_button;
  ::GetClientRect(GetDlgItem(IDCANCEL), &rect_button);
  rect.top = rect.bottom - (rect_button.Height() * 2);
  dc.FillRect(rect, ::GetSysColor(COLOR_BTNFACE));
  
  // Paint line
  rect.bottom = rect.top + 1;
  dc.FillRect(rect, ::GetSysColor(COLOR_ACTIVEBORDER));
}

// =============================================================================

void FeedConditionDialog::ChooseElement(int element_index) {
  // Operator
  int op_index = operator_combo_.GetCurSel();
  operator_combo_.ResetContent();

  #define ADD_OPERATOR(op) \
    operator_combo_.AddString(Aggregator.filter_manager.TranslateOperator(op).c_str())

  switch (element_index) {
    case FEED_FILTER_ELEMENT_ANIME_ID:
    case FEED_FILTER_ELEMENT_ANIME_EPISODE_NUMBER:
      ADD_OPERATOR(FEED_FILTER_OPERATOR_IS);
      ADD_OPERATOR(FEED_FILTER_OPERATOR_ISNOT);
      ADD_OPERATOR(FEED_FILTER_OPERATOR_ISGREATERTHAN);
      ADD_OPERATOR(FEED_FILTER_OPERATOR_ISLESSTHAN);
      break;
    case FEED_FILTER_ELEMENT_ANIME_EPISODE_AVAILABLE:
    case FEED_FILTER_ELEMENT_ANIME_SERIESSTATUS:
    case FEED_FILTER_ELEMENT_ANIME_MYSTATUS:
      ADD_OPERATOR(FEED_FILTER_OPERATOR_IS);
      ADD_OPERATOR(FEED_FILTER_OPERATOR_ISNOT);
      break;
    default:
      for (int i = 0; i < FEED_FILTER_OPERATOR_COUNT; i++) {
        ADD_OPERATOR(i);
      }
  }

  #undef ADD_OPERATOR
  operator_combo_.SetCurSel(op_index < 0 || op_index > operator_combo_.GetCount() - 1 ? 0 : op_index);
  
  // ===========================================================================
  
  // Value
  value_combo_.ResetContent();

  RECT rect;
  value_combo_.GetWindowRect(&rect);
  int width = rect.right - rect.left;
  int height = rect.bottom - rect.top;
  ::ScreenToClient(m_hWindow, reinterpret_cast<LPPOINT>(&rect));

  #define RECREATE_COMBO(style) \
    value_combo_.Create(0, WC_COMBOBOX, nullptr, \
      style | CBS_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_VSCROLL, \
      rect.left, rect.top, width, height * 2, \
      m_hWindow, nullptr, nullptr);

  switch (element_index) {
    case FEED_FILTER_ELEMENT_CATEGORY:
      RECREATE_COMBO(CBS_DROPDOWN);
      value_combo_.AddString(L"Anime");
      value_combo_.AddString(L"Batch");
      value_combo_.AddString(L"Hentai");
      value_combo_.AddString(L"Non-English");
      value_combo_.AddString(L"Other");
      value_combo_.AddString(L"Raws");
      break;
    case FEED_FILTER_ELEMENT_ANIME_ID:
    case FEED_FILTER_ELEMENT_ANIME_TITLE: {
      RECREATE_COMBO((element_index == FEED_FILTER_ELEMENT_ANIME_ID ? CBS_DROPDOWNLIST : CBS_DROPDOWN));
      typedef std::pair<LPARAM, wstring> anime_pair;
      vector<anime_pair> title_list;
      for (auto it = AnimeList.items.begin() + 1; it != AnimeList.items.end(); ++it) {
        switch (it->GetStatus()) {
          case MAL_COMPLETED:
          case MAL_DROPPED:
            continue;
          default:
            title_list.push_back(std::make_pair((LPARAM)&(*it), it->series_title));
        }
      }
      std::sort(title_list.begin(), title_list.end(), 
        [](const anime_pair& a1, const anime_pair& a2) {
          return CompareStrings(a1.second, a2.second) < 0;
        });
      if (element_index == FEED_FILTER_ELEMENT_ANIME_ID) {
        value_combo_.AddString(L"(Unknown)");
      }
      for (auto it = title_list.begin(); it != title_list.end(); ++it) {
        value_combo_.AddItem(it->second.c_str(), it->first);
      }
      break;
    }
    case FEED_FILTER_ELEMENT_ANIME_SERIESSTATUS:
      RECREATE_COMBO(CBS_DROPDOWNLIST);
      value_combo_.AddItem(MAL.TranslateStatus(MAL_AIRING).c_str(), MAL_AIRING);
      value_combo_.AddItem(MAL.TranslateStatus(MAL_FINISHED).c_str(), MAL_FINISHED);
      value_combo_.AddItem(MAL.TranslateStatus(MAL_NOTYETAIRED).c_str(), MAL_NOTYETAIRED);
      break;
    case FEED_FILTER_ELEMENT_ANIME_MYSTATUS:
      RECREATE_COMBO(CBS_DROPDOWNLIST);
      value_combo_.AddItem(MAL.TranslateMyStatus(MAL_NOTINLIST, false).c_str(), MAL_NOTINLIST);
      value_combo_.AddItem(MAL.TranslateMyStatus(MAL_WATCHING, false).c_str(), MAL_WATCHING);
      value_combo_.AddItem(MAL.TranslateMyStatus(MAL_COMPLETED, false).c_str(), MAL_COMPLETED);
      value_combo_.AddItem(MAL.TranslateMyStatus(MAL_ONHOLD, false).c_str(), MAL_ONHOLD);
      value_combo_.AddItem(MAL.TranslateMyStatus(MAL_DROPPED, false).c_str(), MAL_DROPPED);
      value_combo_.AddItem(MAL.TranslateMyStatus(MAL_PLANTOWATCH, false).c_str(), MAL_PLANTOWATCH);
      break;
    case FEED_FILTER_ELEMENT_ANIME_EPISODE_NUMBER:
      RECREATE_COMBO(CBS_DROPDOWN);
      value_combo_.AddString(L"%watched%");
      value_combo_.AddString(L"%total%");
      break;
    case FEED_FILTER_ELEMENT_ANIME_EPISODE_VERSION:
      RECREATE_COMBO(CBS_DROPDOWN);
      value_combo_.AddString(L"v2");
      value_combo_.AddString(L"v3");
      value_combo_.AddString(L"v4");
      value_combo_.AddString(L"v0");
      break;
    case FEED_FILTER_ELEMENT_ANIME_EPISODE_AVAILABLE:
      RECREATE_COMBO(CBS_DROPDOWNLIST);
      value_combo_.AddString(L"False");
      value_combo_.AddString(L"True");
      break;
    case FEED_FILTER_ELEMENT_ANIME_VIDEO_RESOLUTION:
      RECREATE_COMBO(CBS_DROPDOWN);
      value_combo_.AddString(L"1080p");
      value_combo_.AddString(L"720p");
      value_combo_.AddString(L"480p");
      value_combo_.AddString(L"400p");
      value_combo_.AddString(L"1920x1080");
      value_combo_.AddString(L"1280x720");
      value_combo_.AddString(L"848x480");
      value_combo_.AddString(L"704x400");
      break;
    case FEED_FILTER_ELEMENT_ANIME_VIDEO_TYPE:
      RECREATE_COMBO(CBS_DROPDOWN);
      value_combo_.AddString(L"h264");
      value_combo_.AddString(L"x264");
      value_combo_.AddString(L"XviD");
      break;
    default:
      RECREATE_COMBO(CBS_DROPDOWN);
      break;
  }

  #undef RECREATE_COMBO
  value_combo_.SetCurSel(0);
}