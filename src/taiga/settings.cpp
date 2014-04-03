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

#include "base/crypto.h"
#include "base/file.h"
#include "base/foreach.h"
#include "base/log.h"
#include "base/string.h"
#include "base/xml.h"
#include "library/anime_db.h"
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
#include "ui/menu.h"
#include "ui/theme.h"
#include "ui/ui.h"
#include "win/win_registry.h"
#include "win/win_taskdialog.h"

taiga::AppSettings Settings;

namespace taiga {

const std::wstring kDefaultExternalLinks =
    L"Hummingboard|http://hummingboard.me\r\n"
    L"MALgraph|http://mal.oko.im\r\n"
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
const std::wstring kDefaultFormatMessenger =
    L"Watching: %title%"
    L"$if(%episode%, #%episode%$if(%total%,/%total%)) ~ "
    L"www.myanimelist.net/anime/%id%";
const std::wstring kDefaultFormatMirc =
    L"\00304$if($greater(%episode%,%watched%),Watching,Re-watching):\003 %title%"
    L"$if(%episode%, \00303%episode%$if(%total%,/%total%))\003 "
    L"$if(%score%,\00314[Score: %score%/10]\003) "
    L"\00312www.myanimelist.net/anime/%id%";
const std::wstring kDefaultFormatSkype =
    L"Watching: <a href=\"http://myanimelist.net/anime/%id%\">%title%</a>"
    L"$if(%episode%, #%episode%$if(%total%,/%total%))";
const std::wstring kDefaultFormatTwitter =
    L"$ifequal(%episode%,%total%,Just completed: %title%"
    L"$if(%score%, (Score: %score%/10)) "
    L"www.myanimelist.net/anime/%id%)";
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

////////////////////////////////////////////////////////////////////////////////

Setting::Setting(bool attribute,
                 const std::wstring& path)
    : attribute(attribute),
      path(path) {
}

Setting::Setting(bool attribute,
                 const std::wstring& default_value,
                 const std::wstring& path)
    : attribute(attribute),
      default_value(default_value),
      path(path) {
}

////////////////////////////////////////////////////////////////////////////////

const std::wstring& AppSettings::operator[](AppSettingName name) const {
  return GetWstr(name);
}

bool AppSettings::GetBool(AppSettingName name) const {
  auto it = map_.find(name);

  if (it != map_.end())
    return ToBool(it->second.value);

  return false;
}

int AppSettings::GetInt(AppSettingName name) const {
  auto it = map_.find(name);

  if (it != map_.end())
    return ToInt(it->second.value);

  return 0;
}

const std::wstring& AppSettings::GetWstr(AppSettingName name) const {
  auto it = map_.find(name);

  if (it != map_.end())
    return it->second.value;

  return EmptyString();
}

void AppSettings::Set(AppSettingName name, bool value) {
  map_[name].value = value ? L"true" : L"false";
}

void AppSettings::Set(AppSettingName name, int value) {
  map_[name].value = ToWstr(value);
}

void AppSettings::Set(AppSettingName name, const std::wstring& value) {
  map_[name].value = value;
}

////////////////////////////////////////////////////////////////////////////////

void AppSettings::InitializeKey(AppSettingName name,
                                const wchar_t* default_value,
                                const std::wstring& path) {
  if (default_value) {
    map_.insert(std::pair<AppSettingName, Setting>(name, Setting(true, default_value, path)));
  } else {
    map_.insert(std::pair<AppSettingName, Setting>(name, Setting(true, path)));
  }
}

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

  // Library
  INITKEY(kLibrary_WatchFolders, L"true", L"anime/folders/watch/enabled");

