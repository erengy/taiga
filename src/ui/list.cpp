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

#include "base/string.h"
#include "base/time.h"
#include "library/anime_db.h"
#include "taiga/settings.h"

#include "win/ctrl/win_ctrl.h"

namespace ui {

int SortAsFileSize(LPCWSTR str1, LPCWSTR str2) {
  UINT64 size[2] = {1, 1};

  for (size_t i = 0; i < 2; i++) {
    std::wstring value = i == 0 ? str1 : str2;
    std::wstring unit;

    TrimRight(value, L".\r");
    EraseChars(value, L" ");

    if (value.length() >= 2) {
      for (auto it = value.rbegin(); it != value.rend(); ++it) {
        if (IsNumeric(*it))
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

  if (size[0] > size[1]) {
    return 1;
  } else if (size[0] < size[1]) {
    return -1;
  }

  return 0;
}

int SortAsNumber(LPCWSTR str1, LPCWSTR str2) {
  int num1 = _wtoi(str1);
  int num2 = _wtoi(str2);

  if (num1 > num2) {
    return 1;
  } else if (num1 < num2) {
    return -1;
  }

  return 0;
}

int SortAsText(LPCWSTR str1, LPCWSTR str2) {
  return lstrcmpi(str1, str2);
}

////////////////////////////////////////////////////////////////////////////////

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

    return date2 > date1 ? 1 : -1;
  }

  return 0;
}

int SortListByEpisodeCount(const anime::Item& item1, const anime::Item& item2) {
  if (item1.GetEpisodeCount() > item2.GetEpisodeCount()) {
    return 1;
  } else if (item1.GetEpisodeCount() < item2.GetEpisodeCount()) {
    return -1;
  }

  return 0;
}

int SortListByLastUpdated(const anime::Item& item1, const anime::Item& item2) {
  time_t time1 = _wtoi64(item1.GetMyLastUpdated().c_str());
  time_t time2 = _wtoi64(item2.GetMyLastUpdated().c_str());

  if (time1 > time2) {
    return 1;
  } else if (time1 < time2) {
    return -1;
  }

  return 0;
}

int SortListByPopularity(const anime::Item& item1, const anime::Item& item2) {
  int val1 = 0;
  int val2 = 0;

  if (!item1.GetPopularity().empty())
    val1 = _wtoi(item1.GetPopularity().substr(1).c_str());
  if (!item2.GetPopularity().empty())
    val2 = _wtoi(item2.GetPopularity().substr(1).c_str());

  if (val2 == 0) {
    return -1;
  } else if (val1 == 0) {
    return 1;
  } else if (val1 > val2) {
    return 1;
  } else if (val1 < val2) {
    return -1;
  }

  return 0;
}

int SortListByProgress(const anime::Item& item1, const anime::Item& item2) {
  int total1 = item1.GetEpisodeCount();
  int total2 = item2.GetEpisodeCount();
  int watched1 = item1.GetMyLastWatchedEpisode();
  int watched2 = item2.GetMyLastWatchedEpisode();
  bool available1 = item1.IsNewEpisodeAvailable();
  bool available2 = item2.IsNewEpisodeAvailable();

  if (available1 && !available2) {
    return -1;
  } else if (!available1 && available2) {
    return 1;
  } else if (total1 && total2) {
    float ratio1 = static_cast<float>(watched1) / static_cast<float>(total1);
    float ratio2 = static_cast<float>(watched2) / static_cast<float>(total2);
    if (ratio1 > ratio2) {
      return -1;
    } else if (ratio1 < ratio2) {
      return 1;
    } else {
      if (total1 > total2) {
        return -1;
      } else if (total1 < total2) {
        return 1;
      }
    }
  } else {
    if (watched1 > watched2) {
      return -1;
    } else if (watched1 < watched2) {
      return 1;
    } else {
      if (total1 > total2) {
        return -1;
      } else if (total1 < total2) {
        return 1;
      }
    }
  }

  return 0;
}

int SortListByScore(const anime::Item& item1, const anime::Item& item2) {
  return lstrcmpi(item1.GetScore().c_str(), item2.GetScore().c_str());
}

int SortListByTitle(const anime::Item& item1, const anime::Item& item2) {
  if (Settings.GetBool(taiga::kApp_List_DisplayEnglishTitles)) {
    return CompareStrings(item1.GetEnglishTitle(true),
                          item2.GetEnglishTitle(true));
  } else {
    return CompareStrings(item1.GetTitle(), item2.GetTitle());
  }
}

////////////////////////////////////////////////////////////////////////////////

int SortList(int type, LPCWSTR str1, LPCWSTR str2) {
  switch (type) {
    case kListSortDefault:
    default:
      return SortAsText(str1, str2);
    case kListSortFileSize:
      return SortAsFileSize(str1, str2);
    case kListSortNumber:
      return SortAsNumber(str1, str2);
  }

  return 0;
}

int SortList(int type, int id1, int id2) {
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
      case kListSortScore:
        return SortListByScore(*item1, *item2);
      case kListSortTitle:
        return SortListByTitle(*item1, *item2);
    }
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int CALLBACK ListViewCompareProc(LPARAM lParam1, LPARAM lParam2,
                                 LPARAM lParamSort) {
  if (!lParamSort)
    return 0;

  win::ListView* list = reinterpret_cast<win::ListView*>(lParamSort);
  int return_value = 0;

  switch (list->GetSortType()) {
    case kListSortDefault:
    case kListSortFileSize:
    case kListSortNumber:
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
    case kListSortScore:
    case kListSortTitle: {
      return_value = SortList(list->GetSortType(),
                              static_cast<int>(list->GetItemParam(lParam1)),
                              static_cast<int>(list->GetItemParam(lParam2)));
      break;
    }
  }

  return return_value * list->GetSortOrder();
}

}  // namespace ui