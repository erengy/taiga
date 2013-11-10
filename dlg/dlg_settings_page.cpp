/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

#include "../std.h"

#include "dlg_format.h"
#include "dlg_history.h"
#include "dlg_settings.h"
#include "dlg_settings_page.h"
#include "dlg_feed_filter.h"

#include "../anime_db.h"
#include "../anime_filter.h"
#include "../announce.h"
#include "../common.h"
#include "../history.h"
#include "../http.h"
#include "../media.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../settings.h"
#include "../stats.h"
#include "../string.h"
#include "../taiga.h"
#include "../theme.h"

// =============================================================================

SettingsPage::SettingsPage()
    : parent(nullptr) {
}

void SettingsPage::Create() {
  win32::Rect rect_page;
  parent->tab_.GetWindowRect(parent->GetWindowHandle(), &rect_page);
  parent->tab_.AdjustRect(nullptr, FALSE, &rect_page);

  UINT resource_id = 0;
  switch (index) {
    #define SETRESOURCEID(page, id) case page: resource_id = id; break;
    SETRESOURCEID(PAGE_APP_BEHAVIOR, IDD_SETTINGS_APP_BEHAVIOR);
    SETRESOURCEID(PAGE_APP_CONNECTION, IDD_SETTINGS_APP_CONNECTION);
    SETRESOURCEID(PAGE_APP_INTERFACE, IDD_SETTINGS_APP_INTERFACE);
    SETRESOURCEID(PAGE_APP_LIST, IDD_SETTINGS_APP_LIST);
    SETRESOURCEID(PAGE_LIBRARY_FOLDERS, IDD_SETTINGS_LIBRARY_FOLDERS);
    SETRESOURCEID(PAGE_LIBRARY_CACHE, IDD_SETTINGS_LIBRARY_CACHE);
    SETRESOURCEID(PAGE_RECOGNITION_GENERAL, IDD_SETTINGS_RECOGNITION_GENERAL);
    SETRESOURCEID(PAGE_RECOGNITION_MEDIA, IDD_SETTINGS_RECOGNITION_MEDIA);
    SETRESOURCEID(PAGE_RECOGNITION_STREAM, IDD_SETTINGS_RECOGNITION_STREAM);
    SETRESOURCEID(PAGE_SERVICES_MAL, IDD_SETTINGS_SERVICES_MAL);
    SETRESOURCEID(PAGE_SHARING_HTTP, IDD_SETTINGS_SHARING_HTTP);
    SETRESOURCEID(PAGE_SHARING_MESSENGER, IDD_SETTINGS_SHARING_MESSENGER);
    SETRESOURCEID(PAGE_SHARING_MIRC, IDD_SETTINGS_SHARING_MIRC);
    SETRESOURCEID(PAGE_SHARING_SKYPE, IDD_SETTINGS_SHARING_SKYPE);
    SETRESOURCEID(PAGE_SHARING_TWITTER, IDD_SETTINGS_SHARING_TWITTER);
    SETRESOURCEID(PAGE_TORRENTS_DISCOVERY, IDD_SETTINGS_TORRENTS_DISCOVERY);
    SETRESOURCEID(PAGE_TORRENTS_DOWNLOADS, IDD_SETTINGS_TORRENTS_DOWNLOADS);
    SETRESOURCEID(PAGE_TORRENTS_FILTERS, IDD_SETTINGS_TORRENTS_FILTERS);
    #undef SETRESOURCEID
  }

  win32::Dialog::Create(resource_id, parent->GetWindowHandle(), false);
  SetPosition(nullptr, rect_page, 0);
  EnableThemeDialogTexture(GetWindowHandle(), ETDT_ENABLETAB);
}

// =============================================================================

