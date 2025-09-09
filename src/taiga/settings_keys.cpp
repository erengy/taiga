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

#include <semaver.hpp>
#include <windows/win/registry.h>

#include "taiga/settings_keys.h"

#include "base/file.h"
#include "base/log.h"
#include "base/string.h"
#include "link/discord.h"
#include "link/mirc.h"
#include "media/anime_db.h"
#include "media/anime_season_db.h"
#include "media/library/queue.h"
#include "sync/service.h"
#include "taiga/announce.h"
#include "taiga/app.h"
#include "taiga/config.h"
#include "taiga/settings.h"
#include "track/feed_aggregator.h"
#include "track/monitor.h"
#include "ui/dlg/dlg_anime_list.h"
#include "ui/dlg/dlg_season.h"
#include "ui/list.h"
#include "ui/menu.h"
#include "ui/resource.h"
#include "ui/theme.h"
#include "ui/ui.h"

namespace taiga {

constexpr auto kDefaultExternalLinks =
    L"MALgraph|https://anime.plus/\r\n"
    L"-\r\n"
    L"AniChart|https://anichart.net/airing\r\n"
    L"Monthly.moe|https://www.monthly.moe/weekly\r\n"
    L"Senpai Anime Charts|https://www.senpai.moe/?mode=calendar\r\n"
    L"-\r\n"
    L"Anime Scene Search Engine|https://trace.moe/\r\n"
    L"Anime Streaming Search Engine|https://because.moe/";
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
    L"C:\\Program Files\\qBittorrent\\qbittorrent.exe";
constexpr auto kDefaultTorrentSearch =
    L"https://nyaa.si/?page=rss&c=1_2&f=0&q=%title%";
constexpr auto kDefaultTorrentSource =
    L"https://www.tokyotosho.info/rss.php?filter=1,11&zwnj=0";

// Here we assume that anything less than 10 MiB can't be a valid episode.
constexpr int kDefaultFileSizeThreshold = 1024 * 1024 * 10;

////////////////////////////////////////////////////////////////////////////////

void Settings::InitKeyMap() const {
  if (key_map_.empty()) {
    key_map_ = {
      // Meta
      {AppSettingKey::MetaVersion, {"meta/version", std::wstring{}}},

      // Services
      {AppSettingKey::SyncActiveService, {"account/update/activeservice", std::wstring{L"anilist"}}},
      {AppSettingKey::SyncAutoOnStart, {"account/myanimelist/login", false}},
      {AppSettingKey::SyncServiceMalUsername, {"account/myanimelist/username", std::wstring{}}},
      {AppSettingKey::SyncServiceMalAccessToken, {"account/myanimelist/accesstoken", std::wstring{}}},
      {AppSettingKey::SyncServiceMalRefreshToken, {"account/myanimelist/refreshtoken", std::wstring{}}},
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
      {AppSettingKey::AppListDoubleClickAction, {"program/list/action/doubleclick", ui::kAnimeListActionInfo}},
      {AppSettingKey::AppListMiddleClickAction, {"program/list/action/middleclick", ui::kAnimeListActionPlayNext}},
      {AppSettingKey::AppListDisplayEnglishTitles, {"program/list/action/englishtitles", false}},
      {AppSettingKey::AppListHighlightNewEpisodes, {"program/list/filter/episodes/highlight", true}},
      {AppSettingKey::AppListDisplayHighlightedOnTop, {"program/list/filter/episodes/highlightedontop", false}},
      {AppSettingKey::AppListProgressDisplayAired, {"program/list/progress/showaired", true}},
      {AppSettingKey::AppListProgressDisplayAvailable, {"program/list/progress/showavailable", true}},
      {AppSettingKey::AppListSortColumnPrimary, {"program/list/sort/column", std::wstring{L"anime_title"}}},
      {AppSettingKey::AppListSortColumnSecondary, {"program/list/sort/column2", std::wstring{L"anime_title"}}},
      {AppSettingKey::AppListSortOrderPrimary, {"program/list/sort/order", ui::kListSortAscending}},
      {AppSettingKey::AppListSortOrderSecondary, {"program/list/sort/order2", ui::kListSortAscending}},
      {AppSettingKey::AppListTitleLanguagePreference, {"program/list/action/titlelang", std::wstring{L"romaji"}}},
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
      {AppSettingKey::StreamBilibili, {"recognition/streaming/providers/bilibili", true}},
      {AppSettingKey::StreamCrunchyroll, {"recognition/streaming/providers/crunchyroll", true}},
      {AppSettingKey::StreamFunimation, {"recognition/streaming/providers/funimation", true}},
      {AppSettingKey::StreamHidive, {"recognition/streaming/providers/hidive", true}},
      {AppSettingKey::StreamJellyfin, {"recognition/streaming/providers/jellyfin", true}},
      {AppSettingKey::StreamPlex, {"recognition/streaming/providers/plex", true}},
      {AppSettingKey::StreamRokuChannel, {"recognition/streaming/providers/rokuchannel", true}},
      {AppSettingKey::StreamTubi, {"recognition/streaming/providers/tubi", true}},
      {AppSettingKey::StreamVeoh, {"recognition/streaming/providers/veoh", true}},
      {AppSettingKey::StreamViz, {"recognition/streaming/providers/viz", true}},
      {AppSettingKey::StreamVrv, {"recognition/streaming/providers/vrv", true}},
      {AppSettingKey::StreamWakanim, {"recognition/streaming/providers/wakanim", true}},
      {AppSettingKey::StreamYahoo, {"recognition/streaming/providers/yahoo", true}},
      {AppSettingKey::StreamYoutube, {"recognition/streaming/providers/youtube", true}},

      // Sharing
      {AppSettingKey::ShareDiscordApplicationId, {"announce/discord/applicationid", std::wstring{link::discord::kApplicationId}}},
      {AppSettingKey::ShareDiscordEnabled, {"announce/discord/enabled", false}},
      {AppSettingKey::ShareDiscordGroupEnabled, {"announce/discord/groupenabled", true}},
      {AppSettingKey::ShareDiscordTimeEnabled, {"announce/discord/timeenabled", true}},
      {AppSettingKey::ShareDiscordUsernameEnabled, {"announce/discord/usernameenabled", true}},
      {AppSettingKey::ShareDiscordWatchingEnabled, {"announce/discord/watchingenabled", true}},
      {AppSettingKey::ShareHttpEnabled, {"announce/http/enabled", false}},
      {AppSettingKey::ShareHttpFormat, {"announce/http/format", std::wstring{kDefaultFormatHttp}}},
      {AppSettingKey::ShareHttpUrl, {"announce/http/url", std::wstring{}}},
      {AppSettingKey::ShareMircEnabled, {"announce/mirc/enabled", false}},
      {AppSettingKey::ShareMircMultiServer, {"announce/mirc/multiserver", false}},
      {AppSettingKey::ShareMircUseMeAction, {"announce/mirc/useaction", true}},
      {AppSettingKey::ShareMircMode, {"announce/mirc/mode", link::mirc::kChannelModeActive}},
      {AppSettingKey::ShareMircChannels, {"announce/mirc/channels", std::wstring{L"#kitsu, #myanimelist, #taiga"}}},
      {AppSettingKey::ShareMircFormat, {"announce/mirc/format", std::wstring{kDefaultFormatMirc}}},
      {AppSettingKey::ShareMircService, {"announce/mirc/service", std::wstring{L"mIRC"}}},
      {AppSettingKey::ShareTwitterEnabled, {"announce/twitter/enabled", false}},
      {AppSettingKey::ShareTwitterFormat, {"announce/twitter/format", std::wstring{kDefaultFormatTwitter}}},
      {AppSettingKey::ShareTwitterOauthToken, {"announce/twitter/oauth_token", std::wstring{}}},
      {AppSettingKey::ShareTwitterOauthSecret, {"announce/twitter/oauth_secret", std::wstring{}}},
      {AppSettingKey::ShareTwitterReplyTo, {"announce/twitter/reply_to", std::wstring{}}},
      {AppSettingKey::ShareTwitterUsername, {"announce/twitter/user", std::wstring{}}},

      // Torrents
      {AppSettingKey::TorrentDiscoverySource, {"rss/torrent/source/address", std::wstring{kDefaultTorrentSource}}},
      {AppSettingKey::TorrentDiscoverySearchUrl, {"rss/torrent/search/address", std::wstring{kDefaultTorrentSearch}}},
      {AppSettingKey::TorrentDiscoveryAutoCheckEnabled, {"rss/torrent/options/autocheck", true}},
      {AppSettingKey::TorrentDiscoveryAutoCheckInterval, {"rss/torrent/options/checkinterval", 60}},
      {AppSettingKey::TorrentDiscoveryNewAction, {"rss/torrent/options/newaction", track::kTorrentActionNotify}},
      {AppSettingKey::TorrentDownloadAppMode, {"rss/torrent/application/mode", track::kTorrentAppDefault}},
      {AppSettingKey::TorrentDownloadAppOpen, {"rss/torrent/application/open", true}},
      {AppSettingKey::TorrentDownloadAppPath, {"rss/torrent/application/path", std::wstring{GetDefaultAppPath(L".torrent", kDefaultTorrentAppPath)}}},
      {AppSettingKey::TorrentDownloadLocation, {"rss/torrent/options/downloadpath", std::wstring{}}},
      {AppSettingKey::TorrentDownloadFileLocation, {"rss/torrent/options/filedownloadpath", std::wstring{}}},
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

base::Settings::value_t Settings::GetDefaultValue(
    const AppSettingKey key) const {
  InitKeyMap();
  const auto& app_setting = key_map_[key];
  return app_setting.default_value;
}

////////////////////////////////////////////////////////////////////////////////

template <typename T>
T Settings::value(const AppSettingKey key) const {
  std::lock_guard lock{mutex_};

  InitKeyMap();
  const auto& app_setting = key_map_[key];
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
bool Settings::set_value(const AppSettingKey key, T&& value) {
  std::lock_guard lock{mutex_};

  InitKeyMap();
  const auto& app_setting = key_map_[key];

  if (settings_.set_value(app_setting.key, std::move(value))) {
    modified_ = true;
    return true;
  } else {
    return false;
  }
}

////////////////////////////////////////////////////////////////////////////////

// Meta

semaver::Version Settings::GetMetaVersion() const {
  const auto version = value<std::wstring>(AppSettingKey::MetaVersion);
  return semaver::Version(WstrToStr(version));
}

// Services

sync::ServiceId Settings::GetSyncActiveService() const {
  const auto slug = value<std::wstring>(AppSettingKey::SyncActiveService);
  return sync::GetServiceIdBySlug(slug);
}

void Settings::SetSyncActiveService(const sync::ServiceId service_id) {
  const auto previous_service_id = GetSyncActiveService();

  if (service_id != previous_service_id) {
    if (library::queue.GetItemCount() > 0) {
      ui::OnSettingsServiceChangeFailed();
      return;
    }

    if (!anime::db.items.empty()) {
      if (ui::OnSettingsServiceChangeConfirm(previous_service_id, service_id)) {
        LOGW(L"Changing the active service from {} to {}.",
             sync::GetServiceNameById(previous_service_id),
             sync::GetServiceNameById(service_id));
        anime::db.SaveList(true);
        anime::db.items.clear();
        anime::db.SaveDatabase();
        ui::image_db.Clear();
        anime::season_db.Reset();
      } else {
        return;
      }
    }
  }

  const auto slug = sync::GetServiceSlugById(service_id);
  if (set_value(AppSettingKey::SyncActiveService, slug)) {
    changed_account_or_service_ = true;
  }
}

bool Settings::GetSyncAutoOnStart() const {
  return value<bool>(AppSettingKey::SyncAutoOnStart);
}

void Settings::SetSyncAutoOnStart(const bool enabled) {
  set_value(AppSettingKey::SyncAutoOnStart, enabled);
}

std::wstring Settings::GetSyncServiceMalUsername() const {
  return value<std::wstring>(AppSettingKey::SyncServiceMalUsername);
}

void Settings::SetSyncServiceMalUsername(const std::wstring& username) {
  if (set_value(AppSettingKey::SyncServiceMalUsername, username)) {
    if (sync::GetCurrentServiceId() == sync::ServiceId::MyAnimeList) {
      changed_account_or_service_ = true;
    }
  }
}

std::wstring Settings::GetSyncServiceMalAccessToken() const {
  return value<std::wstring>(AppSettingKey::SyncServiceMalAccessToken);
}

void Settings::SetSyncServiceMalAccessToken(const std::wstring& token) {
  set_value(AppSettingKey::SyncServiceMalAccessToken, token);
}

std::wstring Settings::GetSyncServiceMalRefreshToken() const {
  return value<std::wstring>(AppSettingKey::SyncServiceMalRefreshToken);
}

void Settings::SetSyncServiceMalRefreshToken(const std::wstring& token) {
  set_value(AppSettingKey::SyncServiceMalRefreshToken, token);
}

std::wstring Settings::GetSyncServiceKitsuDisplayName() const {
  return value<std::wstring>(AppSettingKey::SyncServiceKitsuDisplayName);
}

void Settings::SetSyncServiceKitsuDisplayName(const std::wstring& name) {
  set_value(AppSettingKey::SyncServiceKitsuDisplayName, name);
}

std::wstring Settings::GetSyncServiceKitsuEmail() const {
  return value<std::wstring>(AppSettingKey::SyncServiceKitsuEmail);
}

void Settings::SetSyncServiceKitsuEmail(const std::wstring& email) {
  if (set_value(AppSettingKey::SyncServiceKitsuEmail, email)) {
    set_value(AppSettingKey::SyncServiceKitsuDisplayName, std::wstring{});
    set_value(AppSettingKey::SyncServiceKitsuUsername, std::wstring{});

    if (sync::GetCurrentServiceId() == sync::ServiceId::Kitsu) {
      changed_account_or_service_ = true;
    }
  }
}

std::wstring Settings::GetSyncServiceKitsuUsername() const {
  return value<std::wstring>(AppSettingKey::SyncServiceKitsuUsername);
}

void Settings::SetSyncServiceKitsuUsername(const std::wstring& username) {
  set_value(AppSettingKey::SyncServiceKitsuUsername, username);
}

std::wstring Settings::GetSyncServiceKitsuPassword() const {
  return value<std::wstring>(AppSettingKey::SyncServiceKitsuPassword);
}

void Settings::SetSyncServiceKitsuPassword(const std::wstring& password) {
  set_value(AppSettingKey::SyncServiceKitsuPassword, password);
}

bool Settings::GetSyncServiceKitsuPartialLibrary() const {
  return value<bool>(AppSettingKey::SyncServiceKitsuPartialLibrary);
}

void Settings::SetSyncServiceKitsuPartialLibrary(const bool enabled) {
  set_value(AppSettingKey::SyncServiceKitsuPartialLibrary, enabled);
}

std::wstring Settings::GetSyncServiceKitsuRatingSystem() const {
  return value<std::wstring>(AppSettingKey::SyncServiceKitsuRatingSystem);
}

void Settings::SetSyncServiceKitsuRatingSystem(const std::wstring& rating_system) {
  set_value(AppSettingKey::SyncServiceKitsuRatingSystem, rating_system);
}

std::wstring Settings::GetSyncServiceAniListUsername() const {
  return value<std::wstring>(AppSettingKey::SyncServiceAniListUsername);
}

void Settings::SetSyncServiceAniListUsername(const std::wstring& username) {
  if (set_value(AppSettingKey::SyncServiceAniListUsername, username)) {
    if (sync::GetCurrentServiceId() == sync::ServiceId::AniList) {
      changed_account_or_service_ = true;
    }
  }
}

std::wstring Settings::GetSyncServiceAniListRatingSystem() const {
  return value<std::wstring>(AppSettingKey::SyncServiceAniListRatingSystem);
}

void Settings::SetSyncServiceAniListRatingSystem(const std::wstring& rating_system) {
  set_value(AppSettingKey::SyncServiceAniListRatingSystem, rating_system);
}

std::wstring Settings::GetSyncServiceAniListToken() const {
  return value<std::wstring>(AppSettingKey::SyncServiceAniListToken);
}

void Settings::SetSyncServiceAniListToken(const std::wstring& token) {
  set_value(AppSettingKey::SyncServiceAniListToken, token);
}

// Library

int Settings::GetLibraryFileSizeThreshold() const {
  return value<int>(AppSettingKey::LibraryFileSizeThreshold);
}

void Settings::SetLibraryFileSizeThreshold(const int bytes) {
  set_value(AppSettingKey::LibraryFileSizeThreshold, bytes);
}

std::wstring Settings::GetLibraryMediaPlayerPath() const {
  return value<std::wstring>(AppSettingKey::LibraryMediaPlayerPath);
}

void Settings::SetLibraryMediaPlayerPath(const std::wstring& path) {
  set_value(AppSettingKey::LibraryMediaPlayerPath, path);
}

bool Settings::GetLibraryWatchFolders() const {
  return value<bool>(AppSettingKey::LibraryWatchFolders);
}

void Settings::SetLibraryWatchFolders(const bool enabled) {
  set_value(AppSettingKey::LibraryWatchFolders, enabled);
  track::monitor.Enable(enabled);
}

// Application

int Settings::GetAppListDoubleClickAction() const {
  return value<int>(AppSettingKey::AppListDoubleClickAction);
}

void Settings::SetAppListDoubleClickAction(const int action) {
  set_value(AppSettingKey::AppListDoubleClickAction, action);
}

int Settings::GetAppListMiddleClickAction() const {
  return value<int>(AppSettingKey::AppListMiddleClickAction);
}

void Settings::SetAppListMiddleClickAction(const int action) {
  set_value(AppSettingKey::AppListMiddleClickAction, action);
}

bool Settings::GetAppListDisplayEnglishTitles() const {
  return value<bool>(AppSettingKey::AppListDisplayEnglishTitles);
}

void Settings::SetAppListDisplayEnglishTitles(const bool enabled) {
  set_value(AppSettingKey::AppListDisplayEnglishTitles, enabled);
}

bool Settings::GetAppListHighlightNewEpisodes() const {
  return value<bool>(AppSettingKey::AppListHighlightNewEpisodes);
}

void Settings::SetAppListHighlightNewEpisodes(const bool enabled) {
  set_value(AppSettingKey::AppListHighlightNewEpisodes, enabled);
}

bool Settings::GetAppListDisplayHighlightedOnTop() const {
  return value<bool>(AppSettingKey::AppListDisplayHighlightedOnTop);
}

void Settings::SetAppListDisplayHighlightedOnTop(const bool enabled) {
  set_value(AppSettingKey::AppListDisplayHighlightedOnTop, enabled);
}

bool Settings::GetAppListProgressDisplayAired() const {
  return value<bool>(AppSettingKey::AppListProgressDisplayAired);
}

void Settings::SetAppListProgressDisplayAired(const bool enabled) {
  set_value(AppSettingKey::AppListProgressDisplayAired, enabled);
}

bool Settings::GetAppListProgressDisplayAvailable() const {
  return value<bool>(AppSettingKey::AppListProgressDisplayAvailable);
}

void Settings::SetAppListProgressDisplayAvailable(const bool enabled) {
  set_value(AppSettingKey::AppListProgressDisplayAvailable, enabled);
}

std::wstring Settings::GetAppListSortColumnPrimary() const {
  return value<std::wstring>(AppSettingKey::AppListSortColumnPrimary);
}

void Settings::SetAppListSortColumnPrimary(const std::wstring& column) {
  set_value(AppSettingKey::AppListSortColumnPrimary, column);
}

std::wstring Settings::GetAppListSortColumnSecondary() const {
  return value<std::wstring>(AppSettingKey::AppListSortColumnSecondary);
}

void Settings::SetAppListSortColumnSecondary(const std::wstring& column) {
  set_value(AppSettingKey::AppListSortColumnSecondary, column);
}

int Settings::GetAppListSortOrderPrimary() const {
  return value<int>(AppSettingKey::AppListSortOrderPrimary);
}

void Settings::SetAppListSortOrderPrimary(const int order) {
  set_value(AppSettingKey::AppListSortOrderPrimary, order);
}

int Settings::GetAppListSortOrderSecondary() const {
  return value<int>(AppSettingKey::AppListSortOrderSecondary);
}

void Settings::SetAppListSortOrderSecondary(const int order) {
  set_value(AppSettingKey::AppListSortOrderSecondary, order);
}

anime::TitleLanguage Settings::GetAppListTitleLanguagePreference() const {
  const auto slug = value<std::wstring>(AppSettingKey::AppListTitleLanguagePreference);
  if (slug == L"english") return anime::TitleLanguage::English;
  if (slug == L"native") return anime::TitleLanguage::Native;
  return anime::TitleLanguage::Romaji;
}

void Settings::SetAppListTitleLanguagePreference(const anime::TitleLanguage language) {
  const std::wstring slug = [&language]() {
    switch (language) {
      default:
      case anime::TitleLanguage::Romaji: return L"romaji";
      case anime::TitleLanguage::English: return L"english";
      case anime::TitleLanguage::Native: return L"native";
    }
  }();
  set_value(AppSettingKey::AppListTitleLanguagePreference, slug);
}

bool Settings::GetAppBehaviorAutostart() const {
  return value<bool>(AppSettingKey::AppBehaviorAutostart);
}

void Settings::SetAppBehaviorAutostart(const bool enabled) {
  set_value(AppSettingKey::AppBehaviorAutostart, enabled);

  win::Registry registry;
  registry.OpenKey(HKEY_CURRENT_USER,
                   L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                   0, KEY_SET_VALUE);
  if (enabled) {
    registry.SetValue(TAIGA_APP_NAME, app.GetModulePath());
  } else {
    registry.DeleteValue(TAIGA_APP_NAME);
  }
}

bool Settings::GetAppBehaviorStartMinimized() const {
  return value<bool>(AppSettingKey::AppBehaviorStartMinimized);
}

void Settings::SetAppBehaviorStartMinimized(const bool enabled) {
  set_value(AppSettingKey::AppBehaviorStartMinimized, enabled);
}

bool Settings::GetAppBehaviorCheckForUpdates() const {
  return value<bool>(AppSettingKey::AppBehaviorCheckForUpdates);
}

void Settings::SetAppBehaviorCheckForUpdates(const bool enabled) {
  set_value(AppSettingKey::AppBehaviorCheckForUpdates, enabled);
}

bool Settings::GetAppBehaviorScanAvailableEpisodes() const {
  return value<bool>(AppSettingKey::AppBehaviorScanAvailableEpisodes);
}

void Settings::SetAppBehaviorScanAvailableEpisodes(const bool enabled) {
  set_value(AppSettingKey::AppBehaviorScanAvailableEpisodes, enabled);
}

bool Settings::GetAppBehaviorCloseToTray() const {
  return value<bool>(AppSettingKey::AppBehaviorCloseToTray);
}

void Settings::SetAppBehaviorCloseToTray(const bool enabled) {
  set_value(AppSettingKey::AppBehaviorCloseToTray, enabled);
}

bool Settings::GetAppBehaviorMinimizeToTray() const {
  return value<bool>(AppSettingKey::AppBehaviorMinimizeToTray);
}

void Settings::SetAppBehaviorMinimizeToTray(const bool enabled) {
  set_value(AppSettingKey::AppBehaviorMinimizeToTray, enabled);
}

std::wstring Settings::GetAppConnectionProxyHost() const {
  return value<std::wstring>(AppSettingKey::AppConnectionProxyHost);
}

void Settings::SetAppConnectionProxyHost(const std::wstring& host) {
  set_value(AppSettingKey::AppConnectionProxyHost, host);
}

std::wstring Settings::GetAppConnectionProxyUsername() const {
  return value<std::wstring>(AppSettingKey::AppConnectionProxyUsername);
}

void Settings::SetAppConnectionProxyUsername(const std::wstring& username) {
  set_value(AppSettingKey::AppConnectionProxyUsername, username);
}

std::wstring Settings::GetAppConnectionProxyPassword() const {
  return value<std::wstring>(AppSettingKey::AppConnectionProxyPassword);
}

void Settings::SetAppConnectionProxyPassword(const std::wstring& password) {
  set_value(AppSettingKey::AppConnectionProxyPassword, password);
}

bool Settings::GetAppConnectionNoRevoke() const {
  return value<bool>(AppSettingKey::AppConnectionNoRevoke);
}

void Settings::SetAppConnectionNoRevoke(const bool enabled) {
  set_value(AppSettingKey::AppConnectionNoRevoke, enabled);
}

bool Settings::GetAppConnectionReuseActive() const {
  return value<bool>(AppSettingKey::AppConnectionReuseActive);
}

void Settings::SetAppConnectionReuseActive(const bool enabled) {
  set_value(AppSettingKey::AppConnectionReuseActive, enabled);
}

std::wstring Settings::GetAppInterfaceTheme() const {
  return value<std::wstring>(AppSettingKey::AppInterfaceTheme);
}

void Settings::SetAppInterfaceTheme(const std::wstring& theme) {
  const auto previous_theme = GetAppInterfaceTheme();
  if (set_value(AppSettingKey::AppInterfaceTheme, theme)) {
    if (ui::Theme.Load()) {
      ui::OnSettingsThemeChange();
    } else {
      set_value(AppSettingKey::AppInterfaceTheme, previous_theme);
    }
  }
}

std::wstring Settings::GetAppInterfaceExternalLinks() const {
  return value<std::wstring>(AppSettingKey::AppInterfaceExternalLinks);
}

void Settings::SetAppInterfaceExternalLinks(const std::wstring& str) {
  if (set_value(AppSettingKey::AppInterfaceExternalLinks, str)) {
    ui::Menus.UpdateExternalLinks();
  }
}

// Recognition

int Settings::GetRecognitionDetectionInterval() const {
  return value<int>(AppSettingKey::RecognitionDetectionInterval);
}

void Settings::SetRecognitionDetectionInterval(const int seconds) {
  set_value(AppSettingKey::RecognitionDetectionInterval, seconds);
}

bool Settings::GetRecognitionDetectMediaPlayers() const {
  return value<bool>(AppSettingKey::RecognitionDetectMediaPlayers);
}

void Settings::SetRecognitionDetectMediaPlayers(const bool enabled) {
  set_value(AppSettingKey::RecognitionDetectMediaPlayers, enabled);
}

bool Settings::GetRecognitionDetectStreamingMedia() const {
  return value<bool>(AppSettingKey::RecognitionDetectStreamingMedia);
}

void Settings::SetRecognitionDetectStreamingMedia(const bool enabled) {
  set_value(AppSettingKey::RecognitionDetectStreamingMedia, enabled);
}

std::wstring Settings::GetRecognitionIgnoredStrings() const {
  return value<std::wstring>(AppSettingKey::RecognitionIgnoredStrings);
}

void Settings::SetRecognitionIgnoredStrings(const std::wstring& str) {
  set_value(AppSettingKey::RecognitionIgnoredStrings, str);
}

bool Settings::GetRecognitionLookupParentDirectories() const {
  return value<bool>(AppSettingKey::RecognitionLookupParentDirectories);
}

void Settings::SetRecognitionLookupParentDirectories(const bool enabled) {
  set_value(AppSettingKey::RecognitionLookupParentDirectories, enabled);
}

std::wstring Settings::GetRecognitionRelationsLastModified() const {
  return value<std::wstring>(AppSettingKey::RecognitionRelationsLastModified);
}

void Settings::SetRecognitionRelationsLastModified(const std::wstring& last_modified) {
  set_value(AppSettingKey::RecognitionRelationsLastModified, last_modified);
}

int Settings::GetSyncUpdateDelay() const {
  return value<int>(AppSettingKey::SyncUpdateDelay);
}

void Settings::SetSyncUpdateDelay(const int seconds) {
  set_value(AppSettingKey::SyncUpdateDelay, seconds);
}

bool Settings::GetSyncUpdateAskToConfirm() const {
  return value<bool>(AppSettingKey::SyncUpdateAskToConfirm);
}

void Settings::SetSyncUpdateAskToConfirm(const bool enabled) {
  set_value(AppSettingKey::SyncUpdateAskToConfirm, enabled);
}

bool Settings::GetSyncUpdateCheckPlayer() const {
  return value<bool>(AppSettingKey::SyncUpdateCheckPlayer);
}

void Settings::SetSyncUpdateCheckPlayer(const bool enabled) {
  set_value(AppSettingKey::SyncUpdateCheckPlayer, enabled);
}

bool Settings::GetSyncUpdateOutOfRange() const {
  return value<bool>(AppSettingKey::SyncUpdateOutOfRange);
}

void Settings::SetSyncUpdateOutOfRange(const bool enabled) {
  set_value(AppSettingKey::SyncUpdateOutOfRange, enabled);
}

bool Settings::GetSyncUpdateOutOfRoot() const {
  return value<bool>(AppSettingKey::SyncUpdateOutOfRoot);
}

void Settings::SetSyncUpdateOutOfRoot(const bool enabled) {
  set_value(AppSettingKey::SyncUpdateOutOfRoot, enabled);
}

bool Settings::GetSyncUpdateWaitPlayer() const {
  return value<bool>(AppSettingKey::SyncUpdateWaitPlayer);
}

void Settings::SetSyncUpdateWaitPlayer(const bool enabled) {
  set_value(AppSettingKey::SyncUpdateWaitPlayer, enabled);
}

bool Settings::GetSyncGoToNowPlayingRecognized() const {
  return value<bool>(AppSettingKey::SyncGoToNowPlayingRecognized);
}

void Settings::SetSyncGoToNowPlayingRecognized(const bool enabled) {
  set_value(AppSettingKey::SyncGoToNowPlayingRecognized, enabled);
}

bool Settings::GetSyncGoToNowPlayingNotRecognized() const {
  return value<bool>(AppSettingKey::SyncGoToNowPlayingNotRecognized);
}

void Settings::SetSyncGoToNowPlayingNotRecognized(const bool enabled) {
  set_value(AppSettingKey::SyncGoToNowPlayingNotRecognized, enabled);
}

bool Settings::GetSyncNotifyRecognized() const {
  return value<bool>(AppSettingKey::SyncNotifyRecognized);
}

void Settings::SetSyncNotifyRecognized(const bool enabled) {
  set_value(AppSettingKey::SyncNotifyRecognized, enabled);
}

bool Settings::GetSyncNotifyNotRecognized() const {
  return value<bool>(AppSettingKey::SyncNotifyNotRecognized);
}

void Settings::SetSyncNotifyNotRecognized(const bool enabled) {
  set_value(AppSettingKey::SyncNotifyNotRecognized, enabled);
}

std::wstring Settings::GetSyncNotifyFormat() const {
  return value<std::wstring>(AppSettingKey::SyncNotifyFormat);
}

void Settings::SetSyncNotifyFormat(const std::wstring& format) {
  set_value(AppSettingKey::SyncNotifyFormat, format);
}

bool Settings::GetStreamAnimelab() const {
  return value<bool>(AppSettingKey::StreamAnimelab);
}

void Settings::SetStreamAnimelab(const bool enabled) {
  set_value(AppSettingKey::StreamAnimelab, enabled);
}

bool Settings::GetStreamAdn() const {
  return value<bool>(AppSettingKey::StreamAdn);
}

void Settings::SetStreamAdn(const bool enabled) {
  set_value(AppSettingKey::StreamAdn, enabled);
}

bool Settings::GetStreamAnn() const {
  return value<bool>(AppSettingKey::StreamAnn);
}

void Settings::SetStreamAnn(const bool enabled) {
  set_value(AppSettingKey::StreamAnn, enabled);
}

bool Settings::GetStreamBilibili() const {
  return value<bool>(AppSettingKey::StreamBilibili);
}

void Settings::SetStreamBilibili(const bool enabled) {
  set_value(AppSettingKey::StreamBilibili, enabled);
}

bool Settings::GetStreamCrunchyroll() const {
  return value<bool>(AppSettingKey::StreamCrunchyroll);
}

void Settings::SetStreamCrunchyroll(const bool enabled) {
  set_value(AppSettingKey::StreamCrunchyroll, enabled);
}

bool Settings::GetStreamFunimation() const {
  return value<bool>(AppSettingKey::StreamFunimation);
}

void Settings::SetStreamFunimation(const bool enabled) {
  set_value(AppSettingKey::StreamFunimation, enabled);
}

bool Settings::GetStreamHidive() const {
  return value<bool>(AppSettingKey::StreamHidive);
}

void Settings::SetStreamHidive(const bool enabled) {
  set_value(AppSettingKey::StreamHidive, enabled);
}

bool Settings::GetStreamJellyfin() const {
  return value<bool>(AppSettingKey::StreamJellyfin);
}

void Settings::SetStreamJellyfin(const bool enabled) {
  set_value(AppSettingKey::StreamJellyfin, enabled);
}

bool Settings::GetStreamPlex() const {
  return value<bool>(AppSettingKey::StreamPlex);
}

void Settings::SetStreamPlex(const bool enabled) {
  set_value(AppSettingKey::StreamPlex, enabled);
}

bool Settings::GetStreamRokuChannel() const {
  return value<bool>(AppSettingKey::StreamRokuChannel);
}

void Settings::SetStreamRokuChannel(const bool enabled) {
  set_value(AppSettingKey::StreamRokuChannel, enabled);
}

bool Settings::GetStreamTubi() const {
  return value<bool>(AppSettingKey::StreamTubi);
}

void Settings::SetStreamTubi(const bool enabled) {
  set_value(AppSettingKey::StreamTubi, enabled);
}

bool Settings::GetStreamVeoh() const {
  return value<bool>(AppSettingKey::StreamVeoh);
}

void Settings::SetStreamVeoh(const bool enabled) {
  set_value(AppSettingKey::StreamVeoh, enabled);
}

bool Settings::GetStreamViz() const {
  return value<bool>(AppSettingKey::StreamViz);
}

void Settings::SetStreamViz(const bool enabled) {
  set_value(AppSettingKey::StreamViz, enabled);
}

bool Settings::GetStreamVrv() const {
  return value<bool>(AppSettingKey::StreamVrv);
}

void Settings::SetStreamVrv(const bool enabled) {
  set_value(AppSettingKey::StreamVrv, enabled);
}

bool Settings::GetStreamWakanim() const {
  return value<bool>(AppSettingKey::StreamWakanim);
}

void Settings::SetStreamWakanim(const bool enabled) {
  set_value(AppSettingKey::StreamWakanim, enabled);
}

bool Settings::GetStreamYahoo() const {
  return value<bool>(AppSettingKey::StreamYahoo);
}

void Settings::SetStreamYahoo(const bool enabled) {
  set_value(AppSettingKey::StreamYahoo, enabled);
}

bool Settings::GetStreamYoutube() const {
  return value<bool>(AppSettingKey::StreamYoutube);
}

void Settings::SetStreamYoutube(const bool enabled) {
  set_value(AppSettingKey::StreamYoutube, enabled);
}

// Sharing

std::wstring Settings::GetShareDiscordApplicationId() const {
  return value<std::wstring>(AppSettingKey::ShareDiscordApplicationId);
}

void Settings::SetShareDiscordApplicationId(const std::wstring& application_id) {
  set_value(AppSettingKey::ShareDiscordApplicationId, application_id);
}

bool Settings::GetShareDiscordEnabled() const {
  return value<bool>(AppSettingKey::ShareDiscordEnabled);
}

void Settings::SetShareDiscordEnabled(const bool enabled) {
  if (set_value(AppSettingKey::ShareDiscordEnabled, enabled)) {
    if (enabled) {
      link::discord::Initialize();
      taiga::announcer.Do(kAnnounceToDiscord);
    } else {
      link::discord::Shutdown();
    }
  }
}

bool Settings::GetShareDiscordGroupEnabled() const {
  return value<bool>(AppSettingKey::ShareDiscordGroupEnabled);
}

void Settings::SetShareDiscordGroupEnabled(const bool enabled) {
  set_value(AppSettingKey::ShareDiscordGroupEnabled, enabled);
}

bool Settings::GetShareDiscordTimeEnabled() const {
  return value<bool>(AppSettingKey::ShareDiscordTimeEnabled);
}

void Settings::SetShareDiscordTimeEnabled(const bool enabled) {
  set_value(AppSettingKey::ShareDiscordTimeEnabled, enabled);
}

bool Settings::GetShareDiscordUsernameEnabled() const {
  return value<bool>(AppSettingKey::ShareDiscordUsernameEnabled);
}

void Settings::SetShareDiscordUsernameEnabled(const bool enabled) {
  set_value(AppSettingKey::ShareDiscordUsernameEnabled, enabled);
}

bool Settings::GetShareDiscordWatchingEnabled() const {
  return value<bool>(AppSettingKey::ShareDiscordWatchingEnabled);
}

void Settings::SetShareDiscordWatchingEnabled(const bool enabled) {
  set_value(AppSettingKey::ShareDiscordWatchingEnabled, enabled);
}

bool Settings::GetShareHttpEnabled() const {
  return value<bool>(AppSettingKey::ShareHttpEnabled);
}

void Settings::SetShareHttpEnabled(const bool enabled) {
  set_value(AppSettingKey::ShareHttpEnabled, enabled);
}

std::wstring Settings::GetShareHttpFormat() const {
  return value<std::wstring>(AppSettingKey::ShareHttpFormat);
}

void Settings::SetShareHttpFormat(const std::wstring& format) {
  set_value(AppSettingKey::ShareHttpFormat, format);
}

std::wstring Settings::GetShareHttpUrl() const {
  return value<std::wstring>(AppSettingKey::ShareHttpUrl);
}

void Settings::SetShareHttpUrl(const std::wstring& url) {
  set_value(AppSettingKey::ShareHttpUrl, url);
}

bool Settings::GetShareMircEnabled() const {
  return value<bool>(AppSettingKey::ShareMircEnabled);
}

void Settings::SetShareMircEnabled(const bool enabled) {
  set_value(AppSettingKey::ShareMircEnabled, enabled);
}

bool Settings::GetShareMircMultiServer() const {
  return value<bool>(AppSettingKey::ShareMircMultiServer);
}

void Settings::SetShareMircMultiServer(const bool enabled) {
  set_value(AppSettingKey::ShareMircMultiServer, enabled);
}

bool Settings::GetShareMircUseMeAction() const {
  return value<bool>(AppSettingKey::ShareMircUseMeAction);
}

void Settings::SetShareMircUseMeAction(const bool enabled) {
  set_value(AppSettingKey::ShareMircUseMeAction, enabled);
}

int Settings::GetShareMircMode() const {
  return value<int>(AppSettingKey::ShareMircMode);
}

void Settings::SetShareMircMode(const int mode) {
  set_value(AppSettingKey::ShareMircMode, mode);
}

std::wstring Settings::GetShareMircChannels() const {
  return value<std::wstring>(AppSettingKey::ShareMircChannels);
}

void Settings::SetShareMircChannels(const std::wstring& channels) {
  set_value(AppSettingKey::ShareMircChannels, channels);
}

std::wstring Settings::GetShareMircFormat() const {
  return value<std::wstring>(AppSettingKey::ShareMircFormat);
}

void Settings::SetShareMircFormat(const std::wstring& format) {
  set_value(AppSettingKey::ShareMircFormat, format);
}

std::wstring Settings::GetShareMircService() const {
  return value<std::wstring>(AppSettingKey::ShareMircService);
}

void Settings::SetShareMircService(const std::wstring& service) {
  set_value(AppSettingKey::ShareMircService, service);
}

bool Settings::GetShareTwitterEnabled() const {
  return value<bool>(AppSettingKey::ShareTwitterEnabled);
}

void Settings::SetShareTwitterEnabled(const bool enabled) {
  set_value(AppSettingKey::ShareTwitterEnabled, enabled);
}

std::wstring Settings::GetShareTwitterFormat() const {
  return value<std::wstring>(AppSettingKey::ShareTwitterFormat);
}

void Settings::SetShareTwitterFormat(const std::wstring& format) {
  set_value(AppSettingKey::ShareTwitterFormat, format);
}

std::wstring Settings::GetShareTwitterOauthToken() const {
  return value<std::wstring>(AppSettingKey::ShareTwitterOauthToken);
}

void Settings::SetShareTwitterOauthToken(const std::wstring& oauth_token) {
  set_value(AppSettingKey::ShareTwitterOauthToken, oauth_token);
}

std::wstring Settings::GetShareTwitterOauthSecret() const {
  return value<std::wstring>(AppSettingKey::ShareTwitterOauthSecret);
}

void Settings::SetShareTwitterOauthSecret(const std::wstring& oauth_secret) {
  set_value(AppSettingKey::ShareTwitterOauthSecret, oauth_secret);
}

std::wstring Settings::GetShareTwitterReplyTo() const {
  return value<std::wstring>(AppSettingKey::ShareTwitterReplyTo);
}

void Settings::SetShareTwitterReplyTo(const std::wstring& status_id) {
  set_value(AppSettingKey::ShareTwitterReplyTo, status_id);
}

std::wstring Settings::GetShareTwitterUsername() const {
  return value<std::wstring>(AppSettingKey::ShareTwitterUsername);
}

void Settings::SetShareTwitterUsername(const std::wstring& username) {
  set_value(AppSettingKey::ShareTwitterUsername, username);
}

// Torrents

std::wstring Settings::GetTorrentDiscoverySource() const {
  return value<std::wstring>(AppSettingKey::TorrentDiscoverySource);
}

void Settings::SetTorrentDiscoverySource(const std::wstring& url) {
  set_value(AppSettingKey::TorrentDiscoverySource, url);
}

std::wstring Settings::GetTorrentDiscoverySearchUrl() const {
  return value<std::wstring>(AppSettingKey::TorrentDiscoverySearchUrl);
}

void Settings::SetTorrentDiscoverySearchUrl(const std::wstring& url) {
  set_value(AppSettingKey::TorrentDiscoverySearchUrl, url);
}

bool Settings::GetTorrentDiscoveryAutoCheckEnabled() const {
  return value<bool>(AppSettingKey::TorrentDiscoveryAutoCheckEnabled);
}

void Settings::SetTorrentDiscoveryAutoCheckEnabled(const bool enabled) {
  set_value(AppSettingKey::TorrentDiscoveryAutoCheckEnabled, enabled);
}

int Settings::GetTorrentDiscoveryAutoCheckInterval() const {
  return value<int>(AppSettingKey::TorrentDiscoveryAutoCheckInterval);
}

void Settings::SetTorrentDiscoveryAutoCheckInterval(const int minutes) {
  set_value(AppSettingKey::TorrentDiscoveryAutoCheckInterval, minutes);
}

track::TorrentAction Settings::GetTorrentDiscoveryNewAction() const {
  return static_cast<track::TorrentAction>(value<int>(AppSettingKey::TorrentDiscoveryNewAction));
}

void Settings::SetTorrentDiscoveryNewAction(const track::TorrentAction action) {
  set_value(AppSettingKey::TorrentDiscoveryNewAction, action);
}

track::TorrentApp Settings::GetTorrentDownloadAppMode() const {
  return static_cast<track::TorrentApp>(value<int>(AppSettingKey::TorrentDownloadAppMode));
}

void Settings::SetTorrentDownloadAppMode(const track::TorrentApp mode) {
  set_value(AppSettingKey::TorrentDownloadAppMode, mode);
}

bool Settings::GetTorrentDownloadAppOpen() const {
  return value<bool>(AppSettingKey::TorrentDownloadAppOpen);
}

void Settings::SetTorrentDownloadAppOpen(const bool enabled) {
  set_value(AppSettingKey::TorrentDownloadAppOpen, enabled);
}

std::wstring Settings::GetTorrentDownloadAppPath() const {
  return value<std::wstring>(AppSettingKey::TorrentDownloadAppPath);
}

void Settings::SetTorrentDownloadAppPath(const std::wstring& path) {
  set_value(AppSettingKey::TorrentDownloadAppPath, path);
}

std::wstring Settings::GetTorrentDownloadLocation() const {
  return value<std::wstring>(AppSettingKey::TorrentDownloadLocation);
}

void Settings::SetTorrentDownloadLocation(const std::wstring& path) {
  set_value(AppSettingKey::TorrentDownloadLocation, path);
}

std::wstring Settings::GetTorrentDownloadFileLocation() const {
  return value<std::wstring>(AppSettingKey::TorrentDownloadFileLocation);
}

void Settings::SetTorrentDownloadFileLocation(const std::wstring& path) {
  set_value(AppSettingKey::TorrentDownloadFileLocation, path);
}

bool Settings::GetTorrentDownloadUseAnimeFolder() const {
  return value<bool>(AppSettingKey::TorrentDownloadUseAnimeFolder);
}

void Settings::SetTorrentDownloadUseAnimeFolder(const bool enabled) {
  set_value(AppSettingKey::TorrentDownloadUseAnimeFolder, enabled);
}

bool Settings::GetTorrentDownloadFallbackOnFolder() const {
  return value<bool>(AppSettingKey::TorrentDownloadFallbackOnFolder);
}

void Settings::SetTorrentDownloadFallbackOnFolder(const bool enabled) {
  set_value(AppSettingKey::TorrentDownloadFallbackOnFolder, enabled);
}

bool Settings::GetTorrentDownloadCreateSubfolder() const {
  return value<bool>(AppSettingKey::TorrentDownloadCreateSubfolder);
}

void Settings::SetTorrentDownloadCreateSubfolder(const bool enabled) {
  set_value(AppSettingKey::TorrentDownloadCreateSubfolder, enabled);
}

std::wstring Settings::GetTorrentDownloadSortBy() const {
  return value<std::wstring>(AppSettingKey::TorrentDownloadSortBy);
}

void Settings::SetTorrentDownloadSortBy(const std::wstring& sort) {
  set_value(AppSettingKey::TorrentDownloadSortBy, sort);
}

std::wstring Settings::GetTorrentDownloadSortOrder() const {
  return value<std::wstring>(AppSettingKey::TorrentDownloadSortOrder);
}

void Settings::SetTorrentDownloadSortOrder(const std::wstring& order) {
  set_value(AppSettingKey::TorrentDownloadSortOrder, order);
}

bool Settings::GetTorrentDownloadUseMagnet() const {
  return value<bool>(AppSettingKey::TorrentDownloadUseMagnet);
}

void Settings::SetTorrentDownloadUseMagnet(const bool enabled) {
  set_value(AppSettingKey::TorrentDownloadUseMagnet, enabled);
}

bool Settings::GetTorrentFilterEnabled() const {
  return value<bool>(AppSettingKey::TorrentFilterEnabled);
}

void Settings::SetTorrentFilterEnabled(const bool enabled) {
  set_value(AppSettingKey::TorrentFilterEnabled, enabled);
}

int Settings::GetTorrentFilterArchiveMaxCount() const {
  return value<int>(AppSettingKey::TorrentFilterArchiveMaxCount);
}

void Settings::SetTorrentFilterArchiveMaxCount(const int count) {
  set_value(AppSettingKey::TorrentFilterArchiveMaxCount, count);
}

// Internal

int Settings::GetAppPositionX() const {
  return value<int>(AppSettingKey::AppPositionX);
}

void Settings::SetAppPositionX(const int x) {
  set_value(AppSettingKey::AppPositionX, x);
}

int Settings::GetAppPositionY() const {
  return value<int>(AppSettingKey::AppPositionY);
}

void Settings::SetAppPositionY(const int y) {
  set_value(AppSettingKey::AppPositionY, y);
}

int Settings::GetAppPositionW() const {
  return value<int>(AppSettingKey::AppPositionW);
}

void Settings::SetAppPositionW(const int width) {
  set_value(AppSettingKey::AppPositionW, width);
}

int Settings::GetAppPositionH() const {
  return value<int>(AppSettingKey::AppPositionH);
}

void Settings::SetAppPositionH(const int height) {
  set_value(AppSettingKey::AppPositionH, height);
}

bool Settings::GetAppPositionMaximized() const {
  return value<bool>(AppSettingKey::AppPositionMaximized);
}

void Settings::SetAppPositionMaximized(const bool enabled) {
  set_value(AppSettingKey::AppPositionMaximized, enabled);
}

bool Settings::GetAppPositionRemember() const {
  return value<bool>(AppSettingKey::AppPositionRemember);
}

void Settings::SetAppPositionRemember(const bool enabled) {
  set_value(AppSettingKey::AppPositionRemember, enabled);
}

bool Settings::GetAppOptionHideSidebar() const {
  return value<bool>(AppSettingKey::AppOptionHideSidebar);
}

void Settings::SetAppOptionHideSidebar(const bool enabled) {
  set_value(AppSettingKey::AppOptionHideSidebar, enabled);
}

bool Settings::GetAppOptionEnableRecognition() const {
  return value<bool>(AppSettingKey::AppOptionEnableRecognition);
}

void Settings::SetAppOptionEnableRecognition(const bool enabled) {
  set_value(AppSettingKey::AppOptionEnableRecognition, enabled);
}

bool Settings::GetAppOptionEnableSharing() const {
  return value<bool>(AppSettingKey::AppOptionEnableSharing);
}

void Settings::SetAppOptionEnableSharing(const bool enabled) {
  set_value(AppSettingKey::AppOptionEnableSharing, enabled);
}

bool Settings::GetAppOptionEnableSync() const {
  return value<bool>(AppSettingKey::AppOptionEnableSync);
}

void Settings::SetAppOptionEnableSync(const bool enabled) {
  set_value(AppSettingKey::AppOptionEnableSync, enabled);
}

std::wstring Settings::GetAppSeasonsLastSeason() const {
  return value<std::wstring>(AppSettingKey::AppSeasonsLastSeason);
}

void Settings::SetAppSeasonsLastSeason(const std::wstring& season) {
  set_value(AppSettingKey::AppSeasonsLastSeason, season);
}

int Settings::GetAppSeasonsGroupBy() const {
  return value<int>(AppSettingKey::AppSeasonsGroupBy);
}

void Settings::SetAppSeasonsGroupBy(const int group_by) {
  set_value(AppSettingKey::AppSeasonsGroupBy, group_by);
}

int Settings::GetAppSeasonsSortBy() const {
  return value<int>(AppSettingKey::AppSeasonsSortBy);
}

void Settings::SetAppSeasonsSortBy(const int sort_by) {
  set_value(AppSettingKey::AppSeasonsSortBy, sort_by);
}

int Settings::GetAppSeasonsViewAs() const {
  return value<int>(AppSettingKey::AppSeasonsViewAs);
}

void Settings::SetAppSeasonsViewAs(const int view_as) {
  set_value(AppSettingKey::AppSeasonsViewAs, view_as);
}

////////////////////////////////////////////////////////////////////////////////

std::vector<std::wstring> Settings::GetLibraryFolders() const {
  std::lock_guard lock{mutex_};
  return library_folders_;
}

void Settings::SetLibraryFolders(const std::vector<std::wstring>& folders) {
  std::lock_guard lock{mutex_};
  if (library_folders_ != folders) {
    library_folders_ = folders;
    modified_ = true;
  }
}

bool Settings::GetMediaPlayerEnabled(const std::wstring& player) const {
  std::lock_guard lock{mutex_};
  const auto it = media_players_enabled_.find(player);
  return it != media_players_enabled_.end() ? it->second : true;
}

void Settings::SetMediaPlayerEnabled(const std::wstring& player,
                                     const bool enabled) {
  std::lock_guard lock{mutex_};
  const auto it = media_players_enabled_.find(player);
  if (it == media_players_enabled_.end() || it->second != enabled) {
    media_players_enabled_[player] = enabled;
    modified_ = true;
  }
}

std::optional<Settings::AnimeListColumn> Settings::GetAnimeListColumn(
    const std::wstring& key) const {
  std::lock_guard lock{mutex_};
  const auto it = anime_list_columns_.find(key);
  if (it != anime_list_columns_.end()) {
    return it->second;
  }
  return std::nullopt;
}

void Settings::SetAnimeListColumn(const std::wstring& key,
                                  const Settings::AnimeListColumn& column) {
  std::lock_guard lock{mutex_};
  const auto it = anime_list_columns_.find(key);
  if (it == anime_list_columns_.end() || it->second != column) {
    anime_list_columns_[key] = column;
    modified_ = true;
  }
}

std::wstring Settings::GetAnimeFolder(const int id) const {
  std::lock_guard lock{mutex_};
  const auto it = anime_settings_.find(id);
  return it != anime_settings_.end() ? it->second.folder : std::wstring{};
}

void Settings::SetAnimeFolder(const int id, const std::wstring& folder) {
  std::lock_guard lock{mutex_};
  const auto it = anime_settings_.find(id);
  if (it == anime_settings_.end() || it->second.folder != folder) {
    anime_settings_[id].folder = folder;
    modified_ = true;
  }
}

bool Settings::GetAnimeUseAlternative(const int id) const {
  std::lock_guard lock{mutex_};
  const auto it = anime_settings_.find(id);
  return it != anime_settings_.end() ? it->second.use_alternative : false;
}

void Settings::SetAnimeUseAlternative(const int id, const bool enabled) {
  std::lock_guard lock{mutex_};
  const auto it = anime_settings_.find(id);
  if (it == anime_settings_.end() || it->second.use_alternative != enabled) {
    anime_settings_[id].use_alternative = enabled;
    modified_ = true;
  }
}

std::vector<std::wstring> Settings::GetAnimeUserSynonyms(
    const int id) const {
  std::lock_guard lock{mutex_};
  const auto it = anime_settings_.find(id);
  return it != anime_settings_.end() ? it->second.synonyms
                                     : std::vector<std::wstring>{};
}

void Settings::SetAnimeUserSynonyms(const int id,
                                    const std::vector<std::wstring>& synonyms) {
  std::lock_guard lock{mutex_};
  const auto it = anime_settings_.find(id);
  if (it == anime_settings_.end() || it->second.synonyms != synonyms) {
    anime_settings_[id].synonyms = synonyms;
    modified_ = true;
  }
}

}  // namespace taiga
