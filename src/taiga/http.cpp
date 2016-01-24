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

#include "base/file.h"
#include "base/foreach.h"
#include "base/log.h"
#include "base/string.h"
#include "base/url.h"
#include "library/anime_util.h"
#include "library/discover.h"
#include "library/resource.h"
#include "sync/manager.h"
#include "taiga/announce.h"
#include "taiga/http.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "taiga/stats.h"
#include "taiga/taiga.h"
#include "taiga/version.h"
#include "track/recognition.h"
#include "ui/ui.h"

taiga::HttpManager ConnectionManager;

namespace taiga {

// These are the values commonly used by today's web browsers.
// See: http://www.browserscope.org/?category=network
const unsigned int kMaxSimultaneousConnections = 10;
const unsigned int kMaxSimultaneousConnectionsPerHostname = 6;

HttpClient::HttpClient(const HttpRequest& request)
    : base::http::Client(request),
      mode_(kHttpSilent) {
  // Reuse existing connections
  set_allow_reuse(Settings.GetBool(kApp_Connection_ReuseActive));

  // Enable debug mode to log requests and responses
  set_debug_mode(Taiga.debug_mode);

  // The default header (e.g. "User-Agent: Taiga/1.0") will be used, unless
  // another value is specified in the request header
  set_user_agent(
      TAIGA_APP_NAME L"/" +
      ToWstr(TAIGA_VERSION_MAJOR) + L"." + ToWstr(TAIGA_VERSION_MINOR));

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

void HttpClient::OnError(CURLcode error_code) {
  std::wstring error_text = L"HTTP error #" + ToWstr(error_code) + L": " +
                            StrToWstr(curl_easy_strerror(error_code));
  TrimRight(error_text, L"\r\n ");
  switch (error_code) {
    case CURLE_COULDNT_RESOLVE_HOST:
    case CURLE_COULDNT_CONNECT:
      error_text += L" (" + request_.url.host + L")";
      break;
  }

  LOG(LevelError, error_text + L"\nConnection mode: " + ToWstr(mode_));

  ui::OnHttpError(*this, error_text);

  Stats.connections_failed++;

  ConnectionManager.HandleError(response_, error_text);
}

bool HttpClient::OnHeadersAvailable() {
  ui::OnHttpHeadersAvailable(*this);
  return false;
}

bool HttpClient::OnRedirect(const std::wstring& address, bool refresh) {
  LOG(LevelDebug, L"Redirecting... (" + address + L")");

  switch (mode()) {
    case kHttpTaigaUpdateDownload: {
      std::wstring path = GetPathOnly(Taiga.Updater.GetDownloadPath());
      std::wstring file = GetFileName(address);
      Taiga.Updater.SetDownloadPath(path + file);
      break;
    }
  }

  if (refresh) {
    HttpRequest http_request = request_;
    http_request.uid = base::http::GenerateRequestId();
    http_request.url = address;
    ConnectionManager.MakeRequest(http_request, mode());
    Cancel();
    return true;
  } else {
    Url url(address);
    ConnectionManager.HandleRedirect(request_.url.host, url.host);
    return false;
  }
}

bool HttpClient::OnProgress() {
  ui::OnHttpProgress(*this);
  return false;
}

void HttpClient::OnReadComplete() {
  ui::OnHttpReadComplete(*this);

  Stats.connections_succeeded++;

  ConnectionManager.HandleResponse(response_);
}

////////////////////////////////////////////////////////////////////////////////

HttpManager::HttpManager()
    : shutdown_(false) {
}

void HttpManager::CancelRequest(base::uid_t uid) {
  auto client = FindClient(uid);

  if (client && client->busy())
    client->Cancel();
}

void HttpManager::MakeRequest(HttpRequest& request, HttpClientMode mode) {
  AddToQueue(request, mode);
  ProcessQueue();
}

void HttpManager::HandleError(HttpResponse& response, const string_t& error) {
  HttpClient& client = *FindClient(response.uid);

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

  FreeConnection(client.request_.url.host);
  ProcessQueue();
}

void HttpManager::HandleRedirect(const std::wstring& current_host,
                                 const std::wstring& next_host) {
  FreeConnection(current_host);
  AddConnection(next_host);
}

void HttpManager::HandleResponse(HttpResponse& response) {
  HttpClient& client = *FindClient(response.uid);

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
      SaveToFile(client.write_buffer_, anime::GetImagePath(anime_id));
      if (ImageDatabase.Reload(anime_id))
        ui::OnLibraryEntryImageChange(anime_id);
      break;
    }

    case kHttpFeedCheck:
    case kHttpFeedCheckAuto: {
      Feed* feed = reinterpret_cast<Feed*>(response.parameter);
      if (feed) {
        bool automatic = client.mode() == kHttpFeedCheckAuto;
        Aggregator.HandleFeedCheck(*feed, client.write_buffer_, automatic);
      }
      break;
    }
    case kHttpFeedDownload: {
      if (Aggregator.ValidateFeedDownload(client.request(), response)) {
        auto feed = reinterpret_cast<Feed*>(response.parameter);
        if (feed)
          Aggregator.HandleFeedDownload(*feed, client.write_buffer_);
      }
      break;
    }

    case kHttpSeasonsGet: {
      auto filename = GetFileName(client.request().url.path);
      auto path = GetPath(kPathDatabaseSeason) + filename;
      if (SaveToFile(client.write_buffer_, path) &&
          SeasonDatabase.LoadFile(filename)) {
        Settings.Set(taiga::kApp_Seasons_LastSeason,
                     SeasonDatabase.current_season.GetString());
        SeasonDatabase.Review();
        ui::OnSeasonLoad(SeasonDatabase.IsRefreshRequired());
      } else {
        ui::OnSeasonLoadFail();
      }
      break;
    }

    case kHttpTwitterRequest:
    case kHttpTwitterAuth:
    case kHttpTwitterPost:
      ::Twitter.HandleHttpResponse(client.mode(), response);
      break;

    case kHttpTaigaUpdateCheck: {
      if (Taiga.Updater.ParseData(response.body))
        if (Taiga.Updater.IsDownloadAllowed())
          break;
      ui::OnUpdateFinished();
      Taiga.Updater.CheckAnimeRelations();
      break;
    }
    case kHttpTaigaUpdateDownload:
      SaveToFile(client.write_buffer_, Taiga.Updater.GetDownloadPath());
      Taiga.Updater.RunInstaller();
      ui::OnUpdateFinished();
      break;
    case kHttpTaigaUpdateRelations:
      if (Meow.ReadRelations(client.write_buffer_) &&
          SaveToFile(client.write_buffer_, GetPath(kPathDatabaseAnimeRelations))) {
        LOG(LevelDebug, L"Updated anime relation data.");
      } else {
        Meow.ReadRelations();
        LOG(LevelDebug, L"Anime relation data update failed.");
      }
      break;
  }

