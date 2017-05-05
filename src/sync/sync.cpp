/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#include "base/crypto.h"
#include "base/string.h"
#include "base/url.h"
#include "library/anime_db.h"
#include "library/anime_season.h"
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

bool AuthenticateUser() {
  if (taiga::GetCurrentUsername().empty() ||
      taiga::GetCurrentPassword().empty()) {
    ui::ChangeStatusText(
        L"Cannot authenticate. Username or password not available.");
    return false;
  }

  ui::ChangeStatusText(L"Logging in...");
  ui::EnableDialogInput(ui::Dialog::Main, false);

  Request request(kAuthenticateUser);
  SetActiveServiceForRequest(request);

  if (!AddAuthenticationToRequest(request))
    return false;

  ServiceManager.MakeRequest(request);
  return true;
}

void GetUser() {
  if (taiga::GetCurrentUsername().empty()) {
    ui::ChangeStatusText(
        L"Cannot get user information. Username is not available.");
    return;
  }

  ui::ChangeStatusText(L"Retrieving user information...");
  ui::EnableDialogInput(ui::Dialog::Main, false);

  Request request(kGetUser);
  SetActiveServiceForRequest(request);

  if (!AddAuthenticationToRequest(request))
    return;

  ServiceManager.MakeRequest(request);
}

void GetLibraryEntries(const int offset) {
  const auto service = taiga::GetCurrentService();
  switch (service->id()) {
    case sync::kKitsu:
      if (service->user().id.empty()) {
        ui::ChangeStatusText(
            L"Cannot get anime list. User ID is not available.");
        return;
      }
      break;
  }

  ui::ChangeStatusText(L"Downloading anime list...");
  ui::EnableDialogInput(ui::Dialog::Main, false);

  Request request(kGetLibraryEntries);
  SetActiveServiceForRequest(request);
  if (!AddAuthenticationToRequest(request))
    return;
  AddPageOffsetToRequest(offset, request);
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

void GetSeason(const anime::Season season, const int offset) {
  Request request(kGetSeason);
  SetActiveServiceForRequest(request);
  if (!AddAuthenticationToRequest(request))
    return;
  AddPageOffsetToRequest(offset, request);
  request.data[L"year"] = ToWstr(season.year);
  request.data[L"season"] = ToLower_Copy(season.GetName());
  ServiceManager.MakeRequest(request);
}

void SearchTitle(string_t title, int id) {
  Request request(kSearchTitle);
  SetActiveServiceForRequest(request);
  if (!AddAuthenticationToRequest(request))
    return;
  if (id != anime::ID_UNKNOWN)
    AddServiceDataToRequest(request, id);
  request.data[L"title"] = title;
  ServiceManager.MakeRequest(request);
}

void Synchronize() {
  bool authenticated = UserAuthenticated();

  if (!authenticated) {
    authenticated = AuthenticateUser();
    // Special case where we allow downloading lists without authentication
    if (!authenticated && !taiga::GetCurrentUsername().empty()) {
      if (ServiceSupportsRequestType(kGetUser)) {
        GetUser();
      } else {
        GetLibraryEntries();
      }
    }
    return;
  }

  if (History.queue.GetItemCount()) {
    if (taiga::GetCurrentServiceId() == sync::kKitsu) {
      // Library IDs can be missing if the user has recently upgraded from a
      // previous installation without Kitsu support.
      if (anime::ListHasMissingIds()) {
        GetLibraryEntries();
        return;
      }
    }
    History.queue.Check(false);
  } else {
    GetLibraryEntries();
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
    request.data[L"date_start"] = anime_values.date_start->to_string();
  if (anime_values.date_finish)
    request.data[L"date_finish"] = anime_values.date_finish->to_string();
  if (anime_values.enable_rewatching)
    request.data[L"enable_rewatching"] = ToWstr(*anime_values.enable_rewatching);
  if (anime_values.rewatched_times)
    request.data[L"rewatched_times"] = ToWstr(*anime_values.rewatched_times);
  if (anime_values.tags)
    request.data[L"tags"] = *anime_values.tags;
  if (anime_values.notes)
    request.data[L"notes"] = *anime_values.notes;

  ServiceManager.MakeRequest(request);
}

void DownloadImage(int id, const string_t& image_url) {
  if (image_url.empty())
    return;

  HttpRequest http_request;
  http_request.url = image_url;
  http_request.parameter = id;

  ConnectionManager.MakeRequest(http_request, taiga::kHttpGetLibraryEntryImage);
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

void AddPageOffsetToRequest(const int offset, Request& request) {
  switch (taiga::GetCurrentServiceId()) {
    case sync::kKitsu:
      request.data[L"page_offset"] = ToWstr(offset);
      break;
  }
}

bool AddServiceDataToRequest(Request& request, int id) {
  request.data[L"taiga-id"] = ToWstr(id);

  auto anime_item = AnimeDatabase.FindItem(id);

  if (!anime_item)
    return false;

  auto add_data = [&](sync::ServiceId service_id) {
    const auto canonical_name = ServiceManager.service(service_id)->canonical_name();
    request.data[canonical_name + L"-id"] = anime_item->GetId(service_id);
    request.data[canonical_name + L"-library-id"] = anime_item->GetMyId();
  };

  add_data(kMyAnimeList);
  add_data(kKitsu);

  return true;
}

bool RequestNeedsAuthentication(RequestType request_type, ServiceId service_id) {
  auto service = ServiceManager.service(service_id);
  return service->RequestNeedsAuthentication(request_type);
}

void SetActiveServiceForRequest(Request& request) {
  request.service_id = taiga::GetCurrentServiceId();
}

bool UserAuthenticated() {
  auto service = taiga::GetCurrentService();
  return service->authenticated();
}

void InvalidateUserAuthentication() {
  auto service = taiga::GetCurrentService();
  service->set_authenticated(false);
}

////////////////////////////////////////////////////////////////////////////////

bool ServiceSupportsRequestType(RequestType request_type) {
  const auto service_id = taiga::GetCurrentServiceId();

  switch (request_type) {
    case kGetUser:
    case kGetSeason:
      switch (service_id) {
        case sync::kKitsu:
          return true;
        default:
          return false;
      }
    default:
      return true;
  }
}

RequestType ClientModeToRequestType(taiga::HttpClientMode client_mode) {
  switch (client_mode) {
    case taiga::kHttpServiceAuthenticateUser:
      return kAuthenticateUser;
    case taiga::kHttpServiceGetUser:
      return kGetUser;
    case taiga::kHttpServiceGetMetadataById:
      return kGetMetadataById;
    case taiga::kHttpServiceGetSeason:
      return kGetSeason;
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
    case kGetUser:
      return taiga::kHttpServiceGetUser;
    case kGetMetadataById:
      return taiga::kHttpServiceGetMetadataById;
    case kGetSeason:
      return taiga::kHttpServiceGetSeason;
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