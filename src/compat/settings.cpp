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

#include "base/base64.h"
#include "base/log.h"
#include "base/string.h"
#include "base/xml.h"
#include "compat/crypto.h"
#include "library/anime.h"
#include "sync/manager.h"
#include "sync/sync.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "track/media.h"

namespace taiga {

void AppSettings::ReadLegacyValues(const xml_node& settings) {
  // Meta
  if (GetWstr(kMeta_Version).empty()) {
    const auto major = ReadValue(settings, L"meta/version/major", true, L"");
    const auto minor = ReadValue(settings, L"meta/version/minor", true, L"");
    const auto patch = ReadValue(settings, L"meta/version/revision", true, L"");
    if (!major.empty() && !minor.empty() && !patch.empty()) {
      const semaver::Version version(ToInt(major), ToInt(minor), ToInt(patch));
      Set(kMeta_Version, StrToWstr(version.str()));
    } else {
      Set(kMeta_Version, StrToWstr(Taiga.version.str()));
    }
  }

  // Services
  if (GetWstr(kSync_ActiveService) == L"hummingbird") {
    Set(kSync_Service_Kitsu_Username,
        ReadValue(settings, L"account/hummingbird/username", true, L""));
    Set(kSync_Service_Kitsu_Password,
        ReadValue(settings, L"account/hummingbird/password", true, L""));
  }
}

////////////////////////////////////////////////////////////////////////////////

bool AppSettings::HandleCompatibility() {
  const semaver::Version version(WstrToStr(GetWstr(kMeta_Version)));

  if (version == Taiga.version)
    return false;

  LOGW(L"Upgraded from v" + StrToWstr(version.str()) +
       L" to v" + StrToWstr(Taiga.version.str()));

  if (version <= semaver::Version(1, 1, 7)) {
    // Convert old password encoding to base64
    std::wstring password = compat::SimpleDecrypt(GetWstr(kSync_Service_Mal_Password));
    Set(kSync_Service_Mal_Password, Base64Encode(password));
    password = compat::SimpleDecrypt(GetWstr(kSync_Service_Kitsu_Password));
    Set(kSync_Service_Kitsu_Password, Base64Encode(password));

    // Update torrent filters
    for (auto& filter : Aggregator.filter_manager.filters) {
      if (filter.name == L"Discard unknown titles") {
        if (filter.conditions.size() == 1) {
          auto& condition = filter.conditions.front();
          if (condition.element == kFeedFilterElement_Meta_Id) {
            filter.name = L"Discard and deactivate not-in-list anime";
            condition.element = kFeedFilterElement_User_Status;
            condition.value = ToWstr(anime::kNotInList);
          }
        }
        break;
      }
    }
  }

  if (version <= semaver::Version(1, 1, 8)) {
    auto external_links = GetWstr(kApp_Interface_ExternalLinks);
    ReplaceString(external_links, L"http://hummingboard.me", L"http://hb.cybrox.eu");
    Set(kApp_Interface_ExternalLinks, external_links);
  }

  if (version <= semaver::Version(1, 1, 11)) {
    bool detect_streaming_media = false;
    for (auto& media_player : MediaPlayers.items) {
      if (media_player.type == anisthesia::PlayerType::WebBrowser) {
        if (media_player.enabled)
          detect_streaming_media = true;
        media_player.enabled = true;
      }
    }
    Set(kRecognition_DetectStreamingMedia, detect_streaming_media);
  }

  if (version <= semaver::Version(1, 2, 2)) {
    auto external_links = GetWstr(kApp_Interface_ExternalLinks);
    ReplaceString(external_links, L"http://mal.oko.im", L"http://graph.anime.plus");
    std::vector<std::wstring> link_vector;
    Split(external_links, L"\r\n", link_vector);
    for (auto it = link_vector.begin(); it != link_vector.end(); ++it) {
      if (StartsWith(*it, L"Hummingboard")) {
        *it = L"Hummingbird Tools|https://hb.wopian.me";
      } else if (StartsWith(*it, L"Mahou Showtime Schedule")) {
        *it = L"Senpai Anime Charts|http://www.senpai.moe/?mode=calendar";
        it = link_vector.insert(it, L"Monthly.moe|http://www.monthly.moe/weekly");
        it = link_vector.insert(it, L"AniChart|http://anichart.net/airing");
      } else if (StartsWith(*it, L"The Fansub Wiki")) {
        *it = L"The Fansub Database|http://fansubdb.com";
        it = link_vector.insert(it, L"Anime Streaming Search Engine|http://because.moe");
        it = link_vector.insert(it, L"-");
      }
    }
    external_links = Join(link_vector, L"\r\n");
    Set(kApp_Interface_ExternalLinks, external_links);
  }

  if (version <= semaver::Version(1, 2, 3)) {
    if (GetBool(kTorrent_Download_UseAnimeFolder)) {
      auto app_path = GetWstr(kTorrent_Download_AppPath);
      if (IsEqual(GetFileName(app_path), L"deluge.exe")) {
        app_path = GetPathOnly(app_path) + L"deluge-console.exe";
        Set(kTorrent_Download_AppPath, app_path);
        LOGW(L"Changed BitTorrent client from deluge.exe to deluge-console.exe");
      }
    }
  }

  if (version <= semaver::Version(1, 2, 5)) {
    // Change active service to Kitsu
    if (GetWstr(kSync_ActiveService) == L"hummingbird") {
      Set(kSync_ActiveService, ServiceManager.GetServiceNameById(sync::kKitsu));
    }

    // Update mIRC channels
    auto mirc_channels = GetWstr(kShare_Mirc_Channels);
    if (ReplaceString(mirc_channels, L"#hummingbird", L"#kitsu")) {
      Set(kShare_Mirc_Channels, mirc_channels);
    }

    // Hummingbird Tools -> Hibari
    auto external_links = GetWstr(kApp_Interface_ExternalLinks);
    ReplaceString(external_links, L"Hummingbird Tools|", L"Hibari|");
    Set(kApp_Interface_ExternalLinks, external_links);
  }

  return true;
}

}  // namespace taiga
