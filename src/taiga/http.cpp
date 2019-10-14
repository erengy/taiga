/*
** Taiga
** Copyright (C) 2010-2019, Eren Okka
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
#include "base/format.h"
#include "base/log.h"
#include "base/string.h"
#include "base/url.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "media/discover.h"
#include "ui/resource.h"
#include "sync/manager.h"
#include "taiga/announce.h"
#include "taiga/config.h"
#include "taiga/http.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "taiga/stats.h"
#include "taiga/taiga.h"
#include "taiga/update.h"
#include "taiga/version.h"
#include "track/feed.h"
#include "track/feed_aggregator.h"
#include "track/recognition.h"
#include "ui/translate.h"
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
  set_allow_reuse(settings.GetAppConnectionReuseActive());

  // Enable debug mode to log requests and responses
  set_debug_mode(Taiga.options.debug_mode);

  // Disabling certificate revocation checks seems to work for those who get
  // "SSL connect error". See issue #312 for more information.
  set_no_revoke(settings.GetAppConnectionNoRevoke());

  // The default header (e.g. "User-Agent: Taiga/1.0") will be used, unless
  // another value is specified in the request header
  set_user_agent(L"{}/{}.{}"_format(
      TAIGA_APP_NAME, TAIGA_VERSION_MAJOR, TAIGA_VERSION_MINOR));

  // Make sure all new clients use the proxy settings
  set_proxy(settings.GetAppConnectionProxyHost(),
            settings.GetAppConnectionProxyUsername(),
            settings.GetAppConnectionProxyPassword());
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

  LOGE(L"{}\nConnection mode: {}", error_text, mode_);

  ui::OnHttpError(*this, error_text);

  taiga::stats.connections_failed++;

  ConnectionManager.HandleError(response_, error_text);
}

bool HttpClient::OnHeadersAvailable() {
  ui::OnHttpHeadersAvailable(*this);
  return false;
}

bool HttpClient::OnRedirect(const std::wstring& address, bool refresh) {
  LOGD(L"Location: {}", address);

  switch (mode()) {
    case kHttpTaigaUpdateDownload: {
      std::wstring path = GetPathOnly(taiga::updater.GetDownloadPath());
      std::wstring file = GetFileName(address);
      taiga::updater.SetDownloadPath(path + file);
      break;
    }
  }

  auto check_cloudflare = [](const base::http::Response& response) {
    if (response.code >= 500) {
      auto it = response.header.find(L"Server");
      if (it != response.header.end() &&
          InStr(it->second, L"cloudflare", 0, true) > -1) {
        return true;
      }
    }
    return false;
  };

  if (refresh) {
    if (check_cloudflare(response_)) {
      std::wstring error_text = L"Cannot connect to " + request_.url.host +
                                L" because of Cloudflare DDoS protection";
      LOGE(L"{}\nConnection mode: {}", error_text, mode_);
      ui::OnHttpError(*this, error_text);
    } else {
      HttpRequest http_request = request_;
      http_request.uid = base::http::GenerateRequestId();
      http_request.url = address;
      ConnectionManager.MakeRequest(http_request, mode());
    }
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

  taiga::stats.connections_succeeded++;

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

void HttpManager::HandleError(HttpResponse& response, const std::wstring& error) {
  HttpClient& client = *FindClient(response.uid);

  switch (client.mode()) {
    case kHttpServiceAuthenticateUser:
    case kHttpServiceGetUser:
    case kHttpServiceGetMetadataById:
    case kHttpServiceGetSeason:
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
    case kHttpServiceGetUser:
    case kHttpServiceGetMetadataById:
    case kHttpServiceGetSeason:
    case kHttpServiceSearchTitle:
    case kHttpServiceAddLibraryEntry:
    case kHttpServiceDeleteLibraryEntry:
    case kHttpServiceGetLibraryEntries:
    case kHttpServiceUpdateLibraryEntry:
      ServiceManager.HandleHttpResponse(response);
      break;

    case kHttpGetLibraryEntryImage: {
      const int anime_id = static_cast<int>(response.parameter);
      if (response.GetStatusCategory() == 200) {
        SaveToFile(client.write_buffer_, anime::GetImagePath(anime_id));
        if (ui::image_db.Reload(anime_id))
          ui::OnLibraryEntryImageChange(anime_id);
      } else if (response.code == 404) {
        const auto anime_item = anime::db.Find(anime_id);
        if (anime_item)
          anime_item->SetImageUrl({});
      }
      break;
    }

    case kHttpFeedCheck:
    case kHttpFeedCheckAuto: {
      const auto feed = reinterpret_cast<track::Feed*>(response.parameter);
      if (feed) {
        bool automatic = client.mode() == kHttpFeedCheckAuto;
        track::aggregator.HandleFeedCheck(*feed, client.write_buffer_, automatic);
      }
      break;
    }
    case kHttpFeedDownload: {
      const auto feed = reinterpret_cast<track::Feed*>(response.parameter);
      if (feed) {
        if (track::aggregator.ValidateFeedDownload(client.request(), response)) {
          track::aggregator.HandleFeedDownload(*feed, client.write_buffer_);
        } else {
          track::aggregator.HandleFeedDownloadError(*feed);
        }
      }
      break;
    }

    case kHttpTwitterRequest:
    case kHttpTwitterAuth:
    case kHttpTwitterPost:
      link::twitter::HandleHttpResponse(client.mode(), response);
      break;

    case kHttpTaigaUpdateCheck: {
      if (taiga::updater.ParseData(response.body)) {
        if (taiga::updater.IsUpdateAvailable()) {
          ui::OnUpdateAvailable();
          break;
        }
        if (taiga::updater.IsAnimeRelationsAvailable()) {
          taiga::updater.CheckAnimeRelations();
          break;
        }
        ui::OnUpdateNotAvailable(false);
      }
      ui::OnUpdateFinished();
      break;
    }
    case kHttpTaigaUpdateDownload:
      if (response.GetStatusCategory() == 200 &&
          SaveToFile(client.write_buffer_, taiga::updater.GetDownloadPath())) {
        taiga::updater.RunInstaller();
      } else {
        ui::OnUpdateFailed();
      }
      ui::OnUpdateFinished();
      break;
    case kHttpTaigaUpdateRelations: {
      if (Meow.ReadRelations(client.write_buffer_) &&
          SaveToFile(client.write_buffer_, GetPath(Path::DatabaseAnimeRelations))) {
        LOGD(L"Updated anime relation data.");
        ui::OnUpdateNotAvailable(true);
      } else {
        Meow.ReadRelations();
        LOGD(L"Anime relation data update failed.");
        ui::OnUpdateNotAvailable(false);
      }
      ui::OnUpdateFinished();
      break;
    }
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
  for (auto& client : clients_)
    if (client.request().uid == uid)
      return &client;

  return nullptr;
}

HttpClient& HttpManager::GetClient(const HttpRequest& request) {
  for (auto& client : clients_) {
    if (client.allow_reuse() && !client.busy()) {
      if (IsEqual(client.request().url.host, request.url.host)) {
        LOGD(L"Reusing client with the ID: {}\nClient's new ID: {}",
             client.request().uid, request.uid);
        // Proxy settings might have changed since then
        client.set_proxy(settings.GetAppConnectionProxyHost(),
                         settings.GetAppConnectionProxyUsername(),
                         settings.GetAppConnectionProxyPassword());
        return client;
      }
    }
  }

  clients_.push_back(HttpClient(request));
  LOGD(L"Created a new client. Total number of clients is now {}",
        clients_.size());
  return clients_.back();
}

void HttpManager::AddToQueue(HttpRequest& request, HttpClientMode mode) {
  win::Lock lock(critical_section_);

  LOGD(L"ID: {}", request.uid);

  requests_.push_back(std::make_pair(request, mode));
}

void HttpManager::ProcessQueue() {
  win::Lock lock(critical_section_);

  unsigned int connections = 0;
  for (const auto& pair : connections_) {
    connections += pair.second;
  }

  for (size_t i = 0; i < requests_.size(); i++) {
    if (shutdown_) {
      LOGD(L"Shutting down");
      return;
    }

    if (connections == kMaxSimultaneousConnections) {
      LOGD(L"Reached max connections");
      return;
    }

    const HttpRequest& request = requests_.at(i).first;
    HttpClientMode mode = requests_.at(i).second;

    if (connections_[request.url.host] == kMaxSimultaneousConnectionsPerHostname) {
      LOGD(L"Reached max connections for hostname: {}", request.url.host);
      continue;
    } else {
      connections++;
      connections_[request.url.host]++;
      LOGD(L"Connections for hostname is now {}: {}",
           connections_[request.url.host], request.url.host);

      HttpClient& client = GetClient(request);
      client.set_mode(mode);
      client.MakeRequest(request);

      requests_.erase(requests_.begin() + i);
      i--;
    }
  }
}

void HttpManager::AddConnection(const std::wstring& hostname) {
  win::Lock lock(critical_section_);

  connections_[hostname]++;
  LOGD(L"Connections for hostname is now {}: {}",
       connections_[hostname], hostname);
}

void HttpManager::FreeConnection(const std::wstring& hostname) {
  win::Lock lock(critical_section_);

  if (connections_[hostname] > 0) {
    connections_[hostname]--;
    LOGD(L"Connections for hostname is now {}: {}",
         connections_[hostname], hostname);
  } else {
    LOGE(L"Connections for hostname was already zero: {}", hostname);
  }
}

}  // namespace taiga
