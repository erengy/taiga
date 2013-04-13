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

#include "std.h"

#include "http.h"

#include "anime_db.h"
#include "announce.h"
#include "common.h"
#include "debug.h"
#include "feed.h"
#include "history.h"
#include "myanimelist.h"
#include "resource.h"
#include "settings.h"
#include "stats.h"
#include "string.h"
#include "taiga.h"
#include "theme.h"
#include "version.h"

#include "dlg/dlg_anime_info.h"
#include "dlg/dlg_anime_info_page.h"
#include "dlg/dlg_anime_list.h"
#include "dlg/dlg_history.h"
#include "dlg/dlg_input.h"
#include "dlg/dlg_main.h"
#include "dlg/dlg_search.h"
#include "dlg/dlg_season.h"
#include "dlg/dlg_settings.h"
#include "dlg/dlg_torrent.h"
#include "dlg/dlg_update.h"

#include "win32/win_taskbar.h"
#include "win32/win_taskdialog.h"

class HttpClient HttpClient, ImageClient, MainClient, SearchClient, TwitterClient, VersionClient;

// =============================================================================

HttpClient::HttpClient() {
  SetAutoRedirect(FALSE);
  SetUserAgent(APP_NAME L"/" + ToWstr(VERSION_MAJOR) + L"." + ToWstr(VERSION_MINOR));
}

BOOL HttpClient::OnError(DWORD dwError) {
  wstring error_text = L"HTTP error #" + ToWstr(dwError) + L": " + 
    FormatError(dwError, L"winhttp.dll");
  debug::Print(error_text + L"Client mode: " + ToWstr(GetClientMode()) + L"\n");

  Stats.connections_failed++;

  switch (GetClientMode()) {
    case HTTP_MAL_Login:
    case HTTP_MAL_RefreshList:
      MainDialog.ChangeStatus(error_text);
      MainDialog.EnableInput(true);
      break;
    case HTTP_MAL_AnimeAskToDiscuss:
    case HTTP_MAL_AnimeDetails:
    case HTTP_MAL_Image:
    case HTTP_MAL_UserImage:
    case HTTP_Feed_DownloadIcon:
#ifdef _DEBUG
      MainDialog.ChangeStatus(error_text);
#endif
      break;
    case HTTP_MAL_SearchAnime:
      SearchDialog.EnableInput(true);
      break;
    case HTTP_Feed_Check:
    case HTTP_Feed_Download:
    case HTTP_Feed_DownloadAll:
      MainDialog.ChangeStatus(error_text);
      TorrentDialog.EnableInput();
      break;
    case HTTP_UpdateCheck:
      MessageBox(UpdateDialog.GetWindowHandle(), 
        error_text.c_str(), L"Update", MB_ICONERROR | MB_OK);
      UpdateDialog.PostMessage(WM_CLOSE);
      break;
    default:
      History.queue.updating = false;
      MainDialog.ChangeStatus(error_text);
      MainDialog.EnableInput(true);
      break;
  }
  
  TaskbarList.SetProgressState(TBPF_NOPROGRESS);
  return 0;
}

BOOL HttpClient::OnSendRequestComplete() {
  wstring status = L"Connecting...";
  
  switch (GetClientMode()) {
    case HTTP_Feed_Check:
    case HTTP_Feed_Download:
    case HTTP_Feed_DownloadAll:
      MainDialog.ChangeStatus(status);
      break;
    default:
#ifdef _DEBUG
      MainDialog.ChangeStatus(status);
#endif
      break;
  }

  return 0;
}

BOOL HttpClient::OnHeadersAvailable(win32::http_header_t& headers) {
  switch (GetClientMode()) {
    case HTTP_Silent:
      break;
    case HTTP_UpdateCheck:
    case HTTP_UpdateDownload:
      if (m_dwTotal > 0) {
        UpdateDialog.progressbar.SetMarquee(false);
        UpdateDialog.progressbar.SetRange(0, m_dwTotal);
      } else {
        UpdateDialog.progressbar.SetMarquee(true);
      }
      break;
    default:
      TaskbarList.SetProgressState(m_dwTotal > 0 ? TBPF_NORMAL : TBPF_INDETERMINATE);
      break;
  }

  return 0;
}

