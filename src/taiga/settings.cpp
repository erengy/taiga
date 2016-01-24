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

#include "base/base64.h"
#include "base/crypto.h"
#include "base/file.h"
#include "base/foreach.h"
#include "base/log.h"
#include "base/string.h"
#include "base/xml.h"
#include "library/anime_db.h"
#include "library/anime_season.h"
#include "library/discover.h"
#include "library/history.h"
#include "library/resource.h"
#include "sync/manager.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "taiga/stats.h"
#include "taiga/taiga.h"
#include "taiga/timer.h"
#include "taiga/version.h"
#include "track/media.h"
#include "track/monitor.h"
#include "ui/dlg/dlg_anime_list.h"
#include "ui/dlg/dlg_season.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "ui/ui.h"
#include "win/win_registry.h"
#include "win/win_taskdialog.h"

taiga::AppSettings Settings;

namespace taiga {

const std::wstring kDefaultExternalLinks =
    L"Hummingboard|http://hb.cybrox.eu\r\n"
    L"MALgraph|http://graph.anime.plus\r\n"
    L"-\r\n"
    L"Mahou Showtime Schedule|http://www.mahou.org/Showtime/?o=ET#Current\r\n"
    L"The Fansub Wiki|http://www.fansubwiki.com";
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
const std::wstring kDefaultFormatSkype =
    L"Watching: <a href=\"%animeurl%\">%title%</a>"
    L"$if(%episode%, #%episode%$if(%total%,/%total%))";
const std::wstring kDefaultFormatTwitter =
    L"$ifequal(%episode%,%total%,Just completed: %title%"
    L"$if(%score%, (Score: %score%)) "
    L"%animeurl%)";
const std::wstring kDefaultFormatBalloon =
    L"$if(%title%,%title%)\\n"
    L"$if(%episode%,Episode %episode%$if(%total%,/%total%) )"
    L"$if(%group%,by %group%)\\n"
    L"$if(%name%,%name%)";
const std::wstring kDefaultTorrentAppPath =
    L"C:\\Program Files\\uTorrent\\uTorrent.exe";
const std::wstring kDefaultTorrentSearch =
    L"http://www.nyaa.se/?page=rss&cats=1_37&filter=2&term=%title%";
const std::wstring kDefaultTorrentSource =
    L"http://tokyotosho.info/rss.php?filter=1,11&zwnj=0";

// Here we assume that anything less than 10 MiB can't be a valid episode.
const ULONGLONG kDefaultFileSizeThreshold = 1024 * 1024 * 10;

////////////////////////////////////////////////////////////////////////////////

void AppSettings::InitializeMap() {
  if (!map_.empty())
    return;

  #define INITKEY(name, def, path) InitializeKey(name, def, path);

  // Meta
  INITKEY(kMeta_Version_Major, nullptr, L"meta/version/major");
  INITKEY(kMeta_Version_Minor, nullptr, L"meta/version/minor");
  INITKEY(kMeta_Version_Revision, nullptr, L"meta/version/revision");

  // Services
  INITKEY(kSync_ActiveService, L"myanimelist", L"account/update/activeservice");
  INITKEY(kSync_AutoOnStart, nullptr, L"account/myanimelist/login");
  INITKEY(kSync_Service_Mal_Username, nullptr, L"account/myanimelist/username");
  INITKEY(kSync_Service_Mal_Password, nullptr, L"account/myanimelist/password");
  INITKEY(kSync_Service_Hummingbird_Username, nullptr, L"account/hummingbird/username");
  INITKEY(kSync_Service_Hummingbird_Password, nullptr, L"account/hummingbird/password");
  INITKEY(kSync_Service_Hummingbird_UseHttps, L"true", L"account/hummingbird/https");

  // Library
  INITKEY(kLibrary_FileSizeThreshold, ToWstr(kDefaultFileSizeThreshold).c_str(), L"anime/folders/scan/minfilesize");
  INITKEY(kLibrary_WatchFolders, L"true", L"anime/folders/watch/enabled");

  // Application
  INITKEY(kApp_List_DoubleClickAction, L"4", L"program/list/action/doubleclick");
  INITKEY(kApp_List_MiddleClickAction, L"3", L"program/list/action/middleclick");
  INITKEY(kApp_List_DisplayEnglishTitles, nullptr, L"program/list/action/englishtitles");
  INITKEY(kApp_List_HighlightNewEpisodes, L"true", L"program/list/filter/episodes/highlight");
  INITKEY(kApp_List_DisplayHighlightedOnTop, nullptr, L"program/list/filter/episodes/highlightedontop");
  INITKEY(kApp_List_ProgressDisplayAired, L"true", L"program/list/progress/showaired");
  INITKEY(kApp_List_ProgressDisplayAvailable, L"true", L"program/list/progress/showavailable");
  INITKEY(kApp_List_SortColumn, L"anime_title", L"program/list/sort/column");
  INITKEY(kApp_List_SortOrder, L"1", L"program/list/sort/order");
  INITKEY(kApp_Behavior_Autostart, nullptr, L"program/general/autostart");
  INITKEY(kApp_Behavior_StartMinimized, nullptr, L"program/startup/minimize");
  INITKEY(kApp_Behavior_CheckForUpdates, L"true", L"program/startup/checkversion");
  INITKEY(kApp_Behavior_ScanAvailableEpisodes, nullptr, L"program/startup/checkeps");
  INITKEY(kApp_Behavior_CloseToTray, nullptr, L"program/general/close");
  INITKEY(kApp_Behavior_MinimizeToTray, nullptr, L"program/general/minimize");
  INITKEY(kApp_Connection_ProxyHost, nullptr, L"program/proxy/host");
  INITKEY(kApp_Connection_ProxyUsername, nullptr, L"program/proxy/username");
  INITKEY(kApp_Connection_ProxyPassword, nullptr, L"program/proxy/password");
  INITKEY(kApp_Connection_ReuseActive, L"true", L"program/general/reuseconnections");
  INITKEY(kApp_Interface_Theme, L"Default", L"program/general/theme");
  INITKEY(kApp_Interface_ExternalLinks, kDefaultExternalLinks.c_str(), L"program/general/externallinks");

  // Recognition
  INITKEY(kRecognition_BrowserDetectionMethod, L"ui_automation", L"recognition/streaming/browserdetectionmethod");
  INITKEY(kRecognition_MediaPlayerDetectionMethod, L"prioritize_file_handle", L"recognition/mediaplayers/detectionmethod");
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
  INITKEY(kStream_Ann, L"true", L"recognition/streaming/providers/ann");
  INITKEY(kStream_Crunchyroll, L"true", L"recognition/streaming/providers/crunchyroll");
  INITKEY(kStream_Daisuki, L"true", L"recognition/streaming/providers/daisuki");
  INITKEY(kStream_Plex, L"true", L"recognition/streaming/providers/plex");
  INITKEY(kStream_Veoh, L"true", L"recognition/streaming/providers/veoh");
  INITKEY(kStream_Viz, L"true", L"recognition/streaming/providers/viz");
  INITKEY(kStream_Wakanim, L"true", L"recognition/streaming/providers/wakanim");
  INITKEY(kStream_Youtube, L"true", L"recognition/streaming/providers/youtube");

  // Sharing
  INITKEY(kShare_Http_Enabled, nullptr, L"announce/http/enabled");
  INITKEY(kShare_Http_Format, kDefaultFormatHttp.c_str(), L"announce/http/format");
  INITKEY(kShare_Http_Url, nullptr, L"announce/http/url");
  INITKEY(kShare_Mirc_Enabled, nullptr, L"announce/mirc/enabled");
  INITKEY(kShare_Mirc_MultiServer, nullptr, L"announce/mirc/multiserver");
  INITKEY(kShare_Mirc_UseMeAction, L"true", L"announce/mirc/useaction");
  INITKEY(kShare_Mirc_Mode, L"1", L"announce/mirc/mode");
  INITKEY(kShare_Mirc_Channels, L"#hummingbird, #myanimelist, #taiga", L"announce/mirc/channels");
  INITKEY(kShare_Mirc_Format, kDefaultFormatMirc.c_str(), L"announce/mirc/format");
  INITKEY(kShare_Mirc_Service, L"mIRC", L"announce/mirc/service");
  INITKEY(kShare_Skype_Enabled, nullptr, L"announce/skype/enabled");
  INITKEY(kShare_Skype_Format, kDefaultFormatSkype.c_str(), L"announce/skype/format");
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
  INITKEY(kApp_Seasons_MaxSeason, nullptr, L"program/seasons/maxseason");
  INITKEY(kApp_Seasons_GroupBy, ToWstr(ui::kSeasonGroupByType).c_str(), L"program/seasons/groupby");
  INITKEY(kApp_Seasons_SortBy, ToWstr(ui::kSeasonSortByTitle).c_str(), L"program/seasons/sortby");
  INITKEY(kApp_Seasons_ViewAs, ToWstr(ui::kSeasonViewAsTiles).c_str(), L"program/seasons/viewas");

  #undef INITKEY
}

////////////////////////////////////////////////////////////////////////////////

bool AppSettings::Load() {
  xml_document document;
  std::wstring path = taiga::GetPath(taiga::kPathSettings);
  xml_parse_result result = document.load_file(path.c_str());

  xml_node settings = document.child(L"settings");

  InitializeMap();

  for (enum_t i = kAppSettingNameFirst; i < kAppSettingNameLast; ++i)
    ReadValue(settings, i);

  // Meta
  if (GetWstr(kMeta_Version_Major).empty())
    Set(kMeta_Version_Major, ToWstr(Taiga.version.major));
  if (GetWstr(kMeta_Version_Minor).empty())
    Set(kMeta_Version_Minor, ToWstr(Taiga.version.minor));
  if (GetWstr(kMeta_Version_Revision).empty())
    Set(kMeta_Version_Revision, ToWstr(Taiga.version.patch));

  // Folders
  library_folders.clear();
  xml_node node_folders = settings.child(L"anime").child(L"folders");
  foreach_xmlnode_(folder, node_folders, L"root")
    library_folders.push_back(folder.attribute(L"folder").value());

  // Anime items
  xml_node node_items = settings.child(L"anime").child(L"items");
  foreach_xmlnode_(item, node_items, L"item") {
    int anime_id = item.attribute(L"id").as_int();
    auto anime_item = AnimeDatabase.FindItem(anime_id, false);
    if (!anime_item)
      anime_item = &AnimeDatabase.items[anime_id];
    anime_item->SetFolder(item.attribute(L"folder").value());
    anime_item->SetUserSynonyms(item.attribute(L"titles").value());
    anime_item->SetUseAlternative(item.attribute(L"use_alternative").as_bool());
  }

  // Media players
  xml_node node_players = settings.child(L"recognition").child(L"mediaplayers");
  foreach_xmlnode_(player, node_players, L"player") {
    std::wstring name = player.attribute(L"name").value();
    bool enabled = player.attribute(L"enabled").as_bool();
    foreach_(it, MediaPlayers.items) {
      if (it->name == name) {
        it->enabled = enabled;
        break;
      }
    }
  }

  // Anime list columns
  ui::DlgAnimeList.listview.InitializeColumns();
  xml_node node_list_columns = settings.child(L"program").child(L"list").child(L"columns");
  foreach_xmlnode_(column, node_list_columns, L"column") {
    std::wstring name = column.attribute(L"name").value();
    auto column_type = ui::AnimeListDialog::ListView::TranslateColumnName(name);
    if (column_type != ui::kColumnUnknown) {
      auto& data = ui::DlgAnimeList.listview.columns[column_type];
      data.order = column.attribute(L"order").as_int();
      data.visible = column.attribute(L"visible").as_bool();
      data.width = column.attribute(L"width").as_int();
    }
  }

  // Seasons
  anime::Season season_max(GetWstr(kApp_Seasons_MaxSeason));
  if (season_max && season_max > SeasonDatabase.available_seasons.second)
    SeasonDatabase.available_seasons.second = season_max;

  // Torrent application path
  if (GetWstr(kTorrent_Download_AppPath).empty()) {
    Set(kTorrent_Download_AppPath, GetDefaultAppPath(L".torrent", kDefaultTorrentAppPath));
  }

  // Torrent filters
  xml_node node_filter = settings.child(L"rss").child(L"torrent").child(L"filter");
  Aggregator.filter_manager.Import(node_filter, Aggregator.filter_manager.filters);
  if (Aggregator.filter_manager.filters.empty())
    Aggregator.filter_manager.AddPresets();
  auto feed = Aggregator.GetFeed(kFeedCategoryLink);
  if (feed)
    feed->link = GetWstr(kTorrent_Discovery_Source);
  Aggregator.LoadArchive();

  return result.status == pugi::status_ok;
}

////////////////////////////////////////////////////////////////////////////////

bool AppSettings::Save() {
  xml_document document;
  xml_node settings = document.append_child(L"settings");

  // Meta
  Set(kMeta_Version_Major, ToWstr(Taiga.version.major));
  Set(kMeta_Version_Minor, ToWstr(Taiga.version.minor));
  Set(kMeta_Version_Revision, ToWstr(Taiga.version.patch));

  for (enum_t i = kAppSettingNameFirst; i < kAppSettingNameLast; ++i)
    WriteValue(settings, i);

  // Library folders
  xml_node folders = settings.child(L"anime").child(L"folders");
  foreach_(it, library_folders) {
    xml_node root = folders.append_child(L"root");
    root.append_attribute(L"folder") = it->c_str();
  }

  // Anime items
  xml_node items = settings.child(L"anime").append_child(L"items");
  foreach_(it, AnimeDatabase.items) {
    anime::Item& anime_item = it->second;
    if (anime_item.GetFolder().empty() &&
        !anime_item.UserSynonymsAvailable() &&
        !anime_item.GetUseAlternative())
      continue;
    xml_node item = items.append_child(L"item");
    item.append_attribute(L"id") = anime_item.GetId();
    if (!anime_item.GetFolder().empty())
      item.append_attribute(L"folder") = anime_item.GetFolder().c_str();
    if (anime_item.UserSynonymsAvailable())
      item.append_attribute(L"titles") = Join(anime_item.GetUserSynonyms(), L"; ").c_str();
    if (anime_item.GetUseAlternative())
      item.append_attribute(L"use_alternative") = anime_item.GetUseAlternative();
  }

  // Media players
  xml_node mediaplayers = settings.child(L"recognition").child(L"mediaplayers");
  foreach_(it, MediaPlayers.items) {
    xml_node player = mediaplayers.append_child(L"player");
    player.append_attribute(L"name") = it->name.c_str();
    player.append_attribute(L"enabled") = it->enabled;
  }

  // Anime list columns
  xml_node list_columns = settings.child(L"program").child(L"list").append_child(L"columns");
  for (const auto& it : ui::DlgAnimeList.listview.columns) {
    const auto& column = it.second;
    xml_node node = list_columns.append_child(L"column");
    node.append_attribute(L"name") = column.key.c_str();
    node.append_attribute(L"order") = column.order;
    node.append_attribute(L"visible") = column.visible;
    node.append_attribute(L"width") = column.width;
  }

  // Torrent filters
  xml_node torrent_filter = settings.child(L"rss").child(L"torrent").child(L"filter");
  Aggregator.filter_manager.Export(torrent_filter, Aggregator.filter_manager.filters);

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

  std::wstring path = taiga::GetPath(taiga::kPathSettings);
  return XmlWriteDocumentToFile(document, path);
}

////////////////////////////////////////////////////////////////////////////////

void AppSettings::ApplyChanges(const std::wstring& previous_service,
                               const std::wstring& previous_user,
                               const std::wstring& previous_theme) {
  bool changed_service = GetWstr(kSync_ActiveService) != previous_service;
  if (changed_service) {
    if (History.queue.GetItemCount() > 0) {
      ui::OnSettingsServiceChangeFailed();
      Set(kSync_ActiveService, previous_service);
      changed_service = false;
    } else if (!AnimeDatabase.items.empty()) {
      if (ui::OnSettingsServiceChangeConfirm(previous_service,
                                             GetWstr(kSync_ActiveService))) {
        std::wstring current_service = GetWstr(kSync_ActiveService);
        Set(kSync_ActiveService, previous_service);
        AnimeDatabase.SaveList(true);
        Set(kSync_ActiveService, current_service);
        AnimeDatabase.items.clear();
        AnimeDatabase.SaveDatabase();
        ImageDatabase.Clear();
        SeasonDatabase.Reset();
      } else {
        Set(kSync_ActiveService, previous_service);
        changed_service = false;
      }
    }
  }

  bool changed_theme = GetWstr(kApp_Interface_Theme) != previous_theme;
  if (changed_theme) {
    if (ui::Theme.Load()) {
      ui::OnSettingsThemeChange();
    } else {
      Set(kApp_Interface_Theme, previous_theme);
    }
  }

  bool changed_username = GetCurrentUsername() != previous_user;
  if (changed_username || changed_service) {
    AnimeDatabase.LoadList();
    History.Load();
    CurrentEpisode.Set(anime::ID_UNKNOWN);
    Stats.CalculateAll();
    Taiga.logged_in = false;
    ui::OnSettingsUserChange();
    ui::OnSettingsServiceChange();
  } else {
    ui::OnSettingsChange();
  }

  bool enable_monitor = GetBool(kLibrary_WatchFolders);
  FolderMonitor.Enable(enable_monitor);

  ui::Menus.UpdateExternalLinks();
  ui::Menus.UpdateFolders();

  timers.UpdateIntervalsFromSettings();
}

void AppSettings::HandleCompatibility() {
  if (GetInt(kMeta_Version_Major) <= 1 &&
      GetInt(kMeta_Version_Minor) <= 1 &&
      GetInt(kMeta_Version_Revision) <= 7) {
    // Convert old password encoding to base64
    std::wstring password = SimpleDecrypt(GetWstr(kSync_Service_Mal_Password));
    Set(kSync_Service_Mal_Password, Base64Encode(password));
    password = SimpleDecrypt(GetWstr(kSync_Service_Hummingbird_Password));
    Set(kSync_Service_Hummingbird_Password, Base64Encode(password));

    // Update torrent filters
    foreach_(filter, Aggregator.filter_manager.filters) {
      if (filter->name == L"Discard unknown titles") {
        if (filter->conditions.size() == 1) {
          auto condition = filter->conditions.begin();
          if (condition->element == kFeedFilterElement_Meta_Id) {
            filter->name = L"Discard and deactivate not-in-list anime";
            condition->element = kFeedFilterElement_User_Status;
            condition->value = ToWstr(anime::kNotInList);
          }
        }
        break;
      }
    }
  }

  if (GetInt(kMeta_Version_Major) <= 1 &&
      GetInt(kMeta_Version_Minor) <= 1 &&
      GetInt(kMeta_Version_Revision) <= 8) {
    auto external_links = GetWstr(kApp_Interface_ExternalLinks);
    ReplaceString(external_links, L"http://hummingboard.me", L"http://hb.cybrox.eu");
    Set(kApp_Interface_ExternalLinks, external_links);
  }

  if (GetInt(kMeta_Version_Major) <= 1 &&
      GetInt(kMeta_Version_Minor) <= 1 &&
      GetInt(kMeta_Version_Revision) <= 11) {
    bool detect_streaming_media = false;
    for (auto& media_player : MediaPlayers.items) {
      if (media_player.mode == kMediaModeWebBrowser) {
        if (media_player.enabled)
          detect_streaming_media = true;
        media_player.enabled = true;
      }
    }
    Set(kRecognition_DetectStreamingMedia, detect_streaming_media);
  }
}

void AppSettings::RestoreDefaults() {
  // Take a backup
  std::wstring file = taiga::GetPath(taiga::kPathSettings);
  std::wstring backup = file + L".bak";
  DWORD flags = MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH;
  MoveFileEx(file.c_str(), backup.c_str(), flags);

  // Reload settings
  std::wstring previous_service = GetCurrentService()->canonical_name();
  std::wstring previous_user = GetCurrentUsername();
  std::wstring previous_theme = GetWstr(kApp_Interface_Theme);
  Load();
  ApplyChanges(previous_service, previous_user, previous_theme);

  // Reload settings dialog
  ui::OnSettingsRestoreDefaults();
}

////////////////////////////////////////////////////////////////////////////////

const sync::Service* GetCurrentService() {
  std::wstring service_name = Settings[kSync_ActiveService];
  return ServiceManager.service(service_name);
}

sync::ServiceId GetCurrentServiceId() {
  auto service = GetCurrentService();

  if (service)
    return static_cast<sync::ServiceId>(service->id());

  return sync::kMyAnimeList;
}

const std::wstring GetCurrentUsername() {
  std::wstring username;
  auto service = GetCurrentService();

  if (service->id() == sync::kMyAnimeList) {
    username = Settings[kSync_Service_Mal_Username];
  } else if (service->id() == sync::kHummingbird) {
    username = Settings[kSync_Service_Hummingbird_Username];
  }

  return username;
}

const std::wstring GetCurrentPassword() {
  std::wstring password;
  auto service = GetCurrentService();

  if (service->id() == sync::kMyAnimeList) {
    password = Base64Decode(Settings[kSync_Service_Mal_Password]);
  } else if (service->id() == sync::kHummingbird) {
    password = Base64Decode(Settings[kSync_Service_Hummingbird_Password]);
  }

  return password;
}

}  // namespace taiga