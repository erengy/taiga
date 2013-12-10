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

#include "sync.h"
#include "manager.h"

#include "base/foreach.h"
#include "base/string.h"
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "library/history.h"
#include "taiga/http.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"

#include "ui/dlg/dlg_main.h"

namespace sync {

void AuthenticateUser() {
  Request request(kAuthenticateUser);
  AddAuthenticationToRequest(request);
  ServiceManager.MakeRequest(request);
}

void GetLibraryEntries() {
  Request request(kGetLibraryEntries);
  AddAuthenticationToRequest(request);
  request.data[L"myanimelist-username"] = Settings.Account.MAL.user;
  ServiceManager.MakeRequest(request);
}

void GetMetadataById(int id) {
  Request request(kGetMetadataById);
  AddAuthenticationToRequest(request);
  request.data[L"myanimelist-id"] = ToWstr(id);
  ServiceManager.MakeRequest(request);
}

void SearchTitle(string_t title) {
  Request request(kSearchTitle);
  AddAuthenticationToRequest(request);
  request.data[L"title"] = title;
  ServiceManager.MakeRequest(request);
}

void Synchronize() {
#ifdef _DEBUG
  Taiga.logged_in = true;
#endif
  if (!Taiga.logged_in) {
    if (!Settings.Account.MAL.user.empty() &&
        !Settings.Account.MAL.password.empty()) {
      // Log in
      MainDialog.ChangeStatus(L"Logging in...");
      MainDialog.EnableInput(false);
      AuthenticateUser();
    } else if (!Settings.Account.MAL.user.empty()) {
      // Download list
      MainDialog.ChangeStatus(L"Downloading anime list...");
      MainDialog.EnableInput(false);
      GetLibraryEntries();
    } else {
      MainDialog.ChangeStatus(
          L"Cannot synchronize, username and password not available");
    }
  } else {
    if (History.queue.GetItemCount() > 0) {
      // Update items in queue
      History.queue.Check(false);
    } else {
      // Retrieve list
      MainDialog.ChangeStatus(L"Synchronizing anime list...");
      MainDialog.EnableInput(false);
      GetLibraryEntries();
    }
  }
}

void UpdateLibraryEntry(AnimeValues& anime_values, int id,
                        taiga::HttpClientMode http_client_mode) {
  RequestType request_type = ClientModeToRequestType(http_client_mode);

  Request request(request_type);
  AddAuthenticationToRequest(request);
  request.data[L"myanimelist-id"] = ToWstr(id);

  if (anime_values.episode)
    request.data[L"episode"] = ToWstr(*anime_values.episode);
  if (anime_values.status)
    request.data[L"status"] = ToWstr(*anime_values.status);
  if (anime_values.score)
    request.data[L"score"] = ToWstr(*anime_values.score);
  if (anime_values.date_start)
    request.data[L"date_start"] = *anime_values.date_start;
  if (anime_values.date_finish)
    request.data[L"date_finish"] = *anime_values.date_finish;
  if (anime_values.enable_rewatching)
    request.data[L"enable_rewatching"] = ToWstr(*anime_values.enable_rewatching);
  if (anime_values.tags)
    request.data[L"tags"] = *anime_values.tags;
  
  ServiceManager.MakeRequest(request);
}

void DownloadImage(int id, const string_t& image_url) {
  if (image_url.empty())
    return;
  
  win::http::Url url(image_url);
  
  HttpRequest http_request;
  http_request.host = url.host;
  http_request.path = url.path;
  http_request.parameter = id;

  auto& client = ConnectionManager.GetNewClient(http_request.uuid);
  client.set_download_path(::anime::GetImagePath(id));
  ConnectionManager.MakeRequest(client, http_request, taiga::kHttpGetLibraryEntryImage);
}

////////////////////////////////////////////////////////////////////////////////

void AddAuthenticationToRequest(Request& request) {
  Service& service = ServiceManager.service(kMyAnimeList);

  if (RequestNeedsAuthentication(request.type, kMyAnimeList)) {
    request.data[service.canonical_name() + L"-username"] = Settings.Account.MAL.user;
    request.data[service.canonical_name() + L"-password"] = Settings.Account.MAL.password;
  }
}

bool RequestNeedsAuthentication(RequestType request_type, ServiceId service_id) {
  Service& service = ServiceManager.service(service_id);
  return service.RequestNeedsAuthentication(request_type);
}

////////////////////////////////////////////////////////////////////////////////

RequestType ClientModeToRequestType(taiga::HttpClientMode client_mode) {
  switch (client_mode) {
    case taiga::kHttpServiceAuthenticateUser:
      return kAuthenticateUser;
    case taiga::kHttpServiceGetMetadataById:
      return kGetMetadataById;
    case taiga::kHttpServiceSearchTitle:
      return kSearchTitle;
    case taiga::kHttpServiceAddLibraryEntry:
      return kAddLibraryEntry;
    case taiga::kHttpServiceDeleteLibraryEntry:
      return kDeleteLibraryEntry;
    case taiga::kHttpServiceGetLibraryEntries:
      return kGetLibraryEntries;
    case taiga::kHttpServiceUpdateLibraryEntry:
      return kUpdateLibraryEntry;
    default:
      return kGenericRequest;
  }
}

taiga::HttpClientMode RequestTypeToClientMode(RequestType request_type) {
  switch (request_type) {
    case kAuthenticateUser:
      return taiga::kHttpServiceAuthenticateUser;
    case kGetMetadataById:
      return taiga::kHttpServiceGetMetadataById;
    case kSearchTitle:
      return taiga::kHttpServiceSearchTitle;
    case kAddLibraryEntry:
      return taiga::kHttpServiceAddLibraryEntry;
    case kDeleteLibraryEntry:
      return taiga::kHttpServiceDeleteLibraryEntry;
    case kGetLibraryEntries:
      return taiga::kHttpServiceGetLibraryEntries;
    case kUpdateLibraryEntry:
      return taiga::kHttpServiceUpdateLibraryEntry;
    default:
      return taiga::kHttpSilent;
  }
}

}  // namespace sync