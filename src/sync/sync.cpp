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

#include "base/encryption.h"
#include "base/foreach.h"
#include "base/string.h"
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "library/history.h"
#include "sync/manager.h"
#include "sync/sync.h"
#include "taiga/http.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "ui/dialog.h"
#include "ui/ui.h"

namespace sync {

void AuthenticateUser() {
  Request request(kAuthenticateUser);
  SetActiveServiceForRequest(request);
  if (!AddAuthenticationToRequest(request))
    return;
  ServiceManager.MakeRequest(request);
}

void GetLibraryEntries() {
  Request request(kGetLibraryEntries);
  SetActiveServiceForRequest(request);
  if (!AddAuthenticationToRequest(request))
    return;
  ServiceManager.MakeRequest(request);
}

void GetMetadataById(int id) {
  Request request(kGetMetadataById);
  SetActiveServiceForRequest(request);
  if (!AddAuthenticationToRequest(request))
    return;
  AddServiceDataToRequest(request, id);
  ServiceManager.MakeRequest(request);
}

void SearchTitle(string_t title) {
  Request request(kSearchTitle);
  SetActiveServiceForRequest(request);
  if (!AddAuthenticationToRequest(request))
    return;
  request.data[L"title"] = title;
  ServiceManager.MakeRequest(request);
}

void Synchronize() {
#ifdef _DEBUG
  Taiga.logged_in = true;
#else
  // TODO: Remove after an authentication method is made available
  if (taiga::GetCurrentServiceId() == sync::kHerro)
    Taiga.logged_in = true;
#endif

  if (!Taiga.logged_in) {
    if (!taiga::GetCurrentUsername().empty() &&
        !taiga::GetCurrentPassword().empty()) {
      // Log in
      ui::ChangeStatusText(L"Logging in...");
      ui::EnableDialogInput(ui::kDialogMain, false);
      AuthenticateUser();
    } else if (!taiga::GetCurrentUsername().empty()) {
      // Download list
      ui::ChangeStatusText(L"Downloading anime list...");
      ui::EnableDialogInput(ui::kDialogMain, false);
      GetLibraryEntries();
    } else {
      ui::ChangeStatusText(
          L"Cannot synchronize, username and password not available");
    }
  } else {
    if (History.queue.GetItemCount() > 0) {
      // Update items in queue
      History.queue.Check(false);
    } else {
      // Retrieve list
      ui::ChangeStatusText(L"Synchronizing anime list...");
      ui::EnableDialogInput(ui::kDialogMain, false);
      GetLibraryEntries();
    }
  }
}

void UpdateLibraryEntry(AnimeValues& anime_values, int id,
                        taiga::HttpClientMode http_client_mode) {
  RequestType request_type = ClientModeToRequestType(http_client_mode);

  Request request(request_type);
  SetActiveServiceForRequest(request);
  if (!AddAuthenticationToRequest(request))
    return;
  AddServiceDataToRequest(request, id);

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
  ConnectionManager.MakeRequest(client, http_request,
                                taiga::kHttpGetLibraryEntryImage);
}

////////////////////////////////////////////////////////////////////////////////

bool AddAuthenticationToRequest(Request& request) {
  if (RequestNeedsAuthentication(request.type, request.service_id))
    if (taiga::GetCurrentUsername().empty() ||
        taiga::GetCurrentPassword().empty())
      return false;  // Authentication is required but not available

  auto service = taiga::GetCurrentService();
  request.data[service->canonical_name() + L"-username"] =
      taiga::GetCurrentUsername();
  request.data[service->canonical_name() + L"-password"] =
      taiga::GetCurrentPassword();

  return true;
}

bool AddServiceDataToRequest(Request& request, int id) {
  request.data[L"taiga-id"] = id;

  auto anime_item = AnimeDatabase.FindItem(id);

  if (!anime_item)
    return false;

  request.data[ServiceManager.service(kMyAnimeList)->canonical_name() + L"-id"] =
      anime_item->GetId(kMyAnimeList);
  request.data[ServiceManager.service(kHerro)->canonical_name() + L"-id"] =
      anime_item->GetId(kHerro);

  return true;
}

bool RequestNeedsAuthentication(RequestType request_type, ServiceId service_id) {
  auto service = ServiceManager.service(service_id);
  return service->RequestNeedsAuthentication(request_type);
}

void SetActiveServiceForRequest(Request& request) {
  request.service_id = taiga::GetCurrentServiceId();
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