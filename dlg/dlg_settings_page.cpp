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

void AddTorrentFilterToList(HWND hwnd_list, HTREEITEM hInsertAfter, const CFeedFilter& filter, bool enabled, bool expand);

// =============================================================================

CSettingsPage::CSettingsPage() :
  m_hTreeItem(NULL)
{
}

void CSettingsPage::CreateItem(LPCWSTR pszText, HTREEITEM htiParent) {
  m_hTreeItem = SettingsWindow.m_Tree.InsertItem(pszText, reinterpret_cast<LPARAM>(this), htiParent);
}

void CSettingsPage::Select() {
  SettingsWindow.m_Tree.SelectItem(m_hTreeItem);
}

// =============================================================================

BOOL CSettingsPage::OnInitDialog() {
  switch (Index) {
    // Account
    case PAGE_ACCOUNT: {
      SetDlgItemText(IDC_EDIT_USER, Settings.Account.MAL.User.c_str());
      SetDlgItemText(IDC_EDIT_PASS, Settings.Account.MAL.Password.c_str());
      CheckDlgButton(IDC_RADIO_API1 + Settings.Account.MAL.API - 1, TRUE);
      CheckDlgButton(IDC_CHECK_START_LOGIN, Settings.Account.MAL.AutoLogin);
      break;
    }
    // Account > Update
    case PAGE_UPDATE: {
      CheckDlgButton(IDC_RADIO_UPDATE_MODE1 + Settings.Account.Update.Mode - 1, TRUE);
      CheckDlgButton(IDC_RADIO_UPDATE_TIME1 + Settings.Account.Update.Time - 1, TRUE);
      SendDlgItemMessage(IDC_SPIN_DELAY, UDM_SETRANGE32, 0, 3600);
      SendDlgItemMessage(IDC_SPIN_DELAY, UDM_SETPOS32, 0, Settings.Account.Update.Delay);
      CheckDlgButton(IDC_CHECK_UPDATE_CHECKMP, Settings.Account.Update.CheckPlayer);
      CheckDlgButton(IDC_CHECK_UPDATE_RANGE, Settings.Account.Update.OutOfRange);
      break;
    }
              
    // ================================================================================

    // Anime folders > Root
    case PAGE_FOLDERS_ROOT: {
      CListView List = GetDlgItem(IDC_LIST_FOLDERS_ROOT);
      List.InsertColumn(0, 0, 0, 0, L"Folder");
      List.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
      List.SetExtendedStyle(LVS_EX_DOUBLEBUFFER);
      List.SetImageList(UI.ImgList16.GetHandle());
      List.SetTheme();
      for (size_t i = 0; i < Settings.Folders.Root.size(); i++) {
        List.InsertItem(i, -1, Icon16_Folder, 0, NULL, Settings.Folders.Root[i].c_str(), NULL);
      }
      List.SetWindowHandle(NULL);
      CheckDlgButton(IDC_CHECK_FOLDERS_WATCH, Settings.Folders.WatchEnabled);
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
      for (int i = 1; i <= AnimeList.Count; i++) {
        List.InsertItem(i - 1, AnimeList.Item[i].GetStatus(), 
                StatusToIcon(AnimeList.Item[i].GetAiringStatus()), 
                0, NULL, LPSTR_TEXTCALLBACK, 
                reinterpret_cast<LPARAM>(&AnimeList.Item[i]));
        List.SetItem(i - 1, 1, AnimeList.Item[i].Folder.c_str());
      }
      List.Sort(0, 1, 0, ListViewCompareProc);
      List.SetWindowHandle(NULL);
      break;
    }

    // ================================================================================

    // Announcements > HTTP
    case PAGE_HTTP: {
      CheckDlgButton(IDC_CHECK_HTTP, Settings.Announce.HTTP.Enabled);
      SetDlgItemText(IDC_EDIT_HTTP_URL, Settings.Announce.HTTP.URL.c_str());
      break;
    }
      // Announcements > Messenger
    case PAGE_MESSENGER: {
      CheckDlgButton(IDC_CHECK_MESSENGER, Settings.Announce.MSN.Enabled);
      break;
    }
    // Announcements > mIRC
    case PAGE_MIRC: {
      CheckDlgButton(IDC_CHECK_MIRC, Settings.Announce.MIRC.Enabled);
      CheckDlgButton(IDC_CHECK_MIRC_MULTISERVER, Settings.Announce.MIRC.MultiServer);
      CheckDlgButton(IDC_CHECK_MIRC_ACTION, Settings.Announce.MIRC.UseAction);
      SetDlgItemText(IDC_EDIT_MIRC_SERVICE, Settings.Announce.MIRC.Service.c_str());
      CheckDlgButton(IDC_RADIO_MIRC_CHANNEL1 + Settings.Announce.MIRC.Mode - 1, TRUE);
      SetDlgItemText(IDC_EDIT_MIRC_CHANNELS, Settings.Announce.MIRC.Channels.c_str());
      break;
    }
    // Announcements > Skype
    case PAGE_SKYPE: {
      CheckDlgButton(IDC_CHECK_SKYPE, Settings.Announce.Skype.Enabled);
      break;
    }
    // Announcements > Twitter
    case PAGE_TWITTER: {
      CheckDlgButton(IDC_CHECK_TWITTER, Settings.Announce.Twitter.Enabled);
      SettingsWindow.RefreshTwitterLink();
      break;
    }

    // ================================================================================

    // Program > General
    case PAGE_PROGRAM: {
      CheckDlgButton(IDC_CHECK_AUTOSTART, Settings.Program.General.AutoStart);
      CheckDlgButton(IDC_CHECK_GENERAL_CLOSE, Settings.Program.General.Close);
      CheckDlgButton(IDC_CHECK_GENERAL_MINIMIZE, Settings.Program.General.Minimize);
      vector<wstring> theme_list;
      PopulateFolders(theme_list, Taiga.GetDataPath() + L"Theme\\");
      if (theme_list.empty()) {
        EnableDlgItem(IDC_COMBO_THEME, FALSE);
      } else {
        for (size_t i = 0; i < theme_list.size(); i++) {
          AddComboString(IDC_COMBO_THEME, theme_list[i].c_str());
          if (IsEqual(theme_list[i], Settings.Program.General.Theme)) {
            SetComboSelection(IDC_COMBO_THEME, i);
          }
        }
      }
      CheckDlgButton(IDC_CHECK_START_VERSION, Settings.Program.StartUp.CheckNewVersion);
      CheckDlgButton(IDC_CHECK_START_CHECKEPS, Settings.Program.StartUp.CheckNewEpisodes);
      CheckDlgButton(IDC_CHECK_START_MINIMIZE, Settings.Program.StartUp.Minimize);
      CheckDlgButton(IDC_CHECK_EXIT_ASK, Settings.Program.Exit.Ask);
      CheckDlgButton(IDC_CHECK_EXIT_SAVEBUFFER, Settings.Program.Exit.SaveBuffer);
      SetDlgItemText(IDC_EDIT_PROXY_HOST, Settings.Program.Proxy.Host.c_str());
      SetDlgItemText(IDC_EDIT_PROXY_USER, Settings.Program.Proxy.User.c_str());
      SetDlgItemText(IDC_EDIT_PROXY_PASS, Settings.Program.Proxy.Password.c_str());
      break;
    }
    // Program > List
    case PAGE_LIST: {
      AddComboString(IDC_COMBO_DBLCLICK, L"Do nothing");
      AddComboString(IDC_COMBO_DBLCLICK, L"Edit details");
      AddComboString(IDC_COMBO_DBLCLICK, L"Open folder");
      AddComboString(IDC_COMBO_DBLCLICK, L"Play next episode");
      AddComboString(IDC_COMBO_DBLCLICK, L"View anime info");
      SetComboSelection(IDC_COMBO_DBLCLICK, Settings.Program.List.DoubleClick);
      AddComboString(IDC_COMBO_MDLCLICK, L"Do nothing");
      AddComboString(IDC_COMBO_MDLCLICK, L"Edit details");
      AddComboString(IDC_COMBO_MDLCLICK, L"Open folder");
      AddComboString(IDC_COMBO_MDLCLICK, L"Play next episode");
      AddComboString(IDC_COMBO_MDLCLICK, L"View anime info");
      SetComboSelection(IDC_COMBO_MDLCLICK, Settings.Program.List.MiddleClick);
      CheckDlgButton(IDC_CHECK_FILTER_NEWEPS, AnimeList.Filter.NewEps);
      CheckDlgButton(IDC_CHECK_HIGHLIGHT, Settings.Program.List.Highlight);
      CheckDlgButton(IDC_RADIO_LIST_PROGRESS1 + Settings.Program.List.ProgressMode, TRUE);
      CheckDlgButton(IDC_CHECK_LIST_PROGRESS_EPS, Settings.Program.List.ProgressShowEps);
      break;
    }
    // Program > Notifications
    case PAGE_NOTIFICATIONS: {
      CheckDlgButton(IDC_CHECK_BALLOON, Settings.Program.Balloon.Enabled);
      break;
    }

    // ================================================================================

    // Recognition > Media players
    case PAGE_MEDIA: {
      CListView List = GetDlgItem(IDC_LIST_MEDIA);
      List.InsertColumn(0, 0, 0, 0, L"Supported players");
      List.InsertGroup(0, L"By title");
      List.InsertGroup(1, L"By handle");
      List.InsertGroup(2, L"By API");
      List.InsertGroup(3, L"By message");
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
      for (size_t i = 0; i < MediaPlayers.Item.size(); i++) {
        BOOL player_available = MediaPlayers.Item[i].GetPath().empty() ? FALSE : TRUE;
        List.InsertItem(i, MediaPlayers.Item[i].Mode, 
          Icon16_AppGray - player_available, 0, NULL, 
          MediaPlayers.Item[i].Name.c_str(), NULL);
        if (MediaPlayers.Item[i].Enabled) List.SetCheckState(i, TRUE);
      }
      Header.SetWindowHandle(NULL);
      List.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
      List.SetWindowHandle(NULL);
      break;
    }

    // ================================================================================

    // Torrent > Options
    case PAGE_TORRENT1: {
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"http://tokyotosho.info/rss.php?filter=1,11&zwnj=0");
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"http://www.animesuki.com/rss.php?link=enclosure");
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"http://www.baka-updates.com/rss.php");
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"http://www.nyaa.eu/?page=rss&catid=1&subcat=37&filter=2");
      SetDlgItemText(IDC_COMBO_TORRENT_SOURCE, Settings.RSS.Torrent.Source.c_str());
      CheckDlgButton(IDC_CHECK_TORRENT_HIDE, Settings.RSS.Torrent.HideUnidentified);
      CheckDlgButton(IDC_CHECK_TORRENT_AUTOCHECK, Settings.RSS.Torrent.CheckEnabled);
      SendDlgItemMessage(IDC_SPIN_TORRENT_INTERVAL, UDM_SETRANGE32, 0, 3600);
      SendDlgItemMessage(IDC_SPIN_TORRENT_INTERVAL, UDM_SETPOS32, 0, Settings.RSS.Torrent.CheckInterval);
      CheckDlgButton(IDC_RADIO_TORRENT_NEW1 + Settings.RSS.Torrent.NewAction - 1, TRUE);
      CheckDlgButton(IDC_RADIO_TORRENT_APP1 + Settings.RSS.Torrent.AppMode - 1, TRUE);
      SetDlgItemText(IDC_EDIT_TORRENT_APP, Settings.RSS.Torrent.AppPath.c_str());
      EnableDlgItem(IDC_EDIT_TORRENT_APP, Settings.RSS.Torrent.AppMode > 1);
      EnableDlgItem(IDC_BUTTON_TORRENT_BROWSE, Settings.RSS.Torrent.AppMode > 1);
      CheckDlgButton(IDC_CHECK_TORRENT_AUTOSETFOLDER, Settings.RSS.Torrent.SetFolder);
      break;
    }
    // Torrent > Filters
    case PAGE_TORRENT2: {
      CheckDlgButton(IDC_CHECK_TORRENT_FILTERGLOBAL, Settings.RSS.Torrent.Filters.GlobalEnabled);
      CTreeView Tree = GetDlgItem(IDC_TREE_TORRENT_FILTERGLOBAL);
      Tree.SetStyle(TVS_CHECKBOXES, 0);
      Tree.SetTheme();
      SettingsWindow.m_FeedFilters.resize(Aggregator.FilterManager.Filters.size());
      std::copy(Aggregator.FilterManager.Filters.begin(), Aggregator.FilterManager.Filters.end(), 
        SettingsWindow.m_FeedFilters.begin());
      for (auto it = SettingsWindow.m_FeedFilters.begin(); it != SettingsWindow.m_FeedFilters.end(); ++it) {
        AddTorrentFilterToList(Tree.GetWindowHandle(), NULL, *it, it->Enabled, false);
      }
      Tree.SetWindowHandle(NULL);
      break;
    }
  }
  
  return TRUE;
}