  // Application
  INITKEY(kApp_List_DoubleClickAction, L"4", L"program/list/action/doubleclick");
  INITKEY(kApp_List_MiddleClickAction, L"3", L"program/list/action/middleclick");
  INITKEY(kApp_List_DisplayEnglishTitles, nullptr, L"program/list/action/englishtitles");
  INITKEY(kApp_List_HighlightNewEpisodes, L"true", L"program/list/filter/episodes/highlight");
  INITKEY(kApp_List_ProgressDisplayAired, L"true", L"program/list/progress/showaired");
  INITKEY(kApp_List_ProgressDisplayAvailable, L"true", L"program/list/progress/showavailable");
  INITKEY(kApp_List_SortColumn, L"0", L"program/list/sort/column");
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
  INITKEY(kApp_Interface_Theme, L"Default", L"program/general/theme");
  INITKEY(kApp_Interface_ExternalLinks, kDefaultExternalLinks.c_str(), L"program/general/externallinks");

  // Recognition
  INITKEY(kSync_Update_Delay, L"120", L"account/update/delay");
  INITKEY(kSync_Update_AskToConfirm, L"true", L"account/update/asktoconfirm");
  INITKEY(kSync_Update_CheckPlayer, nullptr, L"account/update/checkplayer");
  INITKEY(kSync_Update_GoToNowPlaying, L"true", L"account/update/gotonowplaying");
  INITKEY(kSync_Update_OutOfRange, nullptr, L"account/update/outofrange");
  INITKEY(kSync_Update_OutOfRoot, nullptr, L"account/update/outofroot");
  INITKEY(kSync_Update_WaitPlayer, nullptr, L"account/update/waitplayer");
  INITKEY(kSync_Notify_Recognized, L"true", L"program/notifications/balloon/recognized");
  INITKEY(kSync_Notify_NotRecognized, L"true", L"program/notifications/balloon/notrecognized");
  INITKEY(kSync_Notify_Format, kDefaultFormatBalloon.c_str(), L"program/notifications/balloon/format");
  INITKEY(kStream_Ann, nullptr, L"recognition/streaming/providers/ann");
  INITKEY(kStream_Crunchyroll, nullptr, L"recognition/streaming/providers/crunchyroll");
  INITKEY(kStream_Veoh, nullptr, L"recognition/streaming/providers/veoh");
  INITKEY(kStream_Viz, nullptr, L"recognition/streaming/providers/viz");
  INITKEY(kStream_Youtube, nullptr, L"recognition/streaming/providers/youtube");

  // Sharing
  INITKEY(kShare_Http_Enabled, nullptr, L"announce/http/enabled");
  INITKEY(kShare_Http_Format, kDefaultFormatHttp.c_str(), L"announce/http/format");
  INITKEY(kShare_Http_Url, nullptr, L"announce/http/url");
  INITKEY(kShare_Messenger_Enabled, nullptr, L"announce/messenger/enabled");
  INITKEY(kShare_Messenger_Format, kDefaultFormatMessenger.c_str(), L"announce/messenger/format");
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
  INITKEY(kTorrent_Download_AppPath, nullptr, L"rss/torrent/application/path");
  INITKEY(kTorrent_Download_Location, nullptr, L"rss/torrent/options/downloadpath");
  INITKEY(kTorrent_Download_UseAnimeFolder, L"true", L"rss/torrent/options/autosetfolder");
  INITKEY(kTorrent_Download_FallbackOnFolder, nullptr, L"rss/torrent/options/autousefolder");
  INITKEY(kTorrent_Download_CreateSubfolder, nullptr, L"rss/torrent/options/autocreatefolder");
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

  #undef INITKEY
}

void AppSettings::ReadValue(const xml_node& node_parent, AppSettingName name) {
  Setting& item = map_[name];

  std::vector<std::wstring> node_names;
  Split(item.path, L"/", node_names);

  const wchar_t* node_name = node_names.back().c_str();

  xml_node current_node = node_parent;
  for (int i = 0; i < static_cast<int>(node_names.size()) - 1; i++)
    current_node = current_node.child(node_names.at(i).c_str());

  if (item.attribute) {
    const wchar_t* default_value = item.default_value.c_str();
    item.value = current_node.attribute(node_name).as_string(default_value);
  } else {
    item.value = XmlReadStrValue(current_node, node_name);
  }
}

