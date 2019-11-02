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

#include "sync/sync.h"

#include "base/file.h"
#include "base/format.h"
#include "base/string.h"
#include "media/anime_db.h"
#include "media/anime_season.h"
#include "media/anime_season_db.h"
#include "media/anime_util.h"
#include "media/library/queue.h"
#include "sync/anilist.h"
#include "sync/kitsu.h"
#include "sync/myanimelist.h"
#include "sync/service.h"
#include "taiga/http_new.h"
#include "taiga/settings.h"
#include "ui/dialog.h"
#include "ui/resource.h"
#include "ui/ui.h"

namespace sync {

bool AuthenticateUser() {
  ui::EnableDialogInput(ui::Dialog::Main, false);

  switch (GetCurrentServiceId()) {
    case kMyAnimeList:
      myanimelist::AuthenticateUser();
      break;
    case kKitsu:
      kitsu::AuthenticateUser();
      break;
    case kAniList:
      anilist::AuthenticateUser();
      break;
  }

  return true;
}

void GetUser() {
  ui::EnableDialogInput(ui::Dialog::Main, false);

  switch (GetCurrentServiceId()) {
    case kMyAnimeList:
      myanimelist::GetUser();
      break;
    case kKitsu:
      kitsu::GetUser();
      break;
    case kAniList:
      break;
  }
}

void GetLibraryEntries() {
  ui::EnableDialogInput(ui::Dialog::Main, false);

  switch (GetCurrentServiceId()) {
    case kMyAnimeList:
      myanimelist::GetLibraryEntries();
      break;
    case kKitsu:
      kitsu::GetLibraryEntries();
      break;
    case kAniList:
      anilist::GetLibraryEntries();
      break;
  }
}

void GetMetadataById(const int id) {
  switch (GetCurrentServiceId()) {
    case kMyAnimeList:
      myanimelist::GetMetadataById(id);
      break;
    case kKitsu:
      kitsu::GetMetadataById(id);
      break;
    case kAniList:
      anilist::GetMetadataById(id);
      break;
  }
}

void GetSeason(const anime::Season season) {
  switch (GetCurrentServiceId()) {
    case kMyAnimeList:
      myanimelist::GetSeason(season);
      break;
    case kKitsu:
      kitsu::GetSeason(season);
      break;
    case kAniList:
      anilist::GetSeason(season);
      break;
  }
}

void SearchTitle(const std::wstring& title, const int id) {
  switch (GetCurrentServiceId()) {
    case kMyAnimeList:
      myanimelist::SearchTitle(title);
      break;
    case kKitsu:
      kitsu::SearchTitle(title);
      break;
    case kAniList:
      anilist::SearchTitle(title);
      break;
  }
}

void Synchronize() {
  bool authenticated = UserAuthenticated();

  if (!authenticated) {
    authenticated = AuthenticateUser();

    // Special case where we allow downloading lists without authentication
    if (!authenticated && !taiga::GetCurrentUsername().empty()) {
      switch (GetCurrentServiceId()) {
        case sync::kKitsu:
          GetUser();
          break;
        case sync::kAniList:
          GetLibraryEntries();
          break;
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
  switch (GetCurrentServiceId()) {
    case kMyAnimeList:
      myanimelist::UpdateLibraryEntry(queue_item);
      break;
    case kKitsu:
      kitsu::UpdateLibraryEntry(queue_item);
      break;
    case kAniList:
      anilist::UpdateLibraryEntry(queue_item);
      break;
  }
}

void DownloadImage(const int anime_id, const std::wstring& image_url) {
  if (image_url.empty())
    return;

  taiga::http::Request request;
  request.set_target(WstrToStr(image_url));

  const auto on_response = [anime_id](const taiga::http::Response& response) {
    if (hypp::status::to_class(response.status_code()) == 200) {
      SaveToFile(response.body(), anime::GetImagePath(anime_id));
      if (ui::image_db.Reload(anime_id))
        ui::OnLibraryEntryImageChange(anime_id);
    } else if (response.status_code() == 404) {
      if (const auto anime_item = anime::db.Find(anime_id))
        anime_item->SetImageUrl({});
    }
  };

  taiga::http::Send(request, nullptr, on_response);
}

////////////////////////////////////////////////////////////////////////////////

bool UserAuthenticated() {
  switch (GetCurrentServiceId()) {
    case kMyAnimeList:
      return myanimelist::IsUserAuthenticated();
    case kKitsu:
      return kitsu::IsUserAuthenticated();
    case kAniList:
      return anilist::IsUserAuthenticated();
    default:
      return false;
  }
}

void InvalidateUserAuthentication() {
  // @TODO
}

bool IsUserAccountAvailable() {
  switch (GetCurrentServiceId()) {
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

////////////////////////////////////////////////////////////////////////////////

void AfterGetLibrary() {
  anime::db.SaveDatabase();
  anime::db.SaveList();

  ui::ChangeStatusText(L"Successfully downloaded the list.");
  ui::OnLibraryChange();
}

void AfterGetSeason() {
  anime::season_db.Review();

  ui::ClearStatusText();
  ui::OnLibraryGetSeason();

  for (const auto& anime_id : anime::season_db.items) {
    ui::image_db.Load(anime_id, true, true);
  }
}

void AfterLibraryUpdate() {
  library::queue.updating = false;
  ui::ClearStatusText();

  if (const auto queue_item = library::queue.GetCurrentItem()) {
    anime::db.UpdateItem(*queue_item);
    anime::db.SaveList();
    library::queue.Remove();
    library::queue.Check(false);
  }
}

bool OnTransfer(const taiga::http::Transfer& transfer,
                const std::wstring& status) {
  ui::ChangeStatusText(
      L"{} ({})"_format(status, taiga::http::util::to_string(transfer)));
  return true;
}

}  // namespace sync
