/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
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

#include <windows/win/gdi.h>

#include "base/string.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "taiga/resource.h"
#include "track/feed.h"
#include "track/feed_filter.h"
#include "track/feed_filter_util.h"
#include "ui/dlg/dlg_feed_condition.h"
#include "ui/translate.h"

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
  for (int i = 0; i < track::kFeedFilterElement_Count; i++) {
    element_combo_.AddItem(track::util::TranslateElement(i).c_str(), i);
  }

  // Set element
  element_combo_.SetCurSel(condition.element);
  ChooseElement(condition.element);
  // Set operator
  operator_combo_.SetCurSel(operator_combo_.FindItemData(condition.op));
  // Set value
  switch (condition.element) {
    case track::kFeedFilterElement_Meta_Id: {
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
    case track::kFeedFilterElement_User_Status: {
      int value = ToInt(condition.value);
      value_combo_.SetCurSel(value);
      break;
    }
    case track::kFeedFilterElement_Meta_Status:
    case track::kFeedFilterElement_Meta_Type:
      value_combo_.SetCurSel(ToInt(condition.value) - 1);
      break;
    case track::kFeedFilterElement_Local_EpisodeAvailable:
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
      dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
      dc.DetachDc();
      return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_WINDOW));
    }
  }

  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

void FeedConditionDialog::OnCancel() {
  // We will check against this in FeedFilterDialog later on
  condition.element = track::kFeedFilterElement_None;

  EndDialog(IDCANCEL);
}

