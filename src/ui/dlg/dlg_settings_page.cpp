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

#include <windows.h>
#include <uxtheme.h>

#include <windows/win/common_dialogs.h>
#include <windows/win/task_dialog.h>

#include "base/base64.h"
#include "base/crypto.h"
#include "base/file.h"
#include "base/log.h"
#include "base/string.h"
#include "link/mirc.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "media/library/history.h"
#include "ui/resource.h"
#include "sync/anilist_util.h"
#include "sync/myanimelist.h"
#include "sync/myanimelist_util.h"
#include "taiga/announce.h"
#include "taiga/app.h"
#include "taiga/path.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "taiga/version.h"
#include "track/feed_aggregator.h"
#include "track/feed_filter_manager.h"
#include "track/feed_filter_util.h"
#include "track/media.h"
#include "track/media_stream.h"
#include "ui/dlg/dlg_feed_filter.h"
#include "ui/dlg/dlg_format.h"
#include "ui/dlg/dlg_input.h"
#include "ui/dlg/dlg_settings.h"
#include "ui/dlg/dlg_settings_page.h"
#include "ui/command.h"
#include "ui/list.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "ui/ui.h"

namespace ui {

SettingsPage::SettingsPage()
    : index(-1), parent(nullptr) {
}

void SettingsPage::Create() {
  win::Rect rect_page;
  parent->tab_.GetWindowRect(parent->GetWindowHandle(), &rect_page);
  parent->tab_.AdjustRect(nullptr, FALSE, &rect_page);

  UINT resource_id = 0;
  switch (index) {
    #define SETRESOURCEID(page, id) case page: resource_id = id; break;
    SETRESOURCEID(kSettingsPageAdvancedSettings, IDD_SETTINGS_ADVANCED);
    SETRESOURCEID(kSettingsPageAdvancedCache, IDD_SETTINGS_ADVANCED_CACHE);
    SETRESOURCEID(kSettingsPageAppGeneral, IDD_SETTINGS_APP_GENERAL);
    SETRESOURCEID(kSettingsPageAppList, IDD_SETTINGS_APP_LIST);
    SETRESOURCEID(kSettingsPageLibraryFolders, IDD_SETTINGS_LIBRARY_FOLDERS);
    SETRESOURCEID(kSettingsPageRecognitionGeneral, IDD_SETTINGS_RECOGNITION_GENERAL);
    SETRESOURCEID(kSettingsPageRecognitionMedia, IDD_SETTINGS_RECOGNITION_MEDIA);
    SETRESOURCEID(kSettingsPageRecognitionStream, IDD_SETTINGS_RECOGNITION_STREAM);
    SETRESOURCEID(kSettingsPageServicesMain, IDD_SETTINGS_SERVICES_MAIN);
    SETRESOURCEID(kSettingsPageServicesMal, IDD_SETTINGS_SERVICES_MAL);
    SETRESOURCEID(kSettingsPageServicesKitsu, IDD_SETTINGS_SERVICES_KITSU);
    SETRESOURCEID(kSettingsPageServicesAniList, IDD_SETTINGS_SERVICES_ANILIST);
    SETRESOURCEID(kSettingsPageSharingDiscord, IDD_SETTINGS_SHARING_DISCORD);
    SETRESOURCEID(kSettingsPageSharingHttp, IDD_SETTINGS_SHARING_HTTP);
    SETRESOURCEID(kSettingsPageSharingMirc, IDD_SETTINGS_SHARING_MIRC);
    SETRESOURCEID(kSettingsPageTorrentsDiscovery, IDD_SETTINGS_TORRENTS_DISCOVERY);
    SETRESOURCEID(kSettingsPageTorrentsDownloads, IDD_SETTINGS_TORRENTS_DOWNLOADS);
    SETRESOURCEID(kSettingsPageTorrentsFilters, IDD_SETTINGS_TORRENTS_FILTERS);
    #undef SETRESOURCEID
  }

  win::Dialog::Create(resource_id, parent->GetWindowHandle(), false);
  SetPosition(nullptr, rect_page, 0);
  EnableThemeDialogTexture(GetWindowHandle(), ETDT_ENABLETAB);
}

////////////////////////////////////////////////////////////////////////////////

BOOL SettingsPage::OnInitDialog() {
  switch (index) {
    // Services > Main
    case kSettingsPageServicesMain: {
      win::ComboBox combo(GetDlgItem(IDC_COMBO_SERVICE));
      for (const auto service_id : sync::kServiceIds) {
        combo.AddItem(sync::GetServiceNameById(service_id).c_str(),
                      static_cast<LPARAM>(service_id));
        if (service_id == taiga::settings.GetSyncActiveService())
          combo.SetCurSel(combo.GetCount() - 1);
      }
      combo.SetWindowHandle(nullptr);
      CheckDlgButton(IDC_CHECK_START_LOGIN, taiga::settings.GetSyncAutoOnStart());
      break;
    }
    // Services > MyAnimeList
    case kSettingsPageServicesMal: {
      SetDlgItemText(IDC_EDIT_USER_MAL, taiga::settings.GetSyncServiceMalUsername().c_str());
      if (taiga::settings.GetSyncServiceMalAccessToken().empty()) {
        SetDlgItemText(IDC_BUTTON_MAL_AUTH, L"Authorize...");
      } else {
        SetDlgItemText(IDC_BUTTON_MAL_AUTH, L"Re-authorize...");
      }
      break;
    }
    // Services > Kitsu
    case kSettingsPageServicesKitsu: {
      SetDlgItemText(IDC_EDIT_USER_KITSU, taiga::settings.GetSyncServiceKitsuEmail().c_str());
      SetDlgItemText(IDC_EDIT_PASS_KITSU, Base64Decode(taiga::settings.GetSyncServiceKitsuPassword()).c_str());
      break;
    }
    // Services > AniList
    case kSettingsPageServicesAniList: {
      SetDlgItemText(IDC_EDIT_USER_ANILIST, taiga::settings.GetSyncServiceAniListUsername().c_str());
      if (taiga::settings.GetSyncServiceAniListToken().empty()) {
        SetDlgItemText(IDC_BUTTON_ANILIST_AUTH, L"Authorize...");
      } else {
        SetDlgItemText(IDC_BUTTON_ANILIST_AUTH, L"Re-authorize...");
      }
      break;
    }

    ////////////////////////////////////////////////////////////////////////////

    // Library > Folders
    case kSettingsPageLibraryFolders: {
      win::ListView list = GetDlgItem(IDC_LIST_FOLDERS_ROOT);
      list.InsertColumn(0, 0, 0, 0, L"Folder");
      list.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
      list.SetExtendedStyle(LVS_EX_DOUBLEBUFFER);
      list.SetImageList(ui::Theme.GetImageList16().GetHandle());
      list.SetTheme();
      const auto library_folders = taiga::settings.GetLibraryFolders();
      for (size_t i = 0; i < library_folders.size(); ++i) {
        list.InsertItem(i, -1, ui::kIcon16_Folder, 0, nullptr, library_folders[i].c_str(), 0);
      }
      list.SetWindowHandle(nullptr);
      CheckDlgButton(IDC_CHECK_FOLDERS_WATCH, taiga::settings.GetLibraryWatchFolders());
      break;
    }

    ////////////////////////////////////////////////////////////////////////////

    // Application > General
    case kSettingsPageAppGeneral: {
      CheckDlgButton(IDC_CHECK_AUTOSTART, taiga::settings.GetAppBehaviorAutostart());
      CheckDlgButton(IDC_CHECK_GENERAL_CLOSE, taiga::settings.GetAppBehaviorCloseToTray());
      CheckDlgButton(IDC_CHECK_GENERAL_MINIMIZE, taiga::settings.GetAppBehaviorMinimizeToTray());
      CheckDlgButton(IDC_CHECK_START_VERSION, taiga::settings.GetAppBehaviorCheckForUpdates());
      CheckDlgButton(IDC_CHECK_START_CHECKEPS, taiga::settings.GetAppBehaviorScanAvailableEpisodes());
      CheckDlgButton(IDC_CHECK_START_MINIMIZE, taiga::settings.GetAppBehaviorStartMinimized());
      SetDlgItemText(IDC_EDIT_EXTERNALLINKS, taiga::settings.GetAppInterfaceExternalLinks().c_str());
      break;
    }
    // Application > List
    case kSettingsPageAppList: {
      AddComboString(IDC_COMBO_DBLCLICK, L"Do nothing");
      AddComboString(IDC_COMBO_DBLCLICK, L"Edit details");
      AddComboString(IDC_COMBO_DBLCLICK, L"Open folder");
      AddComboString(IDC_COMBO_DBLCLICK, L"Play next episode");
      AddComboString(IDC_COMBO_DBLCLICK, L"View anime info");
      AddComboString(IDC_COMBO_DBLCLICK, L"View anime page");
      SetComboSelection(IDC_COMBO_DBLCLICK, taiga::settings.GetAppListDoubleClickAction());
      AddComboString(IDC_COMBO_MDLCLICK, L"Do nothing");
      AddComboString(IDC_COMBO_MDLCLICK, L"Edit details");
      AddComboString(IDC_COMBO_MDLCLICK, L"Open folder");
      AddComboString(IDC_COMBO_MDLCLICK, L"Play next episode");
      AddComboString(IDC_COMBO_MDLCLICK, L"View anime info");
      AddComboString(IDC_COMBO_MDLCLICK, L"View anime page");
      SetComboSelection(IDC_COMBO_MDLCLICK, taiga::settings.GetAppListMiddleClickAction());
      AddComboString(IDC_COMBO_TITLELANG, L"Romaji");
      AddComboString(IDC_COMBO_TITLELANG, L"English");
      AddComboString(IDC_COMBO_TITLELANG, L"Native");
      SetComboSelection(IDC_COMBO_TITLELANG, static_cast<int>(taiga::settings.GetAppListTitleLanguagePreference()));
      const bool enabled = taiga::settings.GetAppListHighlightNewEpisodes();
      CheckDlgButton(IDC_CHECK_HIGHLIGHT, enabled);
      CheckDlgButton(IDC_CHECK_HIGHLIGHT_ONTOP, taiga::settings.GetAppListDisplayHighlightedOnTop());
      EnableDlgItem(IDC_CHECK_HIGHLIGHT_ONTOP, enabled);
      CheckDlgButton(IDC_CHECK_LIST_PROGRESS_AIRED, taiga::settings.GetAppListProgressDisplayAired());
      CheckDlgButton(IDC_CHECK_LIST_PROGRESS_AVAILABLE, taiga::settings.GetAppListProgressDisplayAvailable());
      break;
    }

    ////////////////////////////////////////////////////////////////////////////

    // Recognition > General
    case kSettingsPageRecognitionGeneral: {
      CheckDlgButton(IDC_CHECK_UPDATE_CONFIRM, taiga::settings.GetSyncUpdateAskToConfirm());
      CheckDlgButton(IDC_CHECK_UPDATE_CHECKMP, taiga::settings.GetSyncUpdateCheckPlayer());
      CheckDlgButton(IDC_CHECK_UPDATE_RANGE, taiga::settings.GetSyncUpdateOutOfRange());
      CheckDlgButton(IDC_CHECK_UPDATE_ROOT, taiga::settings.GetSyncUpdateOutOfRoot());
      CheckDlgButton(IDC_CHECK_UPDATE_WAITMP, taiga::settings.GetSyncUpdateWaitPlayer());
      SendDlgItemMessage(IDC_SPIN_DELAY, UDM_SETRANGE32, 10, 3600);
      SendDlgItemMessage(IDC_SPIN_DELAY, UDM_SETPOS32, 0, taiga::settings.GetSyncUpdateDelay());
      CheckDlgButton(IDC_CHECK_NOTIFY_RECOGNIZED, taiga::settings.GetSyncNotifyRecognized());
      CheckDlgButton(IDC_CHECK_NOTIFY_NOTRECOGNIZED, taiga::settings.GetSyncNotifyNotRecognized());
      CheckDlgButton(IDC_CHECK_GOTO_RECOGNIZED, taiga::settings.GetSyncGoToNowPlayingRecognized());
      CheckDlgButton(IDC_CHECK_GOTO_NOTRECOGNIZED, taiga::settings.GetSyncGoToNowPlayingNotRecognized());
      break;
    }
    // Recognition > Media players
    case kSettingsPageRecognitionMedia: {
      bool enabled = taiga::settings.GetRecognitionDetectMediaPlayers();
      CheckDlgButton(IDC_CHECK_DETECT_MEDIA_PLAYER, enabled);
      win::ListView list = GetDlgItem(IDC_LIST_MEDIA);
      list.Enable(enabled);
      list.EnableGroupView(true);
      list.InsertColumn(0, 0, 0, 0, L"Select/deselect all");
      list.InsertGroup(0, L"Supported media players");
      list.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);
      list.SetImageList(ui::Theme.GetImageList16().GetHandle());
      list.SetTheme();

      win::Window header = list.GetHeader();
      HDITEM hdi = {0};
      header.SetStyle(HDS_CHECKBOXES, 0);
      hdi.mask = HDI_FORMAT;
      Header_GetItem(header.GetWindowHandle(), 0, &hdi);
      hdi.fmt |= HDF_CHECKBOX;
      Header_SetItem(header.GetWindowHandle(), 0, &hdi);
      header.SetWindowHandle(nullptr);

      for (size_t i = 0; i < track::media_players.items.size(); i++) {
        const auto& player = track::media_players.items[i];
        if (player.type == anisthesia::PlayerType::WebBrowser)
          continue;
        int j = list.InsertItem(i, 0, ui::kIcon16_AppBlue, 0, nullptr,
                                StrToWstr(player.name).c_str(), i);
        list.SetCheckState(
            j, taiga::settings.GetMediaPlayerEnabled(StrToWstr(player.name)));
      }
      list.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
      list.SetWindowHandle(nullptr);
      break;
    }
    // Recognition > Streaming media
    case kSettingsPageRecognitionStream: {
      bool enabled = taiga::settings.GetRecognitionDetectStreamingMedia();
      CheckDlgButton(IDC_CHECK_DETECT_STREAMING_MEDIA, enabled);
      win::ListView list = GetDlgItem(IDC_LIST_STREAM_PROVIDER);
      list.Enable(enabled);
      list.EnableGroupView(true);
      list.InsertColumn(0, 0, 0, 0, L"Select/deselect all");
      list.InsertGroup(0, L"Supported media providers");
      list.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);
      list.SetImageList(ui::Theme.GetImageList16().GetHandle());
      list.SetTheme();

      win::Window header = list.GetHeader();
      HDITEM hdi = {0};
      header.SetStyle(HDS_CHECKBOXES, 0);
      hdi.mask = HDI_FORMAT;
      Header_GetItem(header.GetWindowHandle(), 0, &hdi);
      hdi.fmt |= HDF_CHECKBOX;
      Header_SetItem(header.GetWindowHandle(), 0, &hdi);
      header.SetWindowHandle(nullptr);

      const auto& stream_data = track::recognition::GetStreamData();
      for (int i = 0; i < static_cast<int>(stream_data.size()); i++) {
        const auto& item = stream_data.at(i);
        list.InsertItem(i, 0, ui::kIcon16_AppBlue, 0, nullptr,
                        item.name.c_str(), static_cast<LPARAM>(item.id));
        if (track::recognition::IsStreamEnabled(item.id))
          list.SetCheckState(i, TRUE);
      }
      list.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
      list.SetWindowHandle(nullptr);
      break;
    }

