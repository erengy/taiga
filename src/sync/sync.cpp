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

#include "base/crypto.h"
#include "base/string.h"
#include "base/url.h"
#include "media/anime_db.h"
#include "media/anime_season.h"
#include "media/anime_util.h"
#include "media/library/queue.h"
#include "sync/manager.h"
#include "sync/sync.h"
#include "taiga/http.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "ui/dialog.h"
#include "ui/translate.h"
#include "ui/ui.h"

namespace sync {

bool AuthenticateUser() {
  if (!IsUserAuthenticationAvailable()) {
    ui::ChangeStatusText(
        L"Cannot authenticate. Account settings unavailable.");
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
  if (!IsUserAccountAvailable()) {
    ui::ChangeStatusText(
        L"Cannot get user information. Account settings unavailable.");
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
  request.data[L"season"] = ToLower_Copy(ui::TranslateSeasonName(season.name));
  ServiceManager.MakeRequest(request);
}

void SearchTitle(std::wstring title, int id) {
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

  if (library::queue.GetItemCount()) {
    library::queue.Check(false);
  } else {
    GetLibraryEntries();
  }
}

void UpdateLibraryEntry(const library::QueueItem& queue_item) {
  const auto request_type = ClientModeToRequestType(
      static_cast<taiga::HttpClientMode>(queue_item.mode));

  Request request(request_type);
  SetActiveServiceForRequest(request);
  if (!AddAuthenticationToRequest(request))
    return;
  AddServiceDataToRequest(request, queue_item.anime_id);

  if (queue_item.episode)
    request.data[L"episode"] = ToWstr(*queue_item.episode);
  if (queue_item.status)
    request.data[L"status"] = ToWstr(*queue_item.status);
  if (queue_item.score)
    request.data[L"score"] = ToWstr(*queue_item.score);
  if (queue_item.date_start)
    request.data[L"date_start"] = queue_item.date_start->to_string();
  if (queue_item.date_finish)
    request.data[L"date_finish"] = queue_item.date_finish->to_string();
  if (queue_item.enable_rewatching)
    request.data[L"enable_rewatching"] = ToWstr(*queue_item.enable_rewatching);
  if (queue_item.rewatched_times)
    request.data[L"rewatched_times"] = ToWstr(*queue_item.rewatched_times);
  if (queue_item.tags)
    request.data[L"tags"] = *queue_item.tags;
  if (queue_item.notes)
    request.data[L"notes"] = *queue_item.notes;

  ServiceManager.MakeRequest(request);
}

void DownloadImage(int id, const std::wstring& image_url) {
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
    if (!IsUserAuthenticationAvailable())
      return false;  // Authentication is required but not available

  auto service = taiga::GetCurrentService();
  request.data[service->canonical_name() + L"-username"] =
      taiga::GetCurrentUsername();
  request.data[service->canonical_name() + L"-password"] =
      taiga::GetCurrentPassword();

  switch (service->id()) {
    case sync::kKitsu:
      request.data[service->canonical_name() + L"-email"] =
          taiga::settings.GetSyncServiceKitsuEmail();
      break;
  }

  return true;
}

void AddPageOffsetToRequest(const int offset, Request& request) {
  switch (taiga::GetCurrentServiceId()) {
    case sync::kKitsu:
    case sync::kAniList:
      request.data[L"page_offset"] = ToWstr(offset);
      break;
  }
}

bool AddServiceDataToRequest(Request& request, int id) {
  request.data[L"taiga-id"] = ToWstr(id);

  auto anime_item = anime::db.Find(id);

  if (!anime_item)
    return false;

  auto add_data = [&](sync::ServiceId service_id) {
    const auto canonical_name = ServiceManager.service(service_id)->canonical_name();
    request.data[canonical_name + L"-id"] = anime_item->GetId(service_id);
    request.data[canonical_name + L"-library-id"] = anime_item->GetMyId();
  };

  add_data(kMyAnimeList);
  add_data(kKitsu);
  add_data(kAniList);

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
  return service->user().authenticated;
}

void InvalidateUserAuthentication() {
  auto service = taiga::GetCurrentService();
  service->user().authenticated = false;
}

bool IsUserAccountAvailable() {
  switch (taiga::GetCurrentServiceId()) {
    case sync::kMyAnimeList:
      return !taiga::settings.GetSyncServiceMalUsername().empty();
    case sync::kKitsu:
      return !taiga::settings.GetSyncServiceKitsuEmail().empty() ||
             !taiga::settings.GetSyncServiceKitsuUsername().empty();
    case sync::kAniList:
      return !taiga::settings.GetSyncServiceAniListUsername().empty();
  }

  return false;
}

bool IsUserAuthenticationAvailable() {
  switch (taiga::GetCurrentServiceId()) {
    case sync::kMyAnimeList:
      return !taiga::settings.GetSyncServiceMalUsername().empty() &&
             !taiga::settings.GetSyncServiceMalPassword().empty();
    case sync::kKitsu:
      return (!taiga::settings.GetSyncServiceKitsuEmail().empty() ||
              !taiga::settings.GetSyncServiceKitsuUsername().empty()) &&
             !taiga::settings.GetSyncServiceKitsuPassword().empty();
    case sync::kAniList:
      return !taiga::settings.GetSyncServiceAniListUsername().empty() &&
             !taiga::settings.GetSyncServiceAniListToken().empty();
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

bool ServiceSupportsRequestType(RequestType request_type) {
  const auto service_id = taiga::GetCurrentServiceId();

  switch (request_type) {
    case kGetUser:
      switch (service_id) {
        case sync::kKitsu:
          return true;
        default:
          return false;
      }
    case kGetSeason:
      switch (service_id) {
        case sync::kKitsu:
        case sync::kAniList:
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