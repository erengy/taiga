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

#include <algorithm>

#include "base/foreach.h"
#include "base/string.h"
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "taiga/resource.h"
#include "ui/dlg/dlg_feed_condition.h"
#include "win/win_gdi.h"

namespace ui {

FeedConditionDialog DlgFeedCondition;

FeedConditionDialog::FeedConditionDialog() {
  RegisterDlgClass(L"TaigaFeedConditionW");
}

////////////////////////////////////////////////////////////////////////////////

BOOL FeedConditionDialog::OnInitDialog() {
  // Set title
  if (condition.element != 0 ||
      condition.op != 0 ||
      !condition.value.empty())
    SetText(L"Edit Condition");

  // Initialize
  element_combo_.Attach(GetDlgItem(IDC_COMBO_FEED_ELEMENT));
  operator_combo_.Attach(GetDlgItem(IDC_COMBO_FEED_OPERATOR));
  value_combo_.Attach(GetDlgItem(IDC_COMBO_FEED_VALUE));

  // Add elements
  for (int i = 0; i < FEED_FILTER_ELEMENT_COUNT; i++)
    element_combo_.AddItem(Aggregator.filter_manager.TranslateElement(i).c_str(), i);

  // Set element
  element_combo_.SetCurSel(condition.element);
  ChooseElement(condition.element);
  // Set operator
  operator_combo_.SetCurSel(operator_combo_.FindItemData(condition.op));
  // Set value
  switch (condition.element) {
    case FEED_FILTER_ELEMENT_META_ID: {
      value_combo_.SetCurSel(0);
      for (int i = 0; i < value_combo_.GetCount(); i++) {
        int anime_id = static_cast<int>(value_combo_.GetItemData(i));
        if (anime_id == ToInt(condition.value)) {
          value_combo_.SetCurSel(i);
          break;
        }
      }
      break;
    }
    case FEED_FILTER_ELEMENT_USER_STATUS: {
      int value = ToInt(condition.value);
      value_combo_.SetCurSel(value);
      break;
    }
    case FEED_FILTER_ELEMENT_META_STATUS:
    case FEED_FILTER_ELEMENT_META_TYPE:
      value_combo_.SetCurSel(ToInt(condition.value) - 1);
      break;
    case FEED_FILTER_ELEMENT_LOCAL_EPISODE_AVAILABLE:
      value_combo_.SetCurSel(condition.value == L"True" ? 1 : 0);
      break;
    default:
      value_combo_.SetText(condition.value);
  }

  return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

INT_PTR FeedConditionDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_CTLCOLORSTATIC: {
      win::Dc dc = reinterpret_cast<HDC>(wParam);
      dc.SetBkMode(TRANSPARENT);
      dc.DetachDc();
      return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_WINDOW));
    }
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
  condition.op = operator_combo_.GetItemData(operator_combo_.GetCurSel());
  switch (condition.element) {
    case FEED_FILTER_ELEMENT_META_ID:
    case FEED_FILTER_ELEMENT_META_STATUS:
    case FEED_FILTER_ELEMENT_META_TYPE:
    case FEED_FILTER_ELEMENT_USER_STATUS:
      condition.value = ToWstr(value_combo_.GetItemData(value_combo_.GetCurSel()));
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
  win::Dc dc = hdc;
  win::Rect rect;

  // Paint background
  GetClientRect(&rect);
  dc.FillRect(rect, ::GetSysColor(COLOR_WINDOW));

  // Paint bottom area
  win::Rect rect_button;
  ::GetClientRect(GetDlgItem(IDCANCEL), &rect_button);
  rect.top = rect.bottom - (rect_button.Height() * 2);
  dc.FillRect(rect, ::GetSysColor(COLOR_BTNFACE));

  // Paint line
  rect.bottom = rect.top + 1;
  dc.FillRect(rect, ::GetSysColor(COLOR_ACTIVEBORDER));
}

////////////////////////////////////////////////////////////////////////////////

