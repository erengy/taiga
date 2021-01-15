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

#include "ui/list.h"

#include <nstd/compare.hpp>
#include <windows/win/common_controls.h>
#include <windows/win/gdi.h>

#include "base/string.h"
#include "base/time.h"
#include "media/anime_db.h"
#include "media/anime_season.h"
#include "media/anime_util.h"
#include "sync/service.h"
#include "taiga/settings.h"
#include "track/feed.h"

namespace ui {

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
    return nstd::compare<int>(get_volume(str1), get_volume(str2));
  } else if (!is_volume1 && !is_volume2) {
    return nstd::compare<int>(get_last_number_in_range(str1),
                              get_last_number_in_range(str2));
  } else {
    return is_volume1 ? nstd::cmp::less : nstd::cmp::greater;
  }
}

int SortAsNumber(LPCWSTR str1, LPCWSTR str2) {
  return nstd::compare<int>(ToInt(str1), ToInt(str2));
}

int SortListAsRfc822DateTime(LPCWSTR str1, LPCWSTR str2) {
  return nstd::compare<time_t>(ConvertRfc822(str1), ConvertRfc822(str2));
}

int SortAsText(LPCWSTR str1, LPCWSTR str2) {
  return lstrcmpi(str1, str2);
}

////////////////////////////////////////////////////////////////////////////////

int SortListByAiringStatus(const anime::Item& item1, const anime::Item& item2) {
  return nstd::compare<int>(static_cast<int>(item1.GetAiringStatus()),
                            static_cast<int>(item2.GetAiringStatus()));
}

int SortListByMyDate(const Date& date1, const Date& date2) {
  if (date1 != date2)
    if (!anime::IsValidDate(date1) || !anime::IsValidDate(date2))
      return anime::IsValidDate(date2) ? nstd::cmp::less : nstd::cmp::greater;

  return nstd::compare<Date>(date1, date2);
}

int SortListByMyDateStart(const anime::Item& item1, const anime::Item& item2) {
  return SortListByMyDate(item1.GetMyDateStart(), item2.GetMyDateStart());
}

int SortListByMyDateCompleted(const anime::Item& item1, const anime::Item& item2) {
  return SortListByMyDate(item1.GetMyDateEnd(), item2.GetMyDateEnd());
}

int SortListByDateStart(const anime::Item& item1, const anime::Item& item2) {
  Date date1 = item1.GetDateStart();
  Date date2 = item2.GetDateStart();

  auto assume_worst_case = [](Date& date) {
    // Hello.
    // We come from the future.
    if (!date.year())
      date.set_year(static_cast<decltype(date.year())>(-1));
    if (!date.month())
      date.set_month(12);
    if (!date.day())
      date.set_day(31);
  };

  if (date1 != date2) {
    assume_worst_case(date1);
    assume_worst_case(date2);
  }

  return nstd::compare<Date>(date1, date2);
}

int SortListByEpisodeCount(const anime::Item& item1, const anime::Item& item2) {
  return nstd::compare<int>(item1.GetEpisodeCount(), item2.GetEpisodeCount());
}

int SortListByLastUpdated(const anime::Item& item1, const anime::Item& item2) {
  return nstd::compare<time_t>(ToTime(item1.GetMyLastUpdated()),
                               ToTime(item2.GetMyLastUpdated()));
}

int SortListByPopularity(const anime::Item& item1, const anime::Item& item2) {
  const int val1 = item1.GetPopularity();
  const int val2 = item2.GetPopularity();

  if (val1 != val2)
    if (val1 == 0 || val2 == 0)
      return val2 == 0 ? nstd::cmp::less : nstd::cmp::greater;

  return nstd::compare<int>(val1, val2);
}

int SortListByProgress(const anime::Item& item1, const anime::Item& item2) {
  float ratio1, unused1;
  float ratio2, unused2;
  anime::GetProgressRatios(item1, unused1, ratio1);
  anime::GetProgressRatios(item2, unused2, ratio2);

  if (ratio1 != ratio2) {
    return nstd::compare<float>(ratio1, ratio2);
  } else {
    return nstd::compare<int>(anime::EstimateEpisodeCount(item1),
                              anime::EstimateEpisodeCount(item2));
  }
}

int SortListByMyScore(const anime::Item& item1, const anime::Item& item2) {
  return nstd::compare<double>(item1.GetMyScore(), item2.GetMyScore());
}

int SortListByScore(const anime::Item& item1, const anime::Item& item2) {
  return nstd::compare<double>(item1.GetScore(), item2.GetScore());
}

int SortListByTitle(const anime::Item& item1, const anime::Item& item2) {
  return CompareStrings(anime::GetPreferredTitle(item1),
                        anime::GetPreferredTitle(item2));
}

int SortListBySeason(const anime::Item& item1, const anime::Item& item2) {
  return nstd::compare<anime::Season>(anime::Season{item1.GetDateStart()},
                                      anime::Season{item2.GetDateStart()});
}

////////////////////////////////////////////////////////////////////////////////

