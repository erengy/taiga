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
#include "taiga/settings.h"
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

taiga::AppSettings Settings;

namespace taiga {

const std::wstring kDefaultExternalLinks =
    L"Hibari|https://hb.wopian.me\r\n"
    L"MALgraph|http://graph.anime.plus\r\n"
    L"-\r\n"
    L"AniChart|http://anichart.net/airing\r\n"
    L"Monthly.moe|https://www.monthly.moe/weekly\r\n"
    L"Senpai Anime Charts|http://www.senpai.moe/?mode=calendar\r\n"
    L"-\r\n"
    L"Anime Streaming Search Engine|http://because.moe\r\n"
    L"The Fansub Database|https://fansubdb.com";
const std::wstring kDefaultFormatDiscordDetails =
    L"%title%";
const std::wstring kDefaultFormatDiscordState =
    L"$if(%episode%,Episode %episode%$if(%total%,/%total%) )"
    L"$if(%group%,by %group%)";
const std::wstring kDefaultFormatHttp =
    L"user=%user%"
    L"&name=%title%"
    L"&ep=%episode%"
    L"&eptotal=$if(%total%,%total%,?)"
    L"&score=%score%"
    L"&picurl=%image%"
    L"&playstatus=%playstatus%";
const std::wstring kDefaultFormatMirc =
    L"\00304$if($greater(%episode%,%watched%),Watching,Rewatching):\003 %title%"
    L"$if(%episode%, \00303%episode%$if(%total%,/%total%))\003 "
    L"$if(%score%,\00314[Score: %score%]\003) "
    L"\00312%animeurl%";
const std::wstring kDefaultFormatTwitter =
    L"$ifequal(%episode%,%total%,Just completed: %title%"
    L"$if(%score%, (Score: %score%)) %animeurl%,"
    L"$ifequal(%episode%,1,Started watching: %title% %animeurl%))";
const std::wstring kDefaultFormatBalloon =
    L"$if(%title%,%title%)\\n"
    L"$if(%episode%,Episode %episode%$if(%total%,/%total%) )"
    L"$if(%group%,by %group%)\\n"
    L"$if(%name%,%name%)";
const std::wstring kDefaultTorrentAppPath =
    L"C:\\Program Files\\uTorrent\\uTorrent.exe";
const std::wstring kDefaultTorrentSearch =
    L"https://nyaa.si/?page=rss&c=1_2&f=0&q=%title%";
const std::wstring kDefaultTorrentSource =
    L"https://www.tokyotosho.info/rss.php?filter=1,11&zwnj=0";

// Here we assume that anything less than 10 MiB can't be a valid episode.
const ULONGLONG kDefaultFileSizeThreshold = 1024 * 1024 * 10;

////////////////////////////////////////////////////////////////////////////////

void AppSettings::InitializeMap() {
  if (!map_.empty())
    return;

  #define INITKEY(name, def, path) InitializeKey(name, def, path);

  // Meta
  INITKEY(kMeta_Version, nullptr, L"meta/version");

  // Services
  INITKEY(kSync_ActiveService, L"anilist", L"account/update/activeservice");
  INITKEY(kSync_AutoOnStart, nullptr, L"account/myanimelist/login");
  INITKEY(kSync_Service_Mal_Username, nullptr, L"account/myanimelist/username");
  INITKEY(kSync_Service_Mal_Password, nullptr, L"account/myanimelist/password");
  INITKEY(kSync_Service_Kitsu_DisplayName, nullptr, L"account/kitsu/displayname");
  INITKEY(kSync_Service_Kitsu_Email, nullptr, L"account/kitsu/email");
  INITKEY(kSync_Service_Kitsu_Username, nullptr, L"account/kitsu/username");
  INITKEY(kSync_Service_Kitsu_Password, nullptr, L"account/kitsu/password");
  INITKEY(kSync_Service_Kitsu_PartialLibrary, L"true", L"account/kitsu/partiallibrary");
  INITKEY(kSync_Service_Kitsu_RatingSystem, L"regular", L"account/kitsu/ratingsystem");
  INITKEY(kSync_Service_AniList_Username, nullptr, L"account/anilist/username");
  INITKEY(kSync_Service_AniList_RatingSystem, L"POINT_10", L"account/anilist/ratingsystem");
  INITKEY(kSync_Service_AniList_Token, nullptr, L"account/anilist/token");

  // Library
  INITKEY(kLibrary_FileSizeThreshold, ToWstr(kDefaultFileSizeThreshold).c_str(), L"anime/folders/scan/minfilesize");
  INITKEY(kLibrary_MediaPlayerPath, nullptr, L"recognition/mediaplayers/launchpath");
  INITKEY(kLibrary_WatchFolders, L"true", L"anime/folders/watch/enabled");

  // Episode management
  INITKEY(kLibrary_Management_DeleteAfterWatch, L"false", L"anime/management/delete/afterwatch");
  INITKEY(kLibrary_Management_DeleteAfterCompletion, L"false", L"anime/management/delete/aftercomplete");
  INITKEY(kLibrary_Management_PromptDelete, L"true", L"anime/management/delete/prompt");
  INITKEY(kLibrary_Management_DeletePermanently, L"false", L"anime/management/delete/permanent");
  INITKEY(kLibrary_Management_KeepNum, L"1", L"anime/management/delete/keepnum");

  // Application
  INITKEY(kApp_List_DoubleClickAction, L"4", L"program/list/action/doubleclick");
  INITKEY(kApp_List_MiddleClickAction, L"3", L"program/list/action/middleclick");
  INITKEY(kApp_List_DisplayEnglishTitles, nullptr, L"program/list/action/englishtitles");
  INITKEY(kApp_List_HighlightNewEpisodes, L"true", L"program/list/filter/episodes/highlight");
  INITKEY(kApp_List_DisplayHighlightedOnTop, nullptr, L"program/list/filter/episodes/highlightedontop");
  INITKEY(kApp_List_ProgressDisplayAired, L"true", L"program/list/progress/showaired");
  INITKEY(kApp_List_ProgressDisplayAvailable, L"true", L"program/list/progress/showavailable");
  INITKEY(kApp_List_SortColumnPrimary, L"anime_title", L"program/list/sort/column");
  INITKEY(kApp_List_SortColumnSecondary, L"anime_title", L"program/list/sort/column2");
  INITKEY(kApp_List_SortOrderPrimary, L"1", L"program/list/sort/order");
  INITKEY(kApp_List_SortOrderSecondary, L"1", L"program/list/sort/order2");
  INITKEY(kApp_List_TitleLanguagePreference, nullptr, L"program/list/action/titlelang");
  INITKEY(kApp_Behavior_Autostart, nullptr, L"program/general/autostart");
  INITKEY(kApp_Behavior_StartMinimized, nullptr, L"program/startup/minimize");
  INITKEY(kApp_Behavior_CheckForUpdates, L"true", L"program/startup/checkversion");
  INITKEY(kApp_Behavior_ScanAvailableEpisodes, nullptr, L"program/startup/checkeps");
  INITKEY(kApp_Behavior_CloseToTray, nullptr, L"program/general/close");
  INITKEY(kApp_Behavior_MinimizeToTray, nullptr, L"program/general/minimize");
  INITKEY(kApp_Connection_ProxyHost, nullptr, L"program/proxy/host");
  INITKEY(kApp_Connection_ProxyUsername, nullptr, L"program/proxy/username");
  INITKEY(kApp_Connection_ProxyPassword, nullptr, L"program/proxy/password");
  INITKEY(kApp_Connection_NoRevoke, L"false", L"program/general/sslnorevoke");
  INITKEY(kApp_Connection_ReuseActive, L"true", L"program/general/reuseconnections");
  INITKEY(kApp_Interface_Theme, L"Default", L"program/general/theme");
  INITKEY(kApp_Interface_ExternalLinks, kDefaultExternalLinks.c_str(), L"program/general/externallinks");

  // Recognition
  INITKEY(kRecognition_DetectionInterval, L"3", L"recognition/general/detectioninterval");
  INITKEY(kRecognition_DetectMediaPlayers, L"true", L"recognition/mediaplayers/enabled");
  INITKEY(kRecognition_DetectStreamingMedia, nullptr, L"recognition/streaming/enabled");
  INITKEY(kRecognition_IgnoredStrings, nullptr, L"recognition/anitomy/ignored_strings");
  INITKEY(kRecognition_LookupParentDirectories, L"true", L"recognition/general/lookup_parent_directories");
  INITKEY(kRecognition_RelationsLastModified, nullptr, L"recognition/general/relations_last_modified");
  INITKEY(kSync_Update_Delay, L"120", L"account/update/delay");
  INITKEY(kSync_Update_AskToConfirm, L"true", L"account/update/asktoconfirm");
  INITKEY(kSync_Update_CheckPlayer, nullptr, L"account/update/checkplayer");
  INITKEY(kSync_Update_OutOfRange, nullptr, L"account/update/outofrange");
  INITKEY(kSync_Update_OutOfRoot, nullptr, L"account/update/outofroot");
  INITKEY(kSync_Update_WaitPlayer, nullptr, L"account/update/waitplayer");
  INITKEY(kSync_GoToNowPlaying_Recognized, L"true", L"account/update/gotonowplaying");
  INITKEY(kSync_GoToNowPlaying_NotRecognized, nullptr, L"account/update/gotonowplayingnot");
  INITKEY(kSync_Notify_Recognized, L"true", L"program/notifications/balloon/recognized");
  INITKEY(kSync_Notify_NotRecognized, L"true", L"program/notifications/balloon/notrecognized");
  INITKEY(kSync_Notify_Format, kDefaultFormatBalloon.c_str(), L"program/notifications/balloon/format");
  INITKEY(kStream_Animelab, L"true", L"recognition/streaming/providers/animelab");
  INITKEY(kStream_Adn, L"true", L"recognition/streaming/providers/adn");
  INITKEY(kStream_Ann, L"true", L"recognition/streaming/providers/ann");
  INITKEY(kStream_Crunchyroll, L"true", L"recognition/streaming/providers/crunchyroll");
  INITKEY(kStream_Funimation, L"true", L"recognition/streaming/providers/funimation");
  INITKEY(kStream_Hidive, L"true", L"recognition/streaming/providers/hidive");
  INITKEY(kStream_Plex, L"true", L"recognition/streaming/providers/plex");
  INITKEY(kStream_Veoh, L"true", L"recognition/streaming/providers/veoh");
  INITKEY(kStream_Viz, L"true", L"recognition/streaming/providers/viz");
  INITKEY(kStream_Vrv, L"true", L"recognition/streaming/providers/vrv");
  INITKEY(kStream_Wakanim, L"true", L"recognition/streaming/providers/wakanim");
  INITKEY(kStream_Yahoo, L"true", L"recognition/streaming/providers/yahoo");
  INITKEY(kStream_Youtube, L"true", L"recognition/streaming/providers/youtube");

  // Sharing
  INITKEY(kShare_Discord_ApplicationId, link::discord::kApplicationId, L"announce/discord/applicationid");
  INITKEY(kShare_Discord_Enabled, nullptr, L"announce/discord/enabled");
  INITKEY(kShare_Discord_Format_Details, kDefaultFormatDiscordDetails.c_str(), L"announce/discord/formatdetails");
  INITKEY(kShare_Discord_Format_State, kDefaultFormatDiscordState.c_str(), L"announce/discord/formatstate");
  INITKEY(kShare_Discord_Username_Enabled, L"true", L"announce/discord/usernameenabled");
  INITKEY(kShare_Http_Enabled, nullptr, L"announce/http/enabled");
  INITKEY(kShare_Http_Format, kDefaultFormatHttp.c_str(), L"announce/http/format");
  INITKEY(kShare_Http_Url, nullptr, L"announce/http/url");
  INITKEY(kShare_Mirc_Enabled, nullptr, L"announce/mirc/enabled");
  INITKEY(kShare_Mirc_MultiServer, nullptr, L"announce/mirc/multiserver");
  INITKEY(kShare_Mirc_UseMeAction, L"true", L"announce/mirc/useaction");
  INITKEY(kShare_Mirc_Mode, L"1", L"announce/mirc/mode");
  INITKEY(kShare_Mirc_Channels, L"#kitsu, #myanimelist, #taiga", L"announce/mirc/channels");
  INITKEY(kShare_Mirc_Format, kDefaultFormatMirc.c_str(), L"announce/mirc/format");
  INITKEY(kShare_Mirc_Service, L"mIRC", L"announce/mirc/service");
  INITKEY(kShare_Twitter_Enabled, nullptr, L"announce/twitter/enabled");
  INITKEY(kShare_Twitter_Format, kDefaultFormatTwitter.c_str(), L"announce/twitter/format");
  INITKEY(kShare_Twitter_OauthToken, nullptr, L"announce/twitter/oauth_token");
  INITKEY(kShare_Twitter_OauthSecret, nullptr, L"announce/twitter/oauth_secret");
  INITKEY(kShare_Twitter_Username, nullptr, L"announce/twitter/user");

  // Torrents
  INITKEY(kTorrent_Discovery_Source, kDefaultTorrentSource.c_str(), L"rss/torrent/source/address");
  INITKEY(kTorrent_Discovery_SearchUrl, kDefaultTorrentSearch.c_str(), L"rss/torrent/search/address");
  INITKEY(kTorrent_Discovery_AutoCheckEnabled, L"true", L"rss/torrent/options/autocheck");
  INITKEY(kTorrent_Discovery_AutoCheckInterval, L"60", L"rss/torrent/options/checkinterval");
  INITKEY(kTorrent_Discovery_NewAction, L"1", L"rss/torrent/options/newaction");
  INITKEY(kTorrent_Download_AppMode, L"1", L"rss/torrent/application/mode");
  INITKEY(kTorrent_Download_AppOpen, L"true", L"rss/torrent/application/open");
  INITKEY(kTorrent_Download_AppPath, nullptr, L"rss/torrent/application/path");
  INITKEY(kTorrent_Download_Location, nullptr, L"rss/torrent/options/downloadpath");
  INITKEY(kTorrent_Download_UseAnimeFolder, L"true", L"rss/torrent/options/autosetfolder");
  INITKEY(kTorrent_Download_FallbackOnFolder, nullptr, L"rss/torrent/options/autousefolder");
  INITKEY(kTorrent_Download_CreateSubfolder, nullptr, L"rss/torrent/options/autocreatefolder");
  INITKEY(kTorrent_Download_SortBy, L"episode_number", L"rss/torrent/options/downloadsortby");
  INITKEY(kTorrent_Download_SortOrder, L"ascending", L"rss/torrent/options/downloadsortorder");
  INITKEY(kTorrent_Download_UseMagnet, L"false", L"rss/torrent/options/downloadusemagnet");
  INITKEY(kTorrent_Filter_Enabled, L"true", L"rss/torrent/filter/enabled");
  INITKEY(kTorrent_Filter_ArchiveMaxCount, L"1000", L"rss/torrent/filter/archive_maxcount");

  // Internal
  INITKEY(kApp_Position_X, L"-1", L"program/position/x");
  INITKEY(kApp_Position_Y, L"-1", L"program/position/y");
  INITKEY(kApp_Position_W, L"960", L"program/position/w");
  INITKEY(kApp_Position_H, L"640", L"program/position/h");
  INITKEY(kApp_Position_Maximized, nullptr, L"program/position/maximized");
  INITKEY(kApp_Position_Remember, L"true", L"program/exit/remember_pos_size");
  INITKEY(kApp_Option_HideSidebar, nullptr, L"program/general/hidesidebar");
  INITKEY(kApp_Option_EnableRecognition, L"true", L"program/general/enablerecognition");
  INITKEY(kApp_Option_EnableSharing, L"true", L"program/general/enablesharing");
  INITKEY(kApp_Option_EnableSync, L"true", L"program/general/enablesync");
  INITKEY(kApp_Seasons_LastSeason, nullptr, L"program/seasons/lastseason");
  INITKEY(kApp_Seasons_GroupBy, ToWstr(ui::kSeasonGroupByType).c_str(), L"program/seasons/groupby");
  INITKEY(kApp_Seasons_SortBy, ToWstr(ui::kSeasonSortByTitle).c_str(), L"program/seasons/sortby");
  INITKEY(kApp_Seasons_ViewAs, ToWstr(ui::kSeasonViewAsTiles).c_str(), L"program/seasons/viewas");

  #undef INITKEY
}

////////////////////////////////////////////////////////////////////////////////

bool AppSettings::Load() {
  XmlDocument document;
  const auto path = taiga::GetPath(taiga::Path::Settings);
  const auto parse_result = XmlLoadFileToDocument(document, path);

  if (!parse_result) {
    LOGE(L"Could not read application settings.\nPath: {}\nReason: {}",
         path, StrToWstr(parse_result.description()));
  }

  auto settings = document.child(L"settings");

  InitializeMap();

  for (enum_t i = kAppSettingNameFirst; i < kAppSettingNameLast; ++i) {
    ReadValue(settings, i);
  }

  // Services
  ServiceManager.service(sync::kKitsu)->user().rating_system =
      GetWstr(kSync_Service_Kitsu_RatingSystem);
  ServiceManager.service(sync::kAniList)->user().rating_system =
      GetWstr(kSync_Service_AniList_RatingSystem);
  ServiceManager.service(sync::kAniList)->user().access_token =
      GetWstr(kSync_Service_AniList_Token);

  // Folders
  library_folders.clear();
  auto node_folders = settings.child(L"anime").child(L"folders");
  for (auto folder : node_folders.children(L"root")) {
    library_folders.push_back(folder.attribute(L"folder").value());
  }

  // Anime items
  auto node_items = settings.child(L"anime").child(L"items");
  for (auto item : node_items.children(L"item")) {
    int anime_id = item.attribute(L"id").as_int();
    auto anime_item = anime::db.Find(anime_id, false);
    if (!anime_item)
      anime_item = &anime::db.items[anime_id];
    anime_item->SetFolder(item.attribute(L"folder").value());
    anime_item->SetUserSynonyms(item.attribute(L"titles").value());
    anime_item->SetUseAlternative(item.attribute(L"use_alternative").as_bool());

    // removal settings
    anime_item->SetUseGlobalRemovalSetting(item.attribute(L"use_global_removal_settings").as_bool());
    anime_item->SetEpisodesToKeep(item.attribute(L"keep_num").as_int());
    anime_item->SetRemovedAfterWatching(item.attribute(L"remove_after_watching").as_bool());
    anime_item->SetRemovedWhenCompleted(item.attribute(L"remove_after_completion").as_bool());
    anime_item->SetPromptAtEpisodeDelete(item.attribute(L"prompt_removal").as_bool());
    anime_item->SetEpisodesDeletedPermanently(item.attribute(L"remove_permanent").as_bool());
  }

  // Media players
  auto node_players = settings.child(L"recognition").child(L"mediaplayers");
  for (auto player : node_players.children(L"player")) {
    std::wstring name = player.attribute(L"name").value();
    bool enabled = player.attribute(L"enabled").as_bool();
    for (auto& media_player : track::media_players.items) {
      if (media_player.name == WstrToStr(name)) {
        media_player.enabled = enabled;
        break;
      }
    }
  }

  // Anime list columns
  ui::DlgAnimeList.listview.InitializeColumns();
  auto node_list_columns = settings.child(L"program").child(L"list").child(L"columns");
  for (auto column : node_list_columns.children(L"column")) {
    std::wstring name = column.attribute(L"name").value();
    auto column_type = ui::AnimeListDialog::ListView::TranslateColumnName(name);
    if (column_type != ui::kColumnUnknown) {
      auto& data = ui::DlgAnimeList.listview.columns[column_type];
      data.order = column.attribute(L"order").as_int();
      data.visible = column.attribute(L"visible").as_bool();
      data.width = column.attribute(L"width").as_int();
    }
  }

  // Torrent application path
  if (GetWstr(kTorrent_Download_AppPath).empty()) {
    Set(kTorrent_Download_AppPath, GetDefaultAppPath(L".torrent", kDefaultTorrentAppPath));
  }

  // Torrent filters
  auto node_filter = settings.child(L"rss").child(L"torrent").child(L"filter");
  track::feed_filter_manager.Import(node_filter, track::feed_filter_manager.filters);
  if (track::feed_filter_manager.filters.empty())
    track::feed_filter_manager.AddPresets();
  auto& feed = track::aggregator.GetFeed();
  feed.channel.link = GetWstr(kTorrent_Discovery_Source);
  track::aggregator.LoadArchive();

  return parse_result.status == pugi::status_ok;
}

////////////////////////////////////////////////////////////////////////////////

bool AppSettings::Save() {
  XmlDocument document;
  auto settings = document.append_child(L"settings");

  // Meta
  Set(kMeta_Version, StrToWstr(taiga::version().to_string()));

  for (enum_t i = kAppSettingNameFirst; i < kAppSettingNameLast; ++i) {
    WriteValue(settings, i);
  }

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
        !anime_item.GetUseAlternative() &&
        anime_item.IsRemovalDefault())
      continue;
    auto item = items.append_child(L"item");
    item.append_attribute(L"id") = anime_item.GetId();
    if (!anime_item.GetFolder().empty())
      item.append_attribute(L"folder") = anime_item.GetFolder().c_str();
    if (anime_item.UserSynonymsAvailable())
      item.append_attribute(L"titles") = Join(anime_item.GetUserSynonyms(), L"; ").c_str();
    if (anime_item.GetUseAlternative())
      item.append_attribute(L"use_alternative") = anime_item.GetUseAlternative();

