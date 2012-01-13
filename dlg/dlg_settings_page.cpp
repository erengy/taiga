/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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
#include "../announce.h"
#include "../common.h"
#include "dlg_format.h"
#include "dlg_settings.h"
#include "dlg_settings_page.h"
#include "dlg_feed_filter.h"
#include "../http.h"
#include "../media.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../settings.h"
#include "../string.h"
#include "../taiga.h"
#include "../theme.h"

// =============================================================================

SettingsPage::SettingsPage() :
  parent(nullptr), tree_item_(nullptr)
{
}

void SettingsPage::CreateItem(LPCWSTR pszText, HTREEITEM htiParent) {
  tree_item_ = parent->tree_.InsertItem(
    pszText, -1, reinterpret_cast<LPARAM>(this), htiParent);
}

void SettingsPage::Select() {
  parent->tree_.SelectItem(tree_item_);
}

// =============================================================================

BOOL SettingsPage::OnInitDialog() {
  switch (index) {
    // Account
    case PAGE_ACCOUNT: {
      SetDlgItemText(IDC_EDIT_USER, Settings.Account.MAL.user.c_str());
      SetDlgItemText(IDC_EDIT_PASS, Settings.Account.MAL.password.c_str());
      CheckDlgButton(IDC_RADIO_API1 + Settings.Account.MAL.api - 1, TRUE);
      CheckDlgButton(IDC_CHECK_START_LOGIN, Settings.Account.MAL.auto_login);
      break;
    }
    // Account > Update
    case PAGE_UPDATE: {
      CheckDlgButton(IDC_RADIO_UPDATE_MODE1 + Settings.Account.Update.mode - 1, TRUE);
      CheckDlgButton(IDC_RADIO_UPDATE_TIME1 + Settings.Account.Update.time - 1, TRUE);
      SendDlgItemMessage(IDC_SPIN_DELAY, UDM_SETRANGE32, 0, 3600);
      SendDlgItemMessage(IDC_SPIN_DELAY, UDM_SETPOS32, 0, Settings.Account.Update.delay);
      CheckDlgButton(IDC_CHECK_UPDATE_CHECKMP, Settings.Account.Update.check_player);
      CheckDlgButton(IDC_CHECK_UPDATE_RANGE, Settings.Account.Update.out_of_range);
      break;
    }
              
    // =========================================================================

    // Anime folders > Root
    case PAGE_FOLDERS_ROOT: {
      CListView List = GetDlgItem(IDC_LIST_FOLDERS_ROOT);
      List.InsertColumn(0, 0, 0, 0, L"Folder");
      List.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
      List.SetExtendedStyle(LVS_EX_DOUBLEBUFFER);
      List.SetImageList(UI.ImgList16.GetHandle());
      List.SetTheme();
      for (size_t i = 0; i < Settings.Folders.root.size(); i++) {
        List.InsertItem(i, -1, ICON16_FOLDER, 0, nullptr, Settings.Folders.root[i].c_str(), 0);
      }
      List.SetWindowHandle(nullptr);
      CheckDlgButton(IDC_CHECK_FOLDERS_WATCH, Settings.Folders.watch_enabled);
      break;
    }
    // Anime folders > Specific
    case PAGE_FOLDERS_ANIME: {
      CListView List = GetDlgItem(IDC_LIST_FOLDERS_ANIME);
      List.EnableGroupView(true);
      List.InsertColumn(0, 220, 220, 0, L"Anime title");
      List.InsertColumn(1, 200, 200, 0, L"Folder");
      for (int i = MAL_WATCHING; i <= MAL_PLANTOWATCH; i++) {
        if (i != MAL_UNKNOWN) {
          List.InsertGroup(i, MAL.TranslateMyStatus(i, false).c_str(), true, i != MAL_WATCHING);
        }
      }
      List.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
      List.SetImageList(UI.ImgList16.GetHandle());
      List.SetTheme();
      for (int i = 1; i <= AnimeList.count; i++) {
        List.InsertItem(i - 1, AnimeList.items[i].GetStatus(), 
                StatusToIcon(AnimeList.items[i].GetAiringStatus()), 
                0, nullptr, LPSTR_TEXTCALLBACK, 
                reinterpret_cast<LPARAM>(&AnimeList.items[i]));
        List.SetItem(i - 1, 1, AnimeList.items[i].folder.c_str());
      }
      List.Sort(0, 1, 0, ListViewCompareProc);
      List.SetWindowHandle(nullptr);
      break;
    }

    // =========================================================================

    // Announcements > HTTP
    case PAGE_HTTP: {
      CheckDlgButton(IDC_CHECK_HTTP, Settings.Announce.HTTP.enabled);
      SetDlgItemText(IDC_EDIT_HTTP_URL, Settings.Announce.HTTP.url.c_str());
      break;
    }
      // Announcements > Messenger
    case PAGE_MESSENGER: {
      CheckDlgButton(IDC_CHECK_MESSENGER, Settings.Announce.MSN.enabled);
      break;
    }
    // Announcements > mIRC
    case PAGE_MIRC: {
      CheckDlgButton(IDC_CHECK_MIRC, Settings.Announce.MIRC.enabled);
      CheckDlgButton(IDC_CHECK_MIRC_MULTISERVER, Settings.Announce.MIRC.multi_server);
      CheckDlgButton(IDC_CHECK_MIRC_ACTION, Settings.Announce.MIRC.use_action);
      SetDlgItemText(IDC_EDIT_MIRC_SERVICE, Settings.Announce.MIRC.service.c_str());
      CheckDlgButton(IDC_RADIO_MIRC_CHANNEL1 + Settings.Announce.MIRC.mode - 1, TRUE);
      SetDlgItemText(IDC_EDIT_MIRC_CHANNELS, Settings.Announce.MIRC.channels.c_str());
      break;
    }
    // Announcements > Skype
    case PAGE_SKYPE: {
      CheckDlgButton(IDC_CHECK_SKYPE, Settings.Announce.Skype.enabled);
      break;
    }
    // Announcements > Twitter
    case PAGE_TWITTER: {
      CheckDlgButton(IDC_CHECK_TWITTER, Settings.Announce.Twitter.enabled);
      parent->RefreshTwitterLink();
      break;
    }

    // =========================================================================

    // Program > General
    case PAGE_PROGRAM: {
      CheckDlgButton(IDC_CHECK_AUTOSTART, Settings.Program.General.auto_start);
      CheckDlgButton(IDC_CHECK_GENERAL_CLOSE, Settings.Program.General.close);
      CheckDlgButton(IDC_CHECK_GENERAL_MINIMIZE, Settings.Program.General.minimize);
      vector<wstring> theme_list;
      PopulateFolders(theme_list, Taiga.GetDataPath() + L"Theme\\");
      if (theme_list.empty()) {
        EnableDlgItem(IDC_COMBO_THEME, FALSE);
      } else {
        for (size_t i = 0; i < theme_list.size(); i++) {
          AddComboString(IDC_COMBO_THEME, theme_list[i].c_str());
          if (IsEqual(theme_list[i], Settings.Program.General.theme)) {
            SetComboSelection(IDC_COMBO_THEME, i);
          }
        }
      }
      CheckDlgButton(IDC_CHECK_START_VERSION, Settings.Program.StartUp.check_new_version);
      CheckDlgButton(IDC_CHECK_START_CHECKEPS, Settings.Program.StartUp.check_new_episodes);
      CheckDlgButton(IDC_CHECK_START_MINIMIZE, Settings.Program.StartUp.minimize);
      CheckDlgButton(IDC_CHECK_EXIT_ASK, Settings.Program.Exit.ask);
      CheckDlgButton(IDC_CHECK_EXIT_SAVEBUFFER, Settings.Program.Exit.save_event_queue);
      SetDlgItemText(IDC_EDIT_PROXY_HOST, Settings.Program.Proxy.host.c_str());
      SetDlgItemText(IDC_EDIT_PROXY_USER, Settings.Program.Proxy.user.c_str());
      SetDlgItemText(IDC_EDIT_PROXY_PASS, Settings.Program.Proxy.password.c_str());
      break;
    }
    // Program > List
    case PAGE_LIST: {
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
      CheckDlgButton(IDC_CHECK_FILTER_NEWEPS, AnimeList.filters.new_episodes);
      CheckDlgButton(IDC_CHECK_HIGHLIGHT, Settings.Program.List.highlight);
      CheckDlgButton(IDC_RADIO_LIST_PROGRESS1 + Settings.Program.List.progress_mode, TRUE);
      CheckDlgButton(IDC_CHECK_LIST_PROGRESS_EPS, Settings.Program.List.progress_show_eps);
      break;
    }
    // Program > Notifications
    case PAGE_NOTIFICATIONS: {
      CheckDlgButton(IDC_CHECK_BALLOON, Settings.Program.Balloon.enabled);
      break;
    }

    // =========================================================================

    // Recognition > Media players
    case PAGE_MEDIA: {
      CListView List = GetDlgItem(IDC_LIST_MEDIA);
      List.EnableGroupView(true);
      List.InsertColumn(0, 0, 0, 0, L"Supported players");
      List.InsertGroup(0, L"Media players");
      List.InsertGroup(1, L"Web browsers");
      List.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);
      List.SetImageList(UI.ImgList16.GetHandle());
      List.SetTheme();
      CWindow Header; HDITEM hdi = {0};
      Header.SetWindowHandle(List.GetHeader());
      if (GetWinVersion() >= WINVERSION_VISTA) {
        Header.SetStyle(HDS_CHECKBOXES, 0);
        hdi.mask = HDI_FORMAT;
        Header_GetItem(Header.GetWindowHandle(), 0, &hdi);
        hdi.fmt |= HDF_CHECKBOX;
        Header_SetItem(Header.GetWindowHandle(), 0, &hdi);
      }
      for (size_t i = 0; i < MediaPlayers.items.size(); i++) {
        BOOL player_available = MediaPlayers.items[i].GetPath().empty() ? FALSE : TRUE;
        List.InsertItem(i, MediaPlayers.items[i].mode == 5 ? 1 : 0, 
          ICON16_APP_GRAY - player_available, 0, nullptr, 
          MediaPlayers.items[i].name.c_str(), 0);
        if (MediaPlayers.items[i].enabled) List.SetCheckState(i, TRUE);
      }
      Header.SetWindowHandle(nullptr);
      List.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
      List.SetWindowHandle(nullptr);
      break;
    }

    // Recognition > Streaming
    case PAGE_STREAM: {
      CListView List = GetDlgItem(IDC_LIST_STREAM_PROVIDER);
      List.EnableGroupView(true);
      List.InsertColumn(0, 0, 0, 0, L"Media providers");
      List.InsertGroup(0, L"Media providers");
      List.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);
      List.SetImageList(UI.ImgList16.GetHandle());
      List.SetTheme();
      List.InsertItem(0, 0, ICON16_APP_BLUE, 0, nullptr, L"Anime News Network", 0);
      List.InsertItem(1, 0, ICON16_APP_BLUE, 0, nullptr, L"Crunchyroll", 1);
      List.InsertItem(2, 0, ICON16_APP_BLUE, 0, nullptr, L"Veoh", 3);
      List.InsertItem(3, 0, ICON16_APP_BLUE, 0, nullptr, L"Viz Anime", 4);
      List.InsertItem(4, 0, ICON16_APP_BLUE, 0, nullptr, L"YouTube", 5);
      if (Settings.Recognition.Streaming.ann_enabled) List.SetCheckState(0, TRUE);
      if (Settings.Recognition.Streaming.crunchyroll_enabled) List.SetCheckState(1, TRUE);
      if (Settings.Recognition.Streaming.veoh_enabled) List.SetCheckState(2, TRUE);
      if (Settings.Recognition.Streaming.viz_enabled) List.SetCheckState(3, TRUE);
      if (Settings.Recognition.Streaming.youtube_enabled) List.SetCheckState(4, TRUE);
      List.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
      List.SetWindowHandle(nullptr);
      break;
    }

    // =========================================================================

    // Torrent > Options
    case PAGE_TORRENT1: {
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"http://tokyotosho.info/rss.php?filter=1,11&zwnj=0");
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"http://www.animesuki.com/rss.php?link=enclosure");
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"http://www.baka-updates.com/rss.php");
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"http://www.nyaa.eu/?page=rss&cats=1_37&filter=2");
      SetDlgItemText(IDC_COMBO_TORRENT_SOURCE, Settings.RSS.Torrent.source.c_str());
      CheckDlgButton(IDC_CHECK_TORRENT_HIDE, Settings.RSS.Torrent.hide_unidentified);
      CheckDlgButton(IDC_CHECK_TORRENT_AUTOCHECK, Settings.RSS.Torrent.check_enabled);
      SendDlgItemMessage(IDC_SPIN_TORRENT_INTERVAL, UDM_SETRANGE32, 0, 3600);
      SendDlgItemMessage(IDC_SPIN_TORRENT_INTERVAL, UDM_SETPOS32, 0, Settings.RSS.Torrent.check_interval);
      CheckDlgButton(IDC_RADIO_TORRENT_NEW1 + Settings.RSS.Torrent.new_action - 1, TRUE);
      CheckDlgButton(IDC_RADIO_TORRENT_APP1 + Settings.RSS.Torrent.app_mode - 1, TRUE);
      SetDlgItemText(IDC_EDIT_TORRENT_APP, Settings.RSS.Torrent.app_path.c_str());
      EnableDlgItem(IDC_EDIT_TORRENT_APP, Settings.RSS.Torrent.app_mode > 1);
      EnableDlgItem(IDC_BUTTON_TORRENT_BROWSE, Settings.RSS.Torrent.app_mode > 1);
      CheckDlgButton(IDC_CHECK_TORRENT_AUTOSETFOLDER, Settings.RSS.Torrent.set_folder);
      break;
    }
    // Torrent > Filters
    case PAGE_TORRENT2: {
      CheckDlgButton(IDC_CHECK_TORRENT_FILTER, Settings.RSS.Torrent.Filters.global_enabled);
      CListView List = GetDlgItem(IDC_LIST_TORRENT_FILTER);
      List.EnableGroupView(true);
      List.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
      List.SetImageList(UI.ImgList16.GetHandle());
      List.SetTheme();
      List.InsertColumn(0, 400, 400, 0, L"Name");
      List.InsertGroup(0, L"General filters", true, false);
      List.InsertGroup(1, L"Limited filters", true, false);
      parent->feed_filters_.resize(Aggregator.filter_manager.filters.size());
      std::copy(Aggregator.filter_manager.filters.begin(), Aggregator.filter_manager.filters.end(), 
        parent->feed_filters_.begin());
      parent->RefreshTorrentFilterList(List.GetWindowHandle());
      List.SetWindowHandle(nullptr);
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
        case IDC_BUTTON_FORMAT_HTTP: {
          FormatDialog.mode = FORMAT_MODE_HTTP;
          FormatDialog.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;
        }
        case IDC_BUTTON_FORMAT_MSN: {
          FormatDialog.mode = FORMAT_MODE_MESSENGER;
          FormatDialog.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;
        }
        case IDC_BUTTON_FORMAT_MIRC: {
          FormatDialog.mode = FORMAT_MODE_MIRC;
          FormatDialog.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;
        }
        case IDC_BUTTON_FORMAT_SKYPE: {
          FormatDialog.mode = FORMAT_MODE_SKYPE;
          FormatDialog.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;
        }
        case IDC_BUTTON_FORMAT_TWITTER: {
          FormatDialog.mode = FORMAT_MODE_TWITTER;
          FormatDialog.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;
        }
        case IDC_BUTTON_FORMAT_BALLOON: {
          FormatDialog.mode = FORMAT_MODE_BALLOON;
          FormatDialog.Create(IDD_FORMAT, parent->GetWindowHandle(), true);
          return TRUE;
        }

        // ================================================================================

        // Add folders
        case IDC_BUTTON_ADDFOLDER: {
          wstring path;
          if (BrowseForFolder(m_hWindow, L"Please select a folder:", 
            BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON, path)) {
              CListView List = GetDlgItem(IDC_LIST_FOLDERS_ROOT);
              List.InsertItem(List.GetItemCount(), -1, ICON16_FOLDER, 0, nullptr, path.c_str(), 0);
              List.SetSelectedItem(List.GetItemCount() - 1);
              List.SetWindowHandle(nullptr);
          }
          return TRUE;
        }
        // Remove folders
        case IDC_BUTTON_REMOVEFOLDER: {
          CListView List = GetDlgItem(IDC_LIST_FOLDERS_ROOT);
          while (List.GetSelectedCount() > 0) {
            List.DeleteItem(List.GetNextItem(-1, LVNI_SELECTED));
          }
          EnableDlgItem(IDC_BUTTON_REMOVEFOLDER, FALSE);
          List.SetWindowHandle(nullptr);
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

        // Filter list
        case IDC_BUTTON_FILTERLIST: {
          ExecuteAction(L"Filter");
          return TRUE;
        }

        // ================================================================================

        // Browse for torrent application
        case IDC_BUTTON_TORRENT_BROWSE: {
          wstring path, current_directory;
          current_directory = Taiga.GetCurrentDirectory();
          path = BrowseForFile(m_hWindow, L"Please select a torrent application", 
            L"Executable files (*.exe)\0*.exe\0\0");
          if (current_directory != Taiga.GetCurrentDirectory()) {
            Taiga.SetCurrentDirectory(current_directory);
          }
          if (!path.empty()) {
            SetDlgItemText(IDC_EDIT_TORRENT_APP, path.c_str());
          }
          return TRUE;
        }
        // Add global filter
        case IDC_BUTTON_TORRENT_FILTER_ADD: {
          FeedFilterDialog.filter.Reset();
          ExecuteAction(L"TorrentAddFilter", TRUE, 
            reinterpret_cast<LPARAM>(parent->GetWindowHandle()));
          if (!FeedFilterDialog.filter.conditions.empty()) {
            if (FeedFilterDialog.filter.name.empty()) {
              FeedFilterDialog.filter.name = Aggregator.filter_manager.CreateNameFromConditions(FeedFilterDialog.filter);
            }
            parent->feed_filters_.push_back(FeedFilterDialog.filter);
            CListView List = GetDlgItem(IDC_LIST_TORRENT_FILTER);
            parent->RefreshTorrentFilterList(List.GetWindowHandle());
            List.SetSelectedItem(List.GetItemCount() - 1);
            List.SetWindowHandle(nullptr);
          }
          return TRUE;
        }
        // Remove global filter
        case IDC_BUTTON_TORRENT_FILTER_DELETE: {
          CListView List = GetDlgItem(IDC_LIST_TORRENT_FILTER);
          int item_index = List.GetNextItem(-1, LVNI_SELECTED);
          FeedFilter* feed_filter = reinterpret_cast<FeedFilter*>(List.GetItemParam(item_index));
          for (auto it = parent->feed_filters_.begin(); it != parent->feed_filters_.end(); ++it) {
            if (feed_filter == &(*it)) {
              parent->feed_filters_.erase(it);
              parent->RefreshTorrentFilterList(List.GetWindowHandle());
              break;
            }
          }
          EnableDlgItem(IDC_BUTTON_TORRENT_FILTER_DELETE, FALSE);
          List.SetWindowHandle(nullptr);
          return TRUE;
        }
        // Enable/disable filters
        case IDC_CHECK_TORRENT_FILTER: {
          BOOL enable = IsDlgButtonChecked(LOWORD(wParam));
          EnableDlgItem(IDC_LIST_TORRENT_FILTER, enable);
          EnableDlgItem(IDC_BUTTON_TORRENT_FILTER_ADD, enable);
          if (enable) {
            CListView List = GetDlgItem(IDC_LIST_TORRENT_FILTER);
            enable = List.GetNextItem(-1, LVNI_SELECTED) > -1;
            List.SetWindowHandle(nullptr);
          }
          EnableDlgItem(IDC_BUTTON_TORRENT_FILTER_DELETE, enable);
          return TRUE;
        }

        // ================================================================================

        // Check radio buttons
        case IDC_RADIO_UPDATE_MODE1:
        case IDC_RADIO_UPDATE_MODE2:
        case IDC_RADIO_UPDATE_MODE3: {
          CheckRadioButton(IDC_RADIO_UPDATE_MODE1, IDC_RADIO_UPDATE_MODE3, LOWORD(wParam));
          return TRUE;
        }
        case IDC_RADIO_UPDATE_TIME1:
        case IDC_RADIO_UPDATE_TIME2:
        case IDC_RADIO_UPDATE_TIME3: {
          CheckRadioButton(IDC_RADIO_UPDATE_TIME1, IDC_RADIO_UPDATE_TIME3, LOWORD(wParam));
          return TRUE;
        }
        case IDC_RADIO_MIRC_CHANNEL1:
        case IDC_RADIO_MIRC_CHANNEL2:
        case IDC_RADIO_MIRC_CHANNEL3: {
          CheckRadioButton(IDC_RADIO_MIRC_CHANNEL1, IDC_RADIO_MIRC_CHANNEL3, LOWORD(wParam));
          return TRUE;
        }
        case IDC_RADIO_LIST_PROGRESS1:
        case IDC_RADIO_LIST_PROGRESS2: {
          CheckRadioButton(IDC_RADIO_LIST_PROGRESS1, IDC_RADIO_LIST_PROGRESS2, LOWORD(wParam));
          return TRUE;
        }
        case IDC_RADIO_TORRENT_NEW1:
        case IDC_RADIO_TORRENT_NEW2: {
          CheckRadioButton(IDC_RADIO_TORRENT_NEW1, IDC_RADIO_TORRENT_NEW2, LOWORD(wParam));
          return TRUE;
        }
        case IDC_RADIO_TORRENT_APP1:
        case IDC_RADIO_TORRENT_APP2: {
          CheckRadioButton(IDC_RADIO_TORRENT_APP1, IDC_RADIO_TORRENT_APP2, LOWORD(wParam));
          EnableDlgItem(IDC_EDIT_TORRENT_APP, LOWORD(wParam) == IDC_RADIO_TORRENT_APP2);
          EnableDlgItem(IDC_BUTTON_TORRENT_BROWSE, LOWORD(wParam) == IDC_RADIO_TORRENT_APP2);
          return TRUE;
        }
      }
      break;
    }
  }

  return FALSE;
}

