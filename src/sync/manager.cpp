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

#include "base/string.h"
#include "library/anime_db.h"
#include "library/anime_season.h"
#include "library/discover.h"
#include "library/history.h"
#include "library/resource.h"
#include "sync/kitsu.h"
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
  services_[kKitsu].reset(new kitsu::Service());
}

Manager::~Manager() {
  // Services will automatically free themselves
}

Service* Manager::service(ServiceId service_id) const {
  auto it = services_.find(service_id);
  
  if (it != services_.end())
    return it->second.get();

  return nullptr;
}

Service* Manager::service(const string_t& canonical_name) const {
  for (const auto& pair : services_)
    if (canonical_name == pair.second.get()->canonical_name())
      return pair.second.get();

  return nullptr;
}

ServiceId Manager::GetServiceIdByName(const string_t& canonical_name) const {
  auto found_service = service(canonical_name);

  if (found_service)
    return static_cast<ServiceId>(found_service->id());

  return kTaiga;
}

string_t Manager::GetServiceNameById(ServiceId service_id) const {
  auto found_service = service(service_id);

  if (found_service)
    return found_service->canonical_name();

  return L"taiga";
}

////////////////////////////////////////////////////////////////////////////////

void Manager::MakeRequest(Request& request) {
  for (const auto& pair : services_) {
    if (request.service_id == kAllServices ||
        request.service_id == pair.first) {
      // Create a new HTTP request, and store its UID alongside the service
      // request until we receive a response
      HttpRequest http_request;
      requests_.insert(std::make_pair(http_request.uid, request));

      // Make sure we store the actual service ID
      if (request.service_id == kAllServices)
        requests_[http_request.uid].service_id = pair.first;

      // Let the service build the HTTP request
      pair.second->BuildRequest(request, http_request);
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
  Service& service = *services_[response.service_id].get();

  int anime_id = ::anime::ID_UNKNOWN;
  if (request.data.count(L"taiga-id"))
    anime_id = ToInt(request.data[L"taiga-id"]);
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  switch (response.type) {
    case kAuthenticateUser:
    case kGetUser:
      service.set_authenticated(false);
      ui::OnLogout();
      ui::ChangeStatusText(response.data[L"error"]);
      break;
    case kGetMetadataById:
      ui::OnLibraryEntryChangeFailure(anime_id, response.data[L"error"]);
      if (response.data.count(L"invalid_id")) {
        const bool in_list = anime_item && anime_item->IsInList();
        if (AnimeDatabase.DeleteItem(anime_id)) {
          AnimeDatabase.SaveDatabase();
          if (in_list)
            AnimeDatabase.SaveList();
        }
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
      const auto& username = service.user().username;
      if (!username.empty()) {
        // Update settings with the returned value for the correct letter case
        switch (response.service_id) {
          case kMyAnimeList:
            Settings.Set(taiga::kSync_Service_Mal_Username, username);
            break;
          case kKitsu:
            Settings.Set(taiga::kSync_Service_Kitsu_Username, username);
            break;
        }
      }
      service.set_authenticated(true);
      ui::OnLogin();
      if (response.service_id == kKitsu) {
        // We need to make an additional request to get the user ID
        GetUser();
      } else {
        Synchronize();
      }
      break;
    }

    case kGetUser: {
      ui::OnLogin();
      if (service.authenticated()) {
        Synchronize();
      } else {
        GetLibraryEntries();
      }
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

    case kGetSeason: {
      const auto current_page = ToInt(request.data[L"page_offset"]);
      const auto next_page = ToInt(response.data[L"next_page_offset"]);

      if (current_page == 0)  // first page
        SeasonDatabase.items.clear();

      std::vector<std::wstring> ids;
      Split(response.data[L"ids"], L",", ids);
      for (const auto& id_str : ids) {
        const int id = ToInt(id_str);
        SeasonDatabase.items.push_back(id);
        ui::OnLibraryEntryChange(id);
      }

      if (next_page > 0) {
        GetSeason(anime::Season(request.data[L"season"] + L" " +
                                request.data[L"year"]), next_page);
      } else {
        ui::ClearStatusText();
        ui::OnLibraryGetSeason();
        for (const auto& id : SeasonDatabase.items) {
          ImageDatabase.Load(id, true, true);
        }
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
      const auto next_page = ToInt(response.data[L"next_page_offset"]);
      if (next_page > 0) {
        GetLibraryEntries(next_page);
      } else {
        AnimeDatabase.SaveDatabase();
        AnimeDatabase.SaveList();
        ui::ChangeStatusText(L"Successfully downloaded the list.");
        ui::OnLibraryChange();
      }
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