void FeedConditionDialog::ChooseElement(int element_index) {
  // Operator
  LPARAM op_data = operator_combo_.GetItemData(operator_combo_.GetCurSel());
  operator_combo_.ResetContent();

  #define ADD_OPERATOR(op) \
    operator_combo_.AddItem(Aggregator.filter_manager.TranslateOperator(op).c_str(), op);

  switch (element_index) {
    case FEED_FILTER_ELEMENT_META_ID:
    case FEED_FILTER_ELEMENT_EPISODE_NUMBER:
    case FEED_FILTER_ELEMENT_META_DATE_START:
    case FEED_FILTER_ELEMENT_META_DATE_END:
    case FEED_FILTER_ELEMENT_META_EPISODES:
      ADD_OPERATOR(FEED_FILTER_OPERATOR_EQUALS);
      ADD_OPERATOR(FEED_FILTER_OPERATOR_NOTEQUALS);
      ADD_OPERATOR(FEED_FILTER_OPERATOR_ISGREATERTHAN);
      ADD_OPERATOR(FEED_FILTER_OPERATOR_ISGREATERTHANOREQUALTO);
      ADD_OPERATOR(FEED_FILTER_OPERATOR_ISLESSTHAN);
      ADD_OPERATOR(FEED_FILTER_OPERATOR_ISLESSTHANOREQUALTO);
      break;
    case FEED_FILTER_ELEMENT_LOCAL_EPISODE_AVAILABLE:
    case FEED_FILTER_ELEMENT_META_STATUS:
    case FEED_FILTER_ELEMENT_META_TYPE:
    case FEED_FILTER_ELEMENT_USER_STATUS:
      ADD_OPERATOR(FEED_FILTER_OPERATOR_EQUALS);
      ADD_OPERATOR(FEED_FILTER_OPERATOR_NOTEQUALS);
      break;
    case FEED_FILTER_ELEMENT_EPISODE_TITLE:
    case FEED_FILTER_ELEMENT_EPISODE_GROUP:
    case FEED_FILTER_ELEMENT_EPISODE_VIDEO_TYPE:
    case FEED_FILTER_ELEMENT_FILE_TITLE:
    case FEED_FILTER_ELEMENT_FILE_CATEGORY:
    case FEED_FILTER_ELEMENT_FILE_DESCRIPTION:
    case FEED_FILTER_ELEMENT_FILE_LINK:
      ADD_OPERATOR(FEED_FILTER_OPERATOR_EQUALS);
      ADD_OPERATOR(FEED_FILTER_OPERATOR_NOTEQUALS);
      ADD_OPERATOR(FEED_FILTER_OPERATOR_BEGINSWITH);
      ADD_OPERATOR(FEED_FILTER_OPERATOR_ENDSWITH);
      ADD_OPERATOR(FEED_FILTER_OPERATOR_CONTAINS);
      ADD_OPERATOR(FEED_FILTER_OPERATOR_NOTCONTAINS);
      break;
    default:
      for (int i = 0; i < FEED_FILTER_OPERATOR_COUNT; i++) {
        ADD_OPERATOR(i);
      }
  }

  #undef ADD_OPERATOR

  int op_index = operator_combo_.FindItemData(op_data);
  if (op_index == CB_ERR)
    op_index = 0;
  operator_combo_.SetCurSel(op_index);

  //////////////////////////////////////////////////////////////////////////////
  // Value

  value_combo_.ResetContent();

  RECT rect;
  value_combo_.GetWindowRect(&rect);
  int width = rect.right - rect.left;
  int height = rect.bottom - rect.top;
  ::ScreenToClient(GetWindowHandle(), reinterpret_cast<LPPOINT>(&rect));

  #define RECREATE_COMBO(style) \
    value_combo_.Create(0, WC_COMBOBOX, nullptr, \
        style | CBS_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_VSCROLL, \
        rect.left, rect.top, width, height * 2, \
        GetWindowHandle(), nullptr, nullptr);

  switch (element_index) {
    case FEED_FILTER_ELEMENT_FILE_CATEGORY:
      RECREATE_COMBO(CBS_DROPDOWN);
      value_combo_.AddString(L"Anime");
      value_combo_.AddString(L"Batch");
      value_combo_.AddString(L"Hentai");
      value_combo_.AddString(L"Non-English");
      value_combo_.AddString(L"Other");
      value_combo_.AddString(L"Raws");
      break;
    case FEED_FILTER_ELEMENT_META_ID:
    case FEED_FILTER_ELEMENT_EPISODE_TITLE: {
      RECREATE_COMBO((element_index == FEED_FILTER_ELEMENT_META_ID ? CBS_DROPDOWNLIST : CBS_DROPDOWN));
      typedef std::pair<int, std::wstring> anime_pair;
      std::vector<anime_pair> title_list;
      for (auto it = AnimeDatabase.items.begin(); it != AnimeDatabase.items.end(); ++it) {
        switch (it->second.GetMyStatus()) {
          case anime::kNotInList:
          case anime::kCompleted:
          case anime::kDropped:
            continue;
          default:
            title_list.push_back(std::make_pair(
                it->second.GetId(),
                AnimeDatabase.FindItem(it->second.GetId())->GetTitle()));
        }
      }
      std::sort(title_list.begin(), title_list.end(),
        [](const anime_pair& a1, const anime_pair& a2) {
          return CompareStrings(a1.second, a2.second) < 0;
        });
      if (element_index == FEED_FILTER_ELEMENT_META_ID)
        value_combo_.AddString(L"(Unknown)");
      foreach_(it, title_list)
        value_combo_.AddItem(it->second.c_str(), it->first);
      break;
    }
    case FEED_FILTER_ELEMENT_META_DATE_START:
    case FEED_FILTER_ELEMENT_META_DATE_END:
      RECREATE_COMBO(CBS_DROPDOWN);
      value_combo_.AddString(static_cast<std::wstring>(GetDate()).c_str());
      value_combo_.SetCueBannerText(L"YYYY-MM-DD");
      break;
    case FEED_FILTER_ELEMENT_META_STATUS:
      RECREATE_COMBO(CBS_DROPDOWNLIST);
      value_combo_.AddItem(anime::TranslateStatus(anime::kAiring).c_str(), anime::kAiring);
      value_combo_.AddItem(anime::TranslateStatus(anime::kFinishedAiring).c_str(), anime::kFinishedAiring);
      value_combo_.AddItem(anime::TranslateStatus(anime::kNotYetAired).c_str(), anime::kNotYetAired);
      break;
    case FEED_FILTER_ELEMENT_META_TYPE:
      RECREATE_COMBO(CBS_DROPDOWNLIST);
      value_combo_.AddItem(anime::TranslateType(anime::kTv).c_str(), anime::kTv);
      value_combo_.AddItem(anime::TranslateType(anime::kOva).c_str(), anime::kOva);
      value_combo_.AddItem(anime::TranslateType(anime::kMovie).c_str(), anime::kMovie);
      value_combo_.AddItem(anime::TranslateType(anime::kSpecial).c_str(), anime::kSpecial);
      value_combo_.AddItem(anime::TranslateType(anime::kOna).c_str(), anime::kOna);
      value_combo_.AddItem(anime::TranslateType(anime::kMusic).c_str(), anime::kMusic);
      break;
    case FEED_FILTER_ELEMENT_USER_STATUS:
      RECREATE_COMBO(CBS_DROPDOWNLIST);
      value_combo_.AddItem(anime::TranslateMyStatus(anime::kNotInList, false).c_str(), anime::kNotInList);
      value_combo_.AddItem(anime::TranslateMyStatus(anime::kWatching, false).c_str(), anime::kWatching);
      value_combo_.AddItem(anime::TranslateMyStatus(anime::kCompleted, false).c_str(), anime::kCompleted);
      value_combo_.AddItem(anime::TranslateMyStatus(anime::kOnHold, false).c_str(), anime::kOnHold);
      value_combo_.AddItem(anime::TranslateMyStatus(anime::kDropped, false).c_str(), anime::kDropped);
      value_combo_.AddItem(anime::TranslateMyStatus(anime::kPlanToWatch, false).c_str(), anime::kPlanToWatch);
      break;
    case FEED_FILTER_ELEMENT_EPISODE_NUMBER:
    case FEED_FILTER_ELEMENT_META_EPISODES:
      RECREATE_COMBO(CBS_DROPDOWN);
      value_combo_.AddString(L"%watched%");
      value_combo_.AddString(L"%total%");
      break;
    case FEED_FILTER_ELEMENT_EPISODE_VERSION:
      RECREATE_COMBO(CBS_DROPDOWN);
      value_combo_.AddString(L"2");
      value_combo_.AddString(L"3");
      value_combo_.AddString(L"4");
      value_combo_.AddString(L"0");
      break;
    case FEED_FILTER_ELEMENT_LOCAL_EPISODE_AVAILABLE:
      RECREATE_COMBO(CBS_DROPDOWNLIST);
      value_combo_.AddString(L"False");
      value_combo_.AddString(L"True");
      break;
    case FEED_FILTER_ELEMENT_EPISODE_VIDEO_RESOLUTION:
      RECREATE_COMBO(CBS_DROPDOWN);
      value_combo_.AddString(L"1080p");
      value_combo_.AddString(L"720p");
      value_combo_.AddString(L"480p");
      value_combo_.AddString(L"400p");
      break;
    case FEED_FILTER_ELEMENT_EPISODE_VIDEO_TYPE:
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

}  // namespace ui