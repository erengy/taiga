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
#include "base/string.h"
#include "library/anime_db.h"
#include "library/history.h"
#include "library/resource.h"
#include "sync/manager.h"
#include "taiga/announce.h"
#include "taiga/path.h"
#include "taiga/resource.h"
#include "taiga/script.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "track/media.h"
#include "ui/dlg/dlg_feed_filter.h"
#include "ui/dlg/dlg_format.h"
#include "ui/dlg/dlg_settings.h"
#include "ui/dlg/dlg_settings_page.h"
#include "ui/menu.h"
#include "ui/theme.h"

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
    SETRESOURCEID(kSettingsPageAppBehavior, IDD_SETTINGS_APP_BEHAVIOR);
    SETRESOURCEID(kSettingsPageAppConnection, IDD_SETTINGS_APP_CONNECTION);
    SETRESOURCEID(kSettingsPageAppInterface, IDD_SETTINGS_APP_INTERFACE);
    SETRESOURCEID(kSettingsPageAppList, IDD_SETTINGS_APP_LIST);
    SETRESOURCEID(kSettingsPageLibraryFolders, IDD_SETTINGS_LIBRARY_FOLDERS);
    SETRESOURCEID(kSettingsPageLibraryCache, IDD_SETTINGS_LIBRARY_CACHE);
    SETRESOURCEID(kSettingsPageRecognitionGeneral, IDD_SETTINGS_RECOGNITION_GENERAL);
    SETRESOURCEID(kSettingsPageRecognitionMedia, IDD_SETTINGS_RECOGNITION_MEDIA);
    SETRESOURCEID(kSettingsPageRecognitionStream, IDD_SETTINGS_RECOGNITION_STREAM);
    SETRESOURCEID(kSettingsPageServicesMain, IDD_SETTINGS_SERVICES_MAIN);
    SETRESOURCEID(kSettingsPageServicesMal, IDD_SETTINGS_SERVICES_MAL);
    SETRESOURCEID(kSettingsPageServicesHummingbird, IDD_SETTINGS_SERVICES_HUMMINGBIRD);
    SETRESOURCEID(kSettingsPageSharingHttp, IDD_SETTINGS_SHARING_HTTP);
    SETRESOURCEID(kSettingsPageSharingMessenger, IDD_SETTINGS_SHARING_MESSENGER);
    SETRESOURCEID(kSettingsPageSharingMirc, IDD_SETTINGS_SHARING_MIRC);
    SETRESOURCEID(kSettingsPageSharingSkype, IDD_SETTINGS_SHARING_SKYPE);
    SETRESOURCEID(kSettingsPageSharingTwitter, IDD_SETTINGS_SHARING_TWITTER);
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
      for (int i = sync::kFirstService; i <= sync::kLastService; i++) {
        auto service = ServiceManager.service(static_cast<sync::ServiceId>(i));
        combo.AddItem(service->name().c_str(), service->id());
        auto active_service = ServiceManager.service(Settings[taiga::kSync_ActiveService]);
        if (service->id() == active_service->id())
          combo.SetCurSel(combo.GetCount() - 1);
      }
      combo.SetWindowHandle(nullptr);
      CheckDlgButton(IDC_CHECK_START_LOGIN, Settings.GetBool(taiga::kSync_AutoOnStart));
      break;
    }
    // Services > MyAnimeList
    case kSettingsPageServicesMal: {
      SetDlgItemText(IDC_EDIT_USER_MAL, Settings[taiga::kSync_Service_Mal_Username].c_str());
      SetDlgItemText(IDC_EDIT_PASS_MAL, SimpleDecrypt(Settings[taiga::kSync_Service_Mal_Password]).c_str());
      break;
    }
    // Services > Hummingbird
    case kSettingsPageServicesHummingbird: {
      SetDlgItemText(IDC_EDIT_USER_HUMMINGBIRD, Settings[taiga::kSync_Service_Hummingbird_Username].c_str());
      SetDlgItemText(IDC_EDIT_PASS_HUMMINGBIRD, SimpleDecrypt(Settings[taiga::kSync_Service_Hummingbird_Password]).c_str());
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
      for (size_t i = 0; i < Settings.root_folders.size(); i++)
        list.InsertItem(i, -1, ui::kIcon16_Folder, 0, nullptr, Settings.root_folders[i].c_str(), 0);
      list.SetWindowHandle(nullptr);
      CheckDlgButton(IDC_CHECK_FOLDERS_WATCH, Settings.GetBool(taiga::kLibrary_WatchFolders));
      break;
    }
    // Library > Cache
    case kSettingsPageLibraryCache: {
      parent->RefreshCache();
      break;
    }

    ////////////////////////////////////////////////////////////////////////////

    // Application > Behavior
    case kSettingsPageAppBehavior: {
      CheckDlgButton(IDC_CHECK_AUTOSTART, Settings.GetBool(taiga::kApp_Behavior_Autostart));
      CheckDlgButton(IDC_CHECK_GENERAL_CLOSE, Settings.GetBool(taiga::kApp_Behavior_CloseToTray));
      CheckDlgButton(IDC_CHECK_GENERAL_MINIMIZE, Settings.GetBool(taiga::kApp_Behavior_MinimizeToTray));
      CheckDlgButton(IDC_CHECK_START_VERSION, Settings.GetBool(taiga::kApp_Behavior_CheckForUpdates));
      CheckDlgButton(IDC_CHECK_START_CHECKEPS, Settings.GetBool(taiga::kApp_Behavior_ScanAvailableEpisodes));
      CheckDlgButton(IDC_CHECK_START_MINIMIZE, Settings.GetBool(taiga::kApp_Behavior_StartMinimized));
      break;
    }
    // Application > Connection
    case kSettingsPageAppConnection: {
      SetDlgItemText(IDC_EDIT_PROXY_HOST, Settings[taiga::kApp_Connection_ProxyHost].c_str());
      SetDlgItemText(IDC_EDIT_PROXY_USER, Settings[taiga::kApp_Connection_ProxyUsername].c_str());
      SetDlgItemText(IDC_EDIT_PROXY_PASS, Settings[taiga::kApp_Connection_ProxyPassword].c_str());
      break;
    }
    // Application > Interface
    case kSettingsPageAppInterface: {
      std::vector<std::wstring> theme_list;
      PopulateFolders(theme_list, taiga::GetPath(taiga::kPathTheme));
      if (theme_list.empty()) {
        EnableDlgItem(IDC_COMBO_THEME, FALSE);
      } else {
        for (size_t i = 0; i < theme_list.size(); i++) {
          AddComboString(IDC_COMBO_THEME, theme_list[i].c_str());
          if (IsEqual(theme_list[i], Settings[taiga::kApp_Interface_Theme]))
            SetComboSelection(IDC_COMBO_THEME, i);
        }
      }
      SetDlgItemText(IDC_EDIT_EXTERNALLINKS, Settings[taiga::kApp_Interface_ExternalLinks].c_str());
      break;
    }
    // Application > List
    case kSettingsPageAppList: {
      AddComboString(IDC_COMBO_DBLCLICK, L"Do nothing");
      AddComboString(IDC_COMBO_DBLCLICK, L"Edit details");
      AddComboString(IDC_COMBO_DBLCLICK, L"Open folder");
      AddComboString(IDC_COMBO_DBLCLICK, L"Play next episode");
      AddComboString(IDC_COMBO_DBLCLICK, L"View anime info");
      SetComboSelection(IDC_COMBO_DBLCLICK, Settings.GetInt(taiga::kApp_List_DoubleClickAction));
      AddComboString(IDC_COMBO_MDLCLICK, L"Do nothing");
      AddComboString(IDC_COMBO_MDLCLICK, L"Edit details");
      AddComboString(IDC_COMBO_MDLCLICK, L"Open folder");
      AddComboString(IDC_COMBO_MDLCLICK, L"Play next episode");
      AddComboString(IDC_COMBO_MDLCLICK, L"View anime info");
      SetComboSelection(IDC_COMBO_MDLCLICK, Settings.GetInt(taiga::kApp_List_MiddleClickAction));
      CheckDlgButton(IDC_CHECK_LIST_ENGLISH, Settings.GetBool(taiga::kApp_List_DisplayEnglishTitles));
      CheckDlgButton(IDC_CHECK_HIGHLIGHT, Settings.GetBool(taiga::kApp_List_HighlightNewEpisodes));
      CheckDlgButton(IDC_CHECK_LIST_PROGRESS_AIRED, Settings.GetBool(taiga::kApp_List_ProgressDisplayAired));
      CheckDlgButton(IDC_CHECK_LIST_PROGRESS_AVAILABLE, Settings.GetBool(taiga::kApp_List_ProgressDisplayAvailable));
      break;
    }

    ////////////////////////////////////////////////////////////////////////////

    // Recognition > General
    case kSettingsPageRecognitionGeneral: {
      CheckDlgButton(IDC_CHECK_UPDATE_CONFIRM, Settings.GetBool(taiga::kSync_Update_AskToConfirm));
      CheckDlgButton(IDC_CHECK_UPDATE_CHECKMP, Settings.GetBool(taiga::kSync_Update_CheckPlayer));
      CheckDlgButton(IDC_CHECK_UPDATE_GOTO, Settings.GetBool(taiga::kSync_Update_GoToNowPlaying));
      CheckDlgButton(IDC_CHECK_UPDATE_RANGE, Settings.GetBool(taiga::kSync_Update_OutOfRange));
      CheckDlgButton(IDC_CHECK_UPDATE_ROOT, Settings.GetBool(taiga::kSync_Update_OutOfRoot));
      CheckDlgButton(IDC_CHECK_UPDATE_WAITMP, Settings.GetBool(taiga::kSync_Update_WaitPlayer));
      SendDlgItemMessage(IDC_SPIN_DELAY, UDM_SETRANGE32, 0, 3600);
      SendDlgItemMessage(IDC_SPIN_DELAY, UDM_SETPOS32, 0, Settings.GetInt(taiga::kSync_Update_Delay));
      CheckDlgButton(IDC_CHECK_NOTIFY_RECOGNIZED, Settings.GetBool(taiga::kSync_Notify_Recognized));
      CheckDlgButton(IDC_CHECK_NOTIFY_NOTRECOGNIZED, Settings.GetBool(taiga::kSync_Notify_NotRecognized));
      break;
    }
    // Recognition > Media players
    case kSettingsPageRecognitionMedia: {
      win::ListView list = GetDlgItem(IDC_LIST_MEDIA);
      list.EnableGroupView(true);
      if (win::GetVersion() >= win::kVersionVista) {
        list.InsertColumn(0, 0, 0, 0, L"Select/deselect all");
      } else {
        list.InsertColumn(0, 0, 0, 0, L"Supported players");
      }
      list.InsertGroup(0, L"Media players");
      list.InsertGroup(1, L"Web browsers");
      list.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);
      list.SetImageList(ui::Theme.GetImageList16().GetHandle());
      list.SetTheme();
      if (win::GetVersion() >= win::kVersionVista) {
        win::Window header = list.GetHeader();
        HDITEM hdi = {0};
        header.SetStyle(HDS_CHECKBOXES, 0);
        hdi.mask = HDI_FORMAT;
        Header_GetItem(header.GetWindowHandle(), 0, &hdi);
        hdi.fmt |= HDF_CHECKBOX;
        Header_SetItem(header.GetWindowHandle(), 0, &hdi);
        header.SetWindowHandle(nullptr);
      }
      for (size_t i = 0; i < MediaPlayers.items.size(); i++) {
        BOOL player_available = MediaPlayers.items[i].GetPath().empty() ? FALSE : TRUE;
        list.InsertItem(i, MediaPlayers.items[i].mode == 5 ? 1 : 0,
                        ui::kIcon16_AppGray - player_available, 0, nullptr,
                        MediaPlayers.items[i].name.c_str(), 0);
        if (MediaPlayers.items[i].enabled)
          list.SetCheckState(i, TRUE);
      }
      list.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
      list.SetWindowHandle(nullptr);
      break;
    }
    // Recognition > Media providers
    case kSettingsPageRecognitionStream: {
      win::ListView list = GetDlgItem(IDC_LIST_STREAM_PROVIDER);
      list.EnableGroupView(true);
      list.InsertColumn(0, 0, 0, 0, L"Media providers");
      list.InsertGroup(0, L"Media providers");
      list.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);
      list.SetImageList(ui::Theme.GetImageList16().GetHandle());
      list.SetTheme();
      list.InsertItem(0, 0, ui::kIcon16_AppBlue, 0, nullptr, L"Anime News Network", 0);
      list.InsertItem(1, 0, ui::kIcon16_AppBlue, 0, nullptr, L"Crunchyroll", 1);
      list.InsertItem(2, 0, ui::kIcon16_AppBlue, 0, nullptr, L"Veoh", 3);
      list.InsertItem(3, 0, ui::kIcon16_AppBlue, 0, nullptr, L"Viz Anime", 4);
      list.InsertItem(4, 0, ui::kIcon16_AppBlue, 0, nullptr, L"YouTube", 5);
      if (Settings.GetBool(taiga::kStream_Ann))
        list.SetCheckState(0, TRUE);
      if (Settings.GetBool(taiga::kStream_Crunchyroll))
        list.SetCheckState(1, TRUE);
      if (Settings.GetBool(taiga::kStream_Veoh))
        list.SetCheckState(2, TRUE);
      if (Settings.GetBool(taiga::kStream_Viz))
        list.SetCheckState(3, TRUE);
      if (Settings.GetBool(taiga::kStream_Youtube))
        list.SetCheckState(4, TRUE);
      list.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
      list.SetWindowHandle(nullptr);
      break;
    }

    ////////////////////////////////////////////////////////////////////////////

    // Sharing > HTTP
    case kSettingsPageSharingHttp: {
      CheckDlgButton(IDC_CHECK_HTTP, Settings.GetBool(taiga::kShare_Http_Enabled));
      SetDlgItemText(IDC_EDIT_HTTP_URL, Settings[taiga::kShare_Http_Url].c_str());
      break;
    }
    // Sharing > Messenger
    case kSettingsPageSharingMessenger: {
      CheckDlgButton(IDC_CHECK_MESSENGER, Settings.GetBool(taiga::kShare_Messenger_Enabled));
      break;
    }
    // Sharing > mIRC
    case kSettingsPageSharingMirc: {
      CheckDlgButton(IDC_CHECK_MIRC, Settings.GetBool(taiga::kShare_Mirc_Enabled));
      CheckDlgButton(IDC_CHECK_MIRC_MULTISERVER, Settings.GetBool(taiga::kShare_Mirc_MultiServer));
      CheckDlgButton(IDC_CHECK_MIRC_ACTION, Settings.GetBool(taiga::kShare_Mirc_UseMeAction));
      SetDlgItemText(IDC_EDIT_MIRC_SERVICE, Settings[taiga::kShare_Mirc_Service].c_str());
      CheckRadioButton(IDC_RADIO_MIRC_CHANNEL1, IDC_RADIO_MIRC_CHANNEL3,
                       IDC_RADIO_MIRC_CHANNEL1 + Settings.GetInt(taiga::kShare_Mirc_Mode) - 1);
      SetDlgItemText(IDC_EDIT_MIRC_CHANNELS, Settings[taiga::kShare_Mirc_Channels].c_str());
      EnableDlgItem(IDC_EDIT_MIRC_CHANNELS, Settings.GetInt(taiga::kShare_Mirc_Mode) == 3);
      break;
    }
    // Sharing > Skype
    case kSettingsPageSharingSkype: {
      CheckDlgButton(IDC_CHECK_SKYPE, Settings.GetBool(taiga::kShare_Skype_Enabled));
      break;
    }
    // Sharing > Twitter
    case kSettingsPageSharingTwitter: {
      CheckDlgButton(IDC_CHECK_TWITTER, Settings.GetBool(taiga::kShare_Twitter_Enabled));
      parent->RefreshTwitterLink();
      break;
    }

    ////////////////////////////////////////////////////////////////////////////

    // Torrents > Discovery
    case kSettingsPageTorrentsDiscovery: {
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"http://tokyotosho.info/rss.php?filter=1,11&zwnj=0");
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"http://www.animesuki.com/rss.php?link=enclosure");
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"http://www.baka-updates.com/rss.php");
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"http://www.nyaa.se/?page=rss&cats=1_37&filter=2");
      SetDlgItemText(IDC_COMBO_TORRENT_SOURCE, Settings[taiga::kTorrent_Discovery_Source].c_str());
      AddComboString(IDC_COMBO_TORRENT_SEARCH, L"http://www.nyaa.se/?page=rss&cats=1_37&filter=2&term=%title%");
      AddComboString(IDC_COMBO_TORRENT_SEARCH, L"http://pipes.yahoo.com/pipes/pipe.run?SearchQuery=%title%&_id=7b99f981c5b1f02354642f4e271cca43&_render=rss");
      SetDlgItemText(IDC_COMBO_TORRENT_SEARCH, Settings[taiga::kTorrent_Discovery_SearchUrl].c_str());
      CheckDlgButton(IDC_CHECK_TORRENT_AUTOCHECK, Settings.GetBool(taiga::kTorrent_Discovery_AutoCheckEnabled));
      SendDlgItemMessage(IDC_SPIN_TORRENT_INTERVAL, UDM_SETRANGE32, 0, 3600);
      SendDlgItemMessage(IDC_SPIN_TORRENT_INTERVAL, UDM_SETPOS32, 0, Settings.GetInt(taiga::kTorrent_Discovery_AutoCheckInterval));
      EnableDlgItem(IDC_EDIT_TORRENT_INTERVAL, Settings.GetBool(taiga::kTorrent_Discovery_AutoCheckEnabled));
      EnableDlgItem(IDC_SPIN_TORRENT_INTERVAL, Settings.GetBool(taiga::kTorrent_Discovery_AutoCheckEnabled));
      CheckRadioButton(IDC_RADIO_TORRENT_NEW1, IDC_RADIO_TORRENT_NEW2,
                       IDC_RADIO_TORRENT_NEW1 + Settings.GetInt(taiga::kTorrent_Discovery_NewAction) - 1);
      break;
    }
    // Torrents > Downloads
    case kSettingsPageTorrentsDownloads: {
      CheckRadioButton(IDC_RADIO_TORRENT_APP1, IDC_RADIO_TORRENT_APP2,
                       IDC_RADIO_TORRENT_APP1 + Settings.GetInt(taiga::kTorrent_Download_AppMode) - 1);
      SetDlgItemText(IDC_EDIT_TORRENT_APP, Settings[taiga::kTorrent_Download_AppPath].c_str());
      EnableDlgItem(IDC_EDIT_TORRENT_APP, Settings.GetInt(taiga::kTorrent_Download_AppMode) > 1);
      EnableDlgItem(IDC_BUTTON_TORRENT_BROWSE_APP, Settings.GetInt(taiga::kTorrent_Download_AppMode) > 1);
      CheckDlgButton(IDC_CHECK_TORRENT_AUTOSETFOLDER, Settings.GetBool(taiga::kTorrent_Download_UseAnimeFolder));
      CheckDlgButton(IDC_CHECK_TORRENT_AUTOUSEFOLDER, Settings.GetBool(taiga::kTorrent_Download_FallbackOnFolder));
      CheckDlgButton(IDC_CHECK_TORRENT_AUTOCREATEFOLDER, Settings.GetBool(taiga::kTorrent_Download_CreateSubfolder));
      for (size_t i = 0; i < Settings.root_folders.size(); i++)
        AddComboString(IDC_COMBO_TORRENT_FOLDER, Settings.root_folders[i].c_str());
      SetDlgItemText(IDC_COMBO_TORRENT_FOLDER, Settings[taiga::kTorrent_Download_Location].c_str());
      EnableDlgItem(IDC_CHECK_TORRENT_AUTOUSEFOLDER, Settings.GetBool(taiga::kTorrent_Download_UseAnimeFolder));
      bool enabled = Settings.GetBool(taiga::kTorrent_Download_UseAnimeFolder) &&
                     Settings.GetBool(taiga::kTorrent_Download_FallbackOnFolder);
      EnableDlgItem(IDC_COMBO_TORRENT_FOLDER, enabled);
      EnableDlgItem(IDC_BUTTON_TORRENT_BROWSE_FOLDER, enabled);
      EnableDlgItem(IDC_CHECK_TORRENT_AUTOCREATEFOLDER, enabled);
      break;
    }
    // Torrents > Filters
    case kSettingsPageTorrentsFilters: {
      CheckDlgButton(IDC_CHECK_TORRENT_FILTER, Settings.GetBool(taiga::kTorrent_Filter_Enabled));
      win::ListView list = GetDlgItem(IDC_LIST_TORRENT_FILTER);
      list.EnableGroupView(true);
      list.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
      list.SetImageList(ui::Theme.GetImageList16().GetHandle());
      list.SetTheme();
      list.InsertColumn(0, 0, 0, 0, L"Name");
      list.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
      list.InsertGroup(0, L"General filters", true, false);
      list.InsertGroup(1, L"Limited filters", true, false);
      parent->feed_filters_.resize(Aggregator.filter_manager.filters.size());
      std::copy(Aggregator.filter_manager.filters.begin(),
                Aggregator.filter_manager.filters.end(),
                parent->feed_filters_.begin());
      parent->RefreshTorrentFilterList(list.GetWindowHandle());
      list.SetWindowHandle(nullptr);
      // Initialize toolbar
      win::Toolbar toolbar = GetDlgItem(IDC_TOOLBAR_FEED_FILTER);
      toolbar.SetImageList(ui::Theme.GetImageList16().GetHandle(), 16, 16);
      toolbar.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
      // Add toolbar items
      BYTE fsState1 = TBSTATE_ENABLED;
      BYTE fsState2 = TBSTATE_INDETERMINATE;
      toolbar.InsertButton(0, ui::kIcon16_Plus,      100, fsState1, 0, 0, nullptr, L"Add new filter...");
      toolbar.InsertButton(1, ui::kIcon16_Minus,     101, fsState2, 0, 1, nullptr, L"Delete filter");
      toolbar.InsertButton(2, 0, 0, 0, BTNS_SEP, 0, nullptr, nullptr);
      toolbar.InsertButton(3, ui::kIcon16_ArrowUp,   103, fsState2, 0, 3, nullptr, L"Move up");
      toolbar.InsertButton(4, ui::kIcon16_ArrowDown, 104, fsState2, 0, 4, nullptr, L"Move down");
      toolbar.SetWindowHandle(nullptr);
      break;
    }
  }

  return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

