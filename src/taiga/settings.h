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

#pragma once

#include <string>
#include <vector>

#include "base/settings.h"

namespace pugi {
class xml_node;
}

namespace sync {
class Service;
enum ServiceId;
}

namespace taiga {

enum AppSettingName {
  kAppSettingNameFirst = 0,  // used for iteration

  // Meta
  kMeta_Version = 0,

  // Services
  kSync_ActiveService,
  kSync_AutoOnStart,
  kSync_Service_Mal_Username,
  kSync_Service_Mal_Password,
  kSync_Service_Kitsu_DisplayName,
  kSync_Service_Kitsu_Email,
  kSync_Service_Kitsu_Username,
  kSync_Service_Kitsu_Password,
  kSync_Service_Kitsu_PartialLibrary,
  kSync_Service_Kitsu_RatingSystem,
  kSync_Service_AniList_Username,
  kSync_Service_AniList_RatingSystem,
  kSync_Service_AniList_Token,

  // Library
  kLibrary_FileSizeThreshold,
  kLibrary_MediaPlayerPath,
  kLibrary_WatchFolders,

  // Episode management
  kLibrary_Management_DeleteAfterWatch,
  kLibrary_Management_DeleteAfterCompletion,
  kLibrary_Management_PromptDelete,
  kLibrary_Management_DeletePermanently,
  kLibrary_Management_KeepNum,

  // Application
  kApp_List_DoubleClickAction,
  kApp_List_MiddleClickAction,
  kApp_List_DisplayEnglishTitles,
  kApp_List_HighlightNewEpisodes,
  kApp_List_DisplayHighlightedOnTop,
  kApp_List_ProgressDisplayAired,
  kApp_List_ProgressDisplayAvailable,
  kApp_List_SortColumnPrimary,
  kApp_List_SortColumnSecondary,
  kApp_List_SortOrderPrimary,
  kApp_List_SortOrderSecondary,
  kApp_List_TitleLanguagePreference,
  kApp_Behavior_Autostart,
  kApp_Behavior_StartMinimized,
  kApp_Behavior_CheckForUpdates,
  kApp_Behavior_ScanAvailableEpisodes,
  kApp_Behavior_CloseToTray,
  kApp_Behavior_MinimizeToTray,
  kApp_Connection_ProxyHost,
  kApp_Connection_ProxyUsername,
  kApp_Connection_ProxyPassword,
  kApp_Connection_NoRevoke,
  kApp_Connection_ReuseActive,
  kApp_Interface_Theme,
  kApp_Interface_ExternalLinks,

  // Recognition
  kRecognition_DetectionInterval,
  kRecognition_DetectMediaPlayers,
  kRecognition_DetectStreamingMedia,
  kRecognition_IgnoredStrings,
  kRecognition_LookupParentDirectories,
  kRecognition_RelationsLastModified,
  kSync_Update_Delay,
  kSync_Update_AskToConfirm,
  kSync_Update_CheckPlayer,
  kSync_Update_OutOfRange,
  kSync_Update_OutOfRoot,
  kSync_Update_WaitPlayer,
  kSync_GoToNowPlaying_Recognized,
  kSync_GoToNowPlaying_NotRecognized,
  kSync_Notify_Recognized,
  kSync_Notify_NotRecognized,
  kSync_Notify_Format,
  kStream_Animelab,
  kStream_Adn,
  kStream_Ann,
  kStream_Crunchyroll,
  kStream_Funimation,
  kStream_Hidive,
  kStream_Plex,
  kStream_Veoh,
  kStream_Viz,
  kStream_Vrv,
  kStream_Wakanim,
  kStream_Yahoo,
  kStream_Youtube,

  // Sharing
  kShare_Discord_ApplicationId,
  kShare_Discord_Enabled,
  kShare_Discord_Format_Details,
  kShare_Discord_Format_State,
  kShare_Discord_Username_Enabled,
  kShare_Http_Enabled,
  kShare_Http_Format,
  kShare_Http_Url,
  kShare_Mirc_Enabled,
  kShare_Mirc_MultiServer,
  kShare_Mirc_UseMeAction,
  kShare_Mirc_Mode,
  kShare_Mirc_Channels,
  kShare_Mirc_Format,
  kShare_Mirc_Service,
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
  kTorrent_Download_AppOpen,
  kTorrent_Download_AppPath,
  kTorrent_Download_Location,
  kTorrent_Download_UseAnimeFolder,
  kTorrent_Download_FallbackOnFolder,
  kTorrent_Download_CreateSubfolder,
  kTorrent_Download_SortBy,
  kTorrent_Download_SortOrder,
  kTorrent_Download_UseMagnet,
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
  kApp_Seasons_LastSeason,
  kApp_Seasons_GroupBy,
  kApp_Seasons_SortBy,
  kApp_Seasons_ViewAs,

  kAppSettingNameLast  // used for iteration
};

extern const std::wstring kDefaultTorrentSearch;
extern const std::wstring kDefaultTorrentSource;

class AppSettings : public base::Settings {
public:
  bool Load();
  bool Save();

  void ApplyChanges(const AppSettings previous);
  bool HandleCompatibility();
  void RestoreDefaults();

  sync::Service* GetCurrentService() const;
  sync::ServiceId GetCurrentServiceId() const;
  std::wstring GetUserDisplayName(sync::ServiceId service_id) const;
  std::wstring GetUserEmail(sync::ServiceId service_id) const;
  std::wstring GetUsername(sync::ServiceId service_id) const;
  std::wstring GetPassword(sync::ServiceId service_id) const;
  std::wstring GetCurrentUserDisplayName() const;
  std::wstring GetCurrentUserEmail() const;
  std::wstring GetCurrentUsername() const;
  std::wstring GetCurrentPassword() const;

  std::vector<std::wstring> library_folders;

private:
  void InitializeMap();
};

sync::Service* GetCurrentService();
sync::ServiceId GetCurrentServiceId();
std::wstring GetCurrentUserDisplayName();
std::wstring GetCurrentUserEmail();
std::wstring GetCurrentUsername();
std::wstring GetCurrentPassword();

}  // namespace taiga

extern taiga::AppSettings Settings;
