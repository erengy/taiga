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

#include "list.h"

#include "base/comparable.h"
#include "base/string.h"
#include "base/time.h"
#include "library/anime_db.h"
#include "library/anime_season.h"
#include "library/anime_util.h"
#include "sync/service.h"
#include "taiga/settings.h"

#include "win/ctrl/win_ctrl.h"
#include "win/win_gdi.h"

namespace ui {

template<class T>
static int CompareValues(const T& first, const T& second) {
  if (first != second)
    return first < second ? base::kLessThan : base::kGreaterThan;
  return base::kEqualTo;
}

////////////////////////////////////////////////////////////////////////////////

int SortAsEpisodeRange(const std::wstring& str1, const std::wstring& str2) {
  auto is_volume = [](const std::wstring& str) {
    return !str.empty() && !IsNumericChar(str.front());
  };
  auto get_last_number_in_range = [](const std::wstring& str) {
    auto pos = InStr(str, L"-");
    return pos > -1 ? ToInt(str.substr(pos + 1)) : ToInt(str);
  };
  auto get_volume = [&](const std::wstring& str) {
    auto pos = InStr(str, L" ");
    return pos > -1 ? get_last_number_in_range(str.substr(pos + 1)) : 0;
  };

  bool is_volume1 = is_volume(str1);
  bool is_volume2 = is_volume(str2);

  if (is_volume1 && is_volume2) {
    return CompareValues<int>(get_volume(str1), get_volume(str2));
  } else if (!is_volume1 && !is_volume2) {
    return CompareValues<int>(get_last_number_in_range(str1),
                              get_last_number_in_range(str2));
  } else {
    return is_volume1 ? base::kLessThan : base::kGreaterThan;
  }
}

int SortAsFileSize(LPCWSTR str1, LPCWSTR str2) {
  UINT64 size[2] = {1, 1};

  for (size_t i = 0; i < 2; i++) {
    std::wstring value = i == 0 ? str1 : str2;
    std::wstring unit;

    TrimRight(value, L".\r");
    EraseChars(value, L" ");

    if (value.length() >= 2) {
      for (auto it = value.rbegin(); it != value.rend(); ++it) {
        if (IsNumericChar(*it))
          break;
        unit.insert(unit.begin(), *it);
      }
      value.resize(value.length() - unit.length());
      Trim(unit);
    }

    int index = InStr(value, L".");
    if (index > -1) {
      int length = value.substr(index + 1).length();
      if (length <= 2)
        value.append(2 - length, '0');
      EraseChars(value, L".");
    } else {
      value.append(2, '0');
    }

    if (IsEqual(unit, L"KB")) {
      size[i] *= 1000;
    } else if (IsEqual(unit, L"KiB")) {
      size[i] *= 1024;
    } else if (IsEqual(unit, L"MB")) {
      size[i] *= 1000 * 1000;
    } else if (IsEqual(unit, L"MiB")) {
      size[i] *= 1024 * 1024;
    } else if (IsEqual(unit, L"GB")) {
      size[i] *= 1000 * 1000 * 1000;
    } else if (IsEqual(unit, L"GiB")) {
      size[i] *= 1024 * 1024 * 1024;
    }

    size[i] *= _wtoi(value.c_str());
  }

  return CompareValues<UINT64>(size[0], size[1]);
}

int SortAsNumber(LPCWSTR str1, LPCWSTR str2) {
  return CompareValues<int>(ToInt(str1), ToInt(str2));
}

int SortListAsRfc822DateTime(LPCWSTR str1, LPCWSTR str2) {
  return CompareValues<time_t>(ConvertRfc822(str1), ConvertRfc822(str2));
}

int SortAsText(LPCWSTR str1, LPCWSTR str2) {
  return lstrcmpi(str1, str2);
}

////////////////////////////////////////////////////////////////////////////////

int SortListByAiringStatus(const anime::Item& item1, const anime::Item& item2) {
  return CompareValues<int>(item1.GetAiringStatus(), item2.GetAiringStatus());
}

int SortListByDateStart(const anime::Item& item1, const anime::Item& item2) {
  Date date1 = item1.GetDateStart();
  Date date2 = item2.GetDateStart();

  if (date1 != date2) {
    if (!date1.year)
      date1.year = static_cast<unsigned short>(-1);  // Hello.
    if (!date2.year)
      date2.year = static_cast<unsigned short>(-1);  // We come from the future.
    if (!date1.month)
      date1.month = 12;
    if (!date2.month)
      date2.month = 12;
    if (!date1.day)
      date1.day = 31;
    if (!date2.day)
      date2.day = 31;
  }

  return CompareValues<Date>(date1, date2);
}

int SortListByEpisodeCount(const anime::Item& item1, const anime::Item& item2) {
  return CompareValues<int>(item1.GetEpisodeCount(), item2.GetEpisodeCount());
}

int SortListByLastUpdated(const anime::Item& item1, const anime::Item& item2) {
  return CompareValues<time_t>(_wtoi64(item1.GetMyLastUpdated().c_str()),
                               _wtoi64(item2.GetMyLastUpdated().c_str()));
}

int SortListByPopularity(const anime::Item& item1, const anime::Item& item2) {
  int val1 = item1.GetPopularity();
  int val2 = item2.GetPopularity();

  if (val1 != val2)
    if (val1 == 0 || val2 == 0)
      return val2 == 0 ? base::kLessThan : base::kGreaterThan;

  return CompareValues<int>(val1, val2);
}

int SortListByProgress(const anime::Item& item1, const anime::Item& item2) {
  float ratio1, unused1;
  float ratio2, unused2;
  anime::GetProgressRatios(item1, unused1, ratio1);
  anime::GetProgressRatios(item2, unused2, ratio2);

  if (ratio1 != ratio2) {
    return CompareValues<float>(ratio1, ratio2);
  } else {
    return CompareValues<int>(anime::EstimateEpisodeCount(item1),
                              anime::EstimateEpisodeCount(item2));
  }
}

int SortListByMyScore(const anime::Item& item1, const anime::Item& item2) {
  return CompareValues<double>(item1.GetMyScore(), item2.GetMyScore());
}

int SortListByScore(const anime::Item& item1, const anime::Item& item2) {
  return CompareValues<double>(item1.GetScore(), item2.GetScore());
}

int SortListByTitle(const anime::Item& item1, const anime::Item& item2) {
  if (Settings.GetBool(taiga::kApp_List_DisplayEnglishTitles)) {
    return CompareStrings(item1.GetEnglishTitle(true),
                          item2.GetEnglishTitle(true));
  } else {
    return CompareStrings(item1.GetTitle(), item2.GetTitle());
  }
}

int SortListBySeason(const anime::Item& item1, const anime::Item& item2,
                     int order) {
  anime::Season season1(item1.GetDateStart());
  anime::Season season2(item2.GetDateStart());

  if (season1 != season2)
    return CompareValues<anime::Season>(season1, season2);

  if (item1.GetAiringStatus() != item2.GetAiringStatus())
    return SortListByAiringStatus(item1, item2);

  return SortListByTitle(item1, item2) * order;
}

////////////////////////////////////////////////////////////////////////////////

int SortList(int type, LPCWSTR str1, LPCWSTR str2) {
  switch (type) {
    case kListSortDefault:
    default:
      return SortAsText(str1, str2);
    case kListSortEpisodeRange:
      return SortAsEpisodeRange(str1, str2);
    case kListSortFileSize:
      return SortAsFileSize(str1, str2);
    case kListSortNumber:
      return SortAsNumber(str1, str2);
    case kListSortRfc822DateTime:
      return SortListAsRfc822DateTime(str1, str2);
  }

  return base::kEqualTo;
}

int SortList(int type, int order, int id1, int id2) {
  auto item1 = AnimeDatabase.FindItem(id1);
  auto item2 = AnimeDatabase.FindItem(id2);

  if (item1 && item2) {
    switch (type) {
      case kListSortDateStart:
        return SortListByDateStart(*item1, *item2);
      case kListSortEpisodeCount:
        return SortListByEpisodeCount(*item1, *item2);
      case kListSortLastUpdated:
        return SortListByLastUpdated(*item1, *item2);
      case kListSortPopularity:
        return SortListByPopularity(*item1, *item2);
      case kListSortProgress:
        return SortListByProgress(*item1, *item2);
      case kListSortMyScore:
        return SortListByMyScore(*item1, *item2);
      case kListSortScore:
        return SortListByScore(*item1, *item2);
      case kListSortSeason:
        return SortListBySeason(*item1, *item2, order);
      case kListSortStatus:
        return SortListByAiringStatus(*item1, *item2);
      case kListSortTitle:
        return SortListByTitle(*item1, *item2);
    }
  }

  return base::kEqualTo;
}

////////////////////////////////////////////////////////////////////////////////

int CALLBACK ListViewCompareProc(LPARAM lParam1, LPARAM lParam2,
                                 LPARAM lParamSort) {
  if (!lParamSort)
    return base::kEqualTo;

  win::ListView* list = reinterpret_cast<win::ListView*>(lParamSort);
  int return_value = base::kEqualTo;

  switch (list->GetSortType()) {
    case kListSortDefault:
    case kListSortEpisodeRange:
    case kListSortFileSize:
    case kListSortNumber:
    case kListSortRfc822DateTime:
    default: {
      WCHAR str1[MAX_PATH];
      WCHAR str2[MAX_PATH];
      list->GetItemText(lParam1, list->GetSortColumn(), str1);
      list->GetItemText(lParam2, list->GetSortColumn(), str2);
      return_value = SortList(list->GetSortType(), str1, str2);
      break;
    }

    case kListSortDateStart:
    case kListSortEpisodeCount:
    case kListSortLastUpdated:
    case kListSortPopularity:
    case kListSortProgress:
    case kListSortMyScore:
    case kListSortScore:
    case kListSortSeason:
    case kListSortStatus:
    case kListSortTitle: {
      return_value = SortList(list->GetSortType(), list->GetSortOrder(),
                              static_cast<int>(list->GetItemParam(lParam1)),
                              static_cast<int>(list->GetItemParam(lParam2)));
      break;
    }
  }

  return return_value * list->GetSortOrder();
}


int CALLBACK AnimeListCompareProc(LPARAM lParam1, LPARAM lParam2,
                                  LPARAM lParamSort) {
  if (Settings.GetBool(taiga::kApp_List_HighlightNewEpisodes) &&
      Settings.GetBool(taiga::kApp_List_DisplayHighlightedOnTop)) {
    win::ListView* list = reinterpret_cast<win::ListView*>(lParamSort);
    auto item1 = AnimeDatabase.FindItem(list->GetItemParam(lParam1));
    auto item2 = AnimeDatabase.FindItem(list->GetItemParam(lParam2));
    if (item1 && item2) {
      bool available1 = item1->IsNextEpisodeAvailable();
      bool available2 = item2->IsNextEpisodeAvailable();
      if (available1 != available2)
        return CompareValues<bool>(!available1, !available2);
    }
  }

  return ListViewCompareProc(lParam1, lParam2, lParamSort);
}

////////////////////////////////////////////////////////////////////////////////

int GetAnimeIdFromSelectedListItem(win::ListView& listview) {
  int anime_id = static_cast<int>(GetParamFromSelectedListItem(listview));
  return anime::IsValidId(anime_id) ? anime_id : anime::ID_UNKNOWN;
}

std::vector<int> GetAnimeIdsFromSelectedListItems(win::ListView& listview) {
  std::vector<int> anime_ids;

  auto params = GetParamsFromSelectedListItems(listview);
  for (const auto& param : params) {
    anime_ids.push_back(static_cast<int>(param));
  }

  return anime_ids;
}

LPARAM GetParamFromSelectedListItem(win::ListView& listview) {
  int index = listview.GetNextItem(-1, LVIS_SELECTED);

  if (index > -1) {
    if (listview.GetSelectedCount() > 1) {
      int focused_index = listview.GetNextItem(-1, LVIS_FOCUSED);
      if (focused_index > -1)
        index = focused_index;
    }
    return listview.GetItemParam(index);
  }

  return 0;
}

std::vector<LPARAM> GetParamsFromSelectedListItems(win::ListView& listview) {
  std::vector<LPARAM> params;

  int index = -1;
  while ((index = listview.GetNextItem(index, LVIS_SELECTED)) > -1) {
    params.push_back(listview.GetItemParam(index));
  };

  return params;
}

////////////////////////////////////////////////////////////////////////////////

void GetPopupMenuPositionForSelectedListItem(win::ListView& listview,
                                             POINT& pt) {
  int item_index = listview.GetNextItem(-1, LVIS_SELECTED);

  win::Rect rect;
  listview.GetSubItemRect(item_index, 0, &rect);

  pt = {rect.left, rect.bottom};
  ClientToScreen(listview.GetWindowHandle(), &pt);
}

bool HitTestListHeader(win::ListView& listview, POINT pt) {
  HDHITTESTINFO hti = {0};
  hti.pt = pt;
  ScreenToClient(listview.GetHeader(), &hti.pt);

  SendMessage(listview.GetHeader(), HDM_HITTEST, 0,
              reinterpret_cast<LPARAM>(&hti));

  return (hti.flags & (HHT_NOWHERE | HHT_ONHEADER | HHT_ONDIVIDER)) > 0;
}

}  // namespace ui