BOOL SettingsPage::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (HIWORD(wParam)) {
    case BN_CLICKED: {
      switch (LOWORD(wParam)) {
        // Edit format
        case IDC_BUTTON_FORMAT_HTTP:
          DlgFormat.mode = kFormatModeHttp;
          DlgFormat.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;
        case IDC_BUTTON_FORMAT_MSN:
          DlgFormat.mode = kFormatModeMessenger;
          DlgFormat.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;
        case IDC_BUTTON_FORMAT_MIRC:
          DlgFormat.mode = kFormatModeMirc;
          DlgFormat.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;
        case IDC_BUTTON_FORMAT_SKYPE:
          DlgFormat.mode = kFormatModeSkype;
          DlgFormat.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;
        case IDC_BUTTON_FORMAT_TWITTER:
          DlgFormat.mode = kFormatModeTwitter;
          DlgFormat.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;
        case IDC_BUTTON_FORMAT_BALLOON:
          DlgFormat.mode = kFormatModeBalloon;
          DlgFormat.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;

        ////////////////////////////////////////////////////////////////////////

        // Add folders
        case IDC_BUTTON_ADDFOLDER: {
          std::wstring path;
          if (BrowseForFolder(GetWindowHandle(), L"Please select a folder:", L"", path)) {
            win::ListView list = GetDlgItem(IDC_LIST_FOLDERS_ROOT);
            list.InsertItem(list.GetItemCount(), -1, ui::kIcon16_Folder, 0, nullptr, path.c_str(), 0);
            list.SetSelectedItem(list.GetItemCount() - 1);
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
            History.Clear();
          }
          if (IsDlgButtonChecked(IDC_CHECK_CACHE2)) {
            ImageDatabase.Clear();
          }
          if (IsDlgButtonChecked(IDC_CHECK_CACHE3)) {
            std::wstring path = taiga::GetPath(taiga::kPathFeed);
            DeleteFolder(path);
          }
          parent->RefreshCache();
          CheckDlgButton(IDC_CHECK_CACHE1, FALSE);
          CheckDlgButton(IDC_CHECK_CACHE2, FALSE);
          CheckDlgButton(IDC_CHECK_CACHE3, FALSE);
          EnableDlgItem(IDC_BUTTON_CACHE_CLEAR, FALSE);
          return TRUE;
        }

        ////////////////////////////////////////////////////////////////////////

        // Test DDE connection
        case IDC_BUTTON_MIRC_TEST: {
          std::wstring service;
          GetDlgItemText(IDC_EDIT_MIRC_SERVICE, service);
          Announcer.TestMircConnection(service);
          return TRUE;
        }

        ////////////////////////////////////////////////////////////////////////

        // Authorize Twitter
        case IDC_BUTTON_TWITTER_AUTH: {
          Twitter.RequestToken();
          return TRUE;
        }

        ////////////////////////////////////////////////////////////////////////

        // Browse for torrent application
        case IDC_BUTTON_TORRENT_BROWSE_APP: {
          std::wstring current_directory = Taiga.GetCurrentDirectory();
          std::wstring path = BrowseForFile(
              GetWindowHandle(), L"Please select a torrent application",
              L"Executable files (*.exe)\0*.exe\0\0");
          if (current_directory != Taiga.GetCurrentDirectory())
            Taiga.SetCurrentDirectory(current_directory);
          if (!path.empty())
            SetDlgItemText(IDC_EDIT_TORRENT_APP, path.c_str());
          return TRUE;
        }
        // Browse for torrent download path
        case IDC_BUTTON_TORRENT_BROWSE_FOLDER: {
          std::wstring path;
          if (BrowseForFolder(GetWindowHandle(), L"Please select a folder:", L"", path))
            SetDlgItemText(IDC_COMBO_TORRENT_FOLDER, path.c_str());
          return TRUE;
        }
        // Enable/disable controls
        case IDC_CHECK_CACHE1:
        case IDC_CHECK_CACHE2:
        case IDC_CHECK_CACHE3: {
          bool enable = IsDlgButtonChecked(IDC_CHECK_CACHE1) ||
                        IsDlgButtonChecked(IDC_CHECK_CACHE2) ||
                        IsDlgButtonChecked(IDC_CHECK_CACHE3);
          EnableDlgItem(IDC_BUTTON_CACHE_CLEAR, enable);
          return TRUE;
        }
        case IDC_CHECK_TORRENT_AUTOCHECK: {
          BOOL enable = IsDlgButtonChecked(LOWORD(wParam));
          EnableDlgItem(IDC_EDIT_TORRENT_INTERVAL, enable);
          EnableDlgItem(IDC_SPIN_TORRENT_INTERVAL, enable);
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
          DlgFeedFilter.filter.Reset();
          ExecuteAction(L"TorrentAddFilter", TRUE, reinterpret_cast<LPARAM>(parent->GetWindowHandle()));
          if (!DlgFeedFilter.filter.conditions.empty()) {
            if (DlgFeedFilter.filter.name.empty())
              DlgFeedFilter.filter.name = Aggregator.filter_manager.CreateNameFromConditions(DlgFeedFilter.filter);
            parent->feed_filters_.push_back(DlgFeedFilter.filter);
            win::ListView list = GetDlgItem(IDC_LIST_TORRENT_FILTER);
            parent->RefreshTorrentFilterList(list.GetWindowHandle());
            list.SetSelectedItem(list.GetItemCount() - 1);
            list.SetWindowHandle(nullptr);
          }
          return TRUE;
        }
        // Remove global filter
        case 101: {
          win::ListView list = GetDlgItem(IDC_LIST_TORRENT_FILTER);
          int item_index = list.GetNextItem(-1, LVNI_SELECTED);
          FeedFilter* feed_filter = reinterpret_cast<FeedFilter*>(list.GetItemParam(item_index));
          for (auto it = parent->feed_filters_.begin(); it != parent->feed_filters_.end(); ++it) {
            if (feed_filter == &(*it)) {
              parent->feed_filters_.erase(it);
              parent->RefreshTorrentFilterList(list.GetWindowHandle());
              break;
            }
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
            list.SetSelectedItem(index - 1);
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
            list.SetSelectedItem(index + 1);
          }
          list.SetWindowHandle(nullptr);
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
      win::ListView list = GetDlgItem(IDC_LIST_MEDIA);
      if (pnmh->hwndFrom == list.GetHeader()) {
        auto nmh = reinterpret_cast<LPNMHEADER>(pnmh);
        if (nmh->pitem->mask & HDI_FORMAT) {
          BOOL checked = nmh->pitem->fmt & HDF_CHECKED ? TRUE : FALSE;
          for (size_t i = 0; i < MediaPlayers.items.size(); i++)
            list.SetCheckState(i, checked);
        }
      }
      list.SetWindowHandle(nullptr);
      break;
    }

    case NM_CLICK: {
      switch (pnmh->idFrom) {
        // Execute link
        case IDC_LINK_ACCOUNT_HUMMINGBIRD:
        case IDC_LINK_ACCOUNT_MAL:
        case IDC_LINK_TWITTER: {
          PNMLINK pNMLink = reinterpret_cast<PNMLINK>(pnmh);
          ExecuteAction(pNMLink->item.szUrl);
          return TRUE;
        }
        // Open themes folder
        case IDC_LINK_THEMES: {
          std::wstring theme_name;
          GetDlgItemText(IDC_COMBO_THEME, theme_name);
          std::wstring path = GetPathOnly(taiga::GetPath(taiga::kPathThemeCurrent));
          Execute(path);
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

    // Text callback
    case LVN_GETDISPINFO: {
      NMLVDISPINFO* plvdi = reinterpret_cast<NMLVDISPINFO*>(pnmh);
      auto anime_item = AnimeDatabase.FindItem(static_cast<int>(plvdi->item.lParam));
      if (!anime_item) break;
      switch (plvdi->item.iSubItem) {
        case 0: // Anime title
          plvdi->item.pszText = const_cast<LPWSTR>(anime_item->GetTitle().data());
          break;
      }
      break;
    }
    case TBN_GETINFOTIP: {
      if (pnmh->hwndFrom == GetDlgItem(IDC_TOOLBAR_FEED_FILTER)) {
        win::Toolbar toolbar = GetDlgItem(IDC_TOOLBAR_FEED_FILTER);
        NMTBGETINFOTIP* git = reinterpret_cast<NMTBGETINFOTIP*>(pnmh);
        git->cchTextMax = INFOTIPSIZE;
        git->pszText = (LPWSTR)(toolbar.GetButtonTooltip(git->lParam));
        toolbar.SetWindowHandle(nullptr);
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
      // Media players
      } else if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_MEDIA)) {
        Execute(MediaPlayers.items[lpnmitem->iItem].GetPath());
      // Streaming media providers
      } else if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_STREAM_PROVIDER)) {
        switch (lpnmitem->iItem) {
          case 0:
            ExecuteLink(L"http://www.animenewsnetwork.com/video/");
            break;
          case 1:
            ExecuteLink(L"http://www.crunchyroll.com");
            break;
          case 2:
            ExecuteLink(L"http://www.veoh.com");
            break;
          case 3:
            ExecuteLink(L"http://www.vizanime.com");
            break;
          case 4:
            ExecuteLink(L"http://www.youtube.com");
            break;
        }
      // Torrent filters
      } else if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_TORRENT_FILTER)) {
        win::ListView list = lpnmitem->hdr.hwndFrom;
        FeedFilter* feed_filter = reinterpret_cast<FeedFilter*>(list.GetItemParam(lpnmitem->iItem));
        if (feed_filter) {
          DlgFeedFilter.filter = *feed_filter;
          DlgFeedFilter.Create(IDD_FEED_FILTER, parent->GetWindowHandle());
          if (!DlgFeedFilter.filter.conditions.empty()) {
            *feed_filter = DlgFeedFilter.filter;
            parent->RefreshTorrentFilterList(lpnmitem->hdr.hwndFrom);
            list.SetSelectedItem(lpnmitem->iItem);
          }
        }
        list.SetWindowHandle(nullptr);
      }
      return TRUE;
    }

    // Right click
    case NM_RCLICK: {
      LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
      if (lpnmitem->iItem == -1)
        break;
      win::ListView list = lpnmitem->hdr.hwndFrom;
      // Media players
      if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_MEDIA)) {
        std::wstring answer = ui::Menus.Show(GetWindowHandle(), 0, 0, L"GenericList");
        for (int i = 0; i < list.GetItemCount(); i++) {
          if (answer == L"SelectAll()") {
            list.SetCheckState(i, TRUE);
          } else if (answer == L"DeselectAll()") {
            list.SetCheckState(i, FALSE);
          }
        }
      }
      list.SetWindowHandle(nullptr);
      return TRUE;
    }
  }

  return 0;
}

}  // namespace ui