  FreeConnection(client.request_.url.host);
  ProcessQueue();
}

////////////////////////////////////////////////////////////////////////////////

void HttpManager::FreeMemory() {
  for (auto it = clients_.cbegin(); it != clients_.cend(); ) {
    if (!it->busy()) {
      clients_.erase(it++);
    } else {
      ++it;
    }
  }
}

void HttpManager::Shutdown() {
  shutdown_ = true;

  for (auto& client : clients_) {
    if (client.busy())
      client.Cancel();
  }
}

////////////////////////////////////////////////////////////////////////////////

HttpClient* HttpManager::FindClient(base::uid_t uid) {
  foreach_(it, clients_)
    if (it->request().uid == uid)
      return &(*it);

  return nullptr;
}

HttpClient& HttpManager::GetClient(const HttpRequest& request) {
  HttpClient* client = nullptr;

  foreach_(it, clients_) {
    if (it->allow_reuse() && !it->busy()) {
      if (IsEqual(it->request().url.host, request.url.host)) {
        LOG(LevelDebug, L"Reusing client with the ID: " + it->request().uid +
                        L"\nClient's new ID: " + request.uid);
        client = &(*it);
        // Proxy settings might have changed since then
        client->set_proxy(Settings[kApp_Connection_ProxyHost],
                          Settings[kApp_Connection_ProxyUsername],
                          Settings[kApp_Connection_ProxyPassword]);
        break;
      }
    }
  }

  if (!client) {
    clients_.push_back(HttpClient(request));
    client = &clients_.back();
    LOG(LevelDebug, L"Created a new client. Total number of clients is now " +
                    ToWstr(clients_.size()));
  }

  return *client;
}

void HttpManager::AddToQueue(HttpRequest& request, HttpClientMode mode) {
#ifdef TAIGA_HTTP_MULTITHREADED
  win::Lock lock(critical_section_);

  LOG(LevelDebug, L"ID: " + request.uid);

  requests_.push_back(std::make_pair(request, mode));
#else
  HttpClient& client = GetClient(request);
  client.set_mode(mode);
  client.MakeRequest(request);
#endif
}

void HttpManager::ProcessQueue() {
#ifdef TAIGA_HTTP_MULTITHREADED
  win::Lock lock(critical_section_);

  unsigned int connections = 0;
  foreach_(it, connections_)
    connections += it->second;

  for (size_t i = 0; i < requests_.size(); i++) {
    if (shutdown_) {
      LOG(LevelDebug, L"Shutting down");
      return;
    }

    if (connections == kMaxSimultaneousConnections) {
      LOG(LevelDebug, L"Reached max connections");
      return;
    }

    const HttpRequest& request = requests_.at(i).first;
    HttpClientMode mode = requests_.at(i).second;

    if (connections_[request.url.host] == kMaxSimultaneousConnectionsPerHostname) {
      LOG(LevelDebug, L"Reached max connections for hostname: " + request.url.host);
      continue;
    } else {
      connections++;
      connections_[request.url.host]++;
      LOG(LevelDebug, L"Connections for hostname is now " +
                      ToWstr(connections_[request.url.host]) +
                      L": " + request.url.host);

      HttpClient& client = GetClient(request);
      client.set_mode(mode);
      client.MakeRequest(request);

      requests_.erase(requests_.begin() + i);
      i--;
    }
  }
#endif
}

void HttpManager::AddConnection(const string_t& hostname) {
#ifdef TAIGA_HTTP_MULTITHREADED
  win::Lock lock(critical_section_);

  connections_[hostname]++;
  LOG(LevelDebug, L"Connections for hostname is now " +
                  ToWstr(connections_[hostname]) + L": " + hostname);
#endif
}

void HttpManager::FreeConnection(const string_t& hostname) {
#ifdef TAIGA_HTTP_MULTITHREADED
  win::Lock lock(critical_section_);

  if (connections_[hostname] > 0) {
    connections_[hostname]--;
    LOG(LevelDebug, L"Connections for hostname is now " +
                    ToWstr(connections_[hostname]) + L": " + hostname);
  } else {
    LOG(LevelError, L"Connections for hostname was already zero: " + hostname);
  }
#endif
}

}  // namespace taiga