    // removal settings
    item.append_attribute(L"use_global_removal_settings") = anime_item.GetUseGlobalRemovalSetting();
    item.append_attribute(L"keep_num") = anime_item.GetEpisodesToKeep();
    item.append_attribute(L"remove_after_watching") = anime_item.IsEpisodeRemovedAfterWatching();
    item.append_attribute(L"remove_after_completion") = anime_item.IsEpisodeRemovedWhenCompleted();
    item.append_attribute(L"prompt_removal") = anime_item.IsPromptedAtEpisodeDelete();
    item.append_attribute(L"remove_permanent") = anime_item.IsEpisodesDeletedPermanently();
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

  // Write to registry
  win::Registry reg;
  reg.OpenKey(HKEY_CURRENT_USER,
              L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
              0, KEY_SET_VALUE);
  if (GetBool(kApp_Behavior_Autostart)) {
    std::wstring app_path = Taiga.GetModulePath();
    reg.SetValue(TAIGA_APP_NAME, app_path.c_str());
  } else {
    reg.DeleteValue(TAIGA_APP_NAME);
  }
  reg.CloseKey();

  const auto path = taiga::GetPath(taiga::Path::Settings);
  return XmlSaveDocumentToFile(document, path);
}

////////////////////////////////////////////////////////////////////////////////

