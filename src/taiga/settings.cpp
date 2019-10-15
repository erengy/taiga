/*
** Taiga
** Copyright (C) 2010-2019, Eren Okka
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

#include <windows/win/registry.h>
#include <windows/win/task_dialog.h>

#include "taiga/settings.h"

#include "base/base64.h"
#include "base/crypto.h"
#include "base/file.h"
#include "base/log.h"
#include "base/string.h"
#include "base/xml.h"
#include "media/anime_db.h"
#include "media/anime_season.h"
#include "media/discover.h"
#include "media/library/history.h"
#include "media/library/queue.h"
#include "ui/resource.h"
#include "sync/manager.h"
#include "sync/sync.h"
#include "sync/kitsu_util.h"
#include "taiga/announce.h"
#include "taiga/config.h"
#include "taiga/path.h"
#include "taiga/stats.h"
#include "taiga/taiga.h"
#include "taiga/timer.h"
#include "taiga/version.h"
#include "track/feed.h"
#include "track/feed_aggregator.h"
#include "track/feed_filter_manager.h"
#include "track/media.h"
#include "track/monitor.h"
#include "ui/dlg/dlg_anime_list.h"
#include "ui/dlg/dlg_season.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "ui/ui.h"

namespace taiga {

constexpr auto kDefaultExternalLinks =
    L"Hibari|https://hb.wopian.me\r\n"
    L"MALgraph|http://graph.anime.plus\r\n"
    L"-\r\n"
    L"AniChart|http://anichart.net/airing\r\n"
    L"Monthly.moe|https://www.monthly.moe/weekly\r\n"
    L"Senpai Anime Charts|http://www.senpai.moe/?mode=calendar\r\n"
    L"-\r\n"
    L"Anime Streaming Search Engine|http://because.moe\r\n"
    L"The Fansub Database|https://fansubdb.com";
constexpr auto kDefaultFormatDiscordDetails =
    L"%title%";
constexpr auto kDefaultFormatDiscordState =
    L"$if(%episode%,Episode %episode%$if(%total%,/%total%) )"
    L"$if(%group%,by %group%)";
constexpr auto kDefaultFormatHttp =
    L"user=%user%"
    L"&name=%title%"
    L"&ep=%episode%"
    L"&eptotal=$if(%total%,%total%,?)"
    L"&score=%score%"
    L"&picurl=%image%"
    L"&playstatus=%playstatus%";
constexpr auto kDefaultFormatMirc =
    L"\00304$if($greater(%episode%,%watched%),Watching,Rewatching):\003 %title%"
    L"$if(%episode%, \00303%episode%$if(%total%,/%total%))\003 "
    L"$if(%score%,\00314[Score: %score%]\003) "
    L"\00312%animeurl%";
constexpr auto kDefaultFormatTwitter =
    L"$ifequal(%episode%,%total%,Just completed: %title%"
    L"$if(%score%, (Score: %score%)) %animeurl%,"
    L"$ifequal(%episode%,1,Started watching: %title% %animeurl%))";
constexpr auto kDefaultFormatBalloon =
    L"$if(%title%,%title%)\\n"
    L"$if(%episode%,Episode %episode%$if(%total%,/%total%) )"
    L"$if(%group%,by %group%)\\n"
    L"$if(%name%,%name%)";
constexpr auto kDefaultTorrentAppPath =
    L"C:\\Program Files\\uTorrent\\uTorrent.exe";
constexpr auto kDefaultTorrentSearch =
    L"https://nyaa.si/?page=rss&c=1_2&f=0&q=%title%";
constexpr auto kDefaultTorrentSource =
    L"https://www.tokyotosho.info/rss.php?filter=1,11&zwnj=0";

// Here we assume that anything less than 10 MiB can't be a valid episode.
constexpr int kDefaultFileSizeThreshold = 1024 * 1024 * 10;

////////////////////////////////////////////////////////////////////////////////

AppSettings::~AppSettings() {
  Save();
}

bool AppSettings::Load() {
  std::lock_guard lock{mutex_};

  if (modified_) {
    // @TODO: What to do in this case?
    return false;
  }

  // @TODO: If `<filename>.new` exists
  // - Delete `<filename>`
  // - Rename `<filename>.new` to `<filename>`

  return DeserializeFromXml();
}

bool AppSettings::Save() {
  std::lock_guard lock{mutex_};

  if (!modified_) {
    return false;
  }

  if (!SerializeToXml()) {
    return false;
  }

  // @TODO:
  // - Save as `<filename>.new`
  // - Delete `<filename>`
  // - Rename `<filename>.new` to `<filename>`

  modified_ = false;

  return true;
}

template <typename T>
T AppSettings::value(const AppSettingKey key) const {
  std::lock_guard lock{mutex_};

  const auto& app_setting = GetSetting(key);
  const auto value = settings_.value(app_setting.key);

  if (std::holds_alternative<T>(value)) {
    return std::get<T>(value);
  } else if (std::holds_alternative<T>(app_setting.default_value)) {
    return std::get<T>(app_setting.default_value);
  } else {
    return T{};
  }
}

template <typename T>
bool AppSettings::set_value(const AppSettingKey key, T&& value) {
  std::lock_guard lock{mutex_};

  if (settings_.set_value(GetSetting(key).key, std::move(value))) {
    modified_ = true;
    return true;
  } else {
    return false;
  }
}

void AppSettings::InitKeyMap() const {
  if (key_map_.empty()) {
    key_map_ = {
      // Meta
      {AppSettingKey::MetaVersion, {"meta/version", std::wstring{}}},

      // Services
      {AppSettingKey::SyncActiveService, {"account/update/activeservice", std::wstring{L"anilist"}}},
      {AppSettingKey::SyncAutoOnStart, {"account/myanimelist/login", false}},
      {AppSettingKey::SyncServiceMalUsername, {"account/myanimelist/username", std::wstring{}}},
      {AppSettingKey::SyncServiceMalPassword, {"account/myanimelist/password", std::wstring{}}},
      {AppSettingKey::SyncServiceKitsuDisplayName, {"account/kitsu/displayname", std::wstring{}}},
      {AppSettingKey::SyncServiceKitsuEmail, {"account/kitsu/email", std::wstring{}}},
      {AppSettingKey::SyncServiceKitsuUsername, {"account/kitsu/username", std::wstring{}}},
      {AppSettingKey::SyncServiceKitsuPassword, {"account/kitsu/password", std::wstring{}}},
      {AppSettingKey::SyncServiceKitsuPartialLibrary, {"account/kitsu/partiallibrary", true}},
      {AppSettingKey::SyncServiceKitsuRatingSystem, {"account/kitsu/ratingsystem", std::wstring{L"regular"}}},
      {AppSettingKey::SyncServiceAniListUsername, {"account/anilist/username", std::wstring{}}},
      {AppSettingKey::SyncServiceAniListRatingSystem, {"account/anilist/ratingsystem", std::wstring{L"POINT_10"}}},
      {AppSettingKey::SyncServiceAniListToken, {"account/anilist/token", std::wstring{}}},

      // Library
      {AppSettingKey::LibraryFileSizeThreshold, {"anime/folders/scan/minfilesize", int{kDefaultFileSizeThreshold}}},
      {AppSettingKey::LibraryMediaPlayerPath, {"recognition/mediaplayers/launchpath", std::wstring{}}},
      {AppSettingKey::LibraryWatchFolders, {"anime/folders/watch/enabled", true}},

      // Application
      {AppSettingKey::AppListDoubleClickAction, {"program/list/action/doubleclick", 4}},
      {AppSettingKey::AppListMiddleClickAction, {"program/list/action/middleclick", 3}},
      {AppSettingKey::AppListDisplayEnglishTitles, {"program/list/action/englishtitles", false}},
      {AppSettingKey::AppListHighlightNewEpisodes, {"program/list/filter/episodes/highlight", true}},
      {AppSettingKey::AppListDisplayHighlightedOnTop, {"program/list/filter/episodes/highlightedontop", false}},
      {AppSettingKey::AppListProgressDisplayAired, {"program/list/progress/showaired", true}},
      {AppSettingKey::AppListProgressDisplayAvailable, {"program/list/progress/showavailable", true}},
      {AppSettingKey::AppListSortColumnPrimary, {"program/list/sort/column", std::wstring{L"anime_title"}}},
      {AppSettingKey::AppListSortColumnSecondary, {"program/list/sort/column2", std::wstring{L"anime_title"}}},
      {AppSettingKey::AppListSortOrderPrimary, {"program/list/sort/order", 1}},
      {AppSettingKey::AppListSortOrderSecondary, {"program/list/sort/order2", 1}},
      {AppSettingKey::AppListTitleLanguagePreference, {"program/list/action/titlelang", std::wstring{}}},
      {AppSettingKey::AppBehaviorAutostart, {"program/general/autostart", false}},
      {AppSettingKey::AppBehaviorStartMinimized, {"program/startup/minimize", false}},
      {AppSettingKey::AppBehaviorCheckForUpdates, {"program/startup/checkversion", true}},
      {AppSettingKey::AppBehaviorScanAvailableEpisodes, {"program/startup/checkeps", false}},
      {AppSettingKey::AppBehaviorCloseToTray, {"program/general/close", false}},
      {AppSettingKey::AppBehaviorMinimizeToTray, {"program/general/minimize", false}},
      {AppSettingKey::AppConnectionProxyHost, {"program/proxy/host", std::wstring{}}},
      {AppSettingKey::AppConnectionProxyUsername, {"program/proxy/username", std::wstring{}}},
      {AppSettingKey::AppConnectionProxyPassword, {"program/proxy/password", std::wstring{}}},
      {AppSettingKey::AppConnectionNoRevoke, {"program/general/sslnorevoke", false}},
      {AppSettingKey::AppConnectionReuseActive, {"program/general/reuseconnections", true}},
      {AppSettingKey::AppInterfaceTheme, {"program/general/theme", std::wstring{L"Default"}}},
      {AppSettingKey::AppInterfaceExternalLinks, {"program/general/externallinks", std::wstring{kDefaultExternalLinks}}},

      // Recognition
      {AppSettingKey::RecognitionDetectionInterval, {"recognition/general/detectioninterval", 3}},
      {AppSettingKey::RecognitionDetectMediaPlayers, {"recognition/mediaplayers/enabled", true}},
      {AppSettingKey::RecognitionDetectStreamingMedia, {"recognition/streaming/enabled", false}},
      {AppSettingKey::RecognitionIgnoredStrings, {"recognition/anitomy/ignored_strings", std::wstring{}}},
      {AppSettingKey::RecognitionLookupParentDirectories, {"recognition/general/lookup_parent_directories", true}},
      {AppSettingKey::RecognitionRelationsLastModified, {"recognition/general/relations_last_modified", std::wstring{}}},
      {AppSettingKey::SyncUpdateDelay, {"account/update/delay", 120}},
      {AppSettingKey::SyncUpdateAskToConfirm, {"account/update/asktoconfirm", true}},
      {AppSettingKey::SyncUpdateCheckPlayer, {"account/update/checkplayer", false}},
      {AppSettingKey::SyncUpdateOutOfRange, {"account/update/outofrange", false}},
      {AppSettingKey::SyncUpdateOutOfRoot, {"account/update/outofroot", false}},
      {AppSettingKey::SyncUpdateWaitPlayer, {"account/update/waitplayer", false}},
      {AppSettingKey::SyncGoToNowPlayingRecognized, {"account/update/gotonowplaying", true}},
      {AppSettingKey::SyncGoToNowPlayingNotRecognized, {"account/update/gotonowplayingnot", false}},
      {AppSettingKey::SyncNotifyRecognized, {"program/notifications/balloon/recognized", true}},
      {AppSettingKey::SyncNotifyNotRecognized, {"program/notifications/balloon/notrecognized", true}},
      {AppSettingKey::SyncNotifyFormat, {"program/notifications/balloon/format", std::wstring{kDefaultFormatBalloon}}},
      {AppSettingKey::StreamAnimelab, {"recognition/streaming/providers/animelab", true}},
      {AppSettingKey::StreamAdn, {"recognition/streaming/providers/adn", true}},
      {AppSettingKey::StreamAnn, {"recognition/streaming/providers/ann", true}},
      {AppSettingKey::StreamCrunchyroll, {"recognition/streaming/providers/crunchyroll", true}},
      {AppSettingKey::StreamFunimation, {"recognition/streaming/providers/funimation", true}},
      {AppSettingKey::StreamHidive, {"recognition/streaming/providers/hidive", true}},
      {AppSettingKey::StreamPlex, {"recognition/streaming/providers/plex", true}},
      {AppSettingKey::StreamVeoh, {"recognition/streaming/providers/veoh", true}},
      {AppSettingKey::StreamViz, {"recognition/streaming/providers/viz", true}},
      {AppSettingKey::StreamVrv, {"recognition/streaming/providers/vrv", true}},
      {AppSettingKey::StreamWakanim, {"recognition/streaming/providers/wakanim", true}},
      {AppSettingKey::StreamYahoo, {"recognition/streaming/providers/yahoo", true}},
      {AppSettingKey::StreamYoutube, {"recognition/streaming/providers/youtube", true}},

      // Sharing
      {AppSettingKey::ShareDiscordApplicationId, {"announce/discord/applicationid", std::wstring{link::discord::kApplicationId}}},
      {AppSettingKey::ShareDiscordEnabled, {"announce/discord/enabled", false}},
      {AppSettingKey::ShareDiscordFormatDetails, {"announce/discord/formatdetails", std::wstring{kDefaultFormatDiscordDetails}}},
      {AppSettingKey::ShareDiscordFormatState, {"announce/discord/formatstate", std::wstring{kDefaultFormatDiscordState}}},
      {AppSettingKey::ShareDiscordUsernameEnabled, {"announce/discord/usernameenabled", true}},
      {AppSettingKey::ShareHttpEnabled, {"announce/http/enabled", false}},
      {AppSettingKey::ShareHttpFormat, {"announce/http/format", std::wstring{kDefaultFormatHttp}}},
      {AppSettingKey::ShareHttpUrl, {"announce/http/url", std::wstring{}}},
      {AppSettingKey::ShareMircEnabled, {"announce/mirc/enabled", false}},
      {AppSettingKey::ShareMircMultiServer, {"announce/mirc/multiserver", false}},
      {AppSettingKey::ShareMircUseMeAction, {"announce/mirc/useaction", true}},
      {AppSettingKey::ShareMircMode, {"announce/mirc/mode", 1}},
      {AppSettingKey::ShareMircChannels, {"announce/mirc/channels", std::wstring{L"#kitsu, #myanimelist, #taiga"}}},
      {AppSettingKey::ShareMircFormat, {"announce/mirc/format", std::wstring{kDefaultFormatMirc}}},
      {AppSettingKey::ShareMircService, {"announce/mirc/service", std::wstring{L"mIRC"}}},
      {AppSettingKey::ShareTwitterEnabled, {"announce/twitter/enabled", false}},
      {AppSettingKey::ShareTwitterFormat, {"announce/twitter/format", std::wstring{kDefaultFormatTwitter}}},
      {AppSettingKey::ShareTwitterOauthToken, {"announce/twitter/oauth_token", std::wstring{}}},
      {AppSettingKey::ShareTwitterOauthSecret, {"announce/twitter/oauth_secret", std::wstring{}}},
      {AppSettingKey::ShareTwitterUsername, {"announce/twitter/user", std::wstring{}}},

      // Torrents
      {AppSettingKey::TorrentDiscoverySource, {"rss/torrent/source/address", std::wstring{kDefaultTorrentSource}}},
      {AppSettingKey::TorrentDiscoverySearchUrl, {"rss/torrent/search/address", std::wstring{kDefaultTorrentSearch}}},
      {AppSettingKey::TorrentDiscoveryAutoCheckEnabled, {"rss/torrent/options/autocheck", true}},
      {AppSettingKey::TorrentDiscoveryAutoCheckInterval, {"rss/torrent/options/checkinterval", 60}},
      {AppSettingKey::TorrentDiscoveryNewAction, {"rss/torrent/options/newaction", 1}},
      {AppSettingKey::TorrentDownloadAppMode, {"rss/torrent/application/mode", 1}},
      {AppSettingKey::TorrentDownloadAppOpen, {"rss/torrent/application/open", true}},
      {AppSettingKey::TorrentDownloadAppPath, {"rss/torrent/application/path", std::wstring{}}},
      {AppSettingKey::TorrentDownloadLocation, {"rss/torrent/options/downloadpath", std::wstring{}}},
      {AppSettingKey::TorrentDownloadUseAnimeFolder, {"rss/torrent/options/autosetfolder", true}},
      {AppSettingKey::TorrentDownloadFallbackOnFolder, {"rss/torrent/options/autousefolder", false}},
      {AppSettingKey::TorrentDownloadCreateSubfolder, {"rss/torrent/options/autocreatefolder", false}},
      {AppSettingKey::TorrentDownloadSortBy, {"rss/torrent/options/downloadsortby", std::wstring{L"episode_number"}}},
      {AppSettingKey::TorrentDownloadSortOrder, {"rss/torrent/options/downloadsortorder", std::wstring{L"ascending"}}},
      {AppSettingKey::TorrentDownloadUseMagnet, {"rss/torrent/options/downloadusemagnet", false}},
      {AppSettingKey::TorrentFilterEnabled, {"rss/torrent/filter/enabled", true}},
      {AppSettingKey::TorrentFilterArchiveMaxCount, {"rss/torrent/filter/archive_maxcount", 1000}},

      // Internal
      {AppSettingKey::AppPositionX, {"program/position/x", -1}},
      {AppSettingKey::AppPositionY, {"program/position/y", -1}},
      {AppSettingKey::AppPositionW, {"program/position/w", 960}},
      {AppSettingKey::AppPositionH, {"program/position/h", 640}},
      {AppSettingKey::AppPositionMaximized, {"program/position/maximized", false}},
      {AppSettingKey::AppPositionRemember, {"program/exit/remember_pos_size", true}},
      {AppSettingKey::AppOptionHideSidebar, {"program/general/hidesidebar", false}},
      {AppSettingKey::AppOptionEnableRecognition, {"program/general/enablerecognition", true}},
      {AppSettingKey::AppOptionEnableSharing, {"program/general/enablesharing", true}},
      {AppSettingKey::AppOptionEnableSync, {"program/general/enablesync", true}},
      {AppSettingKey::AppSeasonsLastSeason, {"program/seasons/lastseason", std::wstring{}}},
      {AppSettingKey::AppSeasonsGroupBy, {"program/seasons/groupby", ui::kSeasonGroupByType}},
      {AppSettingKey::AppSeasonsSortBy, {"program/seasons/sortby", ui::kSeasonSortByTitle}},
      {AppSettingKey::AppSeasonsViewAs, {"program/seasons/viewas", ui::kSeasonViewAsTiles}},
    };
  }
}

const AppSettings::AppSetting& AppSettings::GetSetting(const AppSettingKey key) const {
  InitKeyMap();
  return key_map_[key];
}

bool AppSettings::DeserializeFromXml() {
  XmlDocument document;
  const auto path = taiga::GetPath(taiga::Path::Settings);
  const auto parse_result = XmlLoadFileToDocument(document, path);

  if (!parse_result) {
    LOGE(L"Could not read application settings.\nPath: {}\nReason: {}",
         path, StrToWstr(parse_result.description()));
    return false;
  }

  const auto attr_from_path = [](XmlNode node, const std::string& path) {
    std::vector<std::wstring> node_names;
    Split(StrToWstr(path), L"/", node_names);
    const auto attr_name = node_names.back();
    node_names.pop_back();
    for (const auto& node_name : node_names) {
      node = node.child(node_name.c_str());
    }
    return node.attribute(attr_name.c_str());
  };

  const auto settings = document.child(L"settings");

  InitKeyMap();
  for (const auto& [key, app_setting] : key_map_) {
    if (const auto attr = attr_from_path(settings, app_setting.key)) {
      switch (base::GetSettingValueType(app_setting.default_value)) {
        case base::SettingValueType::Bool:
          settings_.set_value(app_setting.key, attr.as_bool(
              std::get<bool>(app_setting.default_value)));
          break;
        case base::SettingValueType::Int:
          settings_.set_value(app_setting.key, attr.as_int(
              std::get<int>(app_setting.default_value)));
          break;
        case base::SettingValueType::Wstring:
          settings_.set_value(app_setting.key, std::wstring{attr.as_string(
              std::get<std::wstring>(app_setting.default_value).c_str())});
          break;
      }
    }
  }

  // Folders
  library_folders.clear();
  const auto node_folders = settings.child(L"anime").child(L"folders");
  for (const auto folder : node_folders.children(L"root")) {
    library_folders.push_back(folder.attribute(L"folder").value());
  }

  // Anime items
  const auto node_items = settings.child(L"anime").child(L"items");
  for (const auto item : node_items.children(L"item")) {
    const int anime_id = item.attribute(L"id").as_int();
    auto anime_item = anime::db.Find(anime_id, false);
    if (!anime_item)
      anime_item = &anime::db.items[anime_id];
    anime_item->SetFolder(item.attribute(L"folder").value());
    anime_item->SetUserSynonyms(item.attribute(L"titles").value());
    anime_item->SetUseAlternative(item.attribute(L"use_alternative").as_bool());
  }

  // Media players
  const auto node_players = settings.child(L"recognition").child(L"mediaplayers");
  for (const auto player : node_players.children(L"player")) {
    const std::wstring name = player.attribute(L"name").value();
    const bool enabled = player.attribute(L"enabled").as_bool();
    for (auto& media_player : track::media_players.items) {
      if (media_player.name == WstrToStr(name)) {
        media_player.enabled = enabled;
        break;
      }
    }
  }

  // Anime list columns
  ui::DlgAnimeList.listview.InitializeColumns();
  const auto node_list_columns = settings.child(L"program").child(L"list").child(L"columns");
  for (const auto column : node_list_columns.children(L"column")) {
    const std::wstring name = column.attribute(L"name").value();
    const auto column_type = ui::AnimeListDialog::ListView::TranslateColumnName(name);
    if (column_type != ui::kColumnUnknown) {
      auto& data = ui::DlgAnimeList.listview.columns[column_type];
      data.order = column.attribute(L"order").as_int();
      data.visible = column.attribute(L"visible").as_bool();
      data.width = column.attribute(L"width").as_int();
    }
  }

  // Torrent filters
  const auto node_filter = settings.child(L"rss").child(L"torrent").child(L"filter");
  track::feed_filter_manager.Import(node_filter, track::feed_filter_manager.filters);
  if (track::feed_filter_manager.filters.empty())
    track::feed_filter_manager.AddPresets();

  return true;
}

bool AppSettings::SerializeToXml() const {
  XmlDocument document;

  auto settings = document.append_child(L"settings");

  const auto attr_from_path = [](XmlNode node, const std::string& path) {
    std::vector<std::wstring> node_names;
    Split(StrToWstr(path), L"/", node_names);
    const auto attr_name = node_names.back();
    node_names.pop_back();
    for (const auto& node_name : node_names) {
      node = XmlChild(node, node_name.c_str());
    }
    return XmlAttr(node, attr_name.c_str());
  };

  InitKeyMap();
  for (const auto& [key, app_setting] : key_map_) {
    auto attr = attr_from_path(settings, app_setting.key);
    const auto value = settings_.value(app_setting.key);

    switch (base::GetSettingValueType(value)) {
      case base::SettingValueType::Bool:
        attr.set_value(std::get<bool>(value));
        break;
      case base::SettingValueType::Int:
        attr.set_value(std::get<int>(value));
        break;
      case base::SettingValueType::Wstring:
        attr.set_value(std::get<std::wstring>(value).c_str());
        break;
    }
  }

  // Meta
  auto meta = XmlChild(settings, L"meta");
  XmlAttr(meta, L"version").set_value(StrToWstr(taiga::version().to_string()).c_str());

  // Library folders
  auto folders = settings.child(L"anime").child(L"folders");
  for (const auto& folder : library_folders) {
    auto root = folders.append_child(L"root");
    root.append_attribute(L"folder") = folder.c_str();
  }

  // Anime items
  auto items = settings.child(L"anime").append_child(L"items");
  for (const auto& [id, anime_item] : anime::db.items) {
    if (anime_item.GetFolder().empty() &&
        !anime_item.UserSynonymsAvailable() &&
        !anime_item.GetUseAlternative())
      continue;
    auto item = items.append_child(L"item");
    item.append_attribute(L"id") = anime_item.GetId();
    if (!anime_item.GetFolder().empty())
      item.append_attribute(L"folder") = anime_item.GetFolder().c_str();
    if (anime_item.UserSynonymsAvailable())
      item.append_attribute(L"titles") = Join(anime_item.GetUserSynonyms(), L"; ").c_str();
    if (anime_item.GetUseAlternative())
      item.append_attribute(L"use_alternative") = anime_item.GetUseAlternative();
  }

  // Media players
  auto mediaplayers = settings.child(L"recognition").child(L"mediaplayers");
  for (const auto& media_player : track::media_players.items) {
    auto player = mediaplayers.append_child(L"player");
    player.append_attribute(L"name") = StrToWstr(media_player.name).c_str();
    player.append_attribute(L"enabled") = media_player.enabled;
  }

  // Anime list columns
  auto list_columns = settings.child(L"program").child(L"list").append_child(L"columns");
  for (const auto& it : ui::DlgAnimeList.listview.columns) {
    const auto& column = it.second;
    auto node = list_columns.append_child(L"column");
    node.append_attribute(L"name") = column.key.c_str();
    node.append_attribute(L"order") = column.order;
    node.append_attribute(L"visible") = column.visible;
    node.append_attribute(L"width") = column.width;
  }

  // Torrent filters
  auto torrent_filter = settings.child(L"rss").child(L"torrent").child(L"filter");
  track::feed_filter_manager.Export(torrent_filter, track::feed_filter_manager.filters);

  const auto path = taiga::GetPath(taiga::Path::Settings) + L".test.xml";  // @TEMP
  return XmlSaveDocumentToFile(document, path);
}

////////////////////////////////////////////////////////////////////////////////

void AppSettings::DoAfterLoad() {
  // Services
  ServiceManager.service(sync::kKitsu)->user().rating_system =
      GetSyncServiceKitsuRatingSystem();
  ServiceManager.service(sync::kAniList)->user().rating_system =
      GetSyncServiceAniListRatingSystem();
  ServiceManager.service(sync::kAniList)->user().access_token =
      GetSyncServiceAniListToken();

  // Torrent application path
  if (GetTorrentDownloadAppPath().empty()) {
    SetTorrentDownloadAppPath(GetDefaultAppPath(L".torrent", kDefaultTorrentAppPath));
  }

  // Torrent filters
  auto& feed = track::aggregator.GetFeed();
  feed.channel.link = GetTorrentDiscoverySource();
}

void AppSettings::ApplyChanges() {
  if (changed_account_or_service_) {
    anime::db.LoadList();
    library::history.Load();
    CurrentEpisode.Set(anime::ID_UNKNOWN);
    taiga::stats.CalculateAll();
    sync::InvalidateUserAuthentication();
    ui::OnSettingsUserChange();
    ui::OnSettingsServiceChange();
    changed_account_or_service_ = false;
  } else {
    ui::OnSettingsChange();
  }

  ui::Menus.UpdateFolders();

  timers.UpdateIntervalsFromSettings();
}

////////////////////////////////////////////////////////////////////////////////

// Meta

semaver::Version AppSettings::GetMetaVersion() const {
  const std::wstring version = value<std::wstring>(AppSettingKey::MetaVersion);
  return semaver::Version(WstrToStr(version));
}

// Services

sync::ServiceId AppSettings::GetSyncActiveService() const {
  const std::wstring name = value<std::wstring>(AppSettingKey::SyncActiveService);
  return ServiceManager.GetServiceIdByName(name);
}

void AppSettings::SetSyncActiveService(const sync::ServiceId service_id) {
  const auto previous_service_id = GetSyncActiveService();

  if (service_id != previous_service_id) {
    if (library::queue.GetItemCount() > 0) {
      ui::OnSettingsServiceChangeFailed();
      return;
    }

    if (!anime::db.items.empty()) {
      if (ui::OnSettingsServiceChangeConfirm(service_id, previous_service_id)) {
        anime::db.SaveList(true);
        anime::db.items.clear();
        anime::db.SaveDatabase();
        ui::image_db.Clear();
        SeasonDatabase.Reset();
      } else {
        return;
      }
    }
  }

  const std::wstring name = ServiceManager.GetServiceNameById(service_id);
  if (set_value(AppSettingKey::SyncActiveService, name)) {
    changed_account_or_service_ = true;
  }
}

bool AppSettings::GetSyncAutoOnStart() const {
  return value<bool>(AppSettingKey::SyncAutoOnStart);
}

void AppSettings::SetSyncAutoOnStart(const bool enabled) {
  set_value(AppSettingKey::SyncAutoOnStart, enabled);
}

std::wstring AppSettings::GetSyncServiceMalUsername() const {
  return value<std::wstring>(AppSettingKey::SyncServiceMalUsername);
}

void AppSettings::SetSyncServiceMalUsername(const std::wstring& username) {
  if (set_value(AppSettingKey::SyncServiceMalUsername, username)) {
    if (GetCurrentServiceId() == sync::kMyAnimeList) {
      changed_account_or_service_ = true;
    }
  }
}

std::wstring AppSettings::GetSyncServiceMalPassword() const {
  return value<std::wstring>(AppSettingKey::SyncServiceMalPassword);
}

void AppSettings::SetSyncServiceMalPassword(const std::wstring& password) {
  set_value(AppSettingKey::SyncServiceMalPassword, password);
}

std::wstring AppSettings::GetSyncServiceKitsuDisplayName() const {
  return value<std::wstring>(AppSettingKey::SyncServiceKitsuDisplayName);
}

void AppSettings::SetSyncServiceKitsuDisplayName(const std::wstring& name) {
  set_value(AppSettingKey::SyncServiceKitsuDisplayName, name);
}

std::wstring AppSettings::GetSyncServiceKitsuEmail() const {
  return value<std::wstring>(AppSettingKey::SyncServiceKitsuEmail);
}

void AppSettings::SetSyncServiceKitsuEmail(const std::wstring& email) {
  if (set_value(AppSettingKey::SyncServiceKitsuEmail, email)) {
    set_value(AppSettingKey::SyncServiceKitsuDisplayName, std::wstring{});
    set_value(AppSettingKey::SyncServiceKitsuUsername, std::wstring{});

    if (GetCurrentServiceId() == sync::kKitsu) {
      changed_account_or_service_ = true;
    }
  }
}

std::wstring AppSettings::GetSyncServiceKitsuUsername() const {
  return value<std::wstring>(AppSettingKey::SyncServiceKitsuUsername);
}

void AppSettings::SetSyncServiceKitsuUsername(const std::wstring& username) {
  set_value(AppSettingKey::SyncServiceKitsuUsername, username);
}

std::wstring AppSettings::GetSyncServiceKitsuPassword() const {
  return value<std::wstring>(AppSettingKey::SyncServiceKitsuPassword);
}

void AppSettings::SetSyncServiceKitsuPassword(const std::wstring& password) {
  set_value(AppSettingKey::SyncServiceKitsuPassword, password);
}

bool AppSettings::GetSyncServiceKitsuPartialLibrary() const {
  return value<bool>(AppSettingKey::SyncServiceKitsuPartialLibrary);
}

void AppSettings::SetSyncServiceKitsuPartialLibrary(const bool enabled) {
  set_value(AppSettingKey::SyncServiceKitsuPartialLibrary, enabled);
}

std::wstring AppSettings::GetSyncServiceKitsuRatingSystem() const {
  return value<std::wstring>(AppSettingKey::SyncServiceKitsuRatingSystem);
}

void AppSettings::SetSyncServiceKitsuRatingSystem(const std::wstring& rating_system) {
  set_value(AppSettingKey::SyncServiceKitsuRatingSystem, rating_system);
}

std::wstring AppSettings::GetSyncServiceAniListUsername() const {
  return value<std::wstring>(AppSettingKey::SyncServiceAniListUsername);
}

void AppSettings::SetSyncServiceAniListUsername(const std::wstring& username) {
  if (set_value(AppSettingKey::SyncServiceAniListUsername, username)) {
    if (GetCurrentServiceId() == sync::kAniList) {
      changed_account_or_service_ = true;
    }
  }
}

std::wstring AppSettings::GetSyncServiceAniListRatingSystem() const {
  return value<std::wstring>(AppSettingKey::SyncServiceAniListRatingSystem);
}

void AppSettings::SetSyncServiceAniListRatingSystem(const std::wstring& rating_system) {
  set_value(AppSettingKey::SyncServiceAniListRatingSystem, rating_system);
}

std::wstring AppSettings::GetSyncServiceAniListToken() const {
  return value<std::wstring>(AppSettingKey::SyncServiceAniListToken);
}

void AppSettings::SetSyncServiceAniListToken(const std::wstring& token) {
  set_value(AppSettingKey::SyncServiceAniListToken, token);
}

// Library

int AppSettings::GetLibraryFileSizeThreshold() const {
  return value<int>(AppSettingKey::LibraryFileSizeThreshold);
}

void AppSettings::SetLibraryFileSizeThreshold(const int bytes) {
  set_value(AppSettingKey::LibraryFileSizeThreshold, bytes);
}

std::wstring AppSettings::GetLibraryMediaPlayerPath() const {
  return value<std::wstring>(AppSettingKey::LibraryMediaPlayerPath);
}

void AppSettings::SetLibraryMediaPlayerPath(const std::wstring& path) {
  set_value(AppSettingKey::LibraryMediaPlayerPath, path);
}

bool AppSettings::GetLibraryWatchFolders() const {
  return value<bool>(AppSettingKey::LibraryWatchFolders);
}

void AppSettings::SetLibraryWatchFolders(const bool enabled) {
  set_value(AppSettingKey::LibraryWatchFolders, enabled);
  track::monitor.Enable(enabled);
}

// Application

int AppSettings::GetAppListDoubleClickAction() const {
  return value<int>(AppSettingKey::AppListDoubleClickAction);
}

void AppSettings::SetAppListDoubleClickAction(const int action) {
  set_value(AppSettingKey::AppListDoubleClickAction, action);
}

int AppSettings::GetAppListMiddleClickAction() const {
  return value<int>(AppSettingKey::AppListMiddleClickAction);
}

void AppSettings::SetAppListMiddleClickAction(const int action) {
  set_value(AppSettingKey::AppListMiddleClickAction, action);
}

bool AppSettings::GetAppListDisplayEnglishTitles() const {
  return value<bool>(AppSettingKey::AppListDisplayEnglishTitles);
}

void AppSettings::SetAppListDisplayEnglishTitles(const bool enabled) {
  set_value(AppSettingKey::AppListDisplayEnglishTitles, enabled);
}

bool AppSettings::GetAppListHighlightNewEpisodes() const {
  return value<bool>(AppSettingKey::AppListHighlightNewEpisodes);
}

void AppSettings::SetAppListHighlightNewEpisodes(const bool enabled) {
  set_value(AppSettingKey::AppListHighlightNewEpisodes, enabled);
}

bool AppSettings::GetAppListDisplayHighlightedOnTop() const {
  return value<bool>(AppSettingKey::AppListDisplayHighlightedOnTop);
}

void AppSettings::SetAppListDisplayHighlightedOnTop(const bool enabled) {
  set_value(AppSettingKey::AppListDisplayHighlightedOnTop, enabled);
}

bool AppSettings::GetAppListProgressDisplayAired() const {
  return value<bool>(AppSettingKey::AppListProgressDisplayAired);
}

void AppSettings::SetAppListProgressDisplayAired(const bool enabled) {
  set_value(AppSettingKey::AppListProgressDisplayAired, enabled);
}

bool AppSettings::GetAppListProgressDisplayAvailable() const {
  return value<bool>(AppSettingKey::AppListProgressDisplayAvailable);
}

void AppSettings::SetAppListProgressDisplayAvailable(const bool enabled) {
  set_value(AppSettingKey::AppListProgressDisplayAvailable, enabled);
}

std::wstring AppSettings::GetAppListSortColumnPrimary() const {
  return value<std::wstring>(AppSettingKey::AppListSortColumnPrimary);
}

void AppSettings::SetAppListSortColumnPrimary(const std::wstring& column) {
  set_value(AppSettingKey::AppListSortColumnPrimary, column);
}

std::wstring AppSettings::GetAppListSortColumnSecondary() const {
  return value<std::wstring>(AppSettingKey::AppListSortColumnSecondary);
}

void AppSettings::SetAppListSortColumnSecondary(const std::wstring& column) {
  set_value(AppSettingKey::AppListSortColumnSecondary, column);
}

int AppSettings::GetAppListSortOrderPrimary() const {
  return value<int>(AppSettingKey::AppListSortOrderPrimary);
}

void AppSettings::SetAppListSortOrderPrimary(const int order) {
  set_value(AppSettingKey::AppListSortOrderPrimary, order);
}

int AppSettings::GetAppListSortOrderSecondary() const {
  return value<int>(AppSettingKey::AppListSortOrderSecondary);
}

void AppSettings::SetAppListSortOrderSecondary(const int order) {
  set_value(AppSettingKey::AppListSortOrderSecondary, order);
}

std::wstring AppSettings::GetAppListTitleLanguagePreference() const {
  return value<std::wstring>(AppSettingKey::AppListTitleLanguagePreference);
}

void AppSettings::SetAppListTitleLanguagePreference(const std::wstring& language) {
  set_value(AppSettingKey::AppListTitleLanguagePreference, language);
}

bool AppSettings::GetAppBehaviorAutostart() const {
  return value<bool>(AppSettingKey::AppBehaviorAutostart);
}

void AppSettings::SetAppBehaviorAutostart(const bool enabled) {
  set_value(AppSettingKey::AppBehaviorAutostart, enabled);

  win::Registry registry;
  registry.OpenKey(HKEY_CURRENT_USER,
                   L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                   0, KEY_SET_VALUE);
  if (enabled) {
    registry.SetValue(TAIGA_APP_NAME, Taiga.GetModulePath());
  } else {
    registry.DeleteValue(TAIGA_APP_NAME);
  }
}

bool AppSettings::GetAppBehaviorStartMinimized() const {
  return value<bool>(AppSettingKey::AppBehaviorStartMinimized);
}

void AppSettings::SetAppBehaviorStartMinimized(const bool enabled) {
  set_value(AppSettingKey::AppBehaviorStartMinimized, enabled);
}

bool AppSettings::GetAppBehaviorCheckForUpdates() const {
  return value<bool>(AppSettingKey::AppBehaviorCheckForUpdates);
}

void AppSettings::SetAppBehaviorCheckForUpdates(const bool enabled) {
  set_value(AppSettingKey::AppBehaviorCheckForUpdates, enabled);
}

bool AppSettings::GetAppBehaviorScanAvailableEpisodes() const {
  return value<bool>(AppSettingKey::AppBehaviorScanAvailableEpisodes);
}

void AppSettings::SetAppBehaviorScanAvailableEpisodes(const bool enabled) {
  set_value(AppSettingKey::AppBehaviorScanAvailableEpisodes, enabled);
}

bool AppSettings::GetAppBehaviorCloseToTray() const {
  return value<bool>(AppSettingKey::AppBehaviorCloseToTray);
}

void AppSettings::SetAppBehaviorCloseToTray(const bool enabled) {
  set_value(AppSettingKey::AppBehaviorCloseToTray, enabled);
}

bool AppSettings::GetAppBehaviorMinimizeToTray() const {
  return value<bool>(AppSettingKey::AppBehaviorMinimizeToTray);
}

void AppSettings::SetAppBehaviorMinimizeToTray(const bool enabled) {
  set_value(AppSettingKey::AppBehaviorMinimizeToTray, enabled);
}

std::wstring AppSettings::GetAppConnectionProxyHost() const {
  return value<std::wstring>(AppSettingKey::AppConnectionProxyHost);
}

void AppSettings::SetAppConnectionProxyHost(const std::wstring& host) {
  set_value(AppSettingKey::AppConnectionProxyHost, host);
}

std::wstring AppSettings::GetAppConnectionProxyUsername() const {
  return value<std::wstring>(AppSettingKey::AppConnectionProxyUsername);
}

void AppSettings::SetAppConnectionProxyUsername(const std::wstring& username) {
  set_value(AppSettingKey::AppConnectionProxyUsername, username);
}

std::wstring AppSettings::GetAppConnectionProxyPassword() const {
  return value<std::wstring>(AppSettingKey::AppConnectionProxyPassword);
}

void AppSettings::SetAppConnectionProxyPassword(const std::wstring& password) {
  set_value(AppSettingKey::AppConnectionProxyPassword, password);
}

bool AppSettings::GetAppConnectionNoRevoke() const {
  return value<bool>(AppSettingKey::AppConnectionNoRevoke);
}

void AppSettings::SetAppConnectionNoRevoke(const bool enabled) {
  set_value(AppSettingKey::AppConnectionNoRevoke, enabled);
}

bool AppSettings::GetAppConnectionReuseActive() const {
  return value<bool>(AppSettingKey::AppConnectionReuseActive);
}

void AppSettings::SetAppConnectionReuseActive(const bool enabled) {
  set_value(AppSettingKey::AppConnectionReuseActive, enabled);
}

std::wstring AppSettings::GetAppInterfaceTheme() const {
  return value<std::wstring>(AppSettingKey::AppInterfaceTheme);
}

void AppSettings::SetAppInterfaceTheme(const std::wstring& theme) {
  const auto previous_theme = GetAppInterfaceTheme();
  if (set_value(AppSettingKey::AppInterfaceTheme, theme)) {
    if (ui::Theme.Load()) {
      ui::OnSettingsThemeChange();
    } else {
      set_value(AppSettingKey::AppInterfaceTheme, previous_theme);
    }
  }
}

std::wstring AppSettings::GetAppInterfaceExternalLinks() const {
  return value<std::wstring>(AppSettingKey::AppInterfaceExternalLinks);
}

void AppSettings::SetAppInterfaceExternalLinks(const std::wstring& str) {
  if (set_value(AppSettingKey::AppInterfaceExternalLinks, str)) {
    ui::Menus.UpdateExternalLinks();
  }
}

// Recognition

int AppSettings::GetRecognitionDetectionInterval() const {
  return value<int>(AppSettingKey::RecognitionDetectionInterval);
}

void AppSettings::SetRecognitionDetectionInterval(const int seconds) {
  set_value(AppSettingKey::RecognitionDetectionInterval, seconds);
}

bool AppSettings::GetRecognitionDetectMediaPlayers() const {
  return value<bool>(AppSettingKey::RecognitionDetectMediaPlayers);
}

void AppSettings::SetRecognitionDetectMediaPlayers(const bool enabled) {
  set_value(AppSettingKey::RecognitionDetectMediaPlayers, enabled);
}

bool AppSettings::GetRecognitionDetectStreamingMedia() const {
  return value<bool>(AppSettingKey::RecognitionDetectStreamingMedia);
}

void AppSettings::SetRecognitionDetectStreamingMedia(const bool enabled) {
  set_value(AppSettingKey::RecognitionDetectStreamingMedia, enabled);
}

std::wstring AppSettings::GetRecognitionIgnoredStrings() const {
  return value<std::wstring>(AppSettingKey::RecognitionIgnoredStrings);
}

void AppSettings::SetRecognitionIgnoredStrings(const std::wstring& str) {
  set_value(AppSettingKey::RecognitionIgnoredStrings, str);
}

bool AppSettings::GetRecognitionLookupParentDirectories() const {
  return value<bool>(AppSettingKey::RecognitionLookupParentDirectories);
}

void AppSettings::SetRecognitionLookupParentDirectories(const bool enabled) {
  set_value(AppSettingKey::RecognitionLookupParentDirectories, enabled);
}

std::wstring AppSettings::GetRecognitionRelationsLastModified() const {
  return value<std::wstring>(AppSettingKey::RecognitionRelationsLastModified);
}

void AppSettings::SetRecognitionRelationsLastModified(const std::wstring& last_modified) {
  set_value(AppSettingKey::RecognitionRelationsLastModified, last_modified);
}

int AppSettings::GetSyncUpdateDelay() const {
  return value<int>(AppSettingKey::SyncUpdateDelay);
}

void AppSettings::SetSyncUpdateDelay(const int seconds) {
  set_value(AppSettingKey::SyncUpdateDelay, seconds);
}

bool AppSettings::GetSyncUpdateAskToConfirm() const {
  return value<bool>(AppSettingKey::SyncUpdateAskToConfirm);
}

void AppSettings::SetSyncUpdateAskToConfirm(const bool enabled) {
  set_value(AppSettingKey::SyncUpdateAskToConfirm, enabled);
}

bool AppSettings::GetSyncUpdateCheckPlayer() const {
  return value<bool>(AppSettingKey::SyncUpdateCheckPlayer);
}

void AppSettings::SetSyncUpdateCheckPlayer(const bool enabled) {
  set_value(AppSettingKey::SyncUpdateCheckPlayer, enabled);
}

bool AppSettings::GetSyncUpdateOutOfRange() const {
  return value<bool>(AppSettingKey::SyncUpdateOutOfRange);
}

void AppSettings::SetSyncUpdateOutOfRange(const bool enabled) {
  set_value(AppSettingKey::SyncUpdateOutOfRange, enabled);
}

bool AppSettings::GetSyncUpdateOutOfRoot() const {
  return value<bool>(AppSettingKey::SyncUpdateOutOfRoot);
}

void AppSettings::SetSyncUpdateOutOfRoot(const bool enabled) {
  set_value(AppSettingKey::SyncUpdateOutOfRoot, enabled);
}

bool AppSettings::GetSyncUpdateWaitPlayer() const {
  return value<bool>(AppSettingKey::SyncUpdateWaitPlayer);
}

void AppSettings::SetSyncUpdateWaitPlayer(const bool enabled) {
  set_value(AppSettingKey::SyncUpdateWaitPlayer, enabled);
}

bool AppSettings::GetSyncGoToNowPlayingRecognized() const {
  return value<bool>(AppSettingKey::SyncGoToNowPlayingRecognized);
}

void AppSettings::SetSyncGoToNowPlayingRecognized(const bool enabled) {
  set_value(AppSettingKey::SyncGoToNowPlayingRecognized, enabled);
}

bool AppSettings::GetSyncGoToNowPlayingNotRecognized() const {
  return value<bool>(AppSettingKey::SyncGoToNowPlayingNotRecognized);
}

void AppSettings::SetSyncGoToNowPlayingNotRecognized(const bool enabled) {
  set_value(AppSettingKey::SyncGoToNowPlayingNotRecognized, enabled);
}

bool AppSettings::GetSyncNotifyRecognized() const {
  return value<bool>(AppSettingKey::SyncNotifyRecognized);
}

void AppSettings::SetSyncNotifyRecognized(const bool enabled) {
  set_value(AppSettingKey::SyncNotifyRecognized, enabled);
}

bool AppSettings::GetSyncNotifyNotRecognized() const {
  return value<bool>(AppSettingKey::SyncNotifyNotRecognized);
}

void AppSettings::SetSyncNotifyNotRecognized(const bool enabled) {
  set_value(AppSettingKey::SyncNotifyNotRecognized, enabled);
}

std::wstring AppSettings::GetSyncNotifyFormat() const {
  return value<std::wstring>(AppSettingKey::SyncNotifyFormat);
}

void AppSettings::SetSyncNotifyFormat(const std::wstring& format) {
  set_value(AppSettingKey::SyncNotifyFormat, format);
}

bool AppSettings::GetStreamAnimelab() const {
  return value<bool>(AppSettingKey::StreamAnimelab);
}

void AppSettings::SetStreamAnimelab(const bool enabled) {
  set_value(AppSettingKey::StreamAnimelab, enabled);
}

bool AppSettings::GetStreamAdn() const {
  return value<bool>(AppSettingKey::StreamAdn);
}

void AppSettings::SetStreamAdn(const bool enabled) {
  set_value(AppSettingKey::StreamAdn, enabled);
}

bool AppSettings::GetStreamAnn() const {
  return value<bool>(AppSettingKey::StreamAnn);
}

void AppSettings::SetStreamAnn(const bool enabled) {
  set_value(AppSettingKey::StreamAnn, enabled);
}

bool AppSettings::GetStreamCrunchyroll() const {
  return value<bool>(AppSettingKey::StreamCrunchyroll);
}

void AppSettings::SetStreamCrunchyroll(const bool enabled) {
  set_value(AppSettingKey::StreamCrunchyroll, enabled);
}

bool AppSettings::GetStreamFunimation() const {
  return value<bool>(AppSettingKey::StreamFunimation);
}

void AppSettings::SetStreamFunimation(const bool enabled) {
  set_value(AppSettingKey::StreamFunimation, enabled);
}

bool AppSettings::GetStreamHidive() const {
  return value<bool>(AppSettingKey::StreamHidive);
}

void AppSettings::SetStreamHidive(const bool enabled) {
  set_value(AppSettingKey::StreamHidive, enabled);
}

bool AppSettings::GetStreamPlex() const {
  return value<bool>(AppSettingKey::StreamPlex);
}

void AppSettings::SetStreamPlex(const bool enabled) {
  set_value(AppSettingKey::StreamPlex, enabled);
}

bool AppSettings::GetStreamVeoh() const {
  return value<bool>(AppSettingKey::StreamVeoh);
}

void AppSettings::SetStreamVeoh(const bool enabled) {
  set_value(AppSettingKey::StreamVeoh, enabled);
}

bool AppSettings::GetStreamViz() const {
  return value<bool>(AppSettingKey::StreamViz);
}

void AppSettings::SetStreamViz(const bool enabled) {
  set_value(AppSettingKey::StreamViz, enabled);
}

bool AppSettings::GetStreamVrv() const {
  return value<bool>(AppSettingKey::StreamVrv);
}

void AppSettings::SetStreamVrv(const bool enabled) {
  set_value(AppSettingKey::StreamVrv, enabled);
}

bool AppSettings::GetStreamWakanim() const {
  return value<bool>(AppSettingKey::StreamWakanim);
}

void AppSettings::SetStreamWakanim(const bool enabled) {
  set_value(AppSettingKey::StreamWakanim, enabled);
}

bool AppSettings::GetStreamYahoo() const {
  return value<bool>(AppSettingKey::StreamYahoo);
}

void AppSettings::SetStreamYahoo(const bool enabled) {
  set_value(AppSettingKey::StreamYahoo, enabled);
}

bool AppSettings::GetStreamYoutube() const {
  return value<bool>(AppSettingKey::StreamYoutube);
}

void AppSettings::SetStreamYoutube(const bool enabled) {
  set_value(AppSettingKey::StreamYoutube, enabled);
}

// Sharing

std::wstring AppSettings::GetShareDiscordApplicationId() const {
  return value<std::wstring>(AppSettingKey::ShareDiscordApplicationId);
}

void AppSettings::SetShareDiscordApplicationId(const std::wstring& application_id) {
  set_value(AppSettingKey::ShareDiscordApplicationId, application_id);
}

bool AppSettings::GetShareDiscordEnabled() const {
  return value<bool>(AppSettingKey::ShareDiscordEnabled);
}

void AppSettings::SetShareDiscordEnabled(const bool enabled) {
  if (set_value(AppSettingKey::ShareDiscordEnabled, enabled)) {
    if (enabled) {
      link::discord::Initialize();
      ::Announcer.Do(kAnnounceToDiscord);
    } else {
      link::discord::Shutdown();
    }
  }
}

std::wstring AppSettings::GetShareDiscordFormatDetails() const {
  return value<std::wstring>(AppSettingKey::ShareDiscordFormatDetails);
}

void AppSettings::SetShareDiscordFormatDetails(const std::wstring& format) {
  set_value(AppSettingKey::ShareDiscordFormatDetails, format);
}

std::wstring AppSettings::GetShareDiscordFormatState() const {
  return value<std::wstring>(AppSettingKey::ShareDiscordFormatState);
}

void AppSettings::SetShareDiscordFormatState(const std::wstring& format) {
  set_value(AppSettingKey::ShareDiscordFormatState, format);
}

bool AppSettings::GetShareDiscordUsernameEnabled() const {
  return value<bool>(AppSettingKey::ShareDiscordUsernameEnabled);
}

void AppSettings::SetShareDiscordUsernameEnabled(const bool enabled) {
  set_value(AppSettingKey::ShareDiscordUsernameEnabled, enabled);
}

bool AppSettings::GetShareHttpEnabled() const {
  return value<bool>(AppSettingKey::ShareHttpEnabled);
}

void AppSettings::SetShareHttpEnabled(const bool enabled) {
  set_value(AppSettingKey::ShareHttpEnabled, enabled);
}

std::wstring AppSettings::GetShareHttpFormat() const {
  return value<std::wstring>(AppSettingKey::ShareHttpFormat);
}

void AppSettings::SetShareHttpFormat(const std::wstring& format) {
  set_value(AppSettingKey::ShareHttpFormat, format);
}

std::wstring AppSettings::GetShareHttpUrl() const {
  return value<std::wstring>(AppSettingKey::ShareHttpUrl);
}

void AppSettings::SetShareHttpUrl(const std::wstring& url) {
  set_value(AppSettingKey::ShareHttpUrl, url);
}

bool AppSettings::GetShareMircEnabled() const {
  return value<bool>(AppSettingKey::ShareMircEnabled);
}

void AppSettings::SetShareMircEnabled(const bool enabled) {
  set_value(AppSettingKey::ShareMircEnabled, enabled);
}

bool AppSettings::GetShareMircMultiServer() const {
  return value<bool>(AppSettingKey::ShareMircMultiServer);
}

void AppSettings::SetShareMircMultiServer(const bool enabled) {
  set_value(AppSettingKey::ShareMircMultiServer, enabled);
}

bool AppSettings::GetShareMircUseMeAction() const {
  return value<bool>(AppSettingKey::ShareMircUseMeAction);
}

void AppSettings::SetShareMircUseMeAction(const bool enabled) {
  set_value(AppSettingKey::ShareMircUseMeAction, enabled);
}

int AppSettings::GetShareMircMode() const {
  return value<int>(AppSettingKey::ShareMircMode);
}

void AppSettings::SetShareMircMode(const int mode) {
  set_value(AppSettingKey::ShareMircMode, mode);
}

std::wstring AppSettings::GetShareMircChannels() const {
  return value<std::wstring>(AppSettingKey::ShareMircChannels);
}

void AppSettings::SetShareMircChannels(const std::wstring& channels) {
  set_value(AppSettingKey::ShareMircChannels, channels);
}

std::wstring AppSettings::GetShareMircFormat() const {
  return value<std::wstring>(AppSettingKey::ShareMircFormat);
}

void AppSettings::SetShareMircFormat(const std::wstring& format) {
  set_value(AppSettingKey::ShareMircFormat, format);
}

std::wstring AppSettings::GetShareMircService() const {
  return value<std::wstring>(AppSettingKey::ShareMircService);
}

void AppSettings::SetShareMircService(const std::wstring& service) {
  set_value(AppSettingKey::ShareMircService, service);
}

bool AppSettings::GetShareTwitterEnabled() const {
  return value<bool>(AppSettingKey::ShareTwitterEnabled);
}

void AppSettings::SetShareTwitterEnabled(const bool enabled) {
  set_value(AppSettingKey::ShareTwitterEnabled, enabled);
}

std::wstring AppSettings::GetShareTwitterFormat() const {
  return value<std::wstring>(AppSettingKey::ShareTwitterFormat);
}

void AppSettings::SetShareTwitterFormat(const std::wstring& format) {
  set_value(AppSettingKey::ShareTwitterFormat, format);
}

std::wstring AppSettings::GetShareTwitterOauthToken() const {
  return value<std::wstring>(AppSettingKey::ShareTwitterOauthToken);
}

void AppSettings::SetShareTwitterOauthToken(const std::wstring& oauth_token) {
  set_value(AppSettingKey::ShareTwitterOauthToken, oauth_token);
}

std::wstring AppSettings::GetShareTwitterOauthSecret() const {
  return value<std::wstring>(AppSettingKey::ShareTwitterOauthSecret);
}

void AppSettings::SetShareTwitterOauthSecret(const std::wstring& oauth_secret) {
  set_value(AppSettingKey::ShareTwitterOauthSecret, oauth_secret);
}

std::wstring AppSettings::GetShareTwitterUsername() const {
  return value<std::wstring>(AppSettingKey::ShareTwitterUsername);
}

void AppSettings::SetShareTwitterUsername(const std::wstring& username) {
  set_value(AppSettingKey::ShareTwitterUsername, username);
}

// Torrents

std::wstring AppSettings::GetTorrentDiscoverySource() const {
  return value<std::wstring>(AppSettingKey::TorrentDiscoverySource);
}

void AppSettings::SetTorrentDiscoverySource(const std::wstring& url) {
  set_value(AppSettingKey::TorrentDiscoverySource, url);
}

std::wstring AppSettings::GetTorrentDiscoverySearchUrl() const {
  return value<std::wstring>(AppSettingKey::TorrentDiscoverySearchUrl);
}

void AppSettings::SetTorrentDiscoverySearchUrl(const std::wstring& url) {
  set_value(AppSettingKey::TorrentDiscoverySearchUrl, url);
}

bool AppSettings::GetTorrentDiscoveryAutoCheckEnabled() const {
  return value<bool>(AppSettingKey::TorrentDiscoveryAutoCheckEnabled);
}

void AppSettings::SetTorrentDiscoveryAutoCheckEnabled(const bool enabled) {
  set_value(AppSettingKey::TorrentDiscoveryAutoCheckEnabled, enabled);
}

int AppSettings::GetTorrentDiscoveryAutoCheckInterval() const {
  return value<int>(AppSettingKey::TorrentDiscoveryAutoCheckInterval);
}

void AppSettings::SetTorrentDiscoveryAutoCheckInterval(const int minutes) {
  set_value(AppSettingKey::TorrentDiscoveryAutoCheckInterval, minutes);
}

int AppSettings::GetTorrentDiscoveryNewAction() const {
  return value<int>(AppSettingKey::TorrentDiscoveryNewAction);
}

void AppSettings::SetTorrentDiscoveryNewAction(const int action) {
  set_value(AppSettingKey::TorrentDiscoveryNewAction, action);
}

int AppSettings::GetTorrentDownloadAppMode() const {
  return value<int>(AppSettingKey::TorrentDownloadAppMode);
}

void AppSettings::SetTorrentDownloadAppMode(const int mode) {
  set_value(AppSettingKey::TorrentDownloadAppMode, mode);
}

bool AppSettings::GetTorrentDownloadAppOpen() const {
  return value<bool>(AppSettingKey::TorrentDownloadAppOpen);
}

void AppSettings::SetTorrentDownloadAppOpen(const bool enabled) {
  set_value(AppSettingKey::TorrentDownloadAppOpen, enabled);
}

std::wstring AppSettings::GetTorrentDownloadAppPath() const {
  return value<std::wstring>(AppSettingKey::TorrentDownloadAppPath);
}

void AppSettings::SetTorrentDownloadAppPath(const std::wstring& path) {
  set_value(AppSettingKey::TorrentDownloadAppPath, path);
}

std::wstring AppSettings::GetTorrentDownloadLocation() const {
  return value<std::wstring>(AppSettingKey::TorrentDownloadLocation);
}

void AppSettings::SetTorrentDownloadLocation(const std::wstring& path) {
  set_value(AppSettingKey::TorrentDownloadLocation, path);
}

bool AppSettings::GetTorrentDownloadUseAnimeFolder() const {
  return value<bool>(AppSettingKey::TorrentDownloadUseAnimeFolder);
}

void AppSettings::SetTorrentDownloadUseAnimeFolder(const bool enabled) {
  set_value(AppSettingKey::TorrentDownloadUseAnimeFolder, enabled);
}

bool AppSettings::GetTorrentDownloadFallbackOnFolder() const {
  return value<bool>(AppSettingKey::TorrentDownloadFallbackOnFolder);
}

void AppSettings::SetTorrentDownloadFallbackOnFolder(const bool enabled) {
  set_value(AppSettingKey::TorrentDownloadFallbackOnFolder, enabled);
}

bool AppSettings::GetTorrentDownloadCreateSubfolder() const {
  return value<bool>(AppSettingKey::TorrentDownloadCreateSubfolder);
}

void AppSettings::SetTorrentDownloadCreateSubfolder(const bool enabled) {
  set_value(AppSettingKey::TorrentDownloadCreateSubfolder, enabled);
}

std::wstring AppSettings::GetTorrentDownloadSortBy() const {
  return value<std::wstring>(AppSettingKey::TorrentDownloadSortBy);
}

void AppSettings::SetTorrentDownloadSortBy(const std::wstring& sort) {
  set_value(AppSettingKey::TorrentDownloadSortBy, sort);
}

std::wstring AppSettings::GetTorrentDownloadSortOrder() const {
  return value<std::wstring>(AppSettingKey::TorrentDownloadSortOrder);
}

void AppSettings::SetTorrentDownloadSortOrder(const std::wstring& order) {
  set_value(AppSettingKey::TorrentDownloadSortOrder, order);
}

bool AppSettings::GetTorrentDownloadUseMagnet() const {
  return value<bool>(AppSettingKey::TorrentDownloadUseMagnet);
}

void AppSettings::SetTorrentDownloadUseMagnet(const bool enabled) {
  set_value(AppSettingKey::TorrentDownloadUseMagnet, enabled);
}

bool AppSettings::GetTorrentFilterEnabled() const {
  return value<bool>(AppSettingKey::TorrentFilterEnabled);
}

void AppSettings::SetTorrentFilterEnabled(const bool enabled) {
  set_value(AppSettingKey::TorrentFilterEnabled, enabled);
}

int AppSettings::GetTorrentFilterArchiveMaxCount() const {
  return value<int>(AppSettingKey::TorrentFilterArchiveMaxCount);
}

void AppSettings::SetTorrentFilterArchiveMaxCount(const int count) {
  set_value(AppSettingKey::TorrentFilterArchiveMaxCount, count);
}

// Internal

int AppSettings::GetAppPositionX() const {
  return value<int>(AppSettingKey::AppPositionX);
}

void AppSettings::SetAppPositionX(const int x) {
  set_value(AppSettingKey::AppPositionX, x);
}

int AppSettings::GetAppPositionY() const {
  return value<int>(AppSettingKey::AppPositionY);
}

void AppSettings::SetAppPositionY(const int y) {
  set_value(AppSettingKey::AppPositionY, y);
}

int AppSettings::GetAppPositionW() const {
  return value<int>(AppSettingKey::AppPositionW);
}

void AppSettings::SetAppPositionW(const int width) {
  set_value(AppSettingKey::AppPositionW, width);
}

int AppSettings::GetAppPositionH() const {
  return value<int>(AppSettingKey::AppPositionH);
}

void AppSettings::SetAppPositionH(const int height) {
  set_value(AppSettingKey::AppPositionH, height);
}

bool AppSettings::GetAppPositionMaximized() const {
  return value<bool>(AppSettingKey::AppPositionMaximized);
}

void AppSettings::SetAppPositionMaximized(const bool enabled) {
  set_value(AppSettingKey::AppPositionMaximized, enabled);
}

bool AppSettings::GetAppPositionRemember() const {
  return value<bool>(AppSettingKey::AppPositionRemember);
}

void AppSettings::SetAppPositionRemember(const bool enabled) {
  set_value(AppSettingKey::AppPositionRemember, enabled);
}

bool AppSettings::GetAppOptionHideSidebar() const {
  return value<bool>(AppSettingKey::AppOptionHideSidebar);
}

void AppSettings::SetAppOptionHideSidebar(const bool enabled) {
  set_value(AppSettingKey::AppOptionHideSidebar, enabled);
}

bool AppSettings::GetAppOptionEnableRecognition() const {
  return value<bool>(AppSettingKey::AppOptionEnableRecognition);
}

void AppSettings::SetAppOptionEnableRecognition(const bool enabled) {
  set_value(AppSettingKey::AppOptionEnableRecognition, enabled);
}

bool AppSettings::GetAppOptionEnableSharing() const {
  return value<bool>(AppSettingKey::AppOptionEnableSharing);
}

void AppSettings::SetAppOptionEnableSharing(const bool enabled) {
  set_value(AppSettingKey::AppOptionEnableSharing, enabled);
}

bool AppSettings::GetAppOptionEnableSync() const {
  return value<bool>(AppSettingKey::AppOptionEnableSync);
}

void AppSettings::SetAppOptionEnableSync(const bool enabled) {
  set_value(AppSettingKey::AppOptionEnableSync, enabled);
}

std::wstring AppSettings::GetAppSeasonsLastSeason() const {
  return value<std::wstring>(AppSettingKey::AppSeasonsLastSeason);
}

void AppSettings::SetAppSeasonsLastSeason(const std::wstring& season) {
  set_value(AppSettingKey::AppSeasonsLastSeason, season);
}

int AppSettings::GetAppSeasonsGroupBy() const {
  return value<int>(AppSettingKey::AppSeasonsGroupBy);
}

void AppSettings::SetAppSeasonsGroupBy(const int group_by) {
  set_value(AppSettingKey::AppSeasonsGroupBy, group_by);
}

int AppSettings::GetAppSeasonsSortBy() const {
  return value<int>(AppSettingKey::AppSeasonsSortBy);
}

void AppSettings::SetAppSeasonsSortBy(const int sort_by) {
  set_value(AppSettingKey::AppSeasonsSortBy, sort_by);
}

int AppSettings::GetAppSeasonsViewAs() const {
  return value<int>(AppSettingKey::AppSeasonsViewAs);
}

void AppSettings::SetAppSeasonsViewAs(const int view_as) {
  set_value(AppSettingKey::AppSeasonsViewAs, view_as);
}

////////////////////////////////////////////////////////////////////////////////

sync::Service* AppSettings::GetCurrentService() const {
  return ServiceManager.service(GetSyncActiveService());
}

sync::ServiceId AppSettings::GetCurrentServiceId() const {
  const auto service = GetCurrentService();

  if (service)
    return static_cast<sync::ServiceId>(service->id());

  return sync::kMyAnimeList;
}

std::wstring AppSettings::GetUserDisplayName(sync::ServiceId service_id) const {
  switch (service_id) {
    case sync::kKitsu: {
      const auto display_name = GetSyncServiceKitsuDisplayName();
      if (!display_name.empty())
        return display_name;
      break;
    }
  }

  return GetUsername(service_id);
}

std::wstring AppSettings::GetUserEmail(sync::ServiceId service_id) const {
  switch (service_id) {
    case sync::kKitsu:
      return GetSyncServiceKitsuEmail();
    default:
      return {};
  }
}

std::wstring AppSettings::GetUsername(sync::ServiceId service_id) const {
  switch (service_id) {
    case sync::kMyAnimeList:
      return GetSyncServiceMalUsername();
    case sync::kKitsu:
      return GetSyncServiceKitsuUsername();
    case sync::kAniList:
      return GetSyncServiceAniListUsername();
    default:
      return {};
  }
}

std::wstring AppSettings::GetPassword(sync::ServiceId service_id) const {
  switch (service_id) {
    case sync::kMyAnimeList:
      return Base64Decode(GetSyncServiceMalPassword());
    case sync::kKitsu:
      return Base64Decode(GetSyncServiceKitsuPassword());
    case sync::kAniList:
      return GetSyncServiceAniListToken();
    default:
      return {};
  }
}

std::wstring AppSettings::GetCurrentUserDisplayName() const {
  return GetUserDisplayName(GetCurrentServiceId());
}

std::wstring AppSettings::GetCurrentUserEmail() const {
  return GetUserEmail(GetCurrentServiceId());
}

std::wstring AppSettings::GetCurrentUsername() const {
  return GetUsername(GetCurrentServiceId());
}

std::wstring AppSettings::GetCurrentPassword() const {
  return GetPassword(GetCurrentServiceId());
}

sync::Service* GetCurrentService() {
  return settings.GetCurrentService();
}

sync::ServiceId GetCurrentServiceId() {
  return settings.GetCurrentServiceId();
}

std::wstring GetCurrentUserDisplayName() {
  return settings.GetCurrentUserDisplayName();
}

std::wstring GetCurrentUserEmail() {
  return settings.GetCurrentUserEmail();
}

std::wstring GetCurrentUsername() {
  return settings.GetCurrentUsername();
}

std::wstring GetCurrentPassword() {
  return settings.GetCurrentPassword();
}

}  // namespace taiga