    ////////////////////////////////////////////////////////////////////////////

    // Sharing > Discord
    case kSettingsPageSharingDiscord: {
      CheckDlgButton(IDC_CHECK_DISCORD, taiga::settings.GetShareDiscordEnabled());
      CheckDlgButton(IDC_CHECK_DISCORD_USERNAME, taiga::settings.GetShareDiscordUsernameEnabled());
      CheckDlgButton(IDC_CHECK_DISCORD_GROUP, taiga::settings.GetShareDiscordGroupEnabled());
      CheckDlgButton(IDC_CHECK_DISCORD_TIME, taiga::settings.GetShareDiscordTimeEnabled());
      break;
    }
    // Sharing > HTTP
    case kSettingsPageSharingHttp: {
      CheckDlgButton(IDC_CHECK_HTTP, taiga::settings.GetShareHttpEnabled());
      SetDlgItemText(IDC_EDIT_HTTP_URL, taiga::settings.GetShareHttpUrl().c_str());
      break;
    }
    // Sharing > mIRC
    case kSettingsPageSharingMirc: {
      CheckDlgButton(IDC_CHECK_MIRC, taiga::settings.GetShareMircEnabled());
      CheckDlgButton(IDC_CHECK_MIRC_MULTISERVER, taiga::settings.GetShareMircMultiServer());
      CheckDlgButton(IDC_CHECK_MIRC_ACTION, taiga::settings.GetShareMircUseMeAction());
      SetDlgItemText(IDC_EDIT_MIRC_SERVICE, taiga::settings.GetShareMircService().c_str());
      CheckRadioButton(IDC_RADIO_MIRC_CHANNEL1, IDC_RADIO_MIRC_CHANNEL3,
                       IDC_RADIO_MIRC_CHANNEL1 + taiga::settings.GetShareMircMode() - 1);
      SetDlgItemText(IDC_EDIT_MIRC_CHANNELS, taiga::settings.GetShareMircChannels().c_str());
      EnableDlgItem(IDC_EDIT_MIRC_CHANNELS, taiga::settings.GetShareMircMode() == 3);
      break;
    }