void AppSettings::ApplyChanges(const AppSettings previous) {
  const auto previous_service = previous[kSync_ActiveService];
  bool changed_service = GetWstr(kSync_ActiveService) != previous_service;
  if (changed_service) {
    if (library::queue.GetItemCount() > 0) {
      ui::OnSettingsServiceChangeFailed();
      Set(kSync_ActiveService, previous_service);
      changed_service = false;
    } else if (!anime::db.items.empty()) {
      if (ui::OnSettingsServiceChangeConfirm(previous_service,
                                             GetWstr(kSync_ActiveService))) {
        std::wstring current_service = GetWstr(kSync_ActiveService);
        Set(kSync_ActiveService, previous_service);
        anime::db.SaveList(true);
        Set(kSync_ActiveService, current_service);
        anime::db.items.clear();
        anime::db.SaveDatabase();
        ui::image_db.Clear();
        SeasonDatabase.Reset();
      } else {
        Set(kSync_ActiveService, previous_service);
        changed_service = false;
      }
    }
  }

  const auto previous_theme = previous[kApp_Interface_Theme];
  bool changed_theme = GetWstr(kApp_Interface_Theme) != previous_theme;
  if (changed_theme) {
    if (ui::Theme.Load()) {
      ui::OnSettingsThemeChange();
    } else {
      Set(kApp_Interface_Theme, previous_theme);
    }
  }

  const bool changed_account = [&]() {
    const auto equal_settings = [&](AppSettingName name) {
      return previous[name] == GetWstr(name);
    };
    switch (GetCurrentServiceId()) {
      case sync::kMyAnimeList:
        return !equal_settings(kSync_Service_Mal_Username);
      case sync::kKitsu:
        return !equal_settings(kSync_Service_Kitsu_Email);
      case sync::kAniList:
        return !equal_settings(kSync_Service_AniList_Username);
    }
    return false;
  }();
  if (changed_account) {
    switch (GetCurrentServiceId()) {
      case sync::kKitsu:
        Set(kSync_Service_Kitsu_DisplayName, std::wstring{});
        Set(kSync_Service_Kitsu_Username, std::wstring{});
        break;
    }
  }
  if (changed_account || changed_service) {
    anime::db.LoadList();
    library::history.Load();
    CurrentEpisode.Set(anime::ID_UNKNOWN);
    taiga::stats.CalculateAll();
    sync::InvalidateUserAuthentication();
    ui::OnSettingsUserChange();
    ui::OnSettingsServiceChange();
  } else {
    ui::OnSettingsChange();
  }

  bool enable_monitor = GetBool(kLibrary_WatchFolders);
  track::monitor.Enable(enable_monitor);

  bool toggled_discord = GetBool(kShare_Discord_Enabled) !=
      previous.GetBool(kShare_Discord_Enabled);
  if (toggled_discord) {
    if (GetBool(kShare_Discord_Enabled)) {
      link::discord::Initialize();
      ::Announcer.Do(kAnnounceToDiscord);
    } else {
      link::discord::Shutdown();
    }
  }

  ui::Menus.UpdateExternalLinks();
  ui::Menus.UpdateFolders();

  timers.UpdateIntervalsFromSettings();
}