BOOL HttpClient::OnRedirect(wstring address) {
  wstring status = L"Redirecting... (" + address + L")";
  switch (GetClientMode()) {
    case HTTP_MAL_Login:
      if (Settings.Account.MAL.api == MAL_API_NONE) {
        return TRUE; // Disable auto-redirection to panel.php
      }
      break;
    case HTTP_Feed_Check:
    case HTTP_Feed_Download:
    case HTTP_Feed_DownloadAll:
      MainDialog.ChangeStatus(status);
      break;
    default:
#ifdef _DEBUG
      MainDialog.ChangeStatus(status);
#endif
      break;
  }

  return 0;
}

BOOL HttpClient::OnReadData() {
  wstring status;

  switch (GetClientMode()) {
    case HTTP_MAL_RefreshList:
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
    case HTTP_MAL_AnimeAskToDiscuss:
    case HTTP_Feed_DownloadIcon:
    case HTTP_MAL_AnimeDetails:
    case HTTP_MAL_SearchAnime:
    case HTTP_MAL_Image:
    case HTTP_MAL_UserImage:
    case HTTP_Silent:
      return 0;
    case HTTP_Feed_Check:
      status = L"Checking new torrents...";
      break;
    case HTTP_Feed_Download:
    case HTTP_Feed_DownloadAll:
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
    case HTTP_UpdateCheck:
    case HTTP_UpdateDownload:
      if (m_dwTotal > 0) {
        UpdateDialog.progressbar.SetPosition(m_dwDownloaded);
      }
      return 0;
    default:
      status = L"Downloading data...";
      break;
  }

  if (m_dwTotal > 0) {
    TaskbarList.SetProgressValue(m_dwDownloaded, m_dwTotal);
    status += L" (" + ToWstr(static_cast<int>(((float)m_dwDownloaded / (float)m_dwTotal) * 100)) + L"%)";
  } else {
    status += L" (" + ToSizeString(m_dwDownloaded) + L")";
  }

  switch (GetClientMode()) {
    case HTTP_Feed_Check:
    case HTTP_Feed_Download:
    case HTTP_Feed_DownloadAll:
      MainDialog.ChangeStatus(status);
      break;
    default:
      MainDialog.ChangeStatus(status);
      break;
  }

  return 0;
}