    ////////////////////////////////////////////////////////////////////////////

    // Torrents > Discovery
    case kSettingsPageTorrentsDiscovery: {
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"https://anidex.info/rss/?cat=1&lang_id=1");
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"https://nyaa.net/feed?c=3_5&s=0");
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"https://nyaa.si/?page=rss&c=1_2&f=0");
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"http://tracker.minglong.org/rss.xml");
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"https://www.shanaproject.com/feeds/site/");
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"https://subsplease.org/rss/?t&r=1080");
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"https://www.tokyotosho.info/rss.php?filter=1,11&zwnj=0");
      SetDlgItemText(IDC_COMBO_TORRENT_SOURCE, taiga::settings.GetTorrentDiscoverySource().c_str());
      AddComboString(IDC_COMBO_TORRENT_SEARCH, L"https://anidex.info/rss/?cat=1&lang_id=1&q=%title%");
      AddComboString(IDC_COMBO_TORRENT_SEARCH, L"https://nyaa.net/feed?c=3_5&s=0&q=%title%");
      AddComboString(IDC_COMBO_TORRENT_SEARCH, L"https://nyaa.si/?page=rss&c=1_2&f=0&q=%title%");
      SetDlgItemText(IDC_COMBO_TORRENT_SEARCH, taiga::settings.GetTorrentDiscoverySearchUrl().c_str());
      CheckDlgButton(IDC_CHECK_TORRENT_AUTOCHECK, taiga::settings.GetTorrentDiscoveryAutoCheckEnabled());
      SendDlgItemMessage(IDC_SPIN_TORRENT_INTERVAL, UDM_SETRANGE32, 10, 3600);
      SendDlgItemMessage(IDC_SPIN_TORRENT_INTERVAL, UDM_SETPOS32, 0, taiga::settings.GetTorrentDiscoveryAutoCheckInterval());
      EnableDlgItem(IDC_EDIT_TORRENT_INTERVAL, taiga::settings.GetTorrentDiscoveryAutoCheckEnabled());
      EnableDlgItem(IDC_SPIN_TORRENT_INTERVAL, taiga::settings.GetTorrentDiscoveryAutoCheckEnabled());
      CheckRadioButton(IDC_RADIO_TORRENT_NEW1, IDC_RADIO_TORRENT_NEW2,
                       IDC_RADIO_TORRENT_NEW1 + taiga::settings.GetTorrentDiscoveryNewAction() - 1);
      EnableDlgItem(IDC_RADIO_TORRENT_NEW1, taiga::settings.GetTorrentDiscoveryAutoCheckEnabled());
      EnableDlgItem(IDC_RADIO_TORRENT_NEW2, taiga::settings.GetTorrentDiscoveryAutoCheckEnabled());
      break;
    }
    // Torrents > Downloads
    case kSettingsPageTorrentsDownloads: {
      bool enabled = true;
      // Queue
      AddComboString(IDC_COMBO_TORRENTS_QUEUE_SORTBY, L"Sort by episode number");
      AddComboString(IDC_COMBO_TORRENTS_QUEUE_SORTBY, L"Sort by release date");
      SetComboSelection(IDC_COMBO_TORRENTS_QUEUE_SORTBY,
                        taiga::settings.GetTorrentDownloadSortBy() == L"episode_number" ? 0 : 1);
      AddComboString(IDC_COMBO_TORRENTS_QUEUE_SORTORDER, L"In ascending order");
      AddComboString(IDC_COMBO_TORRENTS_QUEUE_SORTORDER, L"In descending order");
      SetComboSelection(IDC_COMBO_TORRENTS_QUEUE_SORTORDER,
                        taiga::settings.GetTorrentDownloadSortOrder() == L"ascending" ? 0 : 1);
      // Location
      CheckDlgButton(IDC_CHECK_TORRENT_AUTOSETFOLDER, taiga::settings.GetTorrentDownloadUseAnimeFolder());
      CheckDlgButton(IDC_CHECK_TORRENT_AUTOUSEFOLDER, taiga::settings.GetTorrentDownloadFallbackOnFolder());
      CheckDlgButton(IDC_CHECK_TORRENT_AUTOCREATEFOLDER, taiga::settings.GetTorrentDownloadCreateSubfolder());
      for (const auto folder : taiga::settings.GetLibraryFolders()) {
        AddComboString(IDC_COMBO_TORRENT_FOLDER, folder.c_str());
      }
      SetDlgItemText(IDC_COMBO_TORRENT_FOLDER, taiga::settings.GetTorrentDownloadLocation().c_str());
      EnableDlgItem(IDC_CHECK_TORRENT_AUTOUSEFOLDER, taiga::settings.GetTorrentDownloadUseAnimeFolder());
      enabled = taiga::settings.GetTorrentDownloadUseAnimeFolder() &&
                taiga::settings.GetTorrentDownloadFallbackOnFolder();
      EnableDlgItem(IDC_COMBO_TORRENT_FOLDER, enabled);
      EnableDlgItem(IDC_BUTTON_TORRENT_BROWSE_FOLDER, enabled);
      EnableDlgItem(IDC_CHECK_TORRENT_AUTOCREATEFOLDER, enabled);
      // Application
      CheckDlgButton(IDC_CHECK_TORRENT_APP_OPEN, taiga::settings.GetTorrentDownloadAppOpen());
      CheckRadioButton(IDC_RADIO_TORRENT_APP1, IDC_RADIO_TORRENT_APP2,
                       IDC_RADIO_TORRENT_APP1 + taiga::settings.GetTorrentDownloadAppMode() - 1);
      EnableDlgItem(IDC_RADIO_TORRENT_APP1, taiga::settings.GetTorrentDownloadAppOpen());
      EnableDlgItem(IDC_RADIO_TORRENT_APP2, taiga::settings.GetTorrentDownloadAppOpen());
      SetDlgItemText(IDC_EDIT_TORRENT_APP, taiga::settings.GetTorrentDownloadAppPath().c_str());
      enabled = taiga::settings.GetTorrentDownloadAppOpen() &&
                taiga::settings.GetTorrentDownloadAppMode() > 1;
      EnableDlgItem(IDC_EDIT_TORRENT_APP, enabled);
      EnableDlgItem(IDC_BUTTON_TORRENT_BROWSE_APP, enabled);
      break;
    }
    // Torrents > Filters
    case kSettingsPageTorrentsFilters: {
      BOOL enable = taiga::settings.GetTorrentFilterEnabled();
      CheckDlgButton(IDC_CHECK_TORRENT_FILTER, enable);
      EnableDlgItem(IDC_LIST_TORRENT_FILTER, enable);
      EnableDlgItem(IDC_TOOLBAR_FEED_FILTER, enable);
      win::ListView list = GetDlgItem(IDC_LIST_TORRENT_FILTER);
      list.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
      list.SetImageList(ui::Theme.GetImageList16().GetHandle());
      list.SetTheme();
      list.InsertColumn(0, ScaleX(400), 0, LVCFMT_LEFT, L"Name");
      list.InsertColumn(1, ScaleX(100), 0, LVCFMT_RIGHT, L"Applies to");
      parent->feed_filters_ = track::feed_filter_manager.GetFilters();
      parent->RefreshTorrentFilterList(list.GetWindowHandle());
      list.SetWindowHandle(nullptr);
      // Initialize toolbar
      win::Toolbar toolbar = GetDlgItem(IDC_TOOLBAR_FEED_FILTER);
      toolbar.SetImageList(ui::Theme.GetImageList16().GetHandle(), ScaleX(16), ScaleY(16));
      toolbar.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
      // Add toolbar items (see TBN_GETINFOTIP for actual tooltips)
      BYTE fsState1 = TBSTATE_ENABLED;
      BYTE fsState2 = TBSTATE_INDETERMINATE;
      toolbar.InsertButton(0, ui::kIcon16_Plus,      100, fsState1, 0, 0, nullptr, L"Add new filter...");
      toolbar.InsertButton(1, ui::kIcon16_Minus,     101, fsState2, 0, 1, nullptr, L"Delete filter");
      toolbar.InsertButton(2, 0, 0, 0, BTNS_SEP, 0, nullptr, nullptr);
      toolbar.InsertButton(3, ui::kIcon16_ArrowUp,   103, fsState2, 0, 3, nullptr, L"Move up");
      toolbar.InsertButton(4, ui::kIcon16_ArrowDown, 104, fsState2, 0, 4, nullptr, L"Move down");
      toolbar.InsertButton(5, 0, 0, 0, BTNS_SEP, 0, nullptr, nullptr);
      toolbar.InsertButton(6, ui::kIcon16_Import,    106, fsState1, 0, 6, nullptr, L"Import filters...");
      toolbar.InsertButton(7, ui::kIcon16_Export,    107, fsState1, 0, 7, nullptr, L"Export filters...");
      toolbar.InsertButton(8, 0, 0, 0, BTNS_SEP, 0, nullptr, nullptr);
      toolbar.InsertButton(9, ui::kIcon16_Cross,     109, fsState1, 0, 9, nullptr, L"Reset filters");
      toolbar.SetWindowHandle(nullptr);
      break;
    }

    ////////////////////////////////////////////////////////////////////////////

    // Advanced > Settings
    case kSettingsPageAdvancedSettings: {
      for (const auto key : GetAdvancedSettingKeys()) {
        parent->advanced_settings_[key] = GetAdvancedSettingValue(key);
      }
      win::ListView list = GetDlgItem(IDC_LIST_ADVANCED_SETTINGS);
      list.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
      list.SetTheme();
      list.InsertColumn(0, ScaleX(350), 0, LVCFMT_LEFT, L"Name");
      list.InsertColumn(1, ScaleX(150), 0, LVCFMT_LEFT, L"Value");
      int list_index = 0;
      for (const auto& [key, value] : parent->advanced_settings_) {
        const auto description = GetAdvancedSettingDescription(key);
        list.InsertItem(list_index, -1, -1, 0, nullptr, description.c_str(), static_cast<LPARAM>(key));
        const bool is_password = key == AdvancedSetting::AppConnectionProxyPassword;
        const auto text = is_password ? std::wstring(value.size(), L'\u25cf') : value;
        list.SetItem(list_index, 1, text.c_str());
        ++list_index;
      }
      list.Sort(0, 1, 0, ui::ListViewCompareProc);
      list.SetWindowHandle(nullptr);
      break;
    }
    // Advanced > Cache
    case kSettingsPageAdvancedCache: {
      parent->RefreshCache();
      break;
    }
  }

  return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

