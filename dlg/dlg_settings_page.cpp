/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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
#include "dlg_torrent_filter.h"
#include "../http.h"
#include "../media.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../settings.h"
#include "../string.h"
#include "../taiga.h"
#include "../theme.h"

int TranslateFilterOption(const wstring& option_str);
int TranslateFilterType(const wstring& type_str, wstring& value);

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
        List.InsertItem(i, -1, Icon16_Folder, Settings.Folders.Root[i].c_str(), NULL);
      }
      List.SetWindowHandle(NULL);
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
          List.InsertGroup(i, MAL.TranslateMyStatus(i, false).c_str(), i != MAL_WATCHING);
        }
      }
      List.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
      List.SetImageList(UI.ImgList16.GetHandle());
      List.SetTheme();
      for (int i = 1; i <= AnimeList.Count; i++) {
        List.InsertItem(i - 1, AnimeList.Item[i].GetStatus(), 
                StatusToIcon(AnimeList.Item[i].Series_Status), 
                LPSTR_TEXTCALLBACK, 
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
      SetDlgItemText(IDC_EDIT_TWITTER_USER, Settings.Announce.Twitter.User.c_str());
      SetDlgItemText(IDC_EDIT_TWITTER_PASS, Settings.Announce.Twitter.Password.c_str());
      break;
    }

    // ================================================================================

    // Program > General
    case PAGE_PROGRAM: {
      CheckDlgButton(IDC_CHECK_AUTOSTART, Settings.Program.General.AutoStart);
      CheckDlgButton(IDC_CHECK_GENERAL_CLOSE, Settings.Program.General.Close);
      CheckDlgButton(IDC_CHECK_GENERAL_MINIMIZE, Settings.Program.General.Minimize);
      AddComboString(IDC_COMBO_THEME, Settings.Program.General.Theme.c_str()); // TEMP
      SetComboSelection(IDC_COMBO_THEME, 0); // TEMP
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
          Icon16_AppGray - player_available, 
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
      AddComboString(IDC_COMBO_TORRENT_SOURCE, L"http://www.nyaatorrents.org/?page=rss&catid=1&subcat=37&filter=2");
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
      CListView List = GetDlgItem(IDC_LIST_TORRENT_FILTERGLOBAL);
      List.InsertColumn(0, 220, 220, 0, L"Value");
      List.InsertColumn(1, 100, 100, 0, L"Option");
      List.InsertColumn(2, 100, 100, 0, L"Type");
      List.InsertGroup(0, L"Exclude", false);
      List.InsertGroup(1, L"Include", false);
      List.InsertGroup(2, L"Preference", false);
      List.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
      List.SetImageList(UI.ImgList16.GetHandle());
      List.SetTheme();
      for (size_t i = 0; i < Settings.RSS.Torrent.Filters.Global.size(); i++) {
        AddTorrentFilterToList(List.GetWindowHandle(), 
          Settings.RSS.Torrent.Filters.Global[i].Option, 
          Settings.RSS.Torrent.Filters.Global[i].Type, 
          Settings.RSS.Torrent.Filters.Global[i].Value);
      }
      List.Sort(0, 1, 0, ListViewCompareProc);
      List.SetWindowHandle(NULL);
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
            List.InsertItem(List.GetItemCount(), -1, Icon16_Folder, path.c_str(), 0);
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
        TorrentFilterWindow.SetValues(0, 0, L"");
        ExecuteAction(L"TorrentAddFilter", TRUE, 
          reinterpret_cast<LPARAM>(SettingsWindow.GetWindowHandle()));
        AddTorrentFilterToList(GetDlgItem(IDC_LIST_TORRENT_FILTERGLOBAL), 
          TorrentFilterWindow.m_Filter.Option, 
          TorrentFilterWindow.m_Filter.Type, 
          TorrentFilterWindow.m_Filter.Value);
        return TRUE;
      }
      // Remove global filter
      case IDC_BUTTON_TORRENT_FILTERGLOBAL_DELETE: {
        CListView List = GetDlgItem(IDC_LIST_TORRENT_FILTERGLOBAL);
        while (List.GetSelectedCount() > 0) {
          int list_index = List.GetNextItem(-1, LVNI_SELECTED);
          List.DeleteItem(list_index);
        }
        EnableDlgItem(IDC_BUTTON_TORRENT_FILTERGLOBAL_DELETE, FALSE);
        List.SetWindowHandle(NULL);
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

INT_PTR CSettingsPage::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_NOTIFY: {
      switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
        // Execute link
        case NM_CLICK:
        case NM_RETURN: {
          PNMLINK pNMLink = reinterpret_cast<PNMLINK>(lParam);
          if (CompareStrings(pNMLink->item.szID, L"id_link", false) == 0) {
            ExecuteLink(pNMLink->item.szUrl);
          } else {
            ExecuteAction(pNMLink->item.szID);
          }
          return TRUE;
        }

        // List item select
        case LVN_ITEMCHANGED: {
          LPNMLISTVIEW lplv = reinterpret_cast<LPNMLISTVIEW>(lParam);
          if (lplv->hdr.hwndFrom == GetDlgItem(IDC_LIST_FOLDERS_ROOT)) {
            EnableDlgItem(IDC_BUTTON_REMOVEFOLDER, lplv->uNewState != 0);
          } else if (lplv->hdr.hwndFrom == GetDlgItem(IDC_LIST_TORRENT_FILTERGLOBAL)) {
            EnableDlgItem(IDC_BUTTON_TORRENT_FILTERGLOBAL_DELETE, lplv->uNewState != 0);
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
          CListView List = lpnmitem->hdr.hwndFrom;
          if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_FOLDERS_ROOT)) {
            WCHAR buffer[MAX_PATH];
            List.GetItemText(lpnmitem->iItem, 0, buffer);
            Execute(buffer);
          } else if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_FOLDERS_ANIME)) {
            wstring path, title;
            List.GetItemText(lpnmitem->iItem, 0, title);
            title = L"Anime title: " + title;
            if (BrowseForFolder(m_hWindow, title.c_str(), 
                BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON, path)) {
                  List.SetItem(lpnmitem->iItem, 1, path.c_str());
            }
          } else if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_MEDIA)) {
            Execute(MediaPlayers.Item[lpnmitem->iItem].GetPath());
          } else if (lpnmitem->hdr.hwndFrom == GetDlgItem(IDC_LIST_TORRENT_FILTERGLOBAL)) {
            wstring option_str, type_str, value;
            int option = 0, type = 0;
            List.GetItemText(lpnmitem->iItem, 0, value);
            List.GetItemText(lpnmitem->iItem, 1, option_str);
            List.GetItemText(lpnmitem->iItem, 2, type_str);
            option = TranslateFilterOption(option_str);
            type = TranslateFilterType(type_str, value);
            TorrentFilterWindow.SetValues(option, type, value);
            ExecuteAction(L"TorrentAddFilter", TRUE, 
              reinterpret_cast<LPARAM>(SettingsWindow.GetWindowHandle()));
            if (!TorrentFilterWindow.m_Filter.Value.empty()) {
              List.DeleteItem(lpnmitem->iItem);
              AddTorrentFilterToList(lpnmitem->hdr.hwndFrom, 
                TorrentFilterWindow.m_Filter.Option, 
                TorrentFilterWindow.m_Filter.Type, 
                TorrentFilterWindow.m_Filter.Value);
            }
          }
          List.SetWindowHandle(NULL);
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
            List.InsertItem(List.GetItemCount(), -1, Icon16_Folder, szFileName, 0);
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

