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

#ifndef TAIGA_UI_UI_H
#define TAIGA_UI_UI_H

#include "base/types.h"

namespace anime {
class Episode;
class Item;
};
class HistoryItem;
namespace taiga {
class HttpClient;
}
class Feed;

namespace ui {

void ChangeStatusText(const string_t& status);
void ClearStatusText();
void SetSharedCursor(LPCWSTR name);
int StatusToIcon(int status);

void DisplayErrorMessage(const std::wstring& text, const std::wstring& caption);

void OnHttpError(const taiga::HttpClient& http_client, const string_t& error);
void OnHttpHeadersAvailable(const taiga::HttpClient& http_client);
void OnHttpProgress(const taiga::HttpClient& http_client);
void OnHttpReadComplete(const taiga::HttpClient& http_client);

void OnLibraryChange();
void OnLibraryChangeFailure();
void OnLibraryEntryAdd(int id);
void OnLibraryEntryChange(int id);
void OnLibraryEntryDelete(int id);
void OnLibraryEntryImageChange(int id);
void OnLibrarySearchTitle(int id, const string_t& results);
void OnLibraryEntryChangeFailure(int id, const string_t& reason);
void OnLibraryUpdateFailure(int id, const string_t& reason, bool not_approved);

bool OnLibraryEntriesEditDelete(const std::vector<int> ids);
int OnLibraryEntriesEditEpisode(const std::vector<int> ids);
bool OnLibraryEntriesEditTags(const std::vector<int> ids, std::wstring& tags);

void OnHistoryAddItem(const HistoryItem& history_item);
void OnHistoryChange(const HistoryItem* history_item = nullptr);
bool OnHistoryClear();
int OnHistoryProcessConfirmationQueue(anime::Episode& episode);

void OnAnimeDelete(int id, const string_t& title);
void OnAnimeEpisodeNotFound();
bool OnAnimeFolderNotFound();
void OnAnimeWatchingStart(const anime::Item& anime_item, const anime::Episode& episode);
void OnAnimeWatchingEnd(const anime::Item& anime_item, const anime::Episode& episode);

bool OnRecognitionCancelConfirm();
void OnRecognitionFail();

void OnAnimeListHeaderRatingWarning();

void OnSeasonLoad(bool refresh);
void OnSeasonLoadFail();
bool OnSeasonRefreshRequired();

void OnSettingsAccountEmpty();
bool OnSettingsEditAdvanced(const std::wstring& description, bool is_password, std::wstring& value);
void OnSettingsChange();
void OnSettingsLibraryFoldersEmpty();
void OnSettingsRestoreDefaults();
void OnSettingsServiceChange();
bool OnSettingsServiceChangeConfirm(const string_t& current_service, const string_t& new_service);
void OnSettingsServiceChangeFailed();
void OnSettingsThemeChange();
void OnSettingsUserChange();

void OnScanAvailableEpisodesFinished();

void OnFeedCheck(bool success);
void OnFeedDownload(bool success, const string_t& error);
bool OnFeedNotify(const Feed& feed);

void OnMircNotRunning(bool testing = false);
void OnMircDdeInitFail(bool testing = false);
void OnMircDdeConnectionFail(bool testing = false);
void OnMircDdeConnectionSuccess(const std::vector<std::wstring>& channels, bool testing = false);

void OnTwitterTokenRequest(bool success);
bool OnTwitterTokenEntry(string_t& auth_pin);
void OnTwitterAuth(bool success);
void OnTwitterPost(bool success, const string_t& error);

void OnLogin();
void OnLogout();

void OnUpdateAvailable();
void OnUpdateNotAvailable();
void OnUpdateFinished();

}  // namespace ui

#endif  // TAIGA_UI_UI_H