INT_PTR SettingsPage::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_CTLCOLORSTATIC: {
      HDC hdc = reinterpret_cast<HDC>(wParam);
      HWND hwnd_static = reinterpret_cast<HWND>(lParam);
      // We use WS_EX_TRANSPARENT to identify secondary static controls
      if (::GetWindowLong(hwnd_static, GWL_EXSTYLE) & WS_EX_TRANSPARENT) {
        ::SetBkMode(hdc, TRANSPARENT);
        ::SetTextColor(hdc, ::GetSysColor(COLOR_GRAYTEXT));
        return reinterpret_cast<INT_PTR>(::GetStockObject(HOLLOW_BRUSH));
      }
      break;
    }
  }

  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

BOOL SettingsPage::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (HIWORD(wParam)) {
    case BN_CLICKED: {
      switch (LOWORD(wParam)) {
        // Edit format
        case IDC_BUTTON_FORMAT_HTTP:
          DlgFormat.mode = FormatDialogMode::Http;
          DlgFormat.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;
        case IDC_BUTTON_FORMAT_MIRC:
          DlgFormat.mode = FormatDialogMode::Mirc;
          DlgFormat.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;

        ////////////////////////////////////////////////////////////////////////

        // Add folders
        case IDC_BUTTON_ADDFOLDER: {
          std::wstring path;
          if (win::BrowseForFolder(GetWindowHandle(), L"Add a Library Folder", L"", path)) {
            win::ListView list = GetDlgItem(IDC_LIST_FOLDERS_ROOT);
            list.InsertItem(list.GetItemCount(), -1, ui::kIcon16_Folder, 0, nullptr, path.c_str(), 0);
            list.SelectItem(list.GetItemCount() - 1);
            list.SetWindowHandle(nullptr);
          }
          return TRUE;
        }
        // Remove folders
        case IDC_BUTTON_REMOVEFOLDER: {
          win::ListView list = GetDlgItem(IDC_LIST_FOLDERS_ROOT);
          while (list.GetSelectedCount() > 0)
            list.DeleteItem(list.GetNextItem(-1, LVNI_SELECTED));
          EnableDlgItem(IDC_BUTTON_REMOVEFOLDER, FALSE);
          list.SetWindowHandle(nullptr);
          return TRUE;
        }

        ////////////////////////////////////////////////////////////////////////

        // Clear cache
        case IDC_BUTTON_CACHE_CLEAR: {
          if (IsDlgButtonChecked(IDC_CHECK_CACHE1)) {
            if (ui::OnHistoryClear())
              library::history.Clear();
            CheckDlgButton(IDC_CHECK_CACHE1, FALSE);
          }
          if (IsDlgButtonChecked(IDC_CHECK_CACHE2)) {
            ui::image_db.Clear();
            CheckDlgButton(IDC_CHECK_CACHE2, FALSE);
          }
          if (IsDlgButtonChecked(IDC_CHECK_CACHE3)) {
            std::wstring path = taiga::GetPath(taiga::Path::Feed);
            DeleteFolder(path);
            CheckDlgButton(IDC_CHECK_CACHE3, FALSE);
          }
          if (IsDlgButtonChecked(IDC_CHECK_CACHE4)) {
            track::aggregator.archive.Clear();
            track::aggregator.archive.Save();
            CheckDlgButton(IDC_CHECK_CACHE4, FALSE);
          }
          parent->RefreshCache();
          EnableDlgItem(IDC_BUTTON_CACHE_CLEAR, FALSE);
          return TRUE;
        }

        ////////////////////////////////////////////////////////////////////////

        // Test DDE connection
        case IDC_BUTTON_MIRC_TEST: {
          std::wstring service;
          GetDlgItemText(IDC_EDIT_MIRC_SERVICE, service);

          if (!link::mirc::IsRunning()) {
            ui::OnMircNotRunning(true);
          } else {
            std::vector<std::wstring> channels;
            if (!link::mirc::GetChannels(service, channels)) {
              ui::OnMircDdeConnectionFail(true);
            } else {
              ui::OnMircDdeConnectionSuccess(channels, true);
            }
          }

          return TRUE;
        }

        ////////////////////////////////////////////////////////////////////////

        // Authorize AniList
        case IDC_BUTTON_ANILIST_AUTH: {
          sync::anilist::RequestToken();
          std::wstring auth_pin;
          if (ui::EnterAuthorizationPin(L"AniList", auth_pin)) {
            taiga::settings.SetSyncServiceAniListToken(auth_pin);
          }
          return TRUE;
        }

        // Authorize MyAnimeList
        case IDC_BUTTON_MAL_AUTH: {
          std::wstring code_verifier;
          sync::myanimelist::RequestAuthorizationCode(code_verifier);
          std::wstring authorization_code;
          if (ui::EnterAuthorizationPin(L"MyAnimeList", authorization_code)) {
            sync::myanimelist::RequestAccessToken(authorization_code, code_verifier);
          }
          return TRUE;
        }

        ////////////////////////////////////////////////////////////////////////

        // Browse for torrent application
        case IDC_BUTTON_TORRENT_BROWSE_APP: {
          std::wstring current_directory = taiga::app.GetCurrentDirectory();
          std::wstring path = win::BrowseForFile(
              GetWindowHandle(), L"Select Torrent Application",
              L"Executable files (*.exe)\0*.exe\0\0");
          if (current_directory != taiga::app.GetCurrentDirectory())
            taiga::app.SetCurrentDirectory(current_directory);
          if (!path.empty())
            SetDlgItemText(IDC_EDIT_TORRENT_APP, path.c_str());
          return TRUE;
        }
        // Browse for torrent download path
        case IDC_BUTTON_TORRENT_BROWSE_FOLDER: {
          std::wstring path;
          if (win::BrowseForFolder(GetWindowHandle(), L"Select Download Folder", L"", path))
            SetDlgItemText(IDC_COMBO_TORRENT_FOLDER, path.c_str());
          return TRUE;
        }
        // Enable/disable controls
        case IDC_CHECK_CACHE1:
        case IDC_CHECK_CACHE2:
        case IDC_CHECK_CACHE3:
        case IDC_CHECK_CACHE4: {
          bool enable = IsDlgButtonChecked(IDC_CHECK_CACHE1) ||
                        IsDlgButtonChecked(IDC_CHECK_CACHE2) ||
                        IsDlgButtonChecked(IDC_CHECK_CACHE3) ||
                        IsDlgButtonChecked(IDC_CHECK_CACHE4);
          EnableDlgItem(IDC_BUTTON_CACHE_CLEAR, enable);
          return TRUE;
        }
        case IDC_CHECK_DETECT_MEDIA_PLAYER: {
          BOOL enable = IsDlgButtonChecked(LOWORD(wParam));
          EnableDlgItem(IDC_LIST_MEDIA, enable);
          return TRUE;
        }
        case IDC_CHECK_DETECT_STREAMING_MEDIA: {
          BOOL enable = IsDlgButtonChecked(LOWORD(wParam));
          EnableDlgItem(IDC_LIST_STREAM_PROVIDER, enable);
          return TRUE;
        }
        case IDC_CHECK_HIGHLIGHT: {
          BOOL enable = IsDlgButtonChecked(LOWORD(wParam));
          EnableDlgItem(IDC_CHECK_HIGHLIGHT_ONTOP, enable);
          return TRUE;
        }
        case IDC_CHECK_TORRENT_AUTOCHECK: {
          BOOL enable = IsDlgButtonChecked(LOWORD(wParam));
          EnableDlgItem(IDC_EDIT_TORRENT_INTERVAL, enable);
          EnableDlgItem(IDC_SPIN_TORRENT_INTERVAL, enable);
          EnableDlgItem(IDC_RADIO_TORRENT_NEW1, enable);
          EnableDlgItem(IDC_RADIO_TORRENT_NEW2, enable);
          return TRUE;
        }
        case IDC_CHECK_TORRENT_APP_OPEN: {
          BOOL enable = IsDlgButtonChecked(LOWORD(wParam));
          EnableDlgItem(IDC_RADIO_TORRENT_APP1, enable);
          EnableDlgItem(IDC_RADIO_TORRENT_APP2, enable);
          enable = enable && IsDlgButtonChecked(IDC_RADIO_TORRENT_APP2);
          EnableDlgItem(IDC_EDIT_TORRENT_APP, enable);
          EnableDlgItem(IDC_BUTTON_TORRENT_BROWSE_APP, enable);
          return TRUE;
        }
        case IDC_CHECK_TORRENT_AUTOSETFOLDER: {
          BOOL enable = IsDlgButtonChecked(LOWORD(wParam));
          EnableDlgItem(IDC_CHECK_TORRENT_AUTOUSEFOLDER, enable);
          enable = enable && IsDlgButtonChecked(IDC_CHECK_TORRENT_AUTOUSEFOLDER);
          EnableDlgItem(IDC_COMBO_TORRENT_FOLDER, enable);
          EnableDlgItem(IDC_BUTTON_TORRENT_BROWSE_FOLDER, enable);
          EnableDlgItem(IDC_CHECK_TORRENT_AUTOCREATEFOLDER, enable);
          return TRUE;
        }
        case IDC_CHECK_TORRENT_AUTOUSEFOLDER: {
          BOOL enable = IsDlgButtonChecked(LOWORD(wParam));
          EnableDlgItem(IDC_COMBO_TORRENT_FOLDER, enable);
          EnableDlgItem(IDC_BUTTON_TORRENT_BROWSE_FOLDER, enable);
          EnableDlgItem(IDC_CHECK_TORRENT_AUTOCREATEFOLDER, enable);
          return TRUE;
        }
        // Enable/disable filters
        case IDC_CHECK_TORRENT_FILTER: {
          BOOL enable = IsDlgButtonChecked(LOWORD(wParam));
          EnableDlgItem(IDC_LIST_TORRENT_FILTER, enable);
          EnableDlgItem(IDC_TOOLBAR_FEED_FILTER, enable);
          return TRUE;
        }
        // Add global filter
        case 100: {
          DlgFeedFilter.filter = {};
          ExecuteCommand(L"TorrentAddFilter", TRUE, reinterpret_cast<LPARAM>(parent->GetWindowHandle()));
          if (!DlgFeedFilter.filter.conditions.empty()) {
            if (DlgFeedFilter.filter.name.empty())
              DlgFeedFilter.filter.name = track::util::CreateNameFromConditions(DlgFeedFilter.filter);
            parent->feed_filters_.push_back(DlgFeedFilter.filter);
            win::ListView list = GetDlgItem(IDC_LIST_TORRENT_FILTER);
            parent->RefreshTorrentFilterList(list.GetWindowHandle());
            list.SelectItem(list.GetItemCount() - 1);
            list.SetWindowHandle(nullptr);
          }
          return TRUE;
        }
        // Remove global filter
        case 101: {
          win::ListView list = GetDlgItem(IDC_LIST_TORRENT_FILTER);
          int item_index = list.GetNextItem(-1, LVNI_SELECTED);
          if (item_index > -1) {
            auto feed_filter = reinterpret_cast<track::FeedFilter*>(list.GetItemParam(item_index));
            for (auto it = parent->feed_filters_.begin(); it != parent->feed_filters_.end(); ++it) {
              if (feed_filter == &(*it)) {
                parent->feed_filters_.erase(it);
                break;
              }
            }
            list.DeleteItem(item_index);
            parent->UpdateTorrentFilterList(list.GetWindowHandle());
          }
          list.SetWindowHandle(nullptr);
          return TRUE;
        }
        // Move filter up
        case 103: {
          win::ListView list = GetDlgItem(IDC_LIST_TORRENT_FILTER);
          int index = list.GetNextItem(-1, LVNI_SELECTED);
          if (index > 0) {
            iter_swap(parent->feed_filters_.begin() + index,
                      parent->feed_filters_.begin() + index - 1);
            parent->RefreshTorrentFilterList(list.GetWindowHandle());
            list.SelectItem(index - 1);
          }
          list.SetWindowHandle(nullptr);
          return TRUE;
        }
        // Move filter down
        case 104: {
          win::ListView list = GetDlgItem(IDC_LIST_TORRENT_FILTER);
          int index = list.GetNextItem(-1, LVNI_SELECTED);
          if (index > -1 && index < list.GetItemCount() - 1) {
            iter_swap(parent->feed_filters_.begin() + index,
                      parent->feed_filters_.begin() + index + 1);
            parent->RefreshTorrentFilterList(list.GetWindowHandle());
            list.SelectItem(index + 1);
          }
          list.SetWindowHandle(nullptr);
          return TRUE;
        }
        // Import filters
        case 106: {
          InputDialog dlg;
          dlg.title = L"Import Filters";
          dlg.info = L"Enter encoded text below:";
          dlg.Show(parent->GetWindowHandle());
          if (dlg.result == IDOK) {
            std::wstring data, metadata;
            StringCoder coder;
            bool parsed = coder.Decode(dlg.text, metadata, data);
            if (parsed) {
              parsed = track::feed_filter_manager.Import(data, parent->feed_filters_);
              if (parsed) {
                parent->RefreshTorrentFilterList(GetDlgItem(IDC_LIST_TORRENT_FILTER));
              }
            }
            if (!parsed) {
              LOGE(metadata);
              LOGE(data);
              ui::DisplayErrorMessage(
                  L"Could not parse the filter string. It may be missing characters, "
                  L"or encoded with an incompatible version of the application.",
                  L"Import Filters");
            }
          }
          return TRUE;
        }
        // Export filters
        case 107: {
          std::wstring data;
          track::feed_filter_manager.Export(data, parent->feed_filters_);
          std::wstring metadata = StrToWstr(taiga::version().to_string());
          InputDialog dlg;
          StringCoder coder;
          if (coder.Encode(metadata, data, dlg.text)) {
            dlg.title = L"Export Filters";
            dlg.info = L"Copy the text below and share it with other people:";
            dlg.Show(parent->GetWindowHandle());
          }
          return TRUE;
        }
        // Reset filters
        case 109: {
          win::TaskDialog dlg;
          dlg.SetWindowTitle(L"Reset Torrent Filters");
          dlg.SetMainIcon(TD_ICON_INFORMATION);
          dlg.SetMainInstruction(L"Are you sure you want to reset the filters?");
          dlg.SetContent(L"All custom filters will be lost.");
          dlg.AddButton(L"Yes", IDYES);
          dlg.AddButton(L"No", IDNO);
          dlg.Show(parent->GetWindowHandle());
          if (dlg.GetSelectedButtonID() == IDYES) {
            parent->feed_filters_.clear();
            track::feed_filter_manager.AddPresets(parent->feed_filters_);
            win::ListView list = GetDlgItem(IDC_LIST_TORRENT_FILTER);
            parent->RefreshTorrentFilterList(list.GetWindowHandle());
            list.SetWindowHandle(nullptr);
          }
          return TRUE;
        }

        ////////////////////////////////////////////////////////////////////////

        // Check radio buttons
        case IDC_RADIO_MIRC_CHANNEL1:
        case IDC_RADIO_MIRC_CHANNEL2:
        case IDC_RADIO_MIRC_CHANNEL3:
          CheckRadioButton(IDC_RADIO_MIRC_CHANNEL1, IDC_RADIO_MIRC_CHANNEL3, LOWORD(wParam));
          EnableDlgItem(IDC_EDIT_MIRC_CHANNELS, LOWORD(wParam) == IDC_RADIO_MIRC_CHANNEL3);
          return TRUE;
        case IDC_RADIO_TORRENT_NEW1:
        case IDC_RADIO_TORRENT_NEW2:
          CheckRadioButton(IDC_RADIO_TORRENT_NEW1, IDC_RADIO_TORRENT_NEW2, LOWORD(wParam));
          return TRUE;
        case IDC_RADIO_TORRENT_APP1:
        case IDC_RADIO_TORRENT_APP2:
          CheckRadioButton(IDC_RADIO_TORRENT_APP1, IDC_RADIO_TORRENT_APP2, LOWORD(wParam));
          EnableDlgItem(IDC_EDIT_TORRENT_APP, LOWORD(wParam) == IDC_RADIO_TORRENT_APP2);
          EnableDlgItem(IDC_BUTTON_TORRENT_BROWSE_APP, LOWORD(wParam) == IDC_RADIO_TORRENT_APP2);
          return TRUE;
      }
      break;
    }
  }

  return FALSE;
}