void AddTorrentFilterFromList(HWND hwnd_list, int item_index, vector<CRSSFilter>& filter_vector) {
  wstring option_str, type_str, value;
  int option = 0, type = 0;

  CListView List = hwnd_list;
  List.GetItemText(item_index, 0, value);
  List.GetItemText(item_index, 1, option_str);
  List.GetItemText(item_index, 2, type_str);
  List.SetWindowHandle(NULL);

  option = TranslateFilterOption(option_str);
  type = TranslateFilterType(type_str, value);

  filter_vector.resize(filter_vector.size() + 1);
  filter_vector.back().Option = option;
  filter_vector.back().Type = type;
  filter_vector.back().Value = value;
}

int TranslateFilterOption(const wstring& option_str) {
  if (option_str == L"Exclude") {
    return 0;
  } else if (option_str == L"Include") {
    return 1;
  } else if (option_str == L"Preference") {
    return 2;
  }
  return 0;
}

int TranslateFilterType(const wstring& type_str, wstring& value) {
  int type = 0;
  if (type_str == L"Keyword") {
    type = 0;
  } else if (type_str == L"Airing status") {
    type = 1;
    value = ToWSTR(MAL.TranslateStatus(value));
  } else if (type_str == L"Watching status") {
    type = 2;
    value = ToWSTR(MAL.TranslateMyStatus(value));
  }
  return type;
}

void AddTorrentFilterToList(HWND hwnd_list, int option, int type, wstring value) {
  if (value.empty()) return;
  wstring option_str, type_str;
  int icon = -1;
  
  switch (option) {
    case 0:
      option_str = L"Exclude";
      icon = Icon16_Minus;
      break;
    case 1:
      option_str = L"Include";
      icon = Icon16_Plus;
      break;
    case 2:
      option_str = L"Preference";
      icon = Icon16_TickSmall;
      break;
  }
  switch (type) {
    case 0:
      type_str = L"Keyword";
      break;
    case 1:
      type_str = L"Airing status";
      value = MAL.TranslateStatus(ToINT(value));
      break;
    case 2:
      type_str = L"Watching status";
      value = MAL.TranslateMyStatus(ToINT(value), false);
      break;
  }

  CListView List = hwnd_list;
  List.InsertItem(List.GetItemCount(), option, icon, value.c_str(), NULL);
  List.SetItem(List.GetItemCount() - 1, 1, option_str.c_str());
  List.SetItem(List.GetItemCount() - 1, 2, type_str.c_str());
  List.Sort(0, 1, 0, ListViewCompareProc);
  List.SetWindowHandle(NULL);
}