// =============================================================================

BOOL CSettingsPage::OnCommand(WPARAM wParam, LPARAM lParam) {
  if (HIWORD(wParam) == BN_CLICKED) {
    switch (LOWORD(wParam)) {
      // Edit format
      case IDC_BUTTON_FORMAT_HTTP: {
        FormatWindow.Mode = FORMAT_MODE_HTTP;
        FormatWindow.Create(IDD_FORMAT, SettingsWindow.GetWindowHandle(), true);
        return TRUE;
      }
      case IDC_BUTTON_FORMAT_MSN: {
        FormatWindow.Mode = FORMAT_MODE_MESSENGER;
        FormatWindow.Create(IDD_FORMAT, SettingsWindow.GetWindowHandle(), true);
        return TRUE;
      }
      case IDC_BUTTON_FORMAT_MIRC: {
        FormatWindow.Mode = FORMAT_MODE_MIRC;
        FormatWindow.Create(IDD_FORMAT, SettingsWindow.GetWindowHandle(), true);
        return TRUE;
      }
      case IDC_BUTTON_FORMAT_SKYPE: {
        FormatWindow.Mode = FORMAT_MODE_SKYPE;
        FormatWindow.Create(IDD_FORMAT, SettingsWindow.GetWindowHandle(), true);
        return TRUE;
      }
      case IDC_BUTTON_FORMAT_TWITTER: {
        FormatWindow.Mode = FORMAT_MODE_TWITTER;
        FormatWindow.Create(IDD_FORMAT, SettingsWindow.GetWindowHandle(), true);
        return TRUE;
      }
      case IDC_BUTTON_FORMAT_BALLOON: {
        FormatWindow.Mode = FORMAT_MODE_BALLOON;
        FormatWindow.Create(IDD_FORMAT, SettingsWindow.GetWindowHandle(), true);
        return TRUE;
      }

      // ================================================================================

      // Add folders
      case IDC_BUTTON_ADDFOLDER: {
        wstring path;
        if (BrowseForFolder(m_hWindow, L"Please select a folder:", 
          BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON, path)) {
            CListView List = GetDlgItem(IDC_LIST_FOLDERS_ROOT);
            List.InsertItem(List.GetItemCount(), -1, Icon16_Folder, 0, NULL, path.c_str(), 0);
            List.SetSelectedItem(List.GetItemCount() - 1);
            List.SetWindowHandle(NULL);
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
        List.SetWindowHandle(NULL);
        return TRUE;
      }

      // ================================================================================

      // Test DDE connection
      case IDC_BUTTON_MIRC_TEST: {
        wstring service;
        GetDlgItemText(IDC_EDIT_MIRC_SERVICE, service);
        TestMIRCConnection(service);
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
      case IDC_BUTTON_TORRENT_FILTERGLOBAL_ADD: {
        FeedFilterWindow.m_Filter.Name = L"Filter #" + 
          ToWSTR((int)SettingsWindow.m_FeedFilters.size() + 1);
        FeedFilterWindow.m_Filter.Conditions.clear();
        FeedFilterWindow.Create(IDD_FEED_FILTER, SettingsWindow.GetWindowHandle());
        if (!FeedFilterWindow.m_Filter.Conditions.empty()) {
          SettingsWindow.m_FeedFilters.push_back(FeedFilterWindow.m_Filter);
          AddTorrentFilterToList(GetDlgItem(IDC_TREE_TORRENT_FILTERGLOBAL), 
            NULL, SettingsWindow.m_FeedFilters.back(), true, true);
        }
        return TRUE;
      }
      // Remove global filter
      case IDC_BUTTON_TORRENT_FILTERGLOBAL_DELETE: {
        CTreeView Tree = GetDlgItem(IDC_TREE_TORRENT_FILTERGLOBAL);
        HTREEITEM hti = Tree.GetSelection();
        CFeedFilter* pFeedFilter = reinterpret_cast<CFeedFilter*>(Tree.GetItemData(hti));
        for (auto it = SettingsWindow.m_FeedFilters.begin(); it != SettingsWindow.m_FeedFilters.end(); ++it) {
          if (pFeedFilter == &(*it)) {
            SettingsWindow.m_FeedFilters.erase(it); // BUG: pointers to following items change, hti.lparam remains the same
            break;
          }
        }
        EnableDlgItem(IDC_BUTTON_TORRENT_FILTERGLOBAL_DELETE, FALSE);
        Tree.DeleteItem(hti);
        Tree.SelectItem(NULL);
        Tree.SetWindowHandle(NULL);
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
  }

  return FALSE;
}

// =============================================================================

#define TVN_CUSTOM_CHECK (WM_USER + 100)

INT_PTR CSettingsPage::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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
            // Check TreeView item
            case IDC_TREE_TORRENT_FILTERGLOBAL: {
              HWND hTree = GetDlgItem(IDC_TREE_TORRENT_FILTERGLOBAL);
              TVHITTESTINFO hti = {0};
              GetCursorPos(&hti.pt);
              ScreenToClient(hTree, &hti.pt);
              TreeView_HitTest(hTree, &hti);
              if (hti.hItem && (hti.flags & TVHT_ONITEMSTATEICON)) {
                PostMessage(TVN_CUSTOM_CHECK, 0, reinterpret_cast<LPARAM>(hti.hItem));
              }
              return TRUE;
            }
          }
        }

        // List item select
        case LVN_ITEMCHANGED: {
          LPNMLISTVIEW lplv = reinterpret_cast<LPNMLISTVIEW>(lParam);
          if (lplv->hdr.hwndFrom == GetDlgItem(IDC_LIST_FOLDERS_ROOT)) {
            EnableDlgItem(IDC_BUTTON_REMOVEFOLDER, lplv->uNewState != 0);
          }
          break;
        }

        // Text callback
        case LVN_GETDISPINFO: {
          NMLVDISPINFO* plvdi = reinterpret_cast<NMLVDISPINFO*>(lParam);
          CAnime* pItem = reinterpret_cast<CAnime*>(plvdi->item.lParam);
          if (!pItem) break;
          switch (plvdi->item.iSubItem) {
            case 0: // Anime title
              plvdi->item.pszText = const_cast<LPWSTR>(pItem->Series_Title.data());
              break;
          }
          break;
        }

        // Double click
        case NM_DBLCLK: {
          LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
          if (lpnmitem->iItem == -1) break;
          if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_FOLDERS_ROOT)) {
            CListView List = lpnmitem->hdr.hwndFrom;
            WCHAR buffer[MAX_PATH];
            List.GetItemText(lpnmitem->iItem, 0, buffer);
            Execute(buffer);
            List.SetWindowHandle(NULL);
          } else if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_FOLDERS_ANIME)) {
            CListView List = lpnmitem->hdr.hwndFrom;
            wstring path, title;
            List.GetItemText(lpnmitem->iItem, 0, title);
            title = L"Anime title: " + title;
            if (BrowseForFolder(m_hWindow, title.c_str(), 
                BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON, path)) {
                  List.SetItem(lpnmitem->iItem, 1, path.c_str());
            }
            List.SetWindowHandle(NULL);
          } else if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_MEDIA)) {
            Execute(MediaPlayers.Item[lpnmitem->iItem].GetPath());
          } else if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_TREE_TORRENT_FILTERGLOBAL)) {
            CTreeView Tree = lpnmitem->hdr.hwndFrom;
            HTREEITEM hti = Tree.GetSelection();
            if (hti) {
              CFeedFilter* pFeedFilter = reinterpret_cast<CFeedFilter*>(Tree.GetItemData(hti));
              if (pFeedFilter) {
                FeedFilterWindow.m_Filter = *pFeedFilter;
                FeedFilterWindow.Create(IDD_FEED_FILTER, SettingsWindow.GetWindowHandle());
                if (!FeedFilterWindow.m_Filter.Conditions.empty()) {
                  *pFeedFilter = FeedFilterWindow.m_Filter;
                  AddTorrentFilterToList(lpnmitem->hdr.hwndFrom, hti, *pFeedFilter, true, true);
                  Tree.DeleteItem(hti);
                }
              } else {
                // TODO: Edit condition
              }
            }
            Tree.SetWindowHandle(NULL);
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
              CAnime* pItem = reinterpret_cast<CAnime*>(List.GetItemParam(lpnmitem->iItem));
              if (pItem) {
                pItem->CheckFolder();
                if (!pItem->Folder.empty()) {
                  List.SetItem(lpnmitem->iItem, 1, pItem->Folder.c_str());
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
          List.SetWindowHandle(NULL);
          return TRUE;
        }

        // Capture space key on TreeView
        case TVN_KEYDOWN: {
          LPNMTVKEYDOWN pnmtv = reinterpret_cast<LPNMTVKEYDOWN>(lParam);
          if (pnmtv->hdr.idFrom == IDC_TREE_TORRENT_FILTERGLOBAL) {
            if (pnmtv->wVKey == VK_SPACE) {
              CTreeView Tree = GetDlgItem(IDC_TREE_TORRENT_FILTERGLOBAL);
              HTREEITEM hti = Tree.GetSelection();
              PostMessage(TVN_CUSTOM_CHECK, 0, reinterpret_cast<LPARAM>(hti));
              Tree.SetWindowHandle(NULL);
            }
          }
          return TRUE;
        }

        // TreeView item selection
        case TVN_SELCHANGED: {
          LPNMTREEVIEW pnmtv = reinterpret_cast<LPNMTREEVIEW>(lParam);
          if (pnmtv->hdr.hwndFrom == GetDlgItem(IDC_TREE_TORRENT_FILTERGLOBAL)) {
            EnableDlgItem(IDC_BUTTON_TORRENT_FILTERGLOBAL_DELETE, pnmtv->itemNew.lParam != NULL);
          }
          return TRUE;
        }
      }
      break;
    }

    // Handle TreeView checkbox
    case TVN_CUSTOM_CHECK: {
      HTREEITEM hti = reinterpret_cast<HTREEITEM>(lParam);
      if (hti) {
        CTreeView Tree = GetDlgItem(IDC_TREE_TORRENT_FILTERGLOBAL);
        CFeedFilter* pFeedFilter = reinterpret_cast<CFeedFilter*>(Tree.GetItemData(hti));
        if (pFeedFilter) {
          pFeedFilter->Enabled = Tree.GetCheckState(hti) == 1;
        } else {
          Tree.SetCheckState(hti, -1);
        }
        Tree.SetWindowHandle(NULL);
      }
      break;
    }
    
    // Drop folders
    case WM_DROPFILES: {
      HDROP hDrop = reinterpret_cast<HDROP>(wParam);
      if (hDrop && Index == PAGE_FOLDERS_ROOT) {
        WCHAR szFileName[MAX_PATH + 1];
        UINT nFiles = DragQueryFile(hDrop, static_cast<UINT>(-1), NULL, 0);
        CListView List = GetDlgItem(IDC_LIST_FOLDERS_ROOT);
        for  (UINT i = 0; i < nFiles; i++) {
          ZeroMemory(szFileName, MAX_PATH + 1);
          DragQueryFile(hDrop, i, (LPWSTR)szFileName, MAX_PATH + 1);
          if (GetFileAttributes(szFileName) & FILE_ATTRIBUTE_DIRECTORY) {
            List.InsertItem(List.GetItemCount(), -1, Icon16_Folder, 0, NULL, szFileName, 0);
            List.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
          }
        }
        List.SetWindowHandle(NULL);
        return TRUE;
      }
      break;
    }
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

// =============================================================================

void AddTorrentFilterToList(HWND hwnd_list, HTREEITEM hInsertAfter, const CFeedFilter& filter, bool enabled, bool expand) {
  CTreeView Tree = hwnd_list;
  
  // Insert item
  HTREEITEM hti = Tree.InsertItem(filter.Name.c_str(), reinterpret_cast<LPARAM>(&filter), NULL, hInsertAfter);
  
  // Insert conditions
  for (auto it = filter.Conditions.begin(); it != filter.Conditions.end(); ++it) {
    HTREEITEM htc = Tree.InsertItem(Aggregator.FilterManager.TranslateCondition(*it).c_str(), NULL, hti);
    Tree.SetCheckState(htc, -1);
  }

  Tree.SetCheckState(hti, enabled);
  if (expand) Tree.Expand(hti);
  Tree.SetWindowHandle(NULL);
}