////////////////////////////////////////////////////////////////////////////////

void SettingsPage::OnDropFiles(HDROP hDropInfo) {
  if (index != kSettingsPageLibraryFolders)
    return;

  WCHAR szFileName[MAX_PATH + 1];
  UINT nFiles = DragQueryFile(hDropInfo, static_cast<UINT>(-1), nullptr, 0);
  win::ListView list = GetDlgItem(IDC_LIST_FOLDERS_ROOT);
  for (UINT i = 0; i < nFiles; i++) {
    ZeroMemory(szFileName, MAX_PATH + 1);
    DragQueryFile(hDropInfo, i, (LPWSTR)szFileName, MAX_PATH + 1);
    if (GetFileAttributes(szFileName) & FILE_ATTRIBUTE_DIRECTORY) {
      list.InsertItem(list.GetItemCount(), -1, ui::kIcon16_Folder, 0, nullptr, szFileName, 0);
      list.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
    }
  }
  list.SetWindowHandle(nullptr);
}

LRESULT SettingsPage::OnNotify(int idCtrl, LPNMHDR pnmh) {
  switch (pnmh->code) {
    // Header checkbox click
    case HDN_ITEMCHANGED: {
      win::ListView list = ::GetParent(pnmh->hwndFrom);
      auto nmh = reinterpret_cast<LPNMHEADER>(pnmh);
      if (nmh->pitem->mask & HDI_FORMAT) {
        BOOL checked = (nmh->pitem->fmt & HDF_CHECKED) ? TRUE : FALSE;
        for (int i = 0; i < list.GetItemCount(); i++) {
          list.SetCheckState(i, checked);
        }
      }
      list.SetWindowHandle(nullptr);
      break;
    }

    case NM_CLICK:
    case NM_RETURN: {
      switch (pnmh->idFrom) {
        // Execute link
        case IDC_LINK_ACCOUNT_ANILIST:
        case IDC_LINK_ACCOUNT_KITSU:
        case IDC_LINK_ACCOUNT_MAL: {
          PNMLINK pNMLink = reinterpret_cast<PNMLINK>(pnmh);
          ExecuteCommand(pNMLink->item.szUrl);
          return TRUE;
        }
      }
    }

    // List item select
    case LVN_ITEMCHANGED: {
      LPNMLISTVIEW lplv = reinterpret_cast<LPNMLISTVIEW>(pnmh);
      if (lplv->hdr.hwndFrom == GetDlgItem(IDC_LIST_FOLDERS_ROOT)) {
        EnableDlgItem(IDC_BUTTON_REMOVEFOLDER, ListView_GetSelectedCount(lplv->hdr.hwndFrom) > 0);
      } else if (lplv->hdr.hwndFrom == GetDlgItem(IDC_LIST_TORRENT_FILTER)) {
        win::ListView list = GetDlgItem(IDC_LIST_TORRENT_FILTER);
        win::Toolbar toolbar = GetDlgItem(IDC_TOOLBAR_FEED_FILTER);
        LPNMLISTVIEW pnmv = reinterpret_cast<LPNMLISTVIEW>(pnmh);
        if (pnmv->uOldState != 0 && (pnmv->uNewState == 0x1000 || pnmv->uNewState == 0x2000)) {
          auto feed_filter = reinterpret_cast<track::FeedFilter*>(list.GetItemParam(pnmv->iItem));
          feed_filter->enabled = list.GetCheckState(pnmv->iItem) == TRUE;
        }
        int index = list.GetNextItem(-1, LVNI_SELECTED);
        int count = list.GetItemCount();
        toolbar.EnableButton(101, index > -1);
        toolbar.EnableButton(103, index > 0);
        toolbar.EnableButton(104, index > -1 && index < count - 1);
        list.SetWindowHandle(nullptr);
        toolbar.SetWindowHandle(nullptr);
      }
      break;
    }

    // List key press
    case LVN_KEYDOWN: {
      auto pnkd = reinterpret_cast<LPNMLVKEYDOWN>(pnmh);
      switch (pnkd->hdr.idFrom) {
        case IDC_LIST_FOLDERS_ROOT:
          switch (pnkd->wVKey) {
            case VK_DELETE: {
              win::ListView list = GetDlgItem(IDC_LIST_FOLDERS_ROOT);
              while (list.GetSelectedCount() > 0)
                list.DeleteItem(list.GetNextItem(-1, LVNI_SELECTED));
              EnableDlgItem(IDC_BUTTON_REMOVEFOLDER, FALSE);
              list.SetWindowHandle(nullptr);
              return TRUE;
            }
          }
          break;
        case IDC_LIST_TORRENT_FILTER:
          switch (pnkd->wVKey) {
            case VK_DELETE: {
              win::ListView list = GetDlgItem(IDC_LIST_TORRENT_FILTER);
              int item_index = list.GetNextItem(-1, LVNI_SELECTED);
              if (item_index > -1) {
                auto feed_filter = reinterpret_cast<track::FeedFilter*>(list.GetItemParam(item_index));
                for (auto it = parent->feed_filters_.begin(); it != parent->feed_filters_.end(); ++it) {
                  if (feed_filter == &(*it)) {
                    parent->feed_filters_.erase(it);
                    break;
                  }
                }
                list.DeleteItem(item_index);
                parent->UpdateTorrentFilterList(list.GetWindowHandle());
              }
              list.SetWindowHandle(nullptr);
              return TRUE;
            }
          }
          break;
      }
      break;
    }

    // Text callback
    case TBN_GETINFOTIP: {
      if (pnmh->hwndFrom == GetDlgItem(IDC_TOOLBAR_FEED_FILTER)) {
        // These are normally stored in win::Toolbar, but we don't actually have
        // a persistent object in this case.
        static const std::vector<std::wstring> tooltips{
          L"Add new filter...",
          L"Delete filter",
          L"",
          L"Move up",
          L"Move down",
          L"",
          L"Import filters...",
          L"Export filters...",
          L"",
          L"Reset filters",
        };
        const auto git = reinterpret_cast<NMTBGETINFOTIP*>(pnmh);
        const auto index = static_cast<size_t>(git->lParam);
        if (index < tooltips.size()) {
          git->cchTextMax = INFOTIPSIZE;
          git->pszText = const_cast<LPWSTR>(tooltips.at(index).data());
        }
      }
      break;
    }

    // Double click
    case NM_DBLCLK: {
      LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
      if (lpnmitem->iItem == -1)
        break;
      // Anime folders
      if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_FOLDERS_ROOT)) {
        win::ListView list = lpnmitem->hdr.hwndFrom;
        WCHAR buffer[MAX_PATH];
        list.GetItemText(lpnmitem->iItem, 0, buffer);
        Execute(buffer);
        list.SetWindowHandle(nullptr);
      // Streaming media providers
      } else if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_STREAM_PROVIDER)) {
        const auto& stream_data = track::recognition::GetStreamData();
        if (lpnmitem->iItem > -1 && lpnmitem->iItem < static_cast<int>(stream_data.size()))
          ExecuteLink(stream_data.at(lpnmitem->iItem).url);
      // Torrent filters
      } else if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_TORRENT_FILTER)) {
        win::ListView list = lpnmitem->hdr.hwndFrom;
        const auto feed_filter = reinterpret_cast<track::FeedFilter*>(list.GetItemParam(lpnmitem->iItem));
        if (feed_filter) {
          DlgFeedFilter.filter = *feed_filter;
          DlgFeedFilter.Create(IDD_FEED_FILTER, parent->GetWindowHandle());
          if (!DlgFeedFilter.filter.conditions.empty()) {
            *feed_filter = DlgFeedFilter.filter;
            parent->RefreshTorrentFilterList(lpnmitem->hdr.hwndFrom);
            list.SelectItem(lpnmitem->iItem);
          }
        }
        list.SetWindowHandle(nullptr);
      // Advanced settings
      } else if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_ADVANCED_SETTINGS)) {
        win::ListView list = lpnmitem->hdr.hwndFrom;
        const auto key = static_cast<AdvancedSetting>(list.GetItemParam(lpnmitem->iItem));
        auto& value = parent->advanced_settings_[key];
        if (key == taiga::AppSettingKey::SyncNotifyFormat) {
          DlgFormat.mode = FormatDialogMode::Balloon;
          if (DlgFormat.Create(IDD_FORMAT, parent->GetWindowHandle(), true) == IDOK) {
            value = taiga::settings.GetSyncNotifyFormat();
            list.SetItem(lpnmitem->iItem, 1, value.c_str());
          }
        } else {
          const bool is_password = key == taiga::AppSettingKey::AppConnectionProxyPassword;
          if (OnSettingsEditAdvanced(GetAdvancedSettingDescription(key), is_password, value)) {
            const auto text = is_password ? std::wstring(value.size(), L'\u25cf') : value;
            list.SetItem(lpnmitem->iItem, 1, text.c_str());
          }
        }
        list.SetWindowHandle(nullptr);
      }
      return TRUE;
    }
  }

  return 0;
}

}  // namespace ui
