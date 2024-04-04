/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "taiga/dummy.h"

#include "sync/service.h"
#include "track/recognition.h"

namespace taiga {

void InitializeDummies() {
  dummy_anime.SetSource(sync::GetCurrentServiceId());
  dummy_anime.SetId(L"4224", sync::ServiceId::MyAnimeList);
  dummy_anime.SetId(L"3532", sync::ServiceId::Kitsu);
  dummy_anime.SetId(L"4224", sync::ServiceId::AniList);
  dummy_anime.SetSlug(L"toradora");
  dummy_anime.SetTitle(L"Toradora!");
  dummy_anime.SetEnglishTitle(L"Toradora!");
  dummy_anime.SetJapaneseTitle(L"\u3068\u3089\u30C9\u30E9\uFF01");
  dummy_anime.SetSynonyms(L"Tiger X Dragon");
  dummy_anime.SetType(anime::SeriesType::Tv);
  dummy_anime.SetEpisodeCount(25);
  dummy_anime.SetEpisodeLength(24);
  dummy_anime.SetAiringStatus(anime::SeriesStatus::FinishedAiring);
  dummy_anime.SetDateStart(Date(2008, 10, 1));
  dummy_anime.SetDateEnd(Date(2009, 3, 25));
  dummy_anime.SetImageUrl(
      L"https://cdn.myanimelist.net/images/anime/13/22128.jpg");
  dummy_anime.AddtoUserList();
  dummy_anime.SetMyLastWatchedEpisode(25);
  dummy_anime.SetMyScore(anime::kUserScoreMax);
  dummy_anime.SetMyStatus(anime::MyStatus::Completed);
  dummy_anime.SetMyNotes(L"Best series ever!");

  dummy_episode.anime_id = 74164;

  track::recognition::ParseOptions parse_options;
  parse_options.parse_path = true;
  parse_options.streaming_media = false;

  Meow.Parse(L"D:\\Anime\\Toradora!\\"
             L"[TaigaSubs]_Toradora!_-_01v2_-_Tiger_and_Dragon_[DVD][1280x720_H264_AAC][ABCD1234].mkv",
             parse_options, dummy_episode);
}

}  // namespace taiga
