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
#include "taiga/http.h"
#include "taiga/settings.h"
#include "ui/dialog.h"
#include "ui/resource.h"
#include "ui/translate.h"
#include "ui/ui.h"

namespace sync {

bool AuthenticateUser() {
  ui::EnableDialogInput(ui::Dialog::Main, false);
  ui::ChangeStatusText(L"{}: Authenticating user..."_format(
      sync::GetCurrentServiceName()));

  switch (GetCurrentServiceId()) {
    case ServiceId::MyAnimeList:
      myanimelist::AuthenticateUser();
      break;
    case ServiceId::Kitsu:
      kitsu::AuthenticateUser();
      break;
    case ServiceId::AniList:
      anilist::AuthenticateUser();
      break;
  }

  return true;
}

void GetUser() {
  ui::EnableDialogInput(ui::Dialog::Main, false);
  ui::ChangeStatusText(L"{}: Retrieving user information..."_format(
      sync::GetCurrentServiceName()));

  switch (GetCurrentServiceId()) {
    case ServiceId::MyAnimeList:
      myanimelist::GetUser();
      break;
    case ServiceId::Kitsu:
      kitsu::GetUser();
      break;
    case ServiceId::AniList:
      break;
  }
}

void GetLibraryEntries() {
  ui::EnableDialogInput(ui::Dialog::Main, false);
  ui::ChangeStatusText(L"{}: Retrieving anime list..."_format(
      sync::GetCurrentServiceName()));

  switch (GetCurrentServiceId()) {
    case ServiceId::MyAnimeList:
      myanimelist::GetLibraryEntries();
      break;
    case ServiceId::Kitsu:
      kitsu::GetLibraryEntries();
      break;
    case ServiceId::AniList:
      anilist::GetLibraryEntries();
      break;
  }
}

void GetMetadataById(const int id) {
  ui::ChangeStatusText(L"{}: Retrieving anime information..."_format(
      sync::GetCurrentServiceName()));

  switch (GetCurrentServiceId()) {
    case ServiceId::MyAnimeList:
      myanimelist::GetMetadataById(id);
      break;
    case ServiceId::Kitsu:
      kitsu::GetMetadataById(id);
      break;
    case ServiceId::AniList:
      anilist::GetMetadataById(id);
      break;
  }
}

void GetSeason(const anime::Season season) {
  ui::EnableDialogInput(ui::Dialog::Seasons, false);
  ui::ChangeStatusText(L"{}: Retrieving {} anime season..."_format(
      sync::GetCurrentServiceName(), ui::TranslateSeason(season)));

  switch (GetCurrentServiceId()) {
    case ServiceId::MyAnimeList:
      myanimelist::GetSeason(season);
      break;
    case ServiceId::Kitsu:
      kitsu::GetSeason(season);
      break;
    case ServiceId::AniList:
      anilist::GetSeason(season);
      break;
  }
}

void SearchTitle(const std::wstring& title) {
  ui::ChangeStatusText(L"{}: Searching for \"{}\"..."_format(
      sync::GetCurrentServiceName(), title));

  switch (GetCurrentServiceId()) {
    case ServiceId::MyAnimeList:
      myanimelist::SearchTitle(title);
      break;
    case ServiceId::Kitsu:
      kitsu::SearchTitle(title);
      break;
    case ServiceId::AniList:
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
        case ServiceId::Kitsu:
          GetUser();
          break;
        case ServiceId::AniList:
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
  const auto anime_item = anime::db.Find(queue_item.anime_id);
  if (!anime_item)
    return;

  ui::ChangeStatusText(L"{}: Updating anime list... ({})"_format(
      sync::GetCurrentServiceName(), anime::GetPreferredTitle(*anime_item)));

  switch (GetCurrentServiceId()) {
    case ServiceId::MyAnimeList:
      myanimelist::UpdateLibraryEntry(queue_item);
      break;
    case ServiceId::Kitsu:
      kitsu::UpdateLibraryEntry(queue_item);
      break;
    case ServiceId::AniList:
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
    case ServiceId::MyAnimeList:
      return myanimelist::IsUserAuthenticated();
    case ServiceId::Kitsu:
      return kitsu::IsUserAuthenticated();
    case ServiceId::AniList:
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
    case ServiceId::MyAnimeList:
      return !taiga::settings.GetSyncServiceMalUsername().empty();
    case ServiceId::Kitsu:
      return !taiga::settings.GetSyncServiceKitsuEmail().empty() ||
             !taiga::settings.GetSyncServiceKitsuUsername().empty();
    case ServiceId::AniList:
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
