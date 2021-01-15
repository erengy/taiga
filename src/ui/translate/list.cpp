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

#include <string>

#include "ui/translate.h"

#include "base/format.h"
#include "base/string.h"
#include "base/time.h"
#include "media/anime.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "sync/anilist_ratings.h"
#include "sync/anilist_util.h"
#include "sync/kitsu_util.h"
#include "sync/myanimelist_util.h"
#include "sync/service.h"
#include "taiga/settings.h"

namespace ui {

std::wstring TranslateMyDate(const Date& value,
                             const std::wstring& default_char) { 
  return anime::IsValidDate(value) ? TranslateDate(value) : default_char;
}

std::wstring TranslateMyScore(const int value,
                              const std::wstring& default_char) {
  if (!value)
    return default_char;

  switch (sync::GetCurrentServiceId()) {
    default:
      return ToWstr(value);
    case sync::ServiceId::MyAnimeList:
      return sync::myanimelist::TranslateMyRating(value, false);
    case sync::ServiceId::Kitsu:
      return sync::kitsu::TranslateMyRating(
          value, sync::kitsu::GetRatingSystem());
    case sync::ServiceId::AniList:
      return sync::anilist::TranslateMyRating(
          value, sync::anilist::GetRatingSystem());
  }
}

std::wstring TranslateMyScoreFull(const int value) {
  switch (sync::GetCurrentServiceId()) {
    default:
      return ToWstr(value);
    case sync::ServiceId::MyAnimeList:
      return sync::myanimelist::TranslateMyRating(value, true);
    case sync::ServiceId::Kitsu:
      return sync::kitsu::TranslateMyRating(
          value, sync::kitsu::GetRatingSystem());
    case sync::ServiceId::AniList:
      return sync::anilist::TranslateMyRating(
          value, sync::anilist::GetRatingSystem());
  }
}

std::wstring TranslateMyStatus(const anime::MyStatus value, bool add_count) {
  const auto with_count = [&value, &add_count](std::wstring str) {
    if (add_count)
      str += L" (" + ToWstr(anime::db.GetItemCount(value)) + L")";
    return str;
  };

  switch (value) {
    case anime::MyStatus::NotInList: return with_count(L"Not in list");
    case anime::MyStatus::Watching: return with_count(L"Currently watching");
    case anime::MyStatus::Completed: return with_count(L"Completed");
    case anime::MyStatus::OnHold: return with_count(L"On hold");
    case anime::MyStatus::Dropped: return with_count(L"Dropped");
    case anime::MyStatus::PlanToWatch: return with_count(L"Plan to watch");
    default: return L"";
  }
}

}  // namespace ui
