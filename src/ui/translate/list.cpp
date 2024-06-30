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

}  // namespace ui