// =============================================================================

INT_PTR SettingsPage::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_NOTIFY: {
      switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
        case NM_CLICK: {
          switch (reinterpret_cast<LPNMHDR>(lParam)->idFrom) {
            // Execute link
            case IDC_LINK_MAL:
            case IDC_LINK_TWITTER: {
              PNMLINK pNMLink = reinterpret_cast<PNMLINK>(lParam);
              ExecuteAction(pNMLink->item.szUrl);
              return TRUE;
            }
          }
        }

        // List item select
        case LVN_ITEMCHANGED: {
          LPNMLISTVIEW lplv = reinterpret_cast<LPNMLISTVIEW>(lParam);
          if (lplv->hdr.hwndFrom == GetDlgItem(IDC_LIST_FOLDERS_ROOT)) {
            EnableDlgItem(IDC_BUTTON_REMOVEFOLDER, ListView_GetSelectedCount(lplv->hdr.hwndFrom) > 0);
          } else if (lplv->hdr.hwndFrom == GetDlgItem(IDC_LIST_TORRENT_FILTER)) {
            EnableDlgItem(IDC_BUTTON_TORRENT_FILTER_DELETE, ListView_GetSelectedCount(lplv->hdr.hwndFrom) > 0);
          }
          break;
        }

        // Text callback
        case LVN_GETDISPINFO: {
          NMLVDISPINFO* plvdi = reinterpret_cast<NMLVDISPINFO*>(lParam);
          Anime* anime = reinterpret_cast<Anime*>(plvdi->item.lParam);
          if (!anime) break;
          switch (plvdi->item.iSubItem) {
            case 0: // Anime title
              plvdi->item.pszText = const_cast<LPWSTR>(anime->series_title.data());
              break;
          }
          break;
        }

        // Double click
        case NM_DBLCLK: {
          LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
          if (lpnmitem->iItem == -1) break;
          // Anime folders
          if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_FOLDERS_ROOT)) {
            CListView List = lpnmitem->hdr.hwndFrom;
            WCHAR buffer[MAX_PATH];
            List.GetItemText(lpnmitem->iItem, 0, buffer);
            Execute(buffer);
            List.SetWindowHandle(nullptr);
          } else if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_FOLDERS_ANIME)) {
            CListView List = lpnmitem->hdr.hwndFrom;
            wstring path, title;
            List.GetItemText(lpnmitem->iItem, 0, title);
            title = L"Anime title: " + title;
            if (BrowseForFolder(m_hWindow, title.c_str(), 
                BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON, path)) {
                  List.SetItem(lpnmitem->iItem, 1, path.c_str());
            }
            List.SetWindowHandle(nullptr);
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
            CListView List = lpnmitem->hdr.hwndFrom;
            FeedFilter* feed_filter = reinterpret_cast<FeedFilter*>(List.GetItemParam(lpnmitem->iItem));
            if (feed_filter) {
              FeedFilterDialog.filter = *feed_filter;
              FeedFilterDialog.Create(IDD_FEED_FILTER, parent->GetWindowHandle());
              if (!FeedFilterDialog.filter.conditions.empty()) {
                *feed_filter = FeedFilterDialog.filter;
                parent->RefreshTorrentFilterList(lpnmitem->hdr.hwndFrom);
                List.SetSelectedItem(lpnmitem->iItem);
              }
            }
            List.SetWindowHandle(nullptr);
          }
          return TRUE;
        }

        // Right click
        case NM_RCLICK: {
          LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
          if (lpnmitem->iItem == -1) break;
          CListView List = lpnmitem->hdr.hwndFrom;
          // Anime folders
          if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_FOLDERS_ANIME)) {
            wstring answer = UI.Menus.Show(hwnd, 0, 0, L"AnimeFoldersEdit");
            if (answer == L"AnimeFolders_Browse()") {
              wstring path, title;
              List.GetItemText(lpnmitem->iItem, 0, title);
              title = L"Anime title: " + title;
              if (BrowseForFolder(m_hWindow, title.c_str(), 
                  BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON, path)) {
                    List.SetItem(lpnmitem->iItem, 1, path.c_str());
              }
            } else if (answer == L"AnimeFolders_Clear()") {
              List.SetItem(lpnmitem->iItem, 1, L"");
            } else if (answer == L"AnimeFolders_Search()") {
              Anime* anime = reinterpret_cast<Anime*>(List.GetItemParam(lpnmitem->iItem));
              if (anime) {
                anime->CheckFolder();
                if (!anime->folder.empty()) {
                  List.SetItem(lpnmitem->iItem, 1, anime->folder.c_str());
                }
              }
            } else if (answer == L"AnimeFolders_ClearAll()") {
              for (int i = 0; i < List.GetItemCount(); i++) {
                List.SetItem(i, 1, L"");
              }
            }
          // Media players
          } else if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_MEDIA)) {
            wstring answer = UI.Menus.Show(hwnd, 0, 0, L"GenericList");
            for (int i = 0; i < List.GetItemCount(); i++) {
              if (answer == L"SelectAll()") {
                List.SetCheckState(i, TRUE);
              } else if (answer == L"DeselectAll()") {
                List.SetCheckState(i, FALSE);
              }
            }
          }
          List.SetWindowHandle(nullptr);
          return TRUE;
        }
      }
      break;
    }
    
    // Drop folders
    case WM_DROPFILES: {
      HDROP hDrop = reinterpret_cast<HDROP>(wParam);
      if (hDrop && index == PAGE_FOLDERS_ROOT) {
        WCHAR szFileName[MAX_PATH + 1];
        UINT nFiles = DragQueryFile(hDrop, static_cast<UINT>(-1), nullptr, 0);
        CListView List = GetDlgItem(IDC_LIST_FOLDERS_ROOT);
        for  (UINT i = 0; i < nFiles; i++) {
          ZeroMemory(szFileName, MAX_PATH + 1);
          DragQueryFile(hDrop, i, (LPWSTR)szFileName, MAX_PATH + 1);
          if (GetFileAttributes(szFileName) & FILE_ATTRIBUTE_DIRECTORY) {
            List.InsertItem(List.GetItemCount(), -1, ICON16_FOLDER, 0, nullptr, szFileName, 0);
            List.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
          }
        }
        List.SetWindowHandle(nullptr);
        return TRUE;
      }
      break;
    }
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}