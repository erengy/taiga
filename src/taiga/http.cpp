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

#include "base/common.h"
#include "base/file.h"
#include "base/logger.h"
#include "base/string.h"
#include "http.h"
#include "library/anime_db.h"
#include "library/resource.h"
#include "settings.h"
#include "stats.h"
#include "sync/manager.h"
#include "taiga.h"
#include "taiga/announce.h"
#include "ui/ui.h"
#include "version.h"

taiga::HttpManager ConnectionManager;

namespace taiga {

HttpClient::HttpClient()
    : mode_(kHttpSilent) {
  // We will handle redirections ourselves
  set_auto_redirect(false);

  // The default header (e.g. "User-Agent: Taiga/1.0") will be used, unless
  // another value is specified in the request header
  set_user_agent(
      APP_NAME L"/" + ToWstr(VERSION_MAJOR) + L"." + ToWstr(VERSION_MINOR));

  // Make sure all new clients use the proxy settings
  set_proxy(Settings[kApp_Connection_ProxyHost],
            Settings[kApp_Connection_ProxyUsername],
            Settings[kApp_Connection_ProxyPassword]);
}

HttpClientMode HttpClient::mode() const {
  return mode_;
}

void HttpClient::set_mode(HttpClientMode mode) {
  mode_ = mode;
}

////////////////////////////////////////////////////////////////////////////////

void HttpClient::OnError(DWORD error) {
  wstring error_text = L"HTTP error #" + ToWstr(error) + L": " +
                       FormatError(error, L"winhttp.dll");
  TrimRight(error_text, L"\r\n");

  LOG(LevelError, error_text);
  LOG(LevelError, L"Connection mode: " + ToWstr(mode_));

  ui::OnHttpError(*this, error_text);

  Stats.connections_failed++;

  ConnectionManager.HandleError(response_, error_text);
}

bool HttpClient::OnHeadersAvailable() {
  ui::OnHttpHeadersAvailable(*this);
  return false;
}

bool HttpClient::OnRedirect(const std::wstring& address) {
  LOG(LevelDebug, L"Redirecting... (" + address + L")");

  switch (mode()) {
    case kHttpTaigaUpdateDownload: {
      wstring file = address.substr(address.find_last_of(L"/") + 1);
      download_path_ = GetPathOnly(download_path_) + file;
      Taiga.Updater.SetDownloadPath(download_path_);
      break;
    }
  }

  return false;
}

bool HttpClient::OnReadData() {
  ui::OnHttpProgress(*this);
  return false;
}

bool HttpClient::OnReadComplete() {
  ui::OnHttpReadComplete(*this);

  Stats.connections_succeeded++;

  ConnectionManager.HandleResponse(response_);

  return false;
}

////////////////////////////////////////////////////////////////////////////////

HttpClient& HttpManager::GetNewClient(const base::uuid_t& uuid) {
  return clients_[uuid];
}

void HttpManager::CancelRequest(base::uuid_t uuid) {
  if (!clients_.count(uuid))
    clients_.erase(uuid);
}

void HttpManager::MakeRequest(HttpRequest& request, HttpClientMode mode) {
  if (clients_.count(request.uuid)) {
    LOG(LevelWarning, L"HttpClient already exists. ID: " + request.uuid);
    clients_[request.uuid].Cleanup();
  }

  HttpClient& client = clients_[request.uuid];
  client.set_mode(mode);
  client.MakeRequest(request);
}

void HttpManager::MakeRequest(HttpClient& client, HttpRequest& request,
                              HttpClientMode mode) {
  client.set_mode(mode);
  client.MakeRequest(request);
}

void HttpManager::HandleError(HttpResponse& response, const string_t& error) {
  HttpClient& client = clients_[response.uuid];

  switch (client.mode()) {
    case kHttpServiceAuthenticateUser:
    case kHttpServiceGetMetadataById:
    case kHttpServiceSearchTitle:
    case kHttpServiceAddLibraryEntry:
    case kHttpServiceDeleteLibraryEntry:
    case kHttpServiceGetLibraryEntries:
    case kHttpServiceUpdateLibraryEntry:
      ServiceManager.HandleHttpError(client.response_, error);
      break;
  }

  // TODO: Fix bug
  //clients_.erase(response.uuid);
}

void HttpManager::HandleResponse(HttpResponse& response) {
  HttpClient& client = clients_[response.uuid];

  switch (client.mode()) {
    case kHttpServiceAuthenticateUser:
    case kHttpServiceGetMetadataById:
    case kHttpServiceSearchTitle:
    case kHttpServiceAddLibraryEntry:
    case kHttpServiceDeleteLibraryEntry:
    case kHttpServiceGetLibraryEntries:
    case kHttpServiceUpdateLibraryEntry:
      ServiceManager.HandleHttpResponse(response);
      break;

    case kHttpGetLibraryEntryImage: {
      int anime_id = static_cast<int>(response.parameter);
      if (ImageDatabase.Load(anime_id, true, false))
        ui::OnLibraryEntryImageChange(anime_id);
      break;
    }

    case kHttpFeedCheck:
    case kHttpFeedCheckAuto: {
      Feed* feed = reinterpret_cast<Feed*>(response.parameter);
      if (feed) {
        feed->Load();
        bool success = feed->ExamineData();
        ui::OnFeedCheck(success);
        if (client.mode() == kHttpFeedCheckAuto) {
          switch (Settings.GetInt(kTorrent_Discovery_NewAction)) {
            case 1:  // Notify
              Aggregator.Notify(*feed);
              break;
            case 2:  // Download
              feed->Download(-1);
              break;
          }
        }
      }
      break;
    }
    case kHttpFeedDownload:
    case kHttpFeedDownloadAll: {
      Feed* feed = reinterpret_cast<Feed*>(response.parameter);
      if (feed) {
        FeedItem* feed_item = reinterpret_cast<FeedItem*>(&feed->items[feed->download_index]);
        wstring app_path, cmd, file = feed_item->title;
        ValidateFileName(file);
        file = feed->GetDataPath() + file + L".torrent";
        Aggregator.file_archive.push_back(feed_item->title);
        if (FileExists(file)) {
          switch (Settings.GetInt(kTorrent_Download_AppMode)) {
            case 1:  // Default application
              app_path = GetDefaultAppPath(L".torrent", L"");
              break;
            case 2:  // Custom application
              app_path = Settings[kTorrent_Download_AppPath];
              break;
          }
          if (Settings.GetBool(kTorrent_Download_UseAnimeFolder) &&
              InStr(app_path, L"utorrent", 0, true) > -1) {
            wstring download_path;
            if (Settings.GetBool(kTorrent_Download_FallbackOnFolder) &&
                FolderExists(Settings[kTorrent_Download_Location])) {
              download_path = Settings[kTorrent_Download_Location];
            }
            auto anime_item = AnimeDatabase.FindItem(feed_item->episode_data.anime_id);
            if (anime_item) {
              wstring anime_folder = anime_item->GetFolder();
              if (!anime_folder.empty() && FolderExists(anime_folder)) {
                download_path = anime_folder;
              } else if (Settings.GetBool(kTorrent_Download_CreateSubfolder) && !download_path.empty()) {
                anime_folder = anime_item->GetTitle();
                ValidateFileName(anime_folder);
                TrimRight(anime_folder, L".");
                AddTrailingSlash(download_path);
                download_path += anime_folder;
                CreateDirectory(download_path.c_str(), nullptr);
                anime_item->SetFolder(download_path);
                Settings.Save();
              }
            }
            if (!download_path.empty())
              cmd = L"/directory \"" + download_path + L"\" ";
          }
          cmd += L"\"" + file + L"\"";
          Execute(app_path, cmd);
          feed_item->state = FEEDITEM_DISCARDED;
          ui::OnFeedDownload(true);
        }
        feed->download_index = -1;
        if (client.mode() == kHttpFeedDownloadAll)
          if (feed->Download(-1))
            return;
      }
      ui::OnFeedDownload(false);
      break;
    }

    case kHttpTwitterRequest: {
      OAuthParameters parameters = ::Twitter.oauth.ParseQueryString(response.body);
      if (!parameters[L"oauth_token"].empty()) {
        ExecuteLink(L"http://api.twitter.com/oauth/authorize?oauth_token=" +
                    parameters[L"oauth_token"]);
        string_t auth_pin;
        if (ui::OnTwitterRequest(auth_pin))
          ::Twitter.AccessToken(parameters[L"oauth_token"],
                                parameters[L"oauth_token_secret"],
                                auth_pin);
      }
      break;
    }
    case kHttpTwitterAuth: {
      bool success = false;
      OAuthParameters parameters = ::Twitter.oauth.ParseQueryString(response.body);
      if (!parameters[L"oauth_token"].empty() && !parameters[L"oauth_token_secret"].empty()) {
        Settings.Set(kShare_Twitter_OauthToken, parameters[L"oauth_token"]);
        Settings.Set(kShare_Twitter_OauthSecret, parameters[L"oauth_token_secret"]);
        Settings.Set(kShare_Twitter_Username, parameters[L"screen_name"]);
        success = true;
      }
      ui::OnTwitterAuth(success);
      break;
    }
    case kHttpTwitterPost: {
      if (InStr(response.body, L"\"errors\"", 0) == -1) {
        ui::OnTwitterPost(true, L"");
      } else {
        string_t error;
        int index_begin = InStr(response.body, L"\"message\":\"", 0);
        int index_end = InStr(response.body, L"\",\"", index_begin);
        if (index_begin > -1 && index_end > -1) {
          index_begin += 11;
          error = response.body.substr(index_begin, index_end - index_begin);
        }
        ui::OnTwitterPost(false, error);
      }
      break;
    }

    case kHttpTaigaUpdateCheck:
      if (Taiga.Updater.ParseData(response.body))
        if (Taiga.Updater.IsDownloadAllowed())
          if (Taiga.Updater.Download())
            return;
      ui::OnUpdate();
      break;
    case kHttpTaigaUpdateDownload:
      Taiga.Updater.RunInstaller();
      ui::OnUpdate();
      break;
  }

  // TODO: Fix bug
  //clients_.erase(response.uuid);
}

}  // namespace taiga