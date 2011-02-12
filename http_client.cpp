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

#include "std.h"
#include "animelist.h"
#include "announce.h"
#include "common.h"
#include "dlg/dlg_anime_info.h"
#include "dlg/dlg_anime_info_page.h"
#include "dlg/dlg_main.h"
#include "dlg/dlg_search.h"
#include "dlg/dlg_settings.h"
#include "dlg/dlg_torrent.h"
#include "dlg/dlg_event.h"
#include "dlg/dlg_input.h"
#include "event.h"
#include "http.h"
#include "myanimelist.h"
#include "resource.h"
#include "rss.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"
#include "theme.h"
#include "win32/win_taskbar.h"
#include "win32/win_taskdialog.h"

CHTTPClient MainClient, ImageClient, SearchClient, TwitterClient, VersionClient;

// =============================================================================

BOOL CHTTPClient::OnError(DWORD dwError) {
  wstring error_text = L"HTTP error #" + ToWSTR(dwError) + L": " + 
    FormatError(dwError, L"winhttp.dll");

  switch (GetClientMode()) {
    case HTTP_MAL_Login:
    case HTTP_MAL_RefreshList:
    case HTTP_MAL_RefreshAndLogin:
      MainWindow.ChangeStatus(error_text);
      MainWindow.m_Toolbar.EnableButton(0, true);
      MainWindow.m_Toolbar.EnableButton(1, true);
      break;
    case HTTP_MAL_AnimeDetails:
    case HTTP_MAL_Image:
      break;
    case HTTP_MAL_SearchAnime:
      SearchWindow.EnableDlgItem(IDOK, TRUE);
      SearchWindow.SetDlgItemText(IDOK, L"Search");
      break;
    case HTTP_TorrentCheck:
    case HTTP_TorrentDownload:
    case HTTP_TorrentDownloadAll:
      TorrentWindow.ChangeStatus(error_text);
      TorrentWindow.m_Toolbar.EnableButton(0, true);
      TorrentWindow.m_Toolbar.EnableButton(1, true);
      break;
    case HTTP_VersionCheckSilent:
      break;
    default:
      EventQueue.UpdateInProgress = false;
      MainWindow.ChangeStatus(error_text);
      MainWindow.m_Toolbar.EnableButton(1, true);
      break;
  }
  
  TaskbarList.SetProgressState(TBPF_NOPROGRESS);
  return 0;
}

BOOL CHTTPClient::OnSendRequestComplete() {
  wstring status = L"Connecting...";
  
  switch (GetClientMode()) {
    case HTTP_TorrentCheck:
    case HTTP_TorrentDownload:
    case HTTP_TorrentDownloadAll:
      TorrentWindow.ChangeStatus(status);
      break;
    case HTTP_VersionCheck:
      MainWindow.ChangeStatus(L"Checking for updates...");
    default:
      #ifdef _DEBUG
      MainWindow.ChangeStatus(status);
      #endif
      break;
  }

  return 0;
}

BOOL CHTTPClient::OnHeadersAvailable(wstring headers) {
  if (GetClientMode() != HTTP_Silent) {
    TaskbarList.SetProgressState(m_dwTotal > 0 ? TBPF_NORMAL : TBPF_INDETERMINATE);
  }
  return 0;
}

BOOL CHTTPClient::OnRedirect(wstring address) {
  wstring status = L"Redirecting... (" + address + L")";
  
  switch (GetClientMode()) {
    case HTTP_TorrentCheck:
    case HTTP_TorrentDownload:
    case HTTP_TorrentDownloadAll:
      TorrentWindow.ChangeStatus(status);
      break;
    default:
      #ifdef _DEBUG
      MainWindow.ChangeStatus(status);
      #endif
      break;
  }

  return 0;
}

