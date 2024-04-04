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

#include <set>

#include "taiga/script.h"

#include "base/atf.h"
#include "base/string.h"
#include "base/url.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "sync/anilist_util.h"
#include "sync/kitsu_util.h"
#include "sync/myanimelist_util.h"
#include "sync/service.h"
#include "taiga/dummy.h"
#include "taiga/settings.h"
#include "track/episode.h"
#include "track/episode_util.h"
#include "track/media.h"
#include "ui/translate.h"

static const std::set<std::wstring> script_functions = {
      L"and",
      L"cut",
      L"equal",
      L"gequal",
      L"greater",
      L"if",
      L"if2",
      L"ifequal",
      L"lequal",
      L"len",
      L"less",
      L"lower",
      L"not",
      L"num",
      L"or",
      L"pad",
      L"replace",
      L"substr",
      L"triml",
      L"trimr",
      L"upper",
    };

static const std::set<std::wstring> script_variables = {
      L"animeurl",
      L"audio",
      L"checksum",
      L"episode",
      L"file",
      L"folder",
      L"group",
      L"id",
      L"image",
      L"manual",
      L"name",
      L"notes",
      L"playstatus",
      L"resolution",
      L"rewatching",
      L"score",
      L"season",
      L"status",
      L"title",
      L"total",
      L"user",
      L"version",
      L"video",
      L"watched",
    };

bool IsScriptFunction(const std::wstring& str) {
  return script_functions.count(str) > 0;
}

bool IsScriptVariable(const std::wstring& str) {
  return script_variables.count(str) > 0;
}

std::wstring ReplaceVariables(const std::wstring& str,
                              const anime::Episode& episode,
                              bool url_encode,
                              bool is_manual,
                              bool is_preview) {
  atf::field_map_t fields;

  for (const auto& name : script_variables) {
    fields[name] = std::nullopt;
  }

  auto anime_item = anime::db.Find(episode.anime_id);
  if (!anime_item && is_preview)
    anime_item = &taiga::dummy_anime;
 
  fields[L"title"] = anime_item ?
      anime::GetPreferredTitle(*anime_item) : episode.anime_title();

  if (anime_item) {
    fields[L"watched"] = ui::TranslateNumber(anime_item->GetMyLastWatchedEpisode(), L"");
    fields[L"total"] = ui::TranslateNumber(anime_item->GetEpisodeCount(), L"");
    fields[L"score"] = ui::TranslateMyScore(anime_item->GetMyScore(), L"");
    fields[L"season"] = ui::TranslateDateToSeasonString(anime_item->GetDateStart());
    fields[L"id"] = anime_item->GetId(sync::GetCurrentServiceId());
    fields[L"image"] = anime_item->GetImageUrl();
    fields[L"status"] = ToWstr(static_cast<int>(anime_item->GetMyStatus()));
    fields[L"rewatching"] = ToWstr(anime_item->GetMyRewatching());
    fields[L"notes"] = anime_item->GetMyNotes();
    switch (sync::GetCurrentServiceId()) {
      case sync::ServiceId::MyAnimeList:
        fields[L"animeurl"] = sync::myanimelist::GetAnimePage(*anime_item);
        break;
      case sync::ServiceId::Kitsu:
        fields[L"animeurl"] = sync::kitsu::GetAnimePage(*anime_item);
        break;
      case sync::ServiceId::AniList:
        fields[L"animeurl"] = sync::anilist::GetAnimePage(*anime_item);
        break;
    }
  }

  std::wstring episode_number = ToWstr(anime::GetEpisodeHigh(episode));
  TrimLeft(episode_number, L"0");

  std::wstring episode_folder = episode.folder;
  TrimRight(episode_folder, L"\\");

  fields[L"name"] = episode.episode_title();
  fields[L"episode"] = episode_number;
  fields[L"version"] = ToWstr(episode.release_version());
  fields[L"group"] = episode.release_group();
  fields[L"resolution"] = episode.video_resolution();
  fields[L"video"] = episode.video_terms();
  fields[L"audio"] = episode.audio_terms();
  fields[L"checksum"] = episode.file_checksum();
  fields[L"file"] = episode.file_name_with_extension();
  fields[L"folder"] = episode_folder;
  fields[L"user"] = taiga::GetCurrentUsername();

  switch (track::media_players.play_status) {
    case track::recognition::PlayStatus::Stopped:
      fields[L"playstatus"] = L"stopped";
      break;
    case track::recognition::PlayStatus::Playing:
      fields[L"playstatus"] = L"playing";
      break;
    case track::recognition::PlayStatus::Updated:
      fields[L"playstatus"] = L"updated";
      break;
  }

  if (is_manual) {
    fields[L"manual"] = L"true";
  }

  if (url_encode) {
    for (auto& [name, value] : fields) {
      if (value) {
        value = Url::Encode(*value);
      }
    }
  }

  return atf::Replace(str, fields);
}
