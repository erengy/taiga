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

#include "sync/manager.h"

#include "base/string.h"
#include "media/anime_db.h"
#include "media/anime_season.h"
#include "media/anime_season_db.h"
#include "media/library/queue.h"
#include "ui/resource.h"
#include "sync/anilist.h"
#include "sync/kitsu.h"
#include "sync/myanimelist.h"
#include "sync/sync.h"
#include "taiga/http.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "ui/ui.h"

namespace sync {

Manager::Manager() {
  // Create services
  services_[kMyAnimeList].reset(new myanimelist::Service());
  services_[kKitsu].reset(new kitsu::Service());
  services_[kAniList].reset(new anilist::Service());
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

Service* Manager::service(const std::wstring& canonical_name) const {
  for (const auto& [id, service] : services_) {
    if (canonical_name == service.get()->canonical_name())
      return service.get();
  }

  return nullptr;
}

ServiceId GetServiceIdBySlug(const std::wstring& slug) {
  static const std::map<std::wstring, ServiceId> services{
    {L"myanimelist", kMyAnimeList},
    {L"kitsu", kKitsu},
    {L"anilist", kAniList},
  };

  const auto it = services.find(slug);
  return it != services.end() ? it->second : kTaiga;
}

std::wstring GetServiceNameById(const ServiceId service_id) {
  switch (service_id) {
    case kMyAnimeList: return L"MyAnimeList";
    case kKitsu: return L"Kitsu";
    case kAniList: return L"AniList";
    default: return L"Taiga";
  }
}

std::wstring GetServiceSlugById(const ServiceId service_id) {
  switch (service_id) {
    case kMyAnimeList: return L"myanimelist";
    case kKitsu: return L"kitsu";
    case kAniList: return L"anilist";
    default: return L"taiga";
  }
}

////////////////////////////////////////////////////////////////////////////////

void Manager::MakeRequest(Request& request) {
  for (const auto& [service_id, service] : services_) {
    if (request.service_id == kAllServices ||
        request.service_id == service_id) {
      // Create a new HTTP request, and store its UID alongside the service
      // request until we receive a response
      HttpRequest http_request;
      requests_.insert(std::make_pair(http_request.uid, request));

      // Make sure we store the actual service ID
      if (request.service_id == kAllServices)
        requests_[http_request.uid].service_id = service_id;

      // Let the service build the HTTP request
      service->BuildRequest(request, http_request);
      http_request.url.Crack(http_request.url.Build());

      // Make the request
      ConnectionManager.MakeRequest(http_request,
                                    RequestTypeToClientMode(request.type));
    }
  }
}

void Manager::HandleHttpError(HttpResponse& http_response, std::wstring error) {
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
  auto anime_item = anime::db.Find(anime_id);

  switch (response.type) {
    case kAuthenticateUser:
    case kGetUser:
      service.user().authenticated = false;
      ui::OnLogout();
      ui::ChangeStatusText(response.data[L"error"]);
      break;
    case kGetMetadataById:
      ui::OnLibraryEntryChangeFailure(anime_id);
      if (response.data.count(L"invalid_id")) {
        const bool in_list = anime_item && anime_item->IsInList();
        if (anime::db.DeleteItem(anime_id)) {
          anime::db.SaveDatabase();
          if (in_list)
            anime::db.SaveList();
        }
      }
      break;
    case kGetSeason:
      ui::OnSeasonLoadFail();
      ui::ChangeStatusText(response.data[L"error"]);
      break;
    case kGetLibraryEntries:
      ui::OnLibraryChangeFailure();
      ui::ChangeStatusText(response.data[L"error"]);
      break;
    case kAddLibraryEntry:
    case kDeleteLibraryEntry:
    case kUpdateLibraryEntry:
      library::queue.updating = false;
      ui::OnLibraryUpdateFailure(anime_id, response.data[L"error"],
                                 response.data.count(L"not_approved"));
      break;
    default:
      ui::ChangeStatusText(response.data[L"error"]);
      break;
  }
}

void Manager::HandleResponse(Response& response, HttpResponse& http_response) {
  Request& request = requests_[http_response.uid];

  if (request.data.count(L"taiga-id"))
    response.data[L"taiga-id"] = request.data[L"taiga-id"];

  // Let the service do its thing
  Service& service = *services_[response.service_id].get();
  service.HandleResponse(response, http_response);

  // Check for error
  if (response.data.count(L"error")) {
    HandleError(response, http_response);
    return;
  }

  int anime_id = ::anime::ID_UNKNOWN;
  if (request.data.count(L"taiga-id"))
    anime_id = ToInt(request.data[L"taiga-id"]);
  auto anime_item = anime::db.Find(anime_id);

  const auto update_username = [&service, &response]() {
    const auto& username = service.user().username;
    if (!username.empty()) {
      // Update settings with the returned value for the correct letter case
      switch (response.service_id) {
        case kMyAnimeList:
          taiga::settings.SetSyncServiceMalUsername(username);
          break;
        case kKitsu:
          taiga::settings.SetSyncServiceKitsuUsername(username);
          break;
        case kAniList:
          taiga::settings.SetSyncServiceAniListUsername(username);
          break;
      }
    }
  };

  const auto update_tokens = [&service, &response]() {
    switch (response.service_id) {
      case kMyAnimeList:
        if (!service.user().access_token.empty())
          taiga::settings.SetSyncServiceMalAccessToken(service.user().access_token);
        if (!service.user().refresh_token.empty())
          taiga::settings.SetSyncServiceMalRefreshToken(service.user().refresh_token);
        break;
    }
  };

  switch (response.type) {
    case kAuthenticateUser: {
      update_username();
      update_tokens();
      service.user().authenticated = true;
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
      update_username();
      ui::OnLogin();
      if (service.user().authenticated) {
        Synchronize();
      } else {
        GetLibraryEntries();
      }
      break;
    }

    case kGetMetadataById: {
      ui::OnLibraryEntryChange(anime_id);
      break;
    }

    case kGetSeason: {
      const auto current_page = ToInt(request.data[L"page_offset"]);
      const auto next_page = ToInt(response.data[L"next_page_offset"]);

      if (current_page == 0)  // first page
        anime::season_db.items.clear();

      std::vector<std::wstring> ids;
      Split(response.data[L"ids"], L",", ids);
      for (const auto& id_str : ids) {
        const int id = ToInt(id_str);
        anime::season_db.items.push_back(id);
        ui::OnLibraryEntryChange(id);
      }

      if (next_page > 0) {
        GetSeason(anime::Season(request.data[L"season"] + L" " +
                                request.data[L"year"]), next_page);
      } else {
        anime::season_db.Review();
        ui::ClearStatusText();
        ui::OnLibraryGetSeason();
        for (const auto& id : anime::season_db.items) {
          ui::image_db.Load(id, true, true);
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
        anime::db.SaveDatabase();
        anime::db.SaveList();
        ui::ChangeStatusText(L"Successfully downloaded the list.");
        ui::OnLibraryChange();
      }
      break;
    }

    case kAddLibraryEntry:
    case kDeleteLibraryEntry:
    case kUpdateLibraryEntry: {
      library::queue.updating = false;
      ui::ClearStatusText();

      const auto queue_item = library::queue.GetCurrentItem();
      if (queue_item) {
        anime::db.UpdateItem(*queue_item);
        anime::db.SaveList();
        library::queue.Remove();
        library::queue.Check(false);
      }

      break;
    }
  }
}

}  // namespace sync