BOOL CHTTPClient::OnReadData() {
  wstring status;

  switch (GetClientMode()) {
    case HTTP_MAL_RefreshList:
    case HTTP_MAL_RefreshAndLogin:
      status = L"Downloading anime list...";
      break;
    case HTTP_MAL_Login:
      status = L"Reading account information...";
      break;
    case HTTP_MAL_AnimeAdd:
    case HTTP_MAL_AnimeEdit:
    case HTTP_MAL_AnimeUpdate:
    case HTTP_MAL_ScoreUpdate:
    case HTTP_MAL_StatusUpdate:
    case HTTP_MAL_TagUpdate:
      status = L"Updating list...";
      break;
    case HTTP_MAL_AnimeDetails:
    case HTTP_MAL_SearchAnime:
    case HTTP_MAL_Image:
    case HTTP_Silent:
    case HTTP_VersionCheck:
    case HTTP_VersionCheckSilent:
      return 0;
    case HTTP_TorrentCheck:
      status = L"Checking new torrents...";
      break;
    case HTTP_TorrentDownload:
    case HTTP_TorrentDownloadAll:
      status = L"Downloading torrent file...";
      break;
    case HTTP_Twitter_Request:
      status = L"Connecting to Twitter...";
      break;
    case HTTP_Twitter_Auth:
      status = L"Authorizing Twitter...";
      break;
    case HTTP_Twitter_Post:
      status = L"Updating Twitter status...";
      break;
    default:
      status = L"Downloading data...";
      break;
  }

  if (m_dwTotal > 0) {
    TaskbarList.SetProgressValue(m_dwDownloaded, m_dwTotal);
    status += L" (" + ToWSTR(static_cast<int>(((float)m_dwDownloaded / (float)m_dwTotal) * 100)) + L"%)";
  } else {
    status += L" (" + ToSizeString(m_dwDownloaded) + L")";
  }

  switch (GetClientMode()) {
    case HTTP_TorrentCheck:
    case HTTP_TorrentDownload:
    case HTTP_TorrentDownloadAll:
      TorrentWindow.ChangeStatus(status);
      break;
    default:
      MainWindow.ChangeStatus(status);
      break;
  }

  return 0;
}

