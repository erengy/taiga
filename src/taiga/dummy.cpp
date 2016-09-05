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

#include "sync/service.h"
#include "taiga/dummy.h"
#include "track/recognition.h"

namespace taiga {

class DummyAnime DummyAnime;
class DummyEpisode DummyEpisode;

void DummyAnime::Initialize() {
  SetSource(sync::kMyAnimeList);
  SetId(L"4224", sync::kTaiga);
  SetId(L"4224", sync::kMyAnimeList);
  SetId(L"3532", sync::kHummingbird);
  SetSlug(L"toradora");
  SetTitle(L"Toradora!");
  SetSynonyms(L"Tiger X Dragon");
  SetType(anime::kTv);
  SetEpisodeCount(25);
  SetEpisodeLength(24);
  SetAiringStatus(anime::kFinishedAiring);
  SetDateStart(Date(2008, 10, 01));
  SetDateEnd(Date(2009, 03, 25));
  SetImageUrl(L"https://myanimelist.cdn-dena.com/images/anime/13/22128.jpg");
  AddtoUserList();
  SetMyLastWatchedEpisode(25);
  SetMyScore(10);
  SetMyStatus(anime::kCompleted);
  SetMyTags(L"comedy, romance, drama");
}

void DummyEpisode::Initialize() {
  anime_id = 74164;

  track::recognition::ParseOptions parse_options;
  parse_options.parse_path = true;
  parse_options.streaming_media = false;

  Meow.Parse(L"D:\\Anime\\Toradora!\\"
             L"[TaigaSubs]_Toradora!_-_01v2_-_Tiger_and_Dragon_[DVD][1280x720_H264_AAC][ABCD1234].mkv",
             parse_options, *this);
}

}  // namespace taiga