BOOL SettingsPage::OnInitDialog() {
  switch (index) {
    // Services > MyAnimeList
    case PAGE_SERVICES_MAL: {
      SetDlgItemText(IDC_EDIT_USER, Settings.Account.MAL.user.c_str());
      SetDlgItemText(IDC_EDIT_PASS, Settings.Account.MAL.password.c_str());
      CheckDlgButton(IDC_CHECK_START_LOGIN, Settings.Account.MAL.auto_sync);
      break;
    }

    // =========================================================================

    // Library > Folders
    case PAGE_LIBRARY_FOLDERS: {
      win32::ListView list = GetDlgItem(IDC_LIST_FOLDERS_ROOT);
      list.InsertColumn(0, 0, 0, 0, L"Folder");
      list.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
      list.SetExtendedStyle(LVS_EX_DOUBLEBUFFER);
      list.SetImageList(UI.ImgList16.GetHandle());
      list.SetTheme();
      for (size_t i = 0; i < Settings.Folders.root.size(); i++)
        list.InsertItem(i, -1, ICON16_FOLDER, 0, nullptr, Settings.Folders.root[i].c_str(), 0);
      list.SetWindowHandle(nullptr);
      CheckDlgButton(IDC_CHECK_FOLDERS_WATCH, Settings.Folders.watch_enabled);
      break;
    }
    // Library > Cache
    case PAGE_LIBRARY_CACHE: {
      parent->RefreshCache();
      break;
    }

    // =========================================================================

    // Application > Behavior
    case PAGE_APP_BEHAVIOR: {
      CheckDlgButton(IDC_CHECK_AUTOSTART, Settings.Program.General.auto_start);
      CheckDlgButton(IDC_CHECK_GENERAL_CLOSE, Settings.Program.General.close);
      CheckDlgButton(IDC_CHECK_GENERAL_MINIMIZE, Settings.Program.General.minimize);
      CheckDlgButton(IDC_CHECK_START_VERSION, Settings.Program.StartUp.check_new_version);
      CheckDlgButton(IDC_CHECK_START_CHECKEPS, Settings.Program.StartUp.check_new_episodes);
      CheckDlgButton(IDC_CHECK_START_MINIMIZE, Settings.Program.StartUp.minimize);
      break;
    }
    // Application > Connection
    case PAGE_APP_CONNECTION: {
      SetDlgItemText(IDC_EDIT_PROXY_HOST, Settings.Program.Proxy.host.c_str());
      SetDlgItemText(IDC_EDIT_PROXY_USER, Settings.Program.Proxy.user.c_str());
      SetDlgItemText(IDC_EDIT_PROXY_PASS, Settings.Program.Proxy.password.c_str());
      break;
    }
    // Application > Interface
    case PAGE_APP_INTERFACE: {
      vector<wstring> theme_list;
      PopulateFolders(theme_list, Taiga.GetDataPath() + L"Theme\\");
      if (theme_list.empty()) {
        EnableDlgItem(IDC_COMBO_THEME, FALSE);
      } else {
        for (size_t i = 0; i < theme_list.size(); i++) {
          AddComboString(IDC_COMBO_THEME, theme_list[i].c_str());
          if (IsEqual(theme_list[i], Settings.Program.General.theme))
            SetComboSelection(IDC_COMBO_THEME, i);
        }
      }
      SetDlgItemText(IDC_EDIT_EXTERNALLINKS, Settings.Program.General.external_links.c_str());
      break;
    }
    // Application > List
    case PAGE_APP_LIST: {
      AddComboString(IDC_COMBO_DBLCLICK, L"Do nothing");
      AddComboString(IDC_COMBO_DBLCLICK, L"Edit details");
      AddComboString(IDC_COMBO_DBLCLICK, L"Open folder");
      AddComboString(IDC_COMBO_DBLCLICK, L"Play next episode");
      AddComboString(IDC_COMBO_DBLCLICK, L"View anime info");
      SetComboSelection(IDC_COMBO_DBLCLICK, Settings.Program.List.double_click);
      AddComboString(IDC_COMBO_MDLCLICK, L"Do nothing");
      AddComboString(IDC_COMBO_MDLCLICK, L"Edit details");
      AddComboString(IDC_COMBO_MDLCLICK, L"Open folder");
      AddComboString(IDC_COMBO_MDLCLICK, L"Play next episode");
      AddComboString(IDC_COMBO_MDLCLICK, L"View anime info");
      SetComboSelection(IDC_COMBO_MDLCLICK, Settings.Program.List.middle_click);
      CheckDlgButton(IDC_CHECK_LIST_ENGLISH, Settings.Program.List.english_titles);
      CheckDlgButton(IDC_CHECK_HIGHLIGHT, Settings.Program.List.highlight);
      CheckDlgButton(IDC_CHECK_LIST_PROGRESS_AIRED, Settings.Program.List.progress_show_aired);
      CheckDlgButton(IDC_CHECK_LIST_PROGRESS_AVAILABLE, Settings.Program.List.progress_show_available);
      break;
    }

    // =========================================================================

    // Recognition > General
    case PAGE_RECOGNITION_GENERAL: {
      CheckDlgButton(IDC_CHECK_UPDATE_CONFIRM, Settings.Account.Update.ask_to_confirm);
      CheckDlgButton(IDC_CHECK_UPDATE_CHECKMP, Settings.Account.Update.check_player);
      CheckDlgButton(IDC_CHECK_UPDATE_GOTO, Settings.Account.Update.go_to_nowplaying);
      CheckDlgButton(IDC_CHECK_UPDATE_RANGE, Settings.Account.Update.out_of_range);
      CheckDlgButton(IDC_CHECK_UPDATE_ROOT, Settings.Account.Update.out_of_root);
      CheckDlgButton(IDC_CHECK_UPDATE_WAITMP, Settings.Account.Update.wait_mp);
      SendDlgItemMessage(IDC_SPIN_DELAY, UDM_SETRANGE32, 0, 3600);
      SendDlgItemMessage(IDC_SPIN_DELAY, UDM_SETPOS32, 0, Settings.Account.Update.delay);
      CheckDlgButton(IDC_CHECK_NOTIFY_RECOGNIZED, Settings.Program.Notifications.recognized);
      CheckDlgButton(IDC_CHECK_NOTIFY_NOTRECOGNIZED, Settings.Program.Notifications.notrecognized);
      break;
    }
    // Recognition > Media players
    case PAGE_RECOGNITION_MEDIA: {
      win32::ListView list = GetDlgItem(IDC_LIST_MEDIA);
      list.EnableGroupView(true);
      if (win32::GetWinVersion() >= win32::VERSION_VISTA) {
        list.InsertColumn(0, 0, 0, 0, L"Select/deselect all");
      } else {
        list.InsertColumn(0, 0, 0, 0, L"Supported players");
      }
      list.InsertGroup(0, L"Media players");
      list.InsertGroup(1, L"Web browsers");
      list.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);
      list.SetImageList(UI.ImgList16.GetHandle());
      list.SetTheme();
      if (win32::GetWinVersion() >= win32::VERSION_VISTA) {
        win32::Window header = list.GetHeader();
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
                        ICON16_APP_GRAY - player_available, 0, nullptr, 
                        MediaPlayers.items[i].name.c_str(), 0);
        if (MediaPlayers.items[i].enabled)
          list.SetCheckState(i, TRUE);
      }
      list.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
      list.SetWindowHandle(nullptr);
      break;
    }
    // Recognition > Media providers
    case PAGE_RECOGNITION_STREAM: {
      win32::ListView list = GetDlgItem(IDC_LIST_STREAM_PROVIDER);
      list.EnableGroupView(true);
      list.InsertColumn(0, 0, 0, 0, L"Media providers");
      list.InsertGroup(0, L"Media providers");
      list.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);
      list.SetImageList(UI.ImgList16.GetHandle());
      list.SetTheme();
      list.InsertItem(0, 0, ICON16_APP_BLUE, 0, nullptr, L"Anime News Network", 0);
      list.InsertItem(1, 0, ICON16_APP_BLUE, 0, nullptr, L"Crunchyroll", 1);
      list.InsertItem(2, 0, ICON16_APP_BLUE, 0, nullptr, L"Veoh", 3);
      list.InsertItem(3, 0, ICON16_APP_BLUE, 0, nullptr, L"Viz Anime", 4);
      list.InsertItem(4, 0, ICON16_APP_BLUE, 0, nullptr, L"YouTube", 5);
      if (Settings.Recognition.Streaming.ann_enabled)
        list.SetCheckState(0, TRUE);
      if (Settings.Recognition.Streaming.crunchyroll_enabled)
        list.SetCheckState(1, TRUE);
      if (Settings.Recognition.Streaming.veoh_enabled)
        list.SetCheckState(2, TRUE);
      if (Settings.Recognition.Streaming.viz_enabled)
        list.SetCheckState(3, TRUE);
      if (Settings.Recognition.Streaming.youtube_enabled)
        list.SetCheckState(4, TRUE);
      list.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
      list.SetWindowHandle(nullptr);
      break;
    }

    // =========================================================================

    // Sharing > HTTP
    case PAGE_SHARING_HTTP: {
      CheckDlgButton(IDC_CHECK_HTTP, Settings.Announce.HTTP.enabled);
      SetDlgItemText(IDC_EDIT_HTTP_URL, Settings.Announce.HTTP.url.c_str());
      break;
    }
    // Sharing > Messenger
    case PAGE_SHARING_MESSENGER: {
      CheckDlgButton(IDC_CHECK_MESSENGER, Settings.Announce.MSN.enabled);
      break;
    }
    // Sharing > mIRC
    case PAGE_SHARING_MIRC: {
      CheckDlgButton(IDC_CHECK_MIRC, Settings.Announce.MIRC.enabled);
      CheckDlgButton(IDC_CHECK_MIRC_MULTISERVER, Settings.Announce.MIRC.multi_server);
      CheckDlgButton(IDC_CHECK_MIRC_ACTION, Settings.Announce.MIRC.use_action);
      SetDlgItemText(IDC_EDIT_MIRC_SERVICE, Settings.Announce.MIRC.service.c_str());
      CheckRadioButton(IDC_RADIO_MIRC_CHANNEL1, IDC_RADIO_MIRC_CHANNEL3,
                       IDC_RADIO_MIRC_CHANNEL1 + Settings.Announce.MIRC.mode - 1);
      SetDlgItemText(IDC_EDIT_MIRC_CHANNELS, Settings.Announce.MIRC.channels.c_str());
      EnableDlgItem(IDC_EDIT_MIRC_CHANNELS, Settings.Announce.MIRC.mode == 3);
      break;
    }
    // Sharing > Skype
    case PAGE_SHARING_SKYPE: {
      CheckDlgButton(IDC_CHECK_SKYPE, Settings.Announce.Skype.enabled);
      break;
    }
    // Sharing > Twitter
    case PAGE_SHARING_TWITTER: {
      CheckDlgButton(IDC_CHECK_TWITTER, Settings.Announce.Twitter.enabled);
      parent->RefreshTwitterLink();
      break;
    }

    // =========================================================================

    // Torrents > Discovery
    case PAGE_TORRENTS_DISCOVERY: {
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"http://tokyotosho.info/rss.php?filter=1,11&zwnj=0");
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"http://www.animesuki.com/rss.php?link=enclosure");
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"http://www.baka-updates.com/rss.php");
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"http://www.nyaa.se/?page=rss&cats=1_37&filter=2");
      SetDlgItemText(IDC_COMBO_TORRENT_SOURCE, Settings.RSS.Torrent.source.c_str());
      AddComboString(IDC_COMBO_TORRENT_SEARCH, L"http://www.nyaa.se/?page=rss&cats=1_37&filter=2&term=%title%");
      AddComboString(IDC_COMBO_TORRENT_SEARCH, L"http://pipes.yahoo.com/pipes/pipe.run?SearchQuery=%title%&_id=7b99f981c5b1f02354642f4e271cca43&_render=rss");
      SetDlgItemText(IDC_COMBO_TORRENT_SEARCH, Settings.RSS.Torrent.search_url.c_str());
      CheckDlgButton(IDC_CHECK_TORRENT_HIDE, Settings.RSS.Torrent.hide_unidentified);
      CheckDlgButton(IDC_CHECK_TORRENT_AUTOCHECK, Settings.RSS.Torrent.check_enabled);
      SendDlgItemMessage(IDC_SPIN_TORRENT_INTERVAL, UDM_SETRANGE32, 0, 3600);
      SendDlgItemMessage(IDC_SPIN_TORRENT_INTERVAL, UDM_SETPOS32, 0, Settings.RSS.Torrent.check_interval);
      EnableDlgItem(IDC_EDIT_TORRENT_INTERVAL, Settings.RSS.Torrent.check_enabled);
      EnableDlgItem(IDC_SPIN_TORRENT_INTERVAL, Settings.RSS.Torrent.check_enabled);
      CheckRadioButton(IDC_RADIO_TORRENT_NEW1, IDC_RADIO_TORRENT_NEW2,
                       IDC_RADIO_TORRENT_NEW1 + Settings.RSS.Torrent.new_action - 1);
      break;
    }
    // Torrents > Downloads
    case PAGE_TORRENTS_DOWNLOADS: {
      CheckRadioButton(IDC_RADIO_TORRENT_APP1, IDC_RADIO_TORRENT_APP2,
                       IDC_RADIO_TORRENT_APP1 + Settings.RSS.Torrent.app_mode - 1);
      SetDlgItemText(IDC_EDIT_TORRENT_APP, Settings.RSS.Torrent.app_path.c_str());
      EnableDlgItem(IDC_EDIT_TORRENT_APP, Settings.RSS.Torrent.app_mode > 1);
      EnableDlgItem(IDC_BUTTON_TORRENT_BROWSE_APP, Settings.RSS.Torrent.app_mode > 1);
      CheckDlgButton(IDC_CHECK_TORRENT_AUTOSETFOLDER, Settings.RSS.Torrent.set_folder);
      CheckDlgButton(IDC_CHECK_TORRENT_AUTOUSEFOLDER, Settings.RSS.Torrent.use_folder);
      CheckDlgButton(IDC_CHECK_TORRENT_AUTOCREATEFOLDER, Settings.RSS.Torrent.create_folder);
      for (size_t i = 0; i < Settings.Folders.root.size(); i++)
        AddComboString(IDC_COMBO_TORRENT_FOLDER, Settings.Folders.root[i].c_str());
      SetDlgItemText(IDC_COMBO_TORRENT_FOLDER, Settings.RSS.Torrent.download_path.c_str());
      EnableDlgItem(IDC_CHECK_TORRENT_AUTOUSEFOLDER, Settings.RSS.Torrent.set_folder);
      EnableDlgItem(IDC_COMBO_TORRENT_FOLDER, Settings.RSS.Torrent.set_folder && Settings.RSS.Torrent.use_folder);
      EnableDlgItem(IDC_BUTTON_TORRENT_BROWSE_FOLDER, Settings.RSS.Torrent.set_folder && Settings.RSS.Torrent.use_folder);
      EnableDlgItem(IDC_CHECK_TORRENT_AUTOCREATEFOLDER, Settings.RSS.Torrent.set_folder && Settings.RSS.Torrent.use_folder);
      break;
    }
    // Torrents > Filters
    case PAGE_TORRENTS_FILTERS: {
      SendDlgItemMessage(IDC_SPIN_TORRENT_ARCHIVE_COUNT, UDM_SETRANGE32, 0, 3600);
      SendDlgItemMessage(IDC_SPIN_TORRENT_ARCHIVE_COUNT, UDM_SETPOS32, 0, Settings.RSS.Torrent.Filters.archive_maxcount);
      CheckDlgButton(IDC_CHECK_TORRENT_FILTER, Settings.RSS.Torrent.Filters.global_enabled);
      win32::ListView list = GetDlgItem(IDC_LIST_TORRENT_FILTER);
      list.EnableGroupView(true);
      list.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
      list.SetImageList(UI.ImgList16.GetHandle());
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
      win32::Toolbar toolbar = GetDlgItem(IDC_TOOLBAR_FEED_FILTER);
      toolbar.SetImageList(UI.ImgList16.GetHandle(), 16, 16);
      toolbar.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
      // Add toolbar items
      toolbar.InsertButton(0, ICON16_PLUS,       100, true,  0, 0, nullptr, L"Add new filter...");
      toolbar.InsertButton(1, ICON16_MINUS,      101, false, 0, 1, nullptr, L"Delete filter");
      toolbar.InsertButton(2, 0, 0, 0, BTNS_SEP, 0, nullptr, nullptr);
      toolbar.InsertButton(3, ICON16_ARROW_UP,   103, false, 0, 3, nullptr, L"Move up");
      toolbar.InsertButton(4, ICON16_ARROW_DOWN, 104, false, 0, 4, nullptr, L"Move down");
      toolbar.SetWindowHandle(nullptr);
      break;
    }
  }
  
  return TRUE;
}

// =============================================================================

BOOL SettingsPage::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (HIWORD(wParam)) {
    case BN_CLICKED: {
      switch (LOWORD(wParam)) {
        // Edit format
        case IDC_BUTTON_FORMAT_HTTP:
          FormatDialog.mode = FORMAT_MODE_HTTP;
          FormatDialog.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;
        case IDC_BUTTON_FORMAT_MSN:
          FormatDialog.mode = FORMAT_MODE_MESSENGER;
          FormatDialog.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;
        case IDC_BUTTON_FORMAT_MIRC:
          FormatDialog.mode = FORMAT_MODE_MIRC;
          FormatDialog.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;
        case IDC_BUTTON_FORMAT_SKYPE:
          FormatDialog.mode = FORMAT_MODE_SKYPE;
          FormatDialog.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;
        case IDC_BUTTON_FORMAT_TWITTER:
          FormatDialog.mode = FORMAT_MODE_TWITTER;
          FormatDialog.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;
        case IDC_BUTTON_FORMAT_BALLOON:
          FormatDialog.mode = FORMAT_MODE_BALLOON;
          FormatDialog.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;

        // ================================================================================

        // Add folders
        case IDC_BUTTON_ADDFOLDER: {
          wstring path;
          if (BrowseForFolder(m_hWindow, L"Please select a folder:", L"", path)) {
            win32::ListView list = GetDlgItem(IDC_LIST_FOLDERS_ROOT);
            list.InsertItem(list.GetItemCount(), -1, ICON16_FOLDER, 0, nullptr, path.c_str(), 0);
            list.SetSelectedItem(list.GetItemCount() - 1);
            list.SetWindowHandle(nullptr);
          }
          return TRUE;
        }
        // Remove folders
        case IDC_BUTTON_REMOVEFOLDER: {
          win32::ListView list = GetDlgItem(IDC_LIST_FOLDERS_ROOT);
          while (list.GetSelectedCount() > 0)
            list.DeleteItem(list.GetNextItem(-1, LVNI_SELECTED));
          EnableDlgItem(IDC_BUTTON_REMOVEFOLDER, FALSE);
          list.SetWindowHandle(nullptr);
          return TRUE;
        }

        // ================================================================================

        // Clear cache
        case IDC_BUTTON_CACHE_CLEAR: {
          if (IsDlgButtonChecked(IDC_CHECK_CACHE1)) {
            History.items.clear();
            History.Save();
            HistoryDialog.RefreshList();
          }
          if (IsDlgButtonChecked(IDC_CHECK_CACHE2)) {
            wstring path = Taiga.GetDataPath() + L"db\\image";
            DeleteFolder(path);
            ImageDatabase.FreeMemory();
          }
          if (IsDlgButtonChecked(IDC_CHECK_CACHE3)) {
            wstring path = Taiga.GetDataPath() + L"feed";
            DeleteFolder(path);
          }
          parent->RefreshCache();
          CheckDlgButton(IDC_CHECK_CACHE1, FALSE);
          CheckDlgButton(IDC_CHECK_CACHE2, FALSE);
          CheckDlgButton(IDC_CHECK_CACHE3, FALSE);
          EnableDlgItem(IDC_BUTTON_CACHE_CLEAR, FALSE);
          return TRUE;
        }

        // ================================================================================

        // Test DDE connection
        case IDC_BUTTON_MIRC_TEST: {
          wstring service;
          GetDlgItemText(IDC_EDIT_MIRC_SERVICE, service);
          Announcer.TestMircConnection(service);
          return TRUE;
        }

        // ================================================================================
    
        // Authorize Twitter
        case IDC_BUTTON_TWITTER_AUTH: {
          Twitter.RequestToken();
          return TRUE;
        }

        // ================================================================================

        // Browse for torrent application
        case IDC_BUTTON_TORRENT_BROWSE_APP: {
          wstring current_directory = Taiga.GetCurrentDirectory();
          wstring path = BrowseForFile(m_hWindow, L"Please select a torrent application", 
                                       L"Executable files (*.exe)\0*.exe\0\0");
          if (current_directory != Taiga.GetCurrentDirectory())
            Taiga.SetCurrentDirectory(current_directory);
          if (!path.empty())
            SetDlgItemText(IDC_EDIT_TORRENT_APP, path.c_str());
          return TRUE;
        }
        // Browse for torrent download path
        case IDC_BUTTON_TORRENT_BROWSE_FOLDER: {
          wstring path;
          if (BrowseForFolder(m_hWindow, L"Please select a folder:", L"", path))
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
          FeedFilterDialog.filter.Reset();
          ExecuteAction(L"TorrentAddFilter", TRUE, reinterpret_cast<LPARAM>(parent->GetWindowHandle()));
          if (!FeedFilterDialog.filter.conditions.empty()) {
            if (FeedFilterDialog.filter.name.empty())
              FeedFilterDialog.filter.name = Aggregator.filter_manager.CreateNameFromConditions(FeedFilterDialog.filter);
            parent->feed_filters_.push_back(FeedFilterDialog.filter);
            win32::ListView list = GetDlgItem(IDC_LIST_TORRENT_FILTER);
            parent->RefreshTorrentFilterList(list.GetWindowHandle());
            list.SetSelectedItem(list.GetItemCount() - 1);
            list.SetWindowHandle(nullptr);
          }
          return TRUE;
        }
        // Remove global filter
        case 101: {
          win32::ListView list = GetDlgItem(IDC_LIST_TORRENT_FILTER);
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
          win32::ListView list = GetDlgItem(IDC_LIST_TORRENT_FILTER);
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
          win32::ListView list = GetDlgItem(IDC_LIST_TORRENT_FILTER);
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

        // ================================================================================

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

// =============================================================================

void SettingsPage::OnDropFiles(HDROP hDropInfo) {
  if (index != PAGE_LIBRARY_FOLDERS)
    return;
  
  WCHAR szFileName[MAX_PATH + 1];
  UINT nFiles = DragQueryFile(hDropInfo, static_cast<UINT>(-1), nullptr, 0);
  win32::ListView list = GetDlgItem(IDC_LIST_FOLDERS_ROOT);
  for (UINT i = 0; i < nFiles; i++) {
    ZeroMemory(szFileName, MAX_PATH + 1);
    DragQueryFile(hDropInfo, i, (LPWSTR)szFileName, MAX_PATH + 1);
    if (GetFileAttributes(szFileName) & FILE_ATTRIBUTE_DIRECTORY) {
      list.InsertItem(list.GetItemCount(), -1, ICON16_FOLDER, 0, nullptr, szFileName, 0);
      list.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
    }
  }
  list.SetWindowHandle(nullptr);
}

LRESULT SettingsPage::OnNotify(int idCtrl, LPNMHDR pnmh) {
  switch (pnmh->code) {
    // Header checkbox click
    case HDN_ITEMCHANGED: {
      win32::ListView list = GetDlgItem(IDC_LIST_MEDIA);
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
        case IDC_LINK_MAL:
        case IDC_LINK_TWITTER: {
          PNMLINK pNMLink = reinterpret_cast<PNMLINK>(pnmh);
          ExecuteAction(pNMLink->item.szUrl);
          return TRUE;
        }
        // Open themes folder
        case IDC_LINK_THEMES: {
          wstring theme_name;
          GetDlgItemText(IDC_COMBO_THEME, theme_name);
          wstring path = Taiga.GetDataPath() + L"theme\\" + theme_name;
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
        win32::ListView list = GetDlgItem(IDC_LIST_TORRENT_FILTER);
        win32::Toolbar toolbar = GetDlgItem(IDC_TOOLBAR_FEED_FILTER);
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
        win32::Toolbar toolbar = GetDlgItem(IDC_TOOLBAR_FEED_FILTER);
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
      if (lpnmitem->iItem == -1) break;
      // Anime folders
      if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_FOLDERS_ROOT)) {
        win32::ListView list = lpnmitem->hdr.hwndFrom;
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
        win32::ListView list = lpnmitem->hdr.hwndFrom;
        FeedFilter* feed_filter = reinterpret_cast<FeedFilter*>(list.GetItemParam(lpnmitem->iItem));
        if (feed_filter) {
          FeedFilterDialog.filter = *feed_filter;
          FeedFilterDialog.Create(IDD_FEED_FILTER, parent->GetWindowHandle());
          if (!FeedFilterDialog.filter.conditions.empty()) {
            *feed_filter = FeedFilterDialog.filter;
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
      if (lpnmitem->iItem == -1) break;
      win32::ListView list = lpnmitem->hdr.hwndFrom;
      // Media players
      if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_MEDIA)) {
        wstring answer = UI.Menus.Show(GetWindowHandle(), 0, 0, L"GenericList");
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