void AppSettings::WriteValue(const xml_node& node_parent, AppSettingName name) {
  Setting& item = map_[name];

  std::vector<std::wstring> node_names;
  Split(item.path, L"/", node_names);

  const wchar_t* node_name = node_names.back().c_str();

  xml_node current_node = node_parent;
  for (int i = 0; i < static_cast<int>(node_names.size()) - 1; i++) {
    std::wstring child_name = node_names.at(i);
    if (!current_node.child(child_name.c_str())) {
      current_node = current_node.append_child(child_name.c_str());
    } else {
      current_node = current_node.child(child_name.c_str());
    }
  }

  if (item.attribute) {
    current_node.append_attribute(node_name) = item.value.c_str();
  } else {
    XmlWriteStrValue(current_node, node_name, item.value.c_str());
  }
}

////////////////////////////////////////////////////////////////////////////////

bool AppSettings::Load() {
  xml_document document;
  std::wstring path = taiga::GetPath(taiga::kPathSettings);
  xml_parse_result result = document.load_file(path.c_str());

  xml_node settings = document.child(L"settings");

  InitializeMap();

  for (enum_t i = kAppSettingNameFirst; i < kAppSettingNameLast; ++i)
    ReadValue(settings, static_cast<AppSettingName>(i));

  // Meta
  if (GetWstr(kMeta_Version_Major).empty())
    Set(kMeta_Version_Major, ToWstr(static_cast<int>(Taiga.version.major)));
  if (GetWstr(kMeta_Version_Minor).empty())
    Set(kMeta_Version_Minor, ToWstr(static_cast<int>(Taiga.version.minor)));
  if (GetWstr(kMeta_Version_Revision).empty())
    Set(kMeta_Version_Revision, ToWstr(static_cast<int>(Taiga.version.patch)));

  // Folders
  root_folders.clear();
  xml_node node_folders = settings.child(L"anime").child(L"folders");
  foreach_xmlnode_(folder, node_folders, L"root")
    root_folders.push_back(folder.attribute(L"folder").value());

  // Anime items
  xml_node node_items = settings.child(L"anime").child(L"items");
  foreach_xmlnode_(item, node_items, L"item") {
    int anime_id = item.attribute(L"id").as_int();
    auto anime_item = AnimeDatabase.FindItem(anime_id);
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

  // Torrent application path
  if (GetWstr(kTorrent_Download_AppPath).empty()) {
    Set(kTorrent_Download_AppPath, GetDefaultAppPath(L".torrent", kDefaultTorrentAppPath));
  }

  // Torrent filters
  xml_node node_filter = settings.child(L"rss").child(L"torrent").child(L"filter");
  Aggregator.filter_manager.filters.clear();
  foreach_xmlnode_(item, node_filter, L"item") {
    Aggregator.filter_manager.AddFilter(
        static_cast<FeedFilterAction>(
            Aggregator.filter_manager.GetIndexFromShortcode(
                kFeedFilterShortcodeAction, item.attribute(L"action").value())),
        static_cast<FeedFilterMatch>(
            Aggregator.filter_manager.GetIndexFromShortcode(
                kFeedFilterShortcodeMatch, item.attribute(L"match").value())),
        static_cast<FeedFilterOption>(
            Aggregator.filter_manager.GetIndexFromShortcode(
                kFeedFilterShortcodeOption, item.attribute(L"option").value())),
        item.attribute(L"enabled").as_bool(),
        item.attribute(L"name").value());
    foreach_xmlnode_(anime, item, L"anime") {
      auto& filter = Aggregator.filter_manager.filters.back();
      filter.anime_ids.push_back(anime.attribute(L"id").as_int());
    }
    foreach_xmlnode_(condition, item, L"condition") {
      Aggregator.filter_manager.filters.back().AddCondition(
          static_cast<FeedFilterElement>(
              Aggregator.filter_manager.GetIndexFromShortcode(
                  kFeedFilterShortcodeElement,
                  condition.attribute(L"element").value())),
          static_cast<FeedFilterOperator>(
              Aggregator.filter_manager.GetIndexFromShortcode(
                  kFeedFilterShortcodeOperator,
                  condition.attribute(L"operator").value())),
          condition.attribute(L"value").value());
    }
  }

  if (Aggregator.filter_manager.filters.empty())
    Aggregator.filter_manager.AddPresets();
  auto feed = Aggregator.Get(kFeedCategoryLink);
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
  Set(kMeta_Version_Major, ToWstr(static_cast<int>(Taiga.version.major)));
  Set(kMeta_Version_Minor, ToWstr(static_cast<int>(Taiga.version.minor)));
  Set(kMeta_Version_Revision, ToWstr(static_cast<int>(Taiga.version.patch)));

  for (enum_t i = kAppSettingNameFirst; i < kAppSettingNameLast; ++i)
    WriteValue(settings, static_cast<AppSettingName>(i));

  // Root folders
  xml_node folders = settings.child(L"anime").child(L"folders");
  foreach_(it, root_folders) {
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
  xml_node mediaplayers = settings.child(L"recognition").append_child(L"mediaplayers");
  foreach_(it, MediaPlayers.items) {
    xml_node player = mediaplayers.append_child(L"player");
    player.append_attribute(L"name") = it->name.c_str();
    player.append_attribute(L"enabled") = it->enabled;
  }

  // Torrent filters
  xml_node torrent_filter = settings.child(L"rss").child(L"torrent").child(L"filter");
  foreach_(it, Aggregator.filter_manager.filters) {
    xml_node item = torrent_filter.append_child(L"item");
    item.append_attribute(L"action") =
        Aggregator.filter_manager.GetShortcodeFromIndex(
            kFeedFilterShortcodeAction, it->action).c_str();
    item.append_attribute(L"match") =
        Aggregator.filter_manager.GetShortcodeFromIndex(
            kFeedFilterShortcodeMatch, it->match).c_str();
    item.append_attribute(L"option") =
        Aggregator.filter_manager.GetShortcodeFromIndex(
            kFeedFilterShortcodeOption, it->option).c_str();
    item.append_attribute(L"enabled") = it->enabled;
    item.append_attribute(L"name") = it->name.c_str();
    foreach_(ita, it->anime_ids) {
      xml_node anime = item.append_child(L"anime");
      anime.append_attribute(L"id") = *ita;
    }
    foreach_(itc, it->conditions) {
      xml_node condition = item.append_child(L"condition");
      condition.append_attribute(L"element") =
          Aggregator.filter_manager.GetShortcodeFromIndex(
              kFeedFilterShortcodeElement, itc->element).c_str();
      condition.append_attribute(L"operator") =
          Aggregator.filter_manager.GetShortcodeFromIndex(
              kFeedFilterShortcodeOperator, itc->op).c_str();
      condition.append_attribute(L"value") = itc->value.c_str();
    }
  }

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
    } else if (!previous_user.empty()) {
      if (ui::OnSettingsServiceChangeConfirm(previous_service,
                                             GetWstr(kSync_ActiveService))) {
        std::wstring current_service = GetWstr(kSync_ActiveService);
        Set(kSync_ActiveService, previous_service);
        AnimeDatabase.SaveList(true);
        Set(kSync_ActiveService, current_service);
        AnimeDatabase.items.clear();
        ImageDatabase.Clear();
      } else {
        Set(kSync_ActiveService, previous_service);
        changed_service = false;
      }
    }
  }

  bool changed_theme = GetWstr(kApp_Interface_Theme) != previous_theme;
  if (changed_theme) {
    ui::Theme.Load();
    ui::OnSettingsThemeChange();
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

  timers.UpdateIntervalsFromSettings();
}

void AppSettings::HandleCompatibility() {
  // Nothing to do here, for now
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
    password = SimpleDecrypt(Settings[kSync_Service_Mal_Password]);
  } else if (service->id() == sync::kHummingbird) {
    password = SimpleDecrypt(Settings[kSync_Service_Hummingbird_Password]);
  }

  return password;
}

}  // namespace taiga