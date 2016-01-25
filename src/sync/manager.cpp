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

#include "base/foreach.h"
#include "base/string.h"
#include "library/anime_db.h"
#include "library/history.h"
#include "sync/hummingbird.h"
#include "sync/manager.h"
#include "sync/myanimelist.h"
#include "sync/sync.h"
#include "taiga/http.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "ui/ui.h"

sync::Manager ServiceManager;

namespace sync {

Manager::Manager() {
  // Create services
  services_[kMyAnimeList].reset(new myanimelist::Service());
  services_[kHummingbird].reset(new hummingbird::Service());
}

Manager::~Manager() {
  // Services will automatically free themselves
}

const Service* Manager::service(ServiceId service_id) {
  if (services_.count(service_id))
    return services_[service_id].get();

  return nullptr;
}

const Service* Manager::service(const string_t& canonical_name) {
  foreach_(service, services_)
    if (canonical_name == service->second.get()->canonical_name())
      return service->second.get();

  return nullptr;
}

ServiceId Manager::GetServiceIdByName(const string_t& canonical_name) {
  auto found_service = service(canonical_name);

  if (found_service)
    return static_cast<ServiceId>(found_service->id());

  return kTaiga;
}

string_t Manager::GetServiceNameById(ServiceId service_id) {
  auto found_service = service(service_id);

  if (found_service)
    return found_service->canonical_name();

  return L"taiga";
}

////////////////////////////////////////////////////////////////////////////////

void Manager::MakeRequest(Request& request) {
  foreach_(service, services_) {
    if (request.service_id == kAllServices ||
        request.service_id == service->first) {
      // Create a new HTTP request, and store its UID alongside the service
      // request until we receive a response
      HttpRequest http_request;
      requests_.insert(std::make_pair(http_request.uid, request));

      // Make sure we store the actual service ID
      if (request.service_id == kAllServices)
        requests_[http_request.uid].service_id = service->first;

      // Let the service build the HTTP request
      service->second->BuildRequest(request, http_request);
      http_request.url.Crack(http_request.url.Build());

      // Make the request
      ConnectionManager.MakeRequest(http_request,
                                    RequestTypeToClientMode(request.type));
    }
  }
}

void Manager::HandleHttpError(HttpResponse& http_response, string_t error) {
  win::Lock lock(critical_section_);

  const Request& request = requests_[http_response.uid];

  Response response;
  response.service_id = request.service_id;
  response.type = request.type;
  response.data[L"error"] = error;

  HandleError(response, http_response);

  // FIXME: Not thread-safe. Invalidates iterators on other threads.
//requests_.erase(http_response.uid);
}

void Manager::HandleHttpResponse(HttpResponse& http_response) {
  win::Lock lock(critical_section_);

  const Request& request = requests_[http_response.uid];

  Response response;
  response.service_id = request.service_id;
  response.type = request.type;

  HandleResponse(response, http_response);

  // FIXME: Not thread-safe. Invalidates iterators on other threads.
//requests_.erase(http_response.uid);
}

////////////////////////////////////////////////////////////////////////////////

void Manager::HandleError(Response& response, HttpResponse& http_response) {
  Request& request = requests_[http_response.uid];

  int anime_id = ::anime::ID_UNKNOWN;
  if (request.data.count(L"taiga-id"))
    anime_id = ToInt(request.data[L"taiga-id"]);
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  switch (response.type) {
    case kAuthenticateUser:
      Taiga.logged_in = false;
      ui::OnLogout();
      ui::ChangeStatusText(response.data[L"error"]);
      break;
    case kGetMetadataById:
      ui::OnLibraryEntryChangeFailure(anime_id, response.data[L"error"]);
      if (response.data.count(L"invalid_id")) {
        if (AnimeDatabase.DeleteItem(anime_id))
          AnimeDatabase.SaveDatabase();
      } else {
        // Try making the other request, even though this one failed
        if (response.service_id == kMyAnimeList && anime_item) {
          SearchTitle(anime_item->GetTitle(), anime_id);
        }
      }
      break;
    case kGetLibraryEntries:
      ui::OnLibraryChangeFailure();
      ui::ChangeStatusText(response.data[L"error"]);
      break;
    case kAddLibraryEntry:
    case kDeleteLibraryEntry:
    case kUpdateLibraryEntry:
      History.queue.updating = false;
      ui::OnLibraryUpdateFailure(anime_id, response.data[L"error"],
                                 response.data.count(L"not_approved"));
      break;
    default:
      ui::ChangeStatusText(response.data[L"error"]);
      break;
  }
}

void Manager::HandleResponse(Response& response, HttpResponse& http_response) {
  // Let the service do its thing
  Service& service = *services_[response.service_id].get();
  service.HandleResponse(response, http_response);

  // Check for error
  if (response.data.count(L"error")) {
    HandleError(response, http_response);
    return;
  }

  Request& request = requests_[http_response.uid];

  int anime_id = ::anime::ID_UNKNOWN;
  if (request.data.count(L"taiga-id"))
    anime_id = ToInt(request.data[L"taiga-id"]);
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  switch (response.type) {
    case kAuthenticateUser: {
      string_t username = response.data[service.canonical_name() + L"-username"];
      if (response.service_id == kMyAnimeList && !username.empty()) {
        // Update settings with the returned value for the correct letter case
        Settings.Set(taiga::kSync_Service_Mal_Username, username);
      }
      Taiga.logged_in = true;
      ui::OnLogin();
      Synchronize();
      break;
    }

    case kGetMetadataById: {
      ui::OnLibraryEntryChange(anime_id);
      // We need to make another request, because MyAnimeList doesn't have
      // a proper method in its API for metadata retrieval, and the one we use
      // doesn't provide us enough information.
      if (response.service_id == kMyAnimeList && anime_item) {
        SearchTitle(anime_item->GetTitle(), anime_id);
      }
      break;
    }

    case kSearchTitle: {
      if (!anime_item)
        ui::ClearStatusText();
      ui::OnLibrarySearchTitle(anime_id, response.data[L"ids"]);
      break;
    }

    case kGetLibraryEntries: {
      AnimeDatabase.SaveDatabase();
      AnimeDatabase.SaveList();
      ui::ChangeStatusText(L"Successfully downloaded the list.");
      ui::OnLibraryChange();
      break;
    }

    case kAddLibraryEntry:
    case kDeleteLibraryEntry:
    case kUpdateLibraryEntry: {
      History.queue.updating = false;
      ui::ClearStatusText();

      auto history_item = History.queue.GetCurrentItem();
      if (history_item)
        AnimeDatabase.UpdateItem(*history_item);

      break;
    }
  }
}

}  // namespace sync