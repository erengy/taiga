/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
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

#pragma once

#include <string>
#include <vector>

#include <windows/win/taskbar.h>

namespace anime {
enum class SeriesStatus;
class Episode;
class Item;
};
namespace library {
struct QueueItem;
}
namespace sync {
enum class ServiceId;
}
namespace track {
class Feed;
}

namespace ui {

enum class TipType {
  Default,
  NowPlaying,
  Search,
  Torrent,
  UpdateFailed,
  NotApproved,
};

class Taskbar : public win::Taskbar {
public:
  TipType tip_type = TipType::Default;
};

constexpr int kControlMargin = 6;
constexpr unsigned int kAppSysTrayId = 74164;  // TAIGA ^_^

inline Taskbar taskbar;
inline win::TaskbarList taskbar_list;

void ChangeStatusText(const std::wstring& status);
void ClearStatusText();
void SetSharedCursor(LPCWSTR name);
int StatusToIcon(anime::SeriesStatus status);

void DisplayErrorMessage(const std::wstring& text, const std::wstring& caption);
bool EnterAuthorizationPin(const std::wstring& service, std::wstring& auth_pin);

void OnLibraryChange();
void OnLibraryEntryAdd(int id);
void OnLibraryEntryChange(int id);
void OnLibraryEntryDelete(int id);
void OnLibraryEntryImageChange(int id);
void OnLibraryGetSeason();
void OnLibrarySearchTitle(const std::vector<int> ids);
void OnLibraryEntryChangeFailure(int id);
void OnLibraryUpdateFailure(int id, const std::wstring& reason, bool not_approved);

bool OnLibraryEntriesEditDelete(const std::vector<int> ids);
int OnLibraryEntriesEditEpisode(const std::vector<int> ids);
bool OnLibraryEntriesEditNotes(const std::vector<int> ids, std::wstring& notes);

void OnHistoryAddItem(const library::QueueItem& queue_item);
void OnHistoryChange(const library::QueueItem* queue_item = nullptr);
bool OnHistoryClear();
int OnHistoryQueueClear();
int OnHistoryProcessConfirmationQueue(anime::Episode& episode);

void OnAnimeDelete(int id, const std::wstring& title);
void OnAnimeEpisodeNotFound(const std::wstring& title);
bool OnAnimeFolderNotFound();
void OnAnimeWatchingStart(const anime::Item& anime_item, const anime::Episode& episode);
void OnAnimeWatchingEnd(const anime::Item& anime_item, const anime::Episode& episode);

bool OnRecognitionCancelConfirm();
void OnRecognitionFail();

bool OnSettingsEditAdvanced(const std::wstring& description, bool is_password, std::wstring& value);
void OnSettingsChange();
void OnSettingsLibraryFoldersEmpty();
void OnSettingsRestoreDefaults();
void OnSettingsServiceChange();
bool OnSettingsServiceChangeConfirm(const sync::ServiceId current_service, const sync::ServiceId new_service);
void OnSettingsServiceChangeFailed();
void OnSettingsThemeChange();
void OnSettingsUserChange();

void OnEpisodeAvailabilityChange(int id);
void OnScanAvailableEpisodesFinished();

void OnFeedCheck(bool success);
void OnFeedDownloadSuccess(bool is_magnet_link);
void OnFeedDownloadError(const std::wstring& message);
bool OnFeedNotify(const track::Feed& feed);

void OnMircNotRunning(bool testing = false);
void OnMircDdeInitFail(bool testing = false);
void OnMircDdeConnectionFail(bool testing = false);
void OnMircDdeConnectionSuccess(const std::vector<std::wstring>& channels, bool testing = false);

void OnMalRequestAccessTokenSuccess();
void OnMalRequestAccessTokenError(const std::wstring& description);

void OnLogin();

void OnUpdateAvailable();
void OnUpdateNotAvailable(bool relations = false);
void OnUpdateFailed();
void OnUpdateFinished();

}  // namespace ui