BOOL CHTTPClient::OnReadComplete() {
  CAnime* pItem = reinterpret_cast<CAnime*>(GetParam());
  TaskbarList.SetProgressState(TBPF_NOPROGRESS);
  wstring status;
  
  switch (GetClientMode()) {
    // List
    case HTTP_MAL_RefreshList:
    case HTTP_MAL_RefreshAndLogin: {
      AnimeList.Read();
      MainWindow.ChangeStatus(L"List download completed.");
      MainWindow.RefreshList(MAL_WATCHING);
      MainWindow.RefreshTabs(MAL_WATCHING);
      SearchWindow.PostMessage(WM_CLOSE);
      ExecuteAction(L"CheckEpisodes()", TRUE);
      if (GetClientMode() == HTTP_MAL_RefreshList) {
        MainWindow.m_Toolbar.EnableButton(0, true);
        MainWindow.m_Toolbar.EnableButton(1, true);
      } else if (GetClientMode() == HTTP_MAL_RefreshAndLogin) {
        MAL.Login();
        return TRUE;
      }
      break;
    }

    // =========================================================================
    
    // Login
    case HTTP_MAL_Login: {
      switch (Settings.Account.MAL.API) {
        case MAL_API_OFFICIAL:
          Taiga.LoggedIn = InStr(GetData(), L"<username>" + Settings.Account.MAL.User + L"</username>", 0) > -1;
          break;
        case MAL_API_NONE:
          Taiga.LoggedIn = !(GetCookie().empty());
          break;
      }
      if (Taiga.LoggedIn) {
        status = L"Logged in as " + Settings.Account.MAL.User + L".";
        MainWindow.m_Toolbar.SetButtonImage(0, Icon24_Online);
        MainWindow.m_Toolbar.SetButtonText(0, L"Log out");
        MainWindow.m_Toolbar.SetButtonTooltip(0, L"Log out");
      } else {
        status = L"Failed to log in.";
        switch (Settings.Account.MAL.API) {
          case MAL_API_OFFICIAL:
            #ifdef _DEBUG
            status += L" (" + GetData() + L")";
            #else
            status += L" (Invalid user name or password)";
            #endif
            break;
          case MAL_API_NONE:
            if (InStr(GetData(), L"Could not find that username", 1) > -1) {
              status += L" (Invalid user name)";
            } else if (InStr(GetData(), L"Error: Invalid password", 1) > -1) {
              status += L" (Invalid password)";
            }
            break;
        }
      }
      MainWindow.m_Toolbar.EnableButton(0, true);
      MainWindow.m_Toolbar.EnableButton(1, true);
      MainWindow.ChangeStatus(status);
      MainWindow.RefreshMenubar();
      MainWindow.UpdateTip();
      switch (Settings.Account.MAL.API) {
        case MAL_API_OFFICIAL:
          EventQueue.Check();
          return TRUE;
        case MAL_API_NONE:
          MAL.CheckProfile();
          return TRUE;
      }
      break;
    }

    // =========================================================================

    // Check profile
    case HTTP_MAL_Profile: {
      status = L"You have no new messages.";
      if (Settings.Account.MAL.API == MAL_API_NONE) {
        int msg_pos = InStr(GetData(), L"<a href=\"/mymessages.php\">(", 0);
        if (msg_pos > -1) {
          int msg_end = InStr(GetData(), L")", msg_pos);
          if (msg_end > -1) {
            int msg_count = ToINT(GetData().substr(msg_pos + 27, msg_end));
            if (msg_count > 0) {
              status = L"You have " + ToWSTR(msg_count) + L" new message(s)!";
              CTaskDialog dlg;
              dlg.SetWindowTitle(APP_TITLE);
              dlg.SetMainIcon(TD_ICON_INFORMATION);
              dlg.SetMainInstruction(status.c_str());
              dlg.UseCommandLinks(true);
              dlg.AddButton(L"Check\nView your messages now", IDYES);
              dlg.AddButton(L"Cancel\nCheck them later", IDNO);
              dlg.Show(g_hMain);
              if (dlg.GetSelectedButtonID() == IDYES) {
                MAL.ViewMessages();
              }
            }
          }
        }
        MainWindow.ChangeStatus(status);
        EventQueue.Check();
        return TRUE;
      }
      break;
    }

    // =========================================================================

    // Delete anime
    case HTTP_MAL_AnimeDelete: {
      switch (Settings.Account.MAL.API) {
        case MAL_API_OFFICIAL:
          if (GetData() == L"Deleted") {
            if (pItem) {
              MainWindow.ChangeStatus(L"Item deleted. (" + pItem->Series_Title + L")");
              AnimeList.DeleteItem(pItem->Index);
            }
            MainWindow.RefreshList();
            MainWindow.RefreshTabs();
            SearchWindow.PostMessage(WM_CLOSE);
            EventQueue.Remove();
            EventQueue.Check();
            return TRUE;
          } else {
            MainWindow.ChangeStatus(L"Error: " + GetData());
          }
          break;
        case MAL_API_NONE:
          // TODO
          break;
      }
      break;
    }

    // =========================================================================

    // Update list
    case HTTP_MAL_AnimeAdd:
    case HTTP_MAL_AnimeEdit:
    case HTTP_MAL_AnimeUpdate:
    case HTTP_MAL_ScoreUpdate:
    case HTTP_MAL_StatusUpdate:
    case HTTP_MAL_TagUpdate: {
      EventQueue.UpdateInProgress = false;
      MainWindow.ChangeStatus();
      if (pItem && EventQueue.GetItemCount() > 0) {
        int user_index = EventQueue.GetUserIndex();
        if (user_index > -1) {
          pItem->Edit(GetData(), EventQueue.List[user_index].Item[EventQueue.List[user_index].Index]);
          return TRUE;
        }
      }
      break;
    }

    // =========================================================================

    // Anime details
    case HTTP_MAL_AnimeDetails: {
      wstring data = GetData();
      if (pItem && !data.empty()) {
        pItem->Genres = InStr(data, L"Genres:</span> ", L"<br />");
        pItem->Rank = InStr(data, L"Ranked:</span> ", L"<br />");
        pItem->Popularity = InStr(data, L"Popularity:</span> ", L"<br />");
        pItem->Score = InStr(data, L"Score:</span> ", L"<br />");
        StripHTML(pItem->Score);
        AnimeWindow.Refresh(pItem, true, false);
      }
      break;
    }

    // =========================================================================

    // Search anime
    case HTTP_MAL_SearchAnime: {
      if (pItem) {
        if (pItem->ParseSearchResult(GetData())) {
          AnimeWindow.Refresh(pItem, true, false);
          if (MAL.GetAnimeDetails(pItem)) return TRUE;
        } else {
          status = L"Could not read anime information.";
          AnimeWindow.m_Page[TAB_SERIESINFO].SetDlgItemText(IDC_EDIT_ANIME_INFO, status.c_str());
        }
        #ifdef _DEBUG
        MainWindow.ChangeStatus(status);
        #endif
      } else {
        if (SearchWindow.IsWindow()) {
          SearchWindow.ParseResults(GetData());
          SearchWindow.EnableDlgItem(IDOK, TRUE);
          SearchWindow.SetDlgItemText(IDOK, L"Search");
        }
      }
      break;
    }

    // =========================================================================

    // Download image
    case HTTP_MAL_Image: {
      AnimeWindow.Refresh(NULL, false, false);
      break;
    }

    // =========================================================================

    // Version check
    case HTTP_VersionCheck:
    case HTTP_VersionCheckSilent: {
      vector<wstring> data;
      Split(GetData(), L"\r\n", data);
      if (data.size() >= 3) {
        CTaskDialog dlg;
        dlg.SetWindowTitle(APP_TITLE);
        dlg.SetMainIcon(TD_ICON_INFORMATION);
        if (CompareStrings(data[1], APP_BUILD) > 0) {
          wstring content = L"Version: " + data[0] + L"\nBuild: " + data[1];
          wstring details;
          for (unsigned int i = 3; i < data.size(); i++) {
            details += (i > 4 && i < data.size() ? L"\n" : L"") + data[i];
          }
          TrimLeft(data[0], L"\uFEFF");
          if (data[0] != APP_VERSION) {
            dlg.SetMainInstruction(L"A new version of Taiga is available!");
          } else {
            dlg.SetMainInstruction(L"A new build of Taiga is available!");
          }
          dlg.SetContent(content.c_str());
          dlg.SetExpandedInformation(details.c_str());
          dlg.AddButton(L"Download", IDYES);
          dlg.AddButton(L"Cancel", IDNO);
          dlg.Show(g_hMain);
          if (dlg.GetSelectedButtonID() == IDYES) {
            ExecuteLink(data[2]);
          }
        } else {
          if (GetClientMode() != HTTP_VersionCheckSilent) {
            dlg.SetMainInstruction(L"No updates available. Taiga is up to date!");
            dlg.AddButton(L"OK", IDOK);
            dlg.Show(g_hMain);
          }
        }
      } else {
        status = L"Error reading version information.";
      }
      if (GetClientMode() != HTTP_VersionCheckSilent) {
        MainWindow.ChangeStatus(status);
      }
      break;
    }

    // =========================================================================

    // Torrent
    case HTTP_TorrentCheck: {
      Torrents.Read();
      int torrent_count = Torrents.Compare();
      if (torrent_count > 0) {
        status = L"There are new torrents available!";
      } else {
        status = L"No new torrents found.";
      }
      if (TorrentWindow.IsWindow()) {
        TorrentWindow.ChangeStatus(status);
        TorrentWindow.RefreshList();
        TorrentWindow.m_Toolbar.EnableButton(0, true);
        TorrentWindow.m_Toolbar.EnableButton(1, true);
      } else {
        switch (Settings.RSS.Torrent.NewAction) {
          // Notify
          case 1:
            Torrents.Tip();
            break;
          // Download
          case 2:
            if (Torrents.Download(-1)) return TRUE;
            break;
        }
      }
      break;
    }
    case HTTP_TorrentDownload:
    case HTTP_TorrentDownloadAll: {
      CRSSFeedItem* pFeed = reinterpret_cast<CRSSFeedItem*>(GetParam());
      if (pFeed) {
        wstring app_path, cmd, file = pFeed->Title + L".torrent";
        ValidateFileName(file);
        Torrents.Archive.push_back(file);
        file = Taiga.GetDataPath() + L"Torrents\\" + file;
        if (FileExists(file)) {
          switch (Settings.RSS.Torrent.NewAction) {
            // Default application
            case 1:
              app_path = GetDefaultAppPath(L".torrent", L"");
              break;
            // Custom application
            case 2:
              app_path = Settings.RSS.Torrent.AppPath;
              break;
          }
          if (Settings.RSS.Torrent.SetFolder && InStr(app_path, L"utorrent", 0, true) > -1) {
            if (pFeed->EpisodeData.Index > 0 && !AnimeList.Item[pFeed->EpisodeData.Index].Folder.empty()) {
              cmd = L"/directory \"" + AnimeList.Item[pFeed->EpisodeData.Index].Folder + L"\" ";
            }
          }
          cmd += L"\"" + file + L"\"";
          Execute(app_path, cmd);
          pFeed->Download = FALSE;
          TorrentWindow.RefreshList();
        }
      }
      if (GetClientMode() == HTTP_TorrentDownloadAll) {
        if (Torrents.Download(-1)) return TRUE;
      }
      TorrentWindow.ChangeStatus(L"Successfully downloaded all torrents.");
      TorrentWindow.m_Toolbar.EnableButton(0, true);
      TorrentWindow.m_Toolbar.EnableButton(1, true);
      break;
    }

    // =========================================================================

    // Twitter
    case HTTP_Twitter_Request: {
      OAuthParameters response = Twitter.OAuth.ParseQueryString(GetData());
      if (!response[L"oauth_token"].empty()) {
        ExecuteLink(L"http://twitter.com/oauth/authorize?oauth_token=" + response[L"oauth_token"]);
        CInputDialog dlg;
        dlg.Title = L"Twitter Authorization";
        dlg.Info = L"Please enter the PIN shown on the page after logging into Twitter:";
        dlg.Show();
        if (dlg.Result == IDOK && !dlg.Text.empty()) {
          Twitter.AccessToken(response[L"oauth_token"], response[L"oauth_token_secret"], dlg.Text);
          return TRUE;
        }
      }
      MainWindow.ChangeStatus();
      break;
    }
	case HTTP_Twitter_Auth: {
      OAuthParameters access = Twitter.OAuth.ParseQueryString(GetData());
      if (!access[L"oauth_token"].empty() && !access[L"oauth_token_secret"].empty()) {
        Settings.Announce.Twitter.OAuthKey = access[L"oauth_token"];
        Settings.Announce.Twitter.OAuthSecret = access[L"oauth_token_secret"];
        Settings.Announce.Twitter.User = access[L"screen_name"];
        status = L"Taiga is now authorized to post to this Twitter account: ";
        status += Settings.Announce.Twitter.User;
      } else {
        status = L"Twitter authorization failed.";
      }
      MainWindow.ChangeStatus(status);
      SettingsWindow.RefreshTwitterLink();
      break;
    }
    case HTTP_Twitter_Post: {
      if (InStr(GetData(), L"<error>", 0) == -1) {
        status = L"Twitter status updated.";
      } else {
        status = L"Twitter status update failed.";
        int index_begin = InStr(GetData(), L"<error>", 0);
        int index_end = InStr(GetData(), L"</error>", index_begin);
        if (index_begin > -1 && index_end > -1) {
          index_begin += 7;
          status += L" (" + GetData().substr(index_begin, index_end - index_begin) + L")";
        }
      }
      MainWindow.ChangeStatus(status);
      break;
    }

    // =========================================================================
    
    // Debug
    default: {
      #ifdef _DEBUG
      ::MessageBox(0, GetData().c_str(), 0, 0);
      #endif
    }
  }

  return FALSE;
}