void FeedConditionDialog::OnOK() {
  // Set values
  condition.element = static_cast<track::FeedFilterElement>(element_combo_.GetCurSel());
  condition.op = static_cast<track::FeedFilterOperator>(
        operator_combo_.GetItemData(operator_combo_.GetCurSel()));
  switch (condition.element) {
    case track::kFeedFilterElement_Meta_Id:
    case track::kFeedFilterElement_Meta_Status:
    case track::kFeedFilterElement_Meta_Type:
    case track::kFeedFilterElement_User_Status:
      condition.value = ToWstr(value_combo_.GetItemData(value_combo_.GetCurSel()));
      break;
    default:
      value_combo_.GetText(condition.value);
  }

  switch (condition.element) {
    case track::kFeedFilterElement_File_Size:
      if (IsNumericString(condition.value))
        condition.value += L" MiB";
      break;
  }

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
    operator_combo_.AddItem(track::util::TranslateOperator(op).c_str(), op);

  switch (element_index) {
    case track::kFeedFilterElement_File_Size:
    case track::kFeedFilterElement_Meta_Id:
    case track::kFeedFilterElement_Episode_Number:
    case track::kFeedFilterElement_Meta_DateStart:
    case track::kFeedFilterElement_Meta_DateEnd:
    case track::kFeedFilterElement_Meta_Episodes:
      ADD_OPERATOR(track::kFeedFilterOperator_Equals);
      ADD_OPERATOR(track::kFeedFilterOperator_NotEquals);
      ADD_OPERATOR(track::kFeedFilterOperator_IsGreaterThan);
      ADD_OPERATOR(track::kFeedFilterOperator_IsGreaterThanOrEqualTo);
      ADD_OPERATOR(track::kFeedFilterOperator_IsLessThan);
      ADD_OPERATOR(track::kFeedFilterOperator_IsLessThanOrEqualTo);
      break;
    case track::kFeedFilterElement_File_Category:
    case track::kFeedFilterElement_Local_EpisodeAvailable:
    case track::kFeedFilterElement_Meta_Status:
    case track::kFeedFilterElement_Meta_Type:
    case track::kFeedFilterElement_User_Status:
      ADD_OPERATOR(track::kFeedFilterOperator_Equals);
      ADD_OPERATOR(track::kFeedFilterOperator_NotEquals);
      break;
    case track::kFeedFilterElement_User_Notes:
    case track::kFeedFilterElement_Episode_Title:
    case track::kFeedFilterElement_Episode_Group:
    case track::kFeedFilterElement_Episode_VideoType:
    case track::kFeedFilterElement_File_Title:
    case track::kFeedFilterElement_File_Description:
    case track::kFeedFilterElement_File_Link:
      ADD_OPERATOR(track::kFeedFilterOperator_Equals);
      ADD_OPERATOR(track::kFeedFilterOperator_NotEquals);
      ADD_OPERATOR(track::kFeedFilterOperator_BeginsWith);
      ADD_OPERATOR(track::kFeedFilterOperator_EndsWith);
      ADD_OPERATOR(track::kFeedFilterOperator_Contains);
      ADD_OPERATOR(track::kFeedFilterOperator_NotContains);
      break;
    default:
      for (int i = 0; i < track::kFeedFilterOperator_Count; i++) {
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

  win::Rect rect;
  value_combo_.GetWindowRect(GetWindowHandle(), &rect);
  value_combo_.ResetContent();

  #define RECREATE_COMBO(style) \
    value_combo_.Create(0, WC_COMBOBOX, nullptr, \
        style | CBS_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_VSCROLL, \
        rect.left, rect.top, rect.Width(), rect.Height() * 2, \
        GetWindowHandle(), nullptr, nullptr);

  switch (element_index) {
    case track::kFeedFilterElement_File_Category: {
      RECREATE_COMBO(CBS_DROPDOWNLIST);
      static const std::vector<track::TorrentCategory> categories{
        track::TorrentCategory::Anime,
        track::TorrentCategory::Batch,
        track::TorrentCategory::Other,
      };
      for (auto category : categories) {
        value_combo_.AddItem(TranslateTorrentCategory(category).c_str(),
                             static_cast<LPARAM>(category));
      }
      break;
    }
    case track::kFeedFilterElement_File_Size: {
      RECREATE_COMBO(CBS_DROPDOWN);
      value_combo_.AddString(L"10 MiB");
      value_combo_.AddString(L"100 MiB");
      value_combo_.AddString(L"1 GiB");
      break;
    }
    case track::kFeedFilterElement_Meta_Id:
    case track::kFeedFilterElement_Episode_Title: {
      RECREATE_COMBO((element_index == track::kFeedFilterElement_Meta_Id ? CBS_DROPDOWNLIST : CBS_DROPDOWN));
      using anime_pair = std::pair<int, std::wstring>;
      std::vector<anime_pair> title_list;
      for (const auto& [id, item] : anime::db.items) {
        switch (item.GetMyStatus()) {
          case anime::MyStatus::NotInList:
          case anime::MyStatus::Completed:
          case anime::MyStatus::Dropped:
            continue;
          default:
            title_list.push_back(std::make_pair(id, anime::GetPreferredTitle(item)));
            break;
        }
      }
      std::sort(title_list.begin(), title_list.end(),
        [](const anime_pair& a1, const anime_pair& a2) {
          return CompareStrings(a1.second, a2.second) < 0;
        });
      if (element_index == track::kFeedFilterElement_Meta_Id)
        value_combo_.AddString(L"(Unknown)");
      for (const auto& pair : title_list)
        value_combo_.AddItem(pair.second.c_str(), pair.first);
      break;
    }
    case track::kFeedFilterElement_Meta_DateStart:
    case track::kFeedFilterElement_Meta_DateEnd:
      RECREATE_COMBO(CBS_DROPDOWN);
      value_combo_.AddString(GetDate().to_string().c_str());
      value_combo_.SetCueBannerText(L"YYYY-MM-DD");
      break;
    case track::kFeedFilterElement_Meta_Status:
      RECREATE_COMBO(CBS_DROPDOWNLIST);
      for (const auto status : anime::kSeriesStatuses) {
        value_combo_.AddItem(ui::TranslateStatus(status).c_str(), static_cast<int>(status));
      }
      break;
    case track::kFeedFilterElement_Meta_Type:
      RECREATE_COMBO(CBS_DROPDOWNLIST);
      for (const auto type : anime::kSeriesTypes) {
        value_combo_.AddItem(ui::TranslateType(type).c_str(), static_cast<int>(type));
      }
      break;
    case track::kFeedFilterElement_User_Status:
      RECREATE_COMBO(CBS_DROPDOWNLIST);
      value_combo_.AddItem(
          ui::TranslateMyStatus(anime::MyStatus::NotInList, false).c_str(),
          static_cast<int>(anime::MyStatus::NotInList));
      for (const auto status : anime::kMyStatuses) {
        value_combo_.AddItem(ui::TranslateMyStatus(status, false).c_str(), static_cast<int>(status));
      }
      break;
    case track::kFeedFilterElement_Episode_Number:
    case track::kFeedFilterElement_Meta_Episodes:
      RECREATE_COMBO(CBS_DROPDOWN);
      value_combo_.AddString(L"%watched%");
      value_combo_.AddString(L"%total%");
      break;
    case track::kFeedFilterElement_Episode_Version:
      RECREATE_COMBO(CBS_DROPDOWN);
      value_combo_.AddString(L"2");
      value_combo_.AddString(L"3");
      value_combo_.AddString(L"4");
      value_combo_.AddString(L"0");
      break;
    case track::kFeedFilterElement_Local_EpisodeAvailable:
      RECREATE_COMBO(CBS_DROPDOWNLIST);
      value_combo_.AddString(L"False");
      value_combo_.AddString(L"True");
      break;
    case track::kFeedFilterElement_Episode_VideoResolution:
      RECREATE_COMBO(CBS_DROPDOWN);
      value_combo_.AddString(L"1080p");
      value_combo_.AddString(L"720p");
      value_combo_.AddString(L"480p");
      value_combo_.AddString(L"400p");
      break;
    case track::kFeedFilterElement_Episode_VideoType:
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
