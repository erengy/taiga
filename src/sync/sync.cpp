/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
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

void AuthenticateUser() {
  ui::EnableDialogInput(ui::Dialog::Main, false);
  ui::ChangeStatusText(L"{}: Authenticating user..."_format(
      GetCurrentServiceName()));

  switch (GetCurrentServiceId()) {
    case ServiceId::MyAnimeList:
      myanimelist::RefreshAccessToken();
      break;
    case ServiceId::Kitsu:
      kitsu::AuthenticateUser();
      break;
    case ServiceId::AniList:
      anilist::AuthenticateUser();
      break;
  }
}

void GetUser() {
  ui::EnableDialogInput(ui::Dialog::Main, false);
  ui::ChangeStatusText(L"{}: Retrieving user information..."_format(
      GetCurrentServiceName()));

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
      GetCurrentServiceName()));

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
      GetCurrentServiceName()));

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
      GetCurrentServiceName(), ui::TranslateSeason(season)));

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
      GetCurrentServiceName(), title));

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
  if (!IsUserAuthenticated()) {
    if (IsUserAuthenticationAvailable()) {
      AuthenticateUser();

    // Allow downloading lists without authentication
    } else if (IsUserAccountAvailable()) {
      switch (GetCurrentServiceId()) {
        case ServiceId::MyAnimeList:
          // MyAnimeList does not allow this, but we need to display an error
          GetLibraryEntries();
          break;
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

void AddLibraryEntry(const library::QueueItem& queue_item) {
  const auto anime_item = anime::db.Find(queue_item.anime_id);
  if (!anime_item)
    return;

  ui::ChangeStatusText(L"{}: Updating anime list... ({})"_format(
      GetCurrentServiceName(), anime::GetPreferredTitle(*anime_item)));

  switch (GetCurrentServiceId()) {
    case ServiceId::MyAnimeList:
      myanimelist::AddLibraryEntry(queue_item);
      break;
    case ServiceId::Kitsu:
      kitsu::AddLibraryEntry(queue_item);
      break;
    case ServiceId::AniList:
      anilist::AddLibraryEntry(queue_item);
      break;
  }
}

void DeleteLibraryEntry(const int id) {
  const auto anime_item = anime::db.Find(id);
  if (!anime_item)
    return;

  ui::ChangeStatusText(L"{}: Deleting anime from list... ({})"_format(
      GetCurrentServiceName(), anime::GetPreferredTitle(*anime_item)));

  switch (GetCurrentServiceId()) {
    case ServiceId::MyAnimeList:
      myanimelist::DeleteLibraryEntry(id);
      break;
    case ServiceId::Kitsu:
      kitsu::DeleteLibraryEntry(id);
      break;
    case ServiceId::AniList:
      anilist::DeleteLibraryEntry(id);
      break;
  }
}

void UpdateLibraryEntry(const library::QueueItem& queue_item) {
  const auto anime_item = anime::db.Find(queue_item.anime_id);
  if (!anime_item)
    return;

  ui::ChangeStatusText(L"{}: Updating anime list... ({})"_format(
      GetCurrentServiceName(), anime::GetPreferredTitle(*anime_item)));

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
    if (response.status_class() == 200) {
      SaveToFile(response.body(), anime::GetImagePath(anime_id));
      if (ui::image_db.LoadFile(anime_id))
        ui::OnLibraryEntryImageChange(anime_id);
    } else if (response.status_code() == 404) {
      if (const auto anime_item = anime::db.Find(anime_id))
        anime_item->SetImageUrl({});
    }
  };

  taiga::http::Send(request, nullptr, on_response);
}

////////////////////////////////////////////////////////////////////////////////

bool IsUserAuthenticated() {
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
  switch (GetCurrentServiceId()) {
    case ServiceId::MyAnimeList:
      myanimelist::InvalidateUserAuthentication();
      break;
    case ServiceId::Kitsu:
      kitsu::InvalidateUserAuthentication();
      break;
    case ServiceId::AniList:
      anilist::InvalidateUserAuthentication();
      break;
  }
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

bool IsUserAuthenticationAvailable() {
  switch (GetCurrentServiceId()) {
    case ServiceId::MyAnimeList:
      return !taiga::settings.GetSyncServiceMalRefreshToken().empty();
    case ServiceId::Kitsu:
      return !taiga::settings.GetSyncServiceKitsuPassword().empty();
    case ServiceId::AniList:
      return !taiga::settings.GetSyncServiceAniListToken().empty();
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

bool HasProgress(const RequestType type) {
  switch (type) {
    case RequestType::RequestAccessToken:
    case RequestType::RefreshAccessToken:
    case RequestType::AuthenticateUser:
    case RequestType::GetUser:
    case RequestType::GetLibraryEntries:
    case RequestType::GetMetadataById:
    case RequestType::GetSeason:
    case RequestType::SearchTitle:
    case RequestType::AddLibraryEntry:
    case RequestType::DeleteLibraryEntry:
    case RequestType::UpdateLibraryEntry:
    default:
      return true;
  }
}

void OnError(const RequestType type) {
  if (HasProgress(type)) {
    ui::taskbar_list.SetProgressState(TBPF_NOPROGRESS);
  }

  switch (type) {
    case RequestType::RefreshAccessToken:
    case RequestType::AuthenticateUser:
    case RequestType::GetUser:
    case RequestType::GetLibraryEntries:
      ui::EnableDialogInput(ui::Dialog::Main, true);
      break;
    case RequestType::GetSeason:
      ui::OnLibraryGetSeason();
      ui::EnableDialogInput(ui::Dialog::Seasons, true);
      break;
    case RequestType::AddLibraryEntry:
    case RequestType::DeleteLibraryEntry:
    case RequestType::UpdateLibraryEntry:
      library::queue.updating = false;
      break;
  }
}

bool OnTransfer(const RequestType type, const taiga::http::Transfer& transfer,
                const std::wstring& status) {
  if (HasProgress(type)) {
    if (transfer.current == transfer.total) {
      ui::taskbar_list.SetProgressState(TBPF_NOPROGRESS);
    } else if (transfer.total) {
      ui::taskbar_list.SetProgressValue(transfer.current, transfer.total);
    } else {
      ui::taskbar_list.SetProgressState(TBPF_INDETERMINATE);
    }

    ui::ChangeStatusText(
        L"{} ({})"_format(status, taiga::http::util::to_string(transfer)));
  }

  return true;
}

void OnResponse(const RequestType type) {
  if (HasProgress(type)) {
    ui::taskbar_list.SetProgressState(TBPF_NOPROGRESS);
    ui::ClearStatusText();
  }

  switch (type) {
    case RequestType::RequestAccessToken:
      break;

    case RequestType::RefreshAccessToken:
      ui::EnableDialogInput(ui::Dialog::Main, true);
      break;

    case RequestType::AuthenticateUser:
      ui::OnLogin();
      break;

    case RequestType::GetUser:
      ui::OnLogin();
      break;

    case RequestType::GetLibraryEntries:
      anime::db.SaveDatabase();
      anime::db.SaveList();
      ui::OnLibraryChange();
      break;

    case RequestType::GetMetadataById:
      break;

    case RequestType::GetSeason:
      anime::season_db.Review();
      ui::OnLibraryGetSeason();
      ui::image_db.Load(anime::season_db.items);
      ui::EnableDialogInput(ui::Dialog::Seasons, true);
      break;

    case RequestType::SearchTitle:
      break;

    case RequestType::AddLibraryEntry:
    case RequestType::DeleteLibraryEntry:
    case RequestType::UpdateLibraryEntry:
      if (const auto queue_item = library::queue.GetCurrentItem()) {
        anime::db.UpdateItem(*queue_item);
        anime::db.SaveList();
        library::queue.Remove();
      }
      library::queue.updating = false;
      library::queue.Check(false);
      break;
  }
}

void OnInvalidAnimeId(const int id) {
  if (const auto anime_item = anime::db.Find(id)) {
    const bool in_list = anime_item->IsInList();
    if (anime::db.DeleteItem(id)) {
      anime::db.SaveDatabase();
      if (in_list) {
        anime::db.SaveList();
      }
    }
  }
}

}  // namespace sync