int SortList(int type, LPCWSTR str1, LPCWSTR str2) {
  switch (type) {
    case kListSortDefault:
    default:
      return SortAsText(str1, str2);
    case kListSortEpisodeRange:
      return SortAsEpisodeRange(str1, str2);
    case kListSortNumber:
      return SortAsNumber(str1, str2);
    case kListSortRfc822DateTime:
      return SortListAsRfc822DateTime(str1, str2);
  }

  return nstd::cmp::equal;
}

int SortList(int type, int order, int id1, int id2) {
  const auto item1 = anime::db.Find(id1);
  const auto item2 = anime::db.Find(id2);

  if (item1 && item2) {
    switch (type) {
      case kListSortMyDateStart:
        return SortListByMyDateStart(*item1, *item2);
      case kListSortMyDateCompleted:
        return SortListByMyDateCompleted(*item1, *item2);
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
        return SortListBySeason(*item1, *item2);
      case kListSortStatus:
        return SortListByAiringStatus(*item1, *item2);
      case kListSortTitle:
        return SortListByTitle(*item1, *item2);
    }
  }

  return nstd::cmp::equal;
}

////////////////////////////////////////////////////////////////////////////////

static int ListViewCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort,
                           bool secondary) {
  if (!lParamSort)
    return nstd::cmp::equal;

  const auto list = reinterpret_cast<win::ListView*>(lParamSort);
  int return_value = nstd::cmp::equal;

  switch (list->GetSortType(secondary)) {
    case kListSortDefault:
    case kListSortEpisodeRange:
    case kListSortNumber:
    case kListSortRfc822DateTime:
    default: {
      WCHAR str1[MAX_PATH];
      WCHAR str2[MAX_PATH];
      list->GetItemText(lParam1, list->GetSortColumn(secondary), str1);
      list->GetItemText(lParam2, list->GetSortColumn(secondary), str2);
      return_value = SortList(list->GetSortType(secondary), str1, str2);
      break;
    }

    case kListSortFileSize: {
      const auto item1 = reinterpret_cast<track::FeedItem*>(list->GetItemParam(lParam1));
      const auto item2 = reinterpret_cast<track::FeedItem*>(list->GetItemParam(lParam2));
      if (item1 && item2)
        return_value = nstd::compare<UINT64>(item1->file_size, item2->file_size);
      break;
    }

    case kListSortMyDateCompleted:
    case kListSortMyDateStart:
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
      return_value = SortList(list->GetSortType(secondary),
                              list->GetSortOrder(secondary),
                              static_cast<int>(list->GetItemParam(lParam1)),
                              static_cast<int>(list->GetItemParam(lParam2)));
      break;
    }
  }

  if (!secondary && return_value == nstd::cmp::equal) {
    if (list->GetSortColumn(false) != list->GetSortColumn(true)) {
      return ListViewCompare(lParam1, lParam2, lParamSort, true);
    }
  }

  return return_value * list->GetSortOrder(secondary);
}

int CALLBACK ListViewCompareProc(LPARAM lParam1, LPARAM lParam2,
                                 LPARAM lParamSort) {
  return ListViewCompare(lParam1, lParam2, lParamSort, false);
}

int CALLBACK AnimeListCompareProc(LPARAM lParam1, LPARAM lParam2,
                                  LPARAM lParamSort) {
  if (taiga::settings.GetAppListHighlightNewEpisodes() &&
      taiga::settings.GetAppListDisplayHighlightedOnTop()) {
    const auto list = reinterpret_cast<win::ListView*>(lParamSort);
    const auto item1 = anime::db.Find(list->GetItemParam(lParam1));
    const auto item2 = anime::db.Find(list->GetItemParam(lParam2));
    if (item1 && item2) {
      bool available1 = item1->IsNextEpisodeAvailable();
      bool available2 = item2->IsNextEpisodeAvailable();
      if (available1 != available2)
        return nstd::compare<bool>(!available1, !available2);
    }
  }

  return ListViewCompare(lParam1, lParam2, lParamSort, false);
}

////////////////////////////////////////////////////////////////////////////////

int GetAnimeIdFromSelectedListItem(win::ListView& listview) {
  const int anime_id = static_cast<int>(GetParamFromSelectedListItem(listview));
  return anime::IsValidId(anime_id) ? anime_id : anime::ID_UNKNOWN;
}

std::vector<int> GetAnimeIdsFromSelectedListItems(win::ListView& listview) {
  std::vector<int> anime_ids;

  const auto params = GetParamsFromSelectedListItems(listview);
  for (const auto& param : params) {
    anime_ids.push_back(static_cast<int>(param));
  }

  return anime_ids;
}

LPARAM GetParamFromSelectedListItem(win::ListView& listview) {
  int index = listview.GetNextItem(-1, LVIS_SELECTED);

  if (index > -1) {
    if (listview.GetSelectedCount() > 1) {
      const int focused_index = listview.GetNextItem(-1, LVIS_FOCUSED);
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
  const int item_index = listview.GetNextItem(-1, LVIS_SELECTED);

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
