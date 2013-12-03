/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

#include "library/anime_db.h"
#include "taiga/settings.h"
#include "base/string.h"
#include "base/time.h"

#include "win/ctrl/win_ctrl.h"

namespace ui {

int CALLBACK ListViewCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
  if (!lParamSort) return 0;
  
  win::ListView* m_List = reinterpret_cast<win::ListView*>(lParamSort);
  int return_value = 0;

  switch (m_List->GetSortType()) {
    // Number
    case kListSortNumber: {
      WCHAR szItem1[MAX_PATH], szItem2[MAX_PATH];
      m_List->GetItemText(lParam1, m_List->GetSortColumn(), szItem1);
      m_List->GetItemText(lParam2, m_List->GetSortColumn(), szItem2);
      int iItem1 = _wtoi(szItem1);
      int iItem2 = _wtoi(szItem2);
      if (iItem1 > iItem2) {
        return_value = 1;
      } else if (iItem1 < iItem2) {
        return_value = -1;
      }
      break;
    }

    // Popularity
    case kListSortPopularity: {
      auto pItem1 = AnimeDatabase.FindItem(static_cast<int>(m_List->GetItemParam(lParam1)));
      auto pItem2 = AnimeDatabase.FindItem(static_cast<int>(m_List->GetItemParam(lParam2)));
      if (pItem1 && pItem2) {
        int iItem1 = pItem1->GetPopularity().empty() ? 0 : _wtoi(pItem1->GetPopularity().substr(1).c_str());
        int iItem2 = pItem2->GetPopularity().empty() ? 0 : _wtoi(pItem2->GetPopularity().substr(1).c_str());
        if (iItem2 == 0) {
          return_value = -1;
        } else if (iItem1 == 0) {
          return_value = 1;
        } else if (iItem1 > iItem2) {
          return_value = 1;
        } else if (iItem1 < iItem2) {
          return_value = -1;
        }
      }
      break;
    }

    // Progress
    case kListSortProgress: {
      auto pItem1 = AnimeDatabase.FindItem(static_cast<int>(m_List->GetItemParam(lParam1)));
      auto pItem2 = AnimeDatabase.FindItem(static_cast<int>(m_List->GetItemParam(lParam2)));
      if (pItem1 && pItem2) {
        int total1 = pItem1->GetEpisodeCount();
        int total2 = pItem2->GetEpisodeCount();
        int watched1 = pItem1->GetMyLastWatchedEpisode();
        int watched2 = pItem2->GetMyLastWatchedEpisode();
        bool available1 = pItem1->IsNewEpisodeAvailable();
        bool available2 = pItem2->IsNewEpisodeAvailable();
        if (available1 && !available2) {
          return_value = -1;
        } else if (!available1 && available2) {
          return_value = 1;
        } else if (total1 && total2) {
          float ratio1 = static_cast<float>(watched1) / static_cast<float>(total1);
          float ratio2 = static_cast<float>(watched2) / static_cast<float>(total2);
          if (ratio1 > ratio2) {
            return_value = -1;
          } else if (ratio1 < ratio2) {
            return_value = 1;
          }
        } else {
          if (watched1 > watched2) {
            return_value = -1;
          } else if (watched1 < watched2) {
            return_value = 1;
          }
        }
      }
      break;
    }

    // Episodes
    case kListSortEpisodes: {
      auto pItem1 = AnimeDatabase.FindItem(static_cast<int>(m_List->GetItemParam(lParam1)));
      auto pItem2 = AnimeDatabase.FindItem(static_cast<int>(m_List->GetItemParam(lParam2)));
      if (pItem1 && pItem2) {
        if (pItem1->GetEpisodeCount() > pItem2->GetEpisodeCount()) {
          return_value = 1;
        } else if (pItem1->GetEpisodeCount() < pItem2->GetEpisodeCount()) {
          return_value = -1;
        }
      }
      break;
    }

    // Score
    case kListSortScore: {
      auto pItem1 = AnimeDatabase.FindItem(static_cast<int>(m_List->GetItemParam(lParam1)));
      auto pItem2 = AnimeDatabase.FindItem(static_cast<int>(m_List->GetItemParam(lParam2)));
      if (pItem1 && pItem2) {
        return_value = lstrcmpi(pItem1->GetScore().c_str(), pItem2->GetScore().c_str());
      }
      break;
    }
    
    // Start date
    case kListSortStartDate: {
      auto pItem1 = AnimeDatabase.FindItem(static_cast<int>(m_List->GetItemParam(lParam1)));
      auto pItem2 = AnimeDatabase.FindItem(static_cast<int>(m_List->GetItemParam(lParam2)));
      if (pItem1 && pItem2) {
        Date date1 = pItem1->GetDate(anime::DATE_START);
        Date date2 = pItem2->GetDate(anime::DATE_START);
        if (date1 != date2) {
          if (!date1.year) date1.year = static_cast<unsigned short>(-1); // Hello.
          if (!date2.year) date2.year = static_cast<unsigned short>(-1); // We come from the future.
          if (!date1.month) date1.month = 12;
          if (!date2.month) date2.month = 12;
          if (!date1.day) date1.day = 31;
          if (!date2.day) date2.day = 31;
          return_value = date2 > date1 ? 1 : -1;
        }
      }
      break;
    }

    // Last updated
    case kListSortLastUpdated: {
      auto pItem1 = AnimeDatabase.FindItem(static_cast<int>(m_List->GetItemParam(lParam1)));
      auto pItem2 = AnimeDatabase.FindItem(static_cast<int>(m_List->GetItemParam(lParam2)));
      if (pItem1 && pItem2) {
        time_t time1 = _wtoi64(pItem1->GetMyLastUpdated().c_str());
        time_t time2 = _wtoi64(pItem2->GetMyLastUpdated().c_str());
        if (time1 > time2) {
          return_value = 1;
        } else if (time1 < time2) {
          return_value = -1;
        }
      }
      break;
    }

    // File size
    case kListSortFileSize: {
      wstring item[2], unit[2];
      UINT64 size[2] = {1, 1};
      m_List->GetItemText(lParam1, m_List->GetSortColumn(), item[0]);
      m_List->GetItemText(lParam2, m_List->GetSortColumn(), item[1]);
      for (size_t i = 0; i < 2; i++) {
        TrimRight(item[i], L".\r");
        EraseChars(item[i], L" ");
        if (item[i].length() >= 2) {
          for (auto it = item[i].rbegin(); it != item[i].rend(); ++it) {
            if (IsNumeric(*it))
              break;
            unit[i].insert(unit[i].begin(), *it);
          }
          item[i].resize(item[i].length() - unit[i].length());
          Trim(unit[i]);
        }
        int index = InStr(item[i], L".");
        if (index > -1) {
          int length = item[i].substr(index + 1).length();
          if (length <= 2) item[i].append(2 - length, '0');
          EraseChars(item[i], L".");
        } else {
          item[i].append(2, '0');
        }
        if (IsEqual(unit[i], L"KB")) {
          size[i] *= 1000;
        } else if (IsEqual(unit[i], L"KiB")) {
          size[i] *= 1024;
        } else if (IsEqual(unit[i], L"MB")) {
          size[i] *= 1000 * 1000;
        } else if (IsEqual(unit[i], L"MiB")) {
          size[i] *= 1024 * 1024;
        } else if (IsEqual(unit[i], L"GB")) {
          size[i] *= 1000 * 1000 * 1000;
        } else if (IsEqual(unit[i], L"GiB")) {
          size[i] *= 1024 * 1024 * 1024;
        }
        size[i] *= _wtoi(item[i].c_str());
      }
      if (size[0] > size[1]) {
        return_value = 1;
      } else if (size[0] < size[1]) {
        return_value = -1;
      }
      break;
    }

    // Title
    case kListSortTitle: {
      auto pItem1 = AnimeDatabase.FindItem(static_cast<int>(m_List->GetItemParam(lParam1)));
      auto pItem2 = AnimeDatabase.FindItem(static_cast<int>(m_List->GetItemParam(lParam2)));
      if (pItem1 && pItem2) {
        if (Settings.Program.List.english_titles) {
          return_value = CompareStrings(pItem1->GetEnglishTitle(true), pItem2->GetEnglishTitle(true));
        } else {
          return_value = CompareStrings(pItem1->GetTitle(), pItem2->GetTitle());
        }
      }
      break;
    }

    // Text
    case kListSortDefault:
    default:
      WCHAR szItem1[MAX_PATH], szItem2[MAX_PATH];
      m_List->GetItemText(lParam1, m_List->GetSortColumn(), szItem1);
      m_List->GetItemText(lParam2, m_List->GetSortColumn(), szItem2);
      return_value = lstrcmpi(szItem1, szItem2);
  }

  return return_value * m_List->GetSortOrder();
}

}  // namespace ui