void AppSettings::RestoreDefaults() {
  // Take a backup
  std::wstring file = taiga::GetPath(taiga::Path::Settings);
  std::wstring backup = file + L".bak";
  DWORD flags = MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH;
  MoveFileEx(file.c_str(), backup.c_str(), flags);

  // Reload settings
  const AppSettings previous_settings = *this;
  Load();
  ApplyChanges(previous_settings);

  // Reload settings dialog
  ui::OnSettingsRestoreDefaults();
}

////////////////////////////////////////////////////////////////////////////////

sync::Service* AppSettings::GetCurrentService() const {
  return ServiceManager.service(GetWstr(kSync_ActiveService));
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
      const auto display_name = GetWstr(kSync_Service_Kitsu_DisplayName);
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
      return GetWstr(kSync_Service_Kitsu_Email);
    default:
      return {};
  }
}

std::wstring AppSettings::GetUsername(sync::ServiceId service_id) const {
  switch (service_id) {
    case sync::kMyAnimeList:
      return GetWstr(kSync_Service_Mal_Username);
    case sync::kKitsu:
      return GetWstr(kSync_Service_Kitsu_Username);
    case sync::kAniList:
      return GetWstr(kSync_Service_AniList_Username);
    default:
      return {};
  }
}

std::wstring AppSettings::GetPassword(sync::ServiceId service_id) const {
  switch (service_id) {
    case sync::kMyAnimeList:
      return Base64Decode(GetWstr(kSync_Service_Mal_Password));
    case sync::kKitsu:
      return Base64Decode(GetWstr(kSync_Service_Kitsu_Password));
    case sync::kAniList:
      return GetWstr(kSync_Service_AniList_Token);
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
  return Settings.GetCurrentService();
}

sync::ServiceId GetCurrentServiceId() {
  return Settings.GetCurrentServiceId();
}

std::wstring GetCurrentUserDisplayName() {
  return Settings.GetCurrentUserDisplayName();
}

std::wstring GetCurrentUserEmail() {
  return Settings.GetCurrentUserEmail();
}

std::wstring GetCurrentUsername() {
  return Settings.GetCurrentUsername();
}

std::wstring GetCurrentPassword() {
  return Settings.GetCurrentPassword();
}

}  // namespace taiga