BOOL HttpClient::OnReadComplete() {
  TaskbarList.SetProgressState(TBPF_NOPROGRESS);
  wstring status;

  Stats.connections_succeeded++;
  
  switch (GetClientMode()) {
    // List
    case HTTP_MAL_RefreshList: {
      AnimeDatabase.LoadList(true); // Here we assume that MAL provides up-to-date series information in malappinfo.php
      MainDialog.ChangeStatus(L"List download completed.");
      AnimeListDialog.RefreshList(mal::MYSTATUS_WATCHING);
      AnimeListDialog.RefreshTabs(mal::MYSTATUS_WATCHING);
      HistoryDialog.RefreshList();
      SearchDialog.RefreshList();
      MainDialog.EnableInput(true);
      break;
    }

    // =========================================================================
    
    // Login
    case HTTP_MAL_Login: {
      switch (Settings.Account.MAL.api) {
        case MAL_API_OFFICIAL:
          Taiga.logged_in = InStr(GetData(), L"<username>" + Settings.Account.MAL.user + L"</username>", 0) > -1;
          break;
        case MAL_API_NONE:
          Taiga.logged_in = !(GetCookie().empty());
          break;
      }
      if (Taiga.logged_in) {
        status = L"Logged in as " + Settings.Account.MAL.user + L".";
      } else {
        status = L"Failed to log in.";
        switch (Settings.Account.MAL.api) {
          case MAL_API_OFFICIAL:
#ifdef _DEBUG
            status += L" (" + GetData() + L")";
#else
            status += L" (Invalid user name or password)";
#endif
            break;
          case MAL_API_NONE:
            if (InStr(GetData(), L"Could not find that user name", 1) > -1) {
              status += L" (Invalid user name)";
            } else if (InStr(GetData(), L"Error: Invalid password", 1) > -1) {
              status += L" (Invalid password)";
            }
            break;
        }
      }
      MainDialog.ChangeStatus(status);
      MainDialog.EnableInput(true);
      MainDialog.UpdateTip();
      UpdateAllMenus();
      if (Taiga.logged_in) {
        switch (Settings.Account.MAL.api) {
          case MAL_API_OFFICIAL:
            ExecuteAction(L"Synchronize");
            return TRUE;
          case MAL_API_NONE:
            mal::CheckProfile();
            return TRUE;
        }
      }
      break;
    }

    // =========================================================================

    // Check profile
    case HTTP_MAL_Profile: {
      if (Settings.Account.MAL.api == MAL_API_NONE) {
        int pos = 0;
        wstring data = GetData();

        #define CHECK_PROFILE_ITEM(new_item, page) \
        new_item = false; \
        pos = InStr(data, L"<a href=\"/" page L".php", 0); \
        if (pos > -1) { \
          if (IsEqual(data.substr(pos - 18, 18), L"<span class=\"new\">")) { \
            new_item = true; \
          } \
        }
        // Check messages
        bool new_message = false;
        CHECK_PROFILE_ITEM(new_message, L"mymessages");
        // Check friend requests
        bool new_friend_request = false;
        CHECK_PROFILE_ITEM(new_friend_request, L"myfriends");
        // Check club invitations
        bool new_club_invitation = false;
        CHECK_PROFILE_ITEM(new_club_invitation, L"clubs");
        #undef CHECK_PROFILE_ITEM

        if (new_message || new_friend_request || new_club_invitation) {
          status.clear();
          if (new_message)
            AppendString(status, L"messages");
          if (new_friend_request)
            AppendString(status, L"friend requests");
          if (new_club_invitation)
            AppendString(status, L"club invitations");
          status = L"You have new " + status + L" waiting on your profile.";
          MainDialog.ChangeStatus(status);

          win32::TaskDialog dlg(APP_TITLE, TD_ICON_INFORMATION);
          dlg.SetMainInstruction(status.c_str());
          dlg.UseCommandLinks(true);
          dlg.AddButton(L"Check\nView your profile now", IDYES);
          dlg.AddButton(L"Cancel\nCheck them later", IDNO);
          dlg.Show(g_hMain);
          if (dlg.GetSelectedButtonID() == IDYES) {
            mal::ViewProfile();
          }
        }

        History.queue.Check();
        return TRUE;
      }
      break;
    }

    // =========================================================================

    // Update list
    case HTTP_MAL_AnimeAdd:
    case HTTP_MAL_AnimeDelete:
    case HTTP_MAL_AnimeEdit:
    case HTTP_MAL_AnimeUpdate:
    case HTTP_MAL_ScoreUpdate:
    case HTTP_MAL_StatusUpdate:
    case HTTP_MAL_TagUpdate: {
      History.queue.updating = false;
      MainDialog.ChangeStatus();
      int anime_id = static_cast<int>(GetParam());
      auto anime_item = AnimeDatabase.FindItem(anime_id);
      auto event_item = History.queue.GetCurrentItem();
      if (anime_item && event_item) {
        anime_item->Edit(*event_item, GetData(), GetResponseStatusCode());
        return TRUE;
      }
      break;
    }

    // =========================================================================

    // Ask to discuss
    case HTTP_MAL_AnimeAskToDiscuss: {
      wstring data = GetData();
      if (InStr(data, L"trueEp") > -1) {
        wstring url = InStr(data, L"self.parent.document.location='", L"';");
        int anime_id = static_cast<int>(GetParam());
        if (!url.empty()) {
          int episode_number = 0; // TODO
          wstring title = AnimeDatabase.FindItem(anime_id)->GetTitle();
          if (episode_number) title += L" #" + ToWstr(episode_number);
          win32::TaskDialog dlg(title.c_str(), TD_ICON_INFORMATION);
          dlg.SetMainInstruction(L"Someone has already made a discussion topic for this episode!");
          dlg.UseCommandLinks(true);
          dlg.AddButton(L"Discuss it", IDYES);
          dlg.AddButton(L"Don't discuss", IDNO);
          dlg.Show(g_hMain);
          if (dlg.GetSelectedButtonID() == IDYES) {
            ExecuteLink(url);
          }
        }
      }
      break;
    }

    // =========================================================================

    // Anime details
    case HTTP_MAL_AnimeDetails: {
      int anime_id = static_cast<int>(GetParam());
      if (mal::ParseAnimeDetails(GetData())) {
        if (AnimeDialog.GetCurrentId() == anime_id)
          AnimeDialog.Refresh(false, true, false);
        if (NowPlayingDialog.GetCurrentId() == anime_id)
          NowPlayingDialog.Refresh(false, true, false);
        if (SeasonDialog.IsWindow())
          SeasonDialog.RefreshList(true);
      }
      break;
    }

    // =========================================================================

    // Download image
    case HTTP_MAL_Image: {
      int anime_id = static_cast<int>(GetParam());
      if (ImageDatabase.Load(anime_id, true, false)) {
        if (AnimeDialog.GetCurrentId() == anime_id)
          AnimeDialog.Refresh(true, false, false);
        if (AnimeListDialog.IsWindow())
          AnimeListDialog.RefreshListItem(anime_id);
        if (NowPlayingDialog.GetCurrentId() == anime_id)
          NowPlayingDialog.Refresh(true, false, false);
        if (SeasonDialog.IsWindow())
          SeasonDialog.RefreshList(true);
      }
      break;
    }

    case HTTP_MAL_UserImage: {
      // TODO
      break;
    }

    // =========================================================================

    // Search anime
    case HTTP_MAL_SearchAnime: {
      int anime_id = static_cast<int>(GetParam());
      if (anime_id) {
        if (mal::ParseSearchResult(GetData(), anime_id)) {
          if (AnimeDialog.GetCurrentId() == anime_id)
            AnimeDialog.Refresh(false, true, false);
          if (NowPlayingDialog.GetCurrentId() == anime_id)
            NowPlayingDialog.Refresh(false, true, false);
          if (SeasonDialog.IsWindow())
            SeasonDialog.RefreshList(true);
          if (mal::GetAnimeDetails(anime_id, this)) return TRUE;
        } else {
          status = L"Could not read anime information.";
          AnimeDialog.page_series_info.SetDlgItemText(IDC_EDIT_ANIME_INFO, status.c_str());
        }
#ifdef _DEBUG
        MainDialog.ChangeStatus(status);
#endif
      } else {
        if (SearchDialog.IsWindow()) {
          SearchDialog.ParseResults(GetData());
          SearchDialog.EnableInput(true);
        }
      }
      break;
    }

    // =========================================================================

    // Torrent
    case HTTP_Feed_Check: {
      Feed* feed = reinterpret_cast<Feed*>(GetParam());
      if (feed) {
        feed->Load();
        int torrent_count = feed->ExamineData();
        if (torrent_count > 0) {
          status = L"There are new torrents available!";
        } else {
          status = L"No new torrents found.";
        }
        if (TorrentDialog.IsWindow()) {
          MainDialog.ChangeStatus(status);
          TorrentDialog.RefreshList();
          TorrentDialog.EnableInput();
          // TODO: GetIcon() fails if we don't return TRUE here
        } else {
          switch (Settings.RSS.Torrent.new_action) {
            // Notify
            case 1:
              Aggregator.Notify(*feed);
              break;
            // Download
            case 2:
              if (feed->Download(-1)) return TRUE;
              break;
          }
        }
      }
      break;
    }
    case HTTP_Feed_Download:
    case HTTP_Feed_DownloadAll: {
      Feed* feed = reinterpret_cast<Feed*>(GetParam());
      if (feed) {
        FeedItem* feed_item = reinterpret_cast<FeedItem*>(&feed->items[feed->download_index]);
        wstring app_path, cmd, file = feed_item->title;
        ValidateFileName(file);
        file = feed->GetDataPath() + file + L".torrent";
        Aggregator.file_archive.push_back(feed_item->title);
        if (FileExists(file)) {
          switch (Settings.RSS.Torrent.app_mode) {
            // Default application
            case 1:
              app_path = GetDefaultAppPath(L".torrent", L"");
              break;
            // Custom application
            case 2:
              app_path = Settings.RSS.Torrent.app_path;
              break;
          }
          if (Settings.RSS.Torrent.set_folder && InStr(app_path, L"utorrent", 0, true) > -1) {
            auto anime_item = AnimeDatabase.FindItem(feed_item->episode_data.anime_id);
            if (anime_item) {
              wstring anime_folder = anime_item->GetFolder();
              if (anime_folder.empty() && 
                  Settings.RSS.Torrent.create_folder &&
                  FolderExists(Settings.RSS.Torrent.download_path)) {
                anime_folder = anime_item->GetTitle();
                ValidateFileName(anime_folder);
                CheckSlash(Settings.RSS.Torrent.download_path);
                anime_folder = Settings.RSS.Torrent.download_path + anime_folder;
                CreateDirectory(anime_folder.c_str(), nullptr);
              }
              if (!anime_folder.empty())
                cmd = L"/directory \"" + anime_folder + L"\" ";
            }
          }
          cmd += L"\"" + file + L"\"";
          Execute(app_path, cmd);
          feed_item->download = FALSE;
          TorrentDialog.RefreshList();
        }
        feed->download_index = -1;
        if (GetClientMode() == HTTP_Feed_DownloadAll) {
          if (feed->Download(-1)) return TRUE;
        }
      }
      MainDialog.ChangeStatus(L"Successfully downloaded all torrents.");
      TorrentDialog.EnableInput();
      break;
    }
    case HTTP_Feed_DownloadIcon: {
      break;
    }

    // =========================================================================

    // Twitter
    case HTTP_Twitter_Request: {
      OAuthParameters response = Twitter.oauth.ParseQueryString(GetData());
      if (!response[L"oauth_token"].empty()) {
        ExecuteLink(L"http://api.twitter.com/oauth/authorize?oauth_token=" + response[L"oauth_token"]);
        InputDialog dlg;
        dlg.title = L"Twitter Authorization";
        dlg.info = L"Please enter the PIN shown on the page after logging into Twitter:";
        dlg.Show();
        if (dlg.result == IDOK && !dlg.text.empty()) {
          Twitter.AccessToken(response[L"oauth_token"], response[L"oauth_token_secret"], dlg.text);
          return TRUE;
        }
      }
      MainDialog.ChangeStatus();
      break;
    }
	case HTTP_Twitter_Auth: {
      OAuthParameters access = Twitter.oauth.ParseQueryString(GetData());
      if (!access[L"oauth_token"].empty() && !access[L"oauth_token_secret"].empty()) {
        Settings.Announce.Twitter.oauth_key = access[L"oauth_token"];
        Settings.Announce.Twitter.oauth_secret = access[L"oauth_token_secret"];
        Settings.Announce.Twitter.user = access[L"screen_name"];
        status = L"Taiga is now authorized to post to this Twitter account: ";
        status += Settings.Announce.Twitter.user;
      } else {
        status = L"Twitter authorization failed.";
      }
      MainDialog.ChangeStatus(status);
      SettingsDialog.RefreshTwitterLink();
      break;
    }
    case HTTP_Twitter_Post: {
      if (InStr(GetData(), L"\"errors\"", 0) == -1) {
        status = L"Twitter status updated.";
      } else {
        status = L"Twitter status update failed.";
        int index_begin = InStr(GetData(), L"\"message\":\"", 0);
        int index_end = InStr(GetData(), L"\",\"", index_begin);
        if (index_begin > -1 && index_end > -1) {
          index_begin += 11;
          status += L" (" + GetData().substr(index_begin, index_end - index_begin) + L")";
        }
      }
      MainDialog.ChangeStatus(status);
      break;
    }

    // =========================================================================

    // Check updates
    case HTTP_UpdateCheck: {
      if (Taiga.Updater.ParseData(GetData(), HTTP_UpdateDownload)) {
        return TRUE;
      } else {
        UpdateDialog.PostMessage(WM_CLOSE);
      }
      break;
    }

    // Download updates
    case HTTP_UpdateDownload: {
      UpdateFile* file = reinterpret_cast<UpdateFile*>(GetParam());
      file->download = false;
      if (Taiga.Updater.DownloadNextFile(HTTP_UpdateDownload)) {
        return TRUE;
      } else {
        UpdateDialog.PostMessage(WM_CLOSE);
      }
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

// =============================================================================

void SetProxies(const wstring& proxy, const wstring& user, const wstring& pass) {
  #define SET_PROXY(client) client.SetProxy(proxy, user, pass);
  SET_PROXY(HttpClient);
  SET_PROXY(ImageClient);
  SET_PROXY(MainClient);
  SET_PROXY(SearchClient);
  SET_PROXY(TwitterClient);
  SET_PROXY(VersionClient);
  for (unsigned int i = 0; i < Aggregator.feeds.size(); i++) {
    SET_PROXY(Aggregator.feeds[i].client);
  }
  SET_PROXY(Taiga.Updater.client);
  #undef SET_PROXY
}