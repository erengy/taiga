/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#include <semaver/semaver/version.h>

#include "base/file.h"
#include "base/log.h"
#include "base/string.h"
#include "base/xml.h"
#include "library/anime_db.h"
#include "sync/myanimelist_util.h"
#include "sync/service.h"
#include "taiga/path.h"
#include "taiga/settings.h"

namespace anime {

bool Database::CheckOldUserDirectory() {
  auto check_path = [](const std::wstring& path,
                       const sync::ServiceId service_id) {
    if (!FolderExists(path))
      return false;
    const auto new_path = taiga::GetPath(taiga::Path::User) +
                          taiga::GetUserDirectoryName(service_id);
    LOGW(L"Renaming old user directory\n" + path + L"\n" + new_path);
    return MoveFileEx(path.c_str(), new_path.c_str(), 0) != 0;
  };

  // "Taiga\data\user\{username}"
  if (check_path(taiga::GetPath(taiga::Path::User) +
                 taiga::GetCurrentUsername(),
                 sync::kMyAnimeList) ||
  // "Taiga\data\user\{username}@hummingbird"
      check_path(taiga::GetPath(taiga::Path::User) +
                 taiga::GetCurrentUsername() + L"@hummingbird",
                 sync::kKitsu)) {
    return true;
  }

  return false;
}

void Database::HandleCompatibility(const std::wstring& meta_version) {
  const semaver::Version version(WstrToStr(meta_version));

  if (version <= semaver::Version(1, 1, 11)) {
    if (taiga::GetCurrentServiceId() == sync::kKitsu) {
      LOGW(L"Clearing English titles");
      for (auto& item : items) {
        item.second.SetEnglishTitle(EmptyString());
      }
    }
  }

  if (version <= semaver::Version(1, 2, 5)) {
    if (taiga::GetCurrentServiceId() == sync::kKitsu) {
      LOGW(L"Clearing image URLs");
      for (auto& item : items) {
        item.second.SetImageUrl(EmptyString());
      }
    }
  }
}

void Database::ReadDatabaseInCompatibilityMode(xml_document& document) {
  xml_node animedb_node = document.child(L"animedb");

  foreach_xmlnode_(node, animedb_node, L"anime") {
    std::wstring id = XmlReadStrValue(node, L"series_animedb_id");
    Item& item = items[ToInt(id)];  // Creates the item if it doesn't exist
    item.SetId(id, sync::kTaiga);
    item.SetId(id, sync::kMyAnimeList);
    item.SetTitle(XmlReadStrValue(node, L"series_title"));
    item.SetEnglishTitle(XmlReadStrValue(node, L"series_english"));
    item.SetSynonyms(XmlReadStrValue(node, L"series_synonyms"));
    item.SetType(sync::myanimelist::TranslateSeriesTypeFrom(XmlReadIntValue(node, L"series_type")));
    item.SetEpisodeCount(XmlReadIntValue(node, L"series_episodes"));
    item.SetAiringStatus(sync::myanimelist::TranslateSeriesStatusFrom(XmlReadIntValue(node, L"series_status")));
    item.SetDateStart(Date(XmlReadStrValue(node, L"series_start")));
    item.SetDateEnd(Date(XmlReadStrValue(node, L"series_end")));
    item.SetImageUrl(XmlReadStrValue(node, L"series_image"));
    item.SetGenres(XmlReadStrValue(node, L"genres"));
    item.SetProducers(XmlReadStrValue(node, L"producers"));
    item.SetScore(ToDouble(XmlReadStrValue(node, L"score")));
    item.SetPopularity(XmlReadIntValue(node, L"popularity"));
    item.SetSynopsis(XmlReadStrValue(node, L"synopsis"));
    item.SetLastModified(ToTime(XmlReadStrValue(node, L"last_modified")));
  }
}

void Database::ReadListInCompatibilityMode(xml_document& document) {
  xml_node myanimelist = document.child(L"myanimelist");

  foreach_xmlnode_(node, myanimelist, L"anime") {
    Item anime_item;
    anime_item.SetId(XmlReadStrValue(node, L"series_animedb_id"), sync::kMyAnimeList);
    anime_item.SetSource(sync::kMyAnimeList);

    anime_item.SetTitle(XmlReadStrValue(node, L"series_title"));
    anime_item.SetSynonyms(XmlReadStrValue(node, L"series_synonyms"));
    anime_item.SetType(sync::myanimelist::TranslateSeriesTypeFrom(XmlReadIntValue(node, L"series_type")));
    anime_item.SetEpisodeCount(XmlReadIntValue(node, L"series_episodes"));
    anime_item.SetAiringStatus(sync::myanimelist::TranslateSeriesStatusFrom(XmlReadIntValue(node, L"series_status")));
    anime_item.SetDateStart(XmlReadStrValue(node, L"series_start"));
    anime_item.SetDateEnd(XmlReadStrValue(node, L"series_end"));
    anime_item.SetImageUrl(XmlReadStrValue(node, L"series_image"));
    anime_item.SetLastModified(0);

    anime_item.AddtoUserList();
    anime_item.SetMyLastWatchedEpisode(XmlReadIntValue(node, L"my_watched_episodes"));
    anime_item.SetMyDateStart(XmlReadStrValue(node, L"my_start_date"));
    anime_item.SetMyDateEnd(XmlReadStrValue(node, L"my_finish_date"));
    anime_item.SetMyScore(XmlReadIntValue(node, L"my_score"));
    anime_item.SetMyStatus(sync::myanimelist::TranslateMyStatusFrom(XmlReadIntValue(node, L"my_status")));
    anime_item.SetMyRewatching(XmlReadIntValue(node, L"my_rewatching"));
    anime_item.SetMyRewatchingEp(XmlReadIntValue(node, L"my_rewatching_ep"));
    anime_item.SetMyLastUpdated(XmlReadStrValue(node, L"my_last_updated"));
    anime_item.SetMyTags(XmlReadStrValue(node, L"my_tags"));

    UpdateItem(anime_item);
  }
}

}  // namespace anime
