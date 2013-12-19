/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
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

#ifndef TAIGA_TAIGA_SETTINGS_H
#define TAIGA_TAIGA_SETTINGS_H

#include <string>
#include <vector>

namespace pugi {
class xml_node;
}

namespace taiga {

enum AppSettingName {
  kAppSettingNameFirst = 0,  // used for iteration

  // Meta
  kMeta_Version_Major = 0,
  kMeta_Version_Minor,
  kMeta_Version_Revision,

  // Services
  kSync_AutoOnStart,
  kSync_Service_Mal_Username,
  kSync_Service_Mal_Password,

  // Library
  kLibrary_WatchFolders,

  // Application
  kApp_List_DoubleClickAction,
  kApp_List_MiddleClickAction,
  kApp_List_DisplayEnglishTitles,
  kApp_List_HighlightNewEpisodes,
  kApp_List_ProgressDisplayAired,
  kApp_List_ProgressDisplayAvailable,
  kApp_Behavior_Autostart,
  kApp_Behavior_StartMinimized,
  kApp_Behavior_CheckForUpdates,
  kApp_Behavior_ScanAvailableEpisodes,
  kApp_Behavior_CloseToTray,
  kApp_Behavior_MinimizeToTray,
  kApp_Connection_ProxyHost,
  kApp_Connection_ProxyUsername,
  kApp_Connection_ProxyPassword,
  kApp_Interface_Theme,
  kApp_Interface_ExternalLinks,

  // Recognition
  kSync_Update_Delay,
  kSync_Update_AskToConfirm,
  kSync_Update_CheckPlayer,
  kSync_Update_GoToNowPlaying,
  kSync_Update_OutOfRange,
  kSync_Update_OutOfRoot,
  kSync_Update_WaitPlayer,
  kSync_Notify_Recognized,
  kSync_Notify_NotRecognized,
  kSync_Notify_Format,
  kStream_Ann,
  kStream_Crunchyroll,
  kStream_Veoh,
  kStream_Viz,
  kStream_Youtube,

  // Sharing
  kShare_Http_Enabled,
  kShare_Http_Format,
  kShare_Http_Url,
  kShare_Messenger_Enabled,
  kShare_Messenger_Format,
  kShare_Mirc_Enabled,
  kShare_Mirc_MultiServer,
  kShare_Mirc_UseMeAction,
  kShare_Mirc_Mode,
  kShare_Mirc_Channels,
  kShare_Mirc_Format,
  kShare_Mirc_Service,
  kShare_Skype_Enabled,
  kShare_Skype_Format,
  kShare_Twitter_Enabled,
  kShare_Twitter_Format,
  kShare_Twitter_OauthToken,
  kShare_Twitter_OauthSecret,
  kShare_Twitter_Username,

  // Torrents
  kTorrent_Discovery_Source,
  kTorrent_Discovery_SearchUrl,
  kTorrent_Discovery_AutoCheckEnabled,
  kTorrent_Discovery_AutoCheckInterval,
  kTorrent_Discovery_NewAction,
  kTorrent_Download_AppMode,
  kTorrent_Download_AppPath,
  kTorrent_Download_Location,
  kTorrent_Download_UseAnimeFolder,
  kTorrent_Download_FallbackOnFolder,
  kTorrent_Download_CreateSubfolder,
  kTorrent_Filter_Enabled,
  kTorrent_Filter_ArchiveMaxCount,

  // Internal
  kApp_Position_X,
  kApp_Position_Y,
  kApp_Position_W,
  kApp_Position_H,
  kApp_Position_Maximized,
  kApp_Position_Remember,
  kApp_Option_HideSidebar,
  kApp_Option_EnableRecognition,
  kApp_Option_EnableSharing,
  kApp_Option_EnableSync,

  kAppSettingNameLast  // used for iteration
};

class Setting {
public:
  Setting() {}
  Setting(bool attribute, const std::wstring& path);
  Setting(bool attribute, const std::wstring& default_value, const std::wstring& path);
  ~Setting() {}

  bool attribute;
  std::wstring default_value;
  std::wstring path;
  std::wstring value;
};

class AppSettings {
public:
  const std::wstring& operator[](AppSettingName name) const;

  bool Load();
  bool Save();

  bool GetBool(AppSettingName name) const;
  int GetInt(AppSettingName name) const;
  const std::wstring& GetWstr(AppSettingName name) const;

  void Set(AppSettingName name, bool value);
  void Set(AppSettingName name, int value);
  void Set(AppSettingName name, const std::wstring& value);

  void ApplyChanges(const std::wstring& previous_user, const std::wstring& previous_theme);
  void HandleCompatibility();
  void RestoreDefaults();

  std::vector<std::wstring> root_folders;

private:
  void InitializeKey(AppSettingName name, const wchar_t* default_value, const std::wstring& path);
  void InitializeMap();
  void ReadValue(const pugi::xml_node& node_parent, AppSettingName name);
  void WriteValue(const pugi::xml_node& node_parent, AppSettingName name);

  std::map<AppSettingName, Setting> map_;
};

}  // namespace taiga

extern taiga::AppSettings Settings;

#endif  // TAIGA_TAIGA_SETTINGS_H