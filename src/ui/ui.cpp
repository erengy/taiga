/*
** Taiga
** Copyright (C) 2010-2018, Eren Okka
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

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include <windows/win/task_dialog.h>
#include <windows/win/taskbar.h>

#include "base/file.h"
#include "base/format.h"
#include "base/string.h"
#include "library/anime_db.h"
#include "library/anime_episode.h"
#include "library/anime_util.h"
#include "library/discover.h"
#include "library/history.h"
#include "sync/manager.h"
#include "sync/service.h"
#include "sync/sync.h"
#include "taiga/http.h"
#include "taiga/resource.h"
#include "taiga/script.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "track/media.h"
#include "track/recognition.h"
#include "ui/dlg/dlg_anime_info.h"
#include "ui/dlg/dlg_anime_list.h"
#include "ui/dlg/dlg_history.h"
#include "ui/dlg/dlg_input.h"
#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_search.h"
#include "ui/dlg/dlg_season.h"
#include "ui/dlg/dlg_settings.h"
#include "ui/dlg/dlg_stats.h"
#include "ui/dlg/dlg_torrent.h"
#include "ui/dlg/dlg_update.h"
#include "ui/dlg/dlg_update_new.h"
#include "ui/dialog.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "ui/ui.h"

namespace ui {

Taskbar taskbar;
win::TaskbarList taskbar_list;

void ChangeStatusText(const string_t& status) {
  DlgMain.ChangeStatus(status);
}

void ClearStatusText() {
  DlgMain.ChangeStatus(L"");
}

void SetSharedCursor(LPCWSTR name) {
  SetCursor(reinterpret_cast<HCURSOR>(LoadImage(nullptr, name, IMAGE_CURSOR,
                                                0, 0, LR_SHARED)));
}

int StatusToIcon(int status) {
  switch (status) {
    case anime::kAiring:
      return kIcon16_Green;
    case anime::kFinishedAiring:
      return kIcon16_Blue;
    case anime::kNotYetAired:
      return kIcon16_Red;
    default:
      return kIcon16_Gray;
  }
}

void DisplayErrorMessage(const std::wstring& text,
                         const std::wstring& caption) {
  MessageBox(nullptr, text.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
}

bool EnterAuthorizationPin(const string_t& service, string_t& auth_pin) {
  InputDialog dlg;
  dlg.title = L"{} Authorization"_format(service);
  dlg.info = L"Please enter the PIN shown on the page after "
             L"logging into {}:"_format(service);
  dlg.Show();

  if (dlg.result == IDOK && !dlg.text.empty()) {
    auth_pin = dlg.text;
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

void OnHttpError(const taiga::HttpClient& http_client, const string_t& error) {
  switch (http_client.mode()) {
    case taiga::kHttpSilent:
    case taiga::kHttpServiceGetMetadataById:
    case taiga::kHttpServiceSearchTitle:
    case taiga::kHttpGetLibraryEntryImage:
    case taiga::kHttpTaigaUpdateRelations:
      return;
    case taiga::kHttpServiceAuthenticateUser:
    case taiga::kHttpServiceGetUser:
    case taiga::kHttpServiceGetLibraryEntries:
    case taiga::kHttpServiceAddLibraryEntry:
    case taiga::kHttpServiceDeleteLibraryEntry:
    case taiga::kHttpServiceUpdateLibraryEntry:
      ChangeStatusText(error);
      DlgMain.EnableInput(true);
      break;
    case taiga::kHttpFeedCheck:
    case taiga::kHttpFeedCheckAuto:
    case taiga::kHttpFeedDownload:
      ChangeStatusText(error);
      DlgTorrent.EnableInput();
      break;
    case taiga::kHttpServiceGetSeason:
    case taiga::kHttpSeasonsGet:
      ChangeStatusText(error);
      DlgSeason.EnableInput();
      break;
    case taiga::kHttpTwitterRequest:
    case taiga::kHttpTwitterAuth:
    case taiga::kHttpTwitterPost:
      ChangeStatusText(error);
      break;
    case taiga::kHttpTaigaUpdateCheck:
      if (DlgMain.IsWindow())  // Don't display error message on automatic checks
        MessageBox(DlgUpdate.GetWindowHandle(), error.c_str(), L"Update Error",
                   MB_ICONERROR | MB_OK);
      DlgUpdate.PostMessage(WM_CLOSE);
      return;
    case taiga::kHttpTaigaUpdateDownload:
      MessageBox(DlgUpdate.GetWindowHandle(), error.c_str(), L"Download Error",
                 MB_ICONERROR | MB_OK);
      DlgUpdate.PostMessage(WM_CLOSE);
      return;
  }

  taskbar_list.SetProgressState(TBPF_NOPROGRESS);
}

void OnHttpHeadersAvailable(const taiga::HttpClient& http_client) {
  switch (http_client.mode()) {
    case taiga::kHttpSilent:
    case taiga::kHttpServiceGetMetadataById:
    case taiga::kHttpServiceSearchTitle:
    case taiga::kHttpGetLibraryEntryImage:
    case taiga::kHttpTaigaUpdateRelations:
      return;
    case taiga::kHttpTaigaUpdateCheck:
    case taiga::kHttpTaigaUpdateDownload:
      if (http_client.content_length() > 0) {
        DlgUpdate.progressbar.SetMarquee(false);
        DlgUpdate.progressbar.SetRange(0,
            static_cast<UINT>(http_client.content_length()));
      } else {
        DlgUpdate.progressbar.SetMarquee(true);
      }
      if (http_client.mode() == taiga::kHttpTaigaUpdateDownload) {
        DlgUpdate.SetDlgItemText(IDC_STATIC_UPDATE_PROGRESS,
                                 L"Downloading latest update...");
      }
      break;
    default:
      taskbar_list.SetProgressState(http_client.content_length() > 0 ?
                                    TBPF_NORMAL : TBPF_INDETERMINATE);
      break;
  }
}

void OnHttpProgress(const taiga::HttpClient& http_client) {
  std::wstring status;

  switch (http_client.mode()) {
    case taiga::kHttpSilent:
    case taiga::kHttpServiceGetMetadataById:
    case taiga::kHttpServiceSearchTitle:
    case taiga::kHttpGetLibraryEntryImage:
    case taiga::kHttpTaigaUpdateRelations:
      return;
    case taiga::kHttpServiceAuthenticateUser:
    case taiga::kHttpServiceGetUser:
      status = L"Reading account information...";
      break;
    case taiga::kHttpServiceGetLibraryEntries:
      status = L"Downloading anime list...";
      break;
    case taiga::kHttpServiceAddLibraryEntry:
    case taiga::kHttpServiceDeleteLibraryEntry:
    case taiga::kHttpServiceUpdateLibraryEntry:
      status = L"Updating list...";
      break;
    case taiga::kHttpFeedCheck:
    case taiga::kHttpFeedCheckAuto:
      status = L"Checking new torrents via {}..."_format(
               http_client.request().url.host);
      break;
    case taiga::kHttpFeedDownload:
      status = L"Downloading torrent file...";
      break;
    case taiga::kHttpServiceGetSeason:
    case taiga::kHttpSeasonsGet:
      status = L"Downloading anime season data...";
      break;
    case taiga::kHttpTwitterRequest:
      status = L"Connecting to Twitter...";
      break;
    case taiga::kHttpTwitterAuth:
      status = L"Authorizing Twitter...";
      break;
    case taiga::kHttpTwitterPost:
      status = L"Updating Twitter status...";
      break;
    case taiga::kHttpTaigaUpdateCheck:
    case taiga::kHttpTaigaUpdateDownload:
      if (http_client.content_length() > 0)
        DlgUpdate.progressbar.SetPosition(
            static_cast<UINT>(http_client.current_length()));
      return;
  }

  if (http_client.content_length() > 0) {
    float current_length = static_cast<float>(http_client.current_length());
    float content_length = static_cast<float>(http_client.content_length());
    int percentage = static_cast<int>((current_length / content_length) * 100);
    status += L" ({}%)"_format(percentage);
    taskbar_list.SetProgressValue(static_cast<ULONGLONG>(current_length),
                                  static_cast<ULONGLONG>(content_length));
  } else {
    status += L" ({})"_format(ToSizeString(http_client.current_length()));
  }

  ChangeStatusText(status);
}

void OnHttpReadComplete(const taiga::HttpClient& http_client) {
  taskbar_list.SetProgressState(TBPF_NOPROGRESS);
}

////////////////////////////////////////////////////////////////////////////////

void OnLibraryChange() {
  ClearStatusText();

  DlgAnimeList.RefreshList();
  DlgAnimeList.RefreshTabs();
  DlgHistory.RefreshList();
  DlgSearch.RefreshList();

  DlgMain.EnableInput(true);
}

void OnLibraryChangeFailure() {
  DlgMain.EnableInput(true);
}

void OnLibraryEntryAdd(int id) {
  if (DlgAnime.GetCurrentId() == id)
    DlgAnime.Refresh(false, false, true, false);

  DlgAnimeList.RefreshList();
  DlgAnimeList.RefreshTabs();

  if (DlgNowPlaying.GetCurrentId() == id)
    DlgNowPlaying.Refresh();

  DlgSearch.RefreshList();
}

void OnLibraryEntryChange(int id) {
  if (DlgAnime.GetCurrentId() == id)
    DlgAnime.Refresh(false, true, false, false);

  if (DlgAnimeList.IsWindow())
    DlgAnimeList.RefreshListItem(id);

  if (DlgNowPlaying.GetCurrentId() == id)
    DlgNowPlaying.Refresh(false, true, false, false);

  if (DlgSeason.IsWindow())
    DlgSeason.RefreshList(true);
}

void OnLibraryEntryDelete(int id) {
  if (DlgAnime.GetCurrentId() == id)
    DlgAnime.Refresh(false, false, true, false);

  DlgAnimeList.RefreshList();
  DlgAnimeList.RefreshTabs();

  DlgNowPlaying.Refresh(false, false, false, false);

  DlgSearch.RefreshList();

  if (DlgSeason.IsWindow())
    DlgSeason.RefreshList(true);
}

void OnLibraryEntryImageChange(int id) {
  if (DlgAnime.GetCurrentId() == id)
    DlgAnime.Refresh(true, false, false, false);

  if (DlgAnimeList.IsWindow())
    DlgAnimeList.RefreshListItem(id);

  if (DlgNowPlaying.GetCurrentId() == id)
    DlgNowPlaying.Refresh(true, false, false, false);

  if (DlgSeason.IsWindow())
    DlgSeason.RefreshList(true);
}

void OnLibraryGetSeason() {
  DlgSeason.RefreshList();
  DlgSeason.EnableInput();
}

void OnLibrarySearchTitle(int id, const string_t& results) {
  std::vector<string_t> split_vector;
  Split(results, L",", split_vector);
  RemoveEmptyStrings(split_vector);

  std::vector<int> ids;
  for (const auto& id_str : split_vector) {
    int id = ToInt(id_str);
    ids.push_back(id);
    OnLibraryEntryChange(id);
  }

  if (id == anime::ID_UNKNOWN)
    DlgSearch.ParseResults(ids);
}

void OnLibraryEntryChangeFailure(int id, const string_t& reason) {
  if (DlgAnime.GetCurrentId() == id)
    DlgAnime.UpdateTitle(false);
}

void OnLibraryUpdateFailure(int id, const string_t& reason, bool not_approved) {
  auto anime_item = AnimeDatabase.FindItem(id);

  std::wstring text;
  if (anime_item)
    text += L"Title: " + anime::GetPreferredTitle(*anime_item) + L"\n";

  if (not_approved) {
    text += L"Reason: Taiga won't be able to synchronize your list until MAL "
            L"approves the anime, or you remove it from the update queue.\n"
            L"Click to go to History page.";
    taskbar.tip_type = TipType::NotApproved;
  } else {
    if (!reason.empty())
      text += L"Reason: " + reason + L"\n";
    text += L"Click to try again.";
    taskbar.tip_type = TipType::UpdateFailed;
  }

  taskbar.Tip(L"", L"", 0);  // clear previous tips
  taskbar.Tip(text.c_str(), L"Update failed", NIIF_ERROR);

  ChangeStatusText(L"Update failed: " + reason);
}

////////////////////////////////////////////////////////////////////////////////

bool OnLibraryEntriesEditDelete(const std::vector<int> ids) {
  std::wstring content;
  if (ids.size() < 20) {
    for (const auto& id : ids) {
      auto anime_item = AnimeDatabase.FindItem(id);
      if (anime_item)
        AppendString(content, anime::GetPreferredTitle(*anime_item), L"\n");
    }
  } else {
    content = L"Selected " + ToWstr(ids.size()) + L" entries";
  }

  win::TaskDialog dlg;
  dlg.SetWindowTitle(L"Delete List Entries");
  dlg.SetMainIcon(TD_ICON_INFORMATION);
  dlg.SetMainInstruction(L"Are you sure you want to delete these anime from "
                         L"your list?");
  dlg.SetContent(content.c_str());
  dlg.AddButton(L"Yes", IDYES);
  dlg.AddButton(L"No", IDNO);
  dlg.Show(DlgMain.GetWindowHandle());

  return dlg.GetSelectedButtonID() == IDYES;
}

int OnLibraryEntriesEditEpisode(const std::vector<int> ids) {
  std::set<int> current;
  int number_max = 1900;
  for (const auto& id : ids) {
    auto anime_item = AnimeDatabase.FindItem(id);
    if (!anime_item)
      continue;
    current.insert(anime_item->GetMyLastWatchedEpisode());
    if (anime::IsValidEpisodeCount(anime_item->GetEpisodeCount()))
      number_max = std::min(number_max, anime_item->GetEpisodeCount());
  }
  int value = current.size() == 1 ? *current.begin() : 0;

  InputDialog dlg;
  dlg.SetNumbers(true, 0, number_max, value);
  dlg.title = L"Set Last Watched Episode Number";
  dlg.info = L"Enter an episode number for the selected anime:";
  dlg.text = ToWstr(value);
  dlg.Show(DlgMain.GetWindowHandle());

  if (dlg.result == IDOK)
    return ToInt(dlg.text);

  return -1;
}

bool OnLibraryEntriesEditTags(const std::vector<int> ids, std::wstring& tags) {
  std::set<std::wstring> current;
  for (const auto& id : ids) {
    auto anime_item = AnimeDatabase.FindItem(id);
    if (anime_item)
      current.insert(anime_item->GetMyTags());
  }
  std::wstring value = current.size() == 1 ? *current.begin() : L"";

  InputDialog dlg;
  dlg.title = L"Set Tags";
  dlg.info = L"Enter tags for the selected anime, separated by a comma:";
  dlg.text = value;
  dlg.Show(DlgMain.GetWindowHandle());

  if (dlg.result == IDOK) {
    tags = dlg.text;
    return true;
  }

  return false;
}

bool OnLibraryEntriesEditNotes(const std::vector<int> ids, std::wstring& notes) {
  std::wstring value;
  for (const auto& id : ids) {
    auto anime_item = AnimeDatabase.FindItem(id);
    if (anime_item) {
      value = anime_item->GetMyNotes();
      if (!value.empty())
        break;
    }
  }

  InputDialog dlg;
  dlg.title = L"Set Notes";
  dlg.info = L"Enter notes for the selected anime:";
  dlg.text = value;
  dlg.Show(DlgMain.GetWindowHandle());

  if (dlg.result == IDOK) {
    notes = dlg.text;
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

static bool AnimeListNeedsRefresh(const HistoryItem& history_item) {
  return history_item.mode == taiga::kHttpServiceAddLibraryEntry ||
         history_item.mode == taiga::kHttpServiceDeleteLibraryEntry ||
         history_item.status ||
         history_item.enable_rewatching;
}

static bool AnimeListNeedsResort() {
  auto sort_column = DlgAnimeList.listview.TranslateColumnName(
      Settings[taiga::kApp_List_SortColumnPrimary]);
  switch (sort_column) {
    case kColumnUserLastUpdated:
    case kColumnUserProgress:
    case kColumnUserRating:
    case kColumnUserDateStarted:
    case kColumnUserDateCompleted:
      return true;
  }
  return false;
}

void OnHistoryAddItem(const HistoryItem& history_item) {
  DlgHistory.RefreshList();
  DlgSearch.RefreshList();
  DlgMain.treeview.RefreshHistoryCounter();
  DlgNowPlaying.Refresh(false, false, false);

  if (AnimeListNeedsRefresh(history_item)) {
    DlgAnimeList.RefreshList();
    DlgAnimeList.RefreshTabs();
  } else {
    DlgAnimeList.RefreshListItem(history_item.anime_id);
    if (AnimeListNeedsResort())
      DlgAnimeList.listview.SortFromSettings();
  }

  if (!sync::UserAuthenticated()) {
    auto anime_item = AnimeDatabase.FindItem(history_item.anime_id);
    if (anime_item) {
      ChangeStatusText(L"\"{}\" is queued for update."_format(
                       anime::GetPreferredTitle(*anime_item)));
    }
  }
}

void OnHistoryChange(const HistoryItem* history_item) {
  DlgHistory.RefreshList();
  DlgSearch.RefreshList();
  DlgMain.treeview.RefreshHistoryCounter();
  DlgNowPlaying.Refresh(false, false, false);

  if (!history_item || AnimeListNeedsRefresh(*history_item)) {
    DlgAnimeList.RefreshList();
    DlgAnimeList.RefreshTabs();
  } else {
    DlgAnimeList.RefreshListItem(history_item->anime_id);
    if (AnimeListNeedsResort())
      DlgAnimeList.listview.SortFromSettings();
  }
}

bool OnHistoryClear() {
  win::TaskDialog dlg;
  dlg.SetWindowTitle(L"Clear History");
  dlg.SetMainIcon(TD_ICON_INFORMATION);
  dlg.SetMainInstruction(L"Are you sure you want to delete your entire history?");
  dlg.AddButton(L"Yes", IDYES);
  dlg.AddButton(L"No", IDNO);
  dlg.Show(DlgMain.GetWindowHandle());

  return dlg.GetSelectedButtonID() == IDYES;
}

int OnHistoryQueueClear() {
  win::TaskDialog dlg(L"Clear Queue", TD_ICON_INFORMATION);
  dlg.SetMainInstruction(L"Do you want to clear your update queue?");
  const std::wstring content = L"Queued updates will not be sent to {}."_format(
      taiga::GetCurrentService()->name());
  dlg.SetContent(content.c_str());

  dlg.UseCommandLinks(true);
  dlg.AddButton(L"Delete", IDYES);
  dlg.AddButton(L"Merge", IDNO);
  dlg.AddButton(L"Cancel", IDCANCEL);

  dlg.Show(DlgMain.GetWindowHandle());
  return dlg.GetSelectedButtonID();
}

int OnHistoryProcessConfirmationQueue(anime::Episode& episode) {
  auto anime_item = AnimeDatabase.FindItem(episode.anime_id);

  if (!anime_item)
    return IDNO;

  win::TaskDialog dlg;
  std::wstring title = L"Anime title: " + anime::GetPreferredTitle(*anime_item);
  dlg.SetWindowTitle(TAIGA_APP_TITLE);
  dlg.SetMainIcon(TD_ICON_INFORMATION);
  dlg.SetMainInstruction(L"Do you want to update your anime list?");
  dlg.SetContent(title.c_str());
  dlg.SetVerificationText(L"Don't ask again, update automatically");
  dlg.UseCommandLinks(true);

  int number = anime::GetEpisodeHigh(episode);
  if (number == 0)
    number = 1;
  if (anime_item->GetEpisodeCount() == 1)
    episode.set_episode_number(1);

  if (anime_item->GetMyStatus() != anime::kNotInList &&
      anime_item->GetMyRewatching() == FALSE) {
    if (anime_item->GetEpisodeCount() == number) {  // Completed
      dlg.AddButton(L"Update and move\n"
                    L"Update and set as completed", IDCANCEL);
    } else if (anime_item->GetMyStatus() != anime::kWatching) {  // Watching
      dlg.AddButton(L"Update and move\n"
                    L"Update and set as watching", IDCANCEL);
    }
  }
  std::wstring button = L"Update\n"
                        L"Update episode number from " +
                        ToWstr(anime_item->GetMyLastWatchedEpisode()) +
                        L" to " + ToWstr(number);
  dlg.AddButton(button.c_str(), IDYES);
  dlg.AddButton(L"Cancel\n"
                L"Don't update anything", IDNO);

  dlg.Show(DlgMain.GetWindowHandle());
  if (dlg.GetVerificationCheck())
    Settings.Set(taiga::kSync_Update_AskToConfirm, false);
  return dlg.GetSelectedButtonID();
}

////////////////////////////////////////////////////////////////////////////////

void OnAnimeDelete(int id, const string_t& title) {
  ChangeStatusText(L"Anime is removed from the database: " + title);

  if (DlgAnime.GetCurrentId() == id) {
    // We're posting a message rather than directly terminating the dialog,
    // because this function can be called from another thread, and it is not
    // possible to destroy a window created by a different thread.
    DlgAnime.PostMessage(WM_CLOSE);
  }

  DlgAnimeList.RefreshList();
  DlgAnimeList.RefreshTabs();

  if (DlgNowPlaying.GetCurrentId() == id) {
    DlgNowPlaying.SetCurrentId(anime::ID_UNKNOWN);
  } else {
    DlgNowPlaying.Refresh(false, false, false, false);
  }

  DlgHistory.RefreshList();
  DlgMain.treeview.RefreshHistoryCounter();

  DlgSearch.RefreshList();
  DlgSeason.RefreshList();
}

void OnAnimeEpisodeNotFound() {
  win::TaskDialog dlg;
  dlg.SetWindowTitle(L"Play Random Episode");
  dlg.SetMainIcon(TD_ICON_ERROR);
  dlg.SetMainInstruction(L"Could not find any episode to play.");
  dlg.Show(DlgMain.GetWindowHandle());
}

bool OnAnimeFolderNotFound() {
  win::TaskDialog dlg;
  dlg.SetWindowTitle(L"Folder Not Found");
  dlg.SetMainIcon(TD_ICON_INFORMATION);
  dlg.SetMainInstruction(L"Taiga couldn't find the folder of this anime. "
                         L"Would you like to set it manually?");
  dlg.AddButton(L"Yes", IDYES);
  dlg.AddButton(L"No", IDNO);
  dlg.Show(DlgMain.GetWindowHandle());

  return dlg.GetSelectedButtonID() == IDYES;
}

void OnAnimeWatchingStart(const anime::Item& anime_item,
                          const anime::Episode& episode) {
  DlgNowPlaying.SetCurrentId(anime_item.GetId());

  int list_status = anime_item.GetMyStatus();
  if (anime_item.GetMyRewatching())
    list_status = anime::kWatching;
  if (list_status != anime::kNotInList &&
      list_status != DlgAnimeList.GetCurrentStatus()) {
    DlgAnimeList.RefreshList(list_status);
    DlgAnimeList.RefreshTabs(list_status);
  }
  int list_index = DlgAnimeList.GetListIndex(anime_item.GetId());
  if (list_index > -1) {
    DlgAnimeList.listview.SetItemIcon(list_index, ui::kIcon16_Play);
    DlgAnimeList.listview.RedrawItems(list_index, list_index, true);
    DlgAnimeList.listview.EnsureVisible(list_index);
  }

  DlgMain.UpdateTip();
  DlgMain.UpdateTitle();
  if (Settings.GetBool(taiga::kSync_GoToNowPlaying_Recognized))
    DlgMain.navigation.SetCurrentPage(kSidebarItemNowPlaying);

  if (Settings.GetBool(taiga::kSync_Notify_Recognized)) {
    taskbar.tip_type = TipType::NowPlaying;
    std::wstring tip_text =
        ReplaceVariables(Settings[taiga::kSync_Notify_Format], episode);
    taskbar.Tip(L"", L"", 0);
    taskbar.Tip(tip_text.c_str(), L"Now Playing", NIIF_INFO | NIIF_NOSOUND);
  }
}

void OnAnimeWatchingEnd(const anime::Item& anime_item,
                        const anime::Episode& episode) {
  DlgNowPlaying.SetCurrentId(anime::ID_UNKNOWN);

  DlgMain.UpdateTip();
  DlgMain.UpdateTitle();

  int list_index = DlgAnimeList.GetListIndex(anime_item.GetId());
  if (list_index > -1) {
    int icon_index = StatusToIcon(anime_item.GetAiringStatus());
    DlgAnimeList.listview.SetItemIcon(list_index, icon_index);
    DlgAnimeList.listview.RedrawItems(list_index, list_index, true);
  }
}

////////////////////////////////////////////////////////////////////////////////

bool OnRecognitionCancelConfirm() {
  auto anime_item = AnimeDatabase.FindItem(CurrentEpisode.anime_id);

  if (!anime_item)
    return false;

  win::TaskDialog dlg;
  std::wstring title = L"List Update";
  dlg.SetWindowTitle(title.c_str());
  dlg.SetMainIcon(TD_ICON_INFORMATION);
  dlg.SetMainInstruction(L"Would you like to cancel this list update?");
  std::wstring content = anime::GetPreferredTitle(*anime_item) +
      PushString(L" #", anime::GetEpisodeRange(CurrentEpisode));
  dlg.SetContent(content.c_str());
  dlg.AddButton(L"Yes", IDYES);
  dlg.AddButton(L"No", IDNO);
  dlg.Show(DlgMain.GetWindowHandle());

  return dlg.GetSelectedButtonID() == IDYES;
}

void OnRecognitionFail() {
  if (!CurrentEpisode.anime_title().empty()) {
    MediaPlayers.set_title_changed(false);
    DlgNowPlaying.SetScores(Meow.GetScores());
    DlgNowPlaying.SetCurrentId(anime::ID_NOTINLIST);
    ChangeStatusText(L"Watching: {}{} (Not recognized)"_format(
                     CurrentEpisode.anime_title(),
                     PushString(L" #", anime::GetEpisodeRange(CurrentEpisode))));
    if (Settings.GetBool(taiga::kSync_GoToNowPlaying_NotRecognized))
      DlgMain.navigation.SetCurrentPage(kSidebarItemNowPlaying);
    if (Settings.GetBool(taiga::kSync_Notify_NotRecognized)) {
      std::wstring tip_text =
          ReplaceVariables(Settings[taiga::kSync_Notify_Format], CurrentEpisode) +
          L"\nClick here to view similar titles for this anime.";
      taskbar.tip_type = TipType::NowPlaying;
      taskbar.Tip(L"", L"", 0);
      taskbar.Tip(tip_text.c_str(), L"Media is not in your list", NIIF_WARNING);
    }

  } else {
    if (Taiga.debug_mode)
      ChangeStatusText(StrToWstr(MediaPlayers.current_player_name()) +
                       L" is running.");
  }
}

////////////////////////////////////////////////////////////////////////////////

void OnAnimeListHeaderRatingWarning() {
  win::TaskDialog dlg(L"Average Score", TD_ICON_INFORMATION);
  dlg.SetMainInstruction(L"Technical limitation");
  dlg.SetContent(L"Please note that some average values may be outdated, as "
                 L"MyAnimeList doesn't provide a way to get them in batch.");
  dlg.AddButton(L"OK", IDOK);
  dlg.Show(DlgMain.GetWindowHandle());
}

////////////////////////////////////////////////////////////////////////////////

void OnSeasonLoad(bool refresh) {
  DlgSeason.RefreshList();
  DlgSeason.RefreshStatus();
  DlgSeason.RefreshToolbar();
  DlgSeason.EnableInput();

  if (refresh && OnSeasonRefreshRequired())
    DlgSeason.RefreshData();
}

void OnSeasonLoadFail() {
  ChangeStatusText(L"Could not load anime season data.");
  DlgSeason.EnableInput();
}

bool OnSeasonRefreshRequired() {
  win::TaskDialog dlg;
  std::wstring title = L"Season - " + SeasonDatabase.current_season.GetString();
  dlg.SetWindowTitle(title.c_str());
  dlg.SetMainIcon(TD_ICON_INFORMATION);
  dlg.SetMainInstruction(L"Would you like to refresh this season's data?");
  auto service = taiga::GetCurrentService();
  std::wstring content =
      L"Taiga will connect to " + service->name() + L" to retrieve missing "
      L"information and images. Note that it may take about a minute until "
      L"Taiga gets all the data.";
  dlg.SetContent(content.c_str());
  dlg.AddButton(L"Yes", IDYES);
  dlg.AddButton(L"No", IDNO);
  dlg.Show(DlgMain.GetWindowHandle());

  return dlg.GetSelectedButtonID() == IDYES;
}

////////////////////////////////////////////////////////////////////////////////

void OnSettingsAccountEmpty() {
  win::TaskDialog dlg(TAIGA_APP_TITLE, TD_ICON_INFORMATION);
  dlg.SetMainInstruction(L"Would you like to set your account information?");
  dlg.SetContent(L"Anime search requires authentication, which means, you need "
                 L"to enter a valid username and password to search "
                 L"MyAnimeList.");
  dlg.AddButton(L"Yes", IDYES);
  dlg.AddButton(L"No", IDNO);
  dlg.Show(DlgMain.GetWindowHandle());
  if (dlg.GetSelectedButtonID() == IDYES)
    ShowDlgSettings(kSettingsSectionServices, kSettingsPageServicesMain);
}

bool OnSettingsEditAdvanced(const std::wstring& description,
                            bool is_password, std::wstring& value) {
  InputDialog dlg;
  dlg.title = L"Advanced Setting";
  dlg.info = description + L":";
  dlg.text = value;
  dlg.SetPassword(is_password);
  dlg.Show(DlgSettings.GetWindowHandle());

  if (dlg.result == IDOK) {
    value = dlg.text;
    return true;
  }

  return false;
}

void OnSettingsChange() {
  DlgAnimeList.RefreshList();
}

void OnSettingsLibraryFoldersEmpty() {
  win::TaskDialog dlg(TAIGA_APP_TITLE, TD_ICON_INFORMATION);
  dlg.SetMainInstruction(L"Would you like to set library folders first?");
  dlg.SetContent(L"You need to have at least one library folder set before "
                 L"scanning available episodes.");
  dlg.AddButton(L"Yes", IDYES);
  dlg.AddButton(L"No", IDNO);
  dlg.Show(DlgMain.GetWindowHandle());
  if (dlg.GetSelectedButtonID() == IDYES)
    ShowDlgSettings(kSettingsSectionLibrary, kSettingsPageLibraryFolders);
}

void OnSettingsRestoreDefaults() {
  if (DlgSettings.IsWindow()) {
    EndDialog(Dialog::Settings);
  }
}

void OnSettingsServiceChange() {
  int current_page = DlgMain.navigation.GetCurrentPage();
  DlgMain.navigation.RefreshSearchText(current_page);

  string_t tooltip = L"Synchronize list with " +
                     taiga::GetCurrentService()->name();
  DlgMain.toolbar_main.SetButtonTooltip(0, tooltip.c_str());
}

bool OnSettingsServiceChangeConfirm(const string_t& current_service,
                                    const string_t& new_service) {
  win::TaskDialog dlg(TAIGA_APP_TITLE, TD_ICON_WARNING);
  std::wstring instruction =
      L"Do you want to change the active service from {} to {}?"_format(
      ServiceManager.service(current_service)->name(),
      ServiceManager.service(new_service)->name());
  dlg.SetMainInstruction(instruction.c_str());
  dlg.SetContent(L"Note that:\n"
                 L"- Taiga cannot move your list from one service to another. "
                 L"You must use the export and import features of these services.\n"
                 L"- Local settings associated with an anime will be lost or "
                 L"broken.");
  dlg.AddButton(L"Yes", IDYES);
  dlg.AddButton(L"No", IDNO);
  dlg.Show(DlgSettings.GetWindowHandle());

  return dlg.GetSelectedButtonID() == IDYES;
}

void OnSettingsServiceChangeFailed() {
  win::TaskDialog dlg(TAIGA_APP_TITLE, TD_ICON_ERROR);
  dlg.SetMainInstruction(L"You cannot change the active service while there "
                         L"are queued items in your History.");
  dlg.SetContent(L"Synchronize your list or clear the queue, and try again.");
  dlg.AddButton(L"OK", IDOK);
  dlg.Show(DlgMain.GetWindowHandle());
}

void OnSettingsThemeChange() {
  Menus.UpdateAll();

  DlgMain.rebar.RedrawWindow();
}

void OnSettingsUserChange() {
  DlgMain.treeview.RefreshHistoryCounter();
  DlgMain.UpdateTitle();
  DlgAnimeList.RefreshList(anime::kWatching);
  DlgAnimeList.RefreshTabs(anime::kWatching);
  DlgHistory.RefreshList();
  DlgNowPlaying.Refresh();
  DlgSearch.RefreshList();
  DlgSeason.RefreshList();
  DlgSeason.RefreshToolbar();
  DlgStats.Refresh();
}

////////////////////////////////////////////////////////////////////////////////

void OnEpisodeAvailabilityChange(int id) {
  if (DlgAnimeList.IsWindow())
    DlgAnimeList.RefreshListItem(id);

  if (DlgNowPlaying.GetCurrentId() == anime::ID_UNKNOWN)
    DlgNowPlaying.Refresh(false, false, false, false);
}

void OnScanAvailableEpisodesFinished() {
  DlgNowPlaying.Refresh(false, false, false);
}

////////////////////////////////////////////////////////////////////////////////

void OnFeedCheck(bool success) {
  ChangeStatusText(success ?
      L"There are new torrents available!" : L"No new torrents found.");

  DlgTorrent.RefreshList();
  DlgTorrent.EnableInput();
}

void OnFeedDownloadSuccess(bool is_magnet_link) {
  ChangeStatusText(!is_magnet_link ?
      L"Successfully downloaded the torrent file." :
      L"Found magnet link for the torrent file.");

  DlgTorrent.RefreshList();
  DlgTorrent.EnableInput();
}

void OnFeedDownloadError(const string_t& message) {
  ChangeStatusText(L"Torrent download error: " + message);

  DlgTorrent.EnableInput();
}

bool OnFeedNotify(const Feed& feed) {
  std::map<std::wstring, std::set<std::wstring>> found_episodes;

  for (const auto& feed_item : feed.items) {
    if (feed_item.state == FeedItemState::Selected) {
      const auto& episode = feed_item.episode_data;
      auto anime_item = AnimeDatabase.FindItem(episode.anime_id);
      auto anime_title = anime_item ? anime::GetPreferredTitle(*anime_item) : episode.anime_title();

      auto episode_l = anime::GetEpisodeLow(episode);
      auto episode_h = anime::GetEpisodeHigh(episode);
      if (anime_item)
        episode_l = std::max(episode_l, anime_item->GetMyLastWatchedEpisode() + 1);
      std::wstring episode_number = ToWstr(episode_h);
      if (episode_l < episode_h)
        episode_number = ToWstr(episode_l) + L"-" + episode_number;

      found_episodes[anime_title].insert(episode_number);
    }
  }

  if (found_episodes.empty())
    return false;

  std::wstring tip_text;
  std::wstring tip_title = L"New torrents available";

  for (const auto& pair : found_episodes) {
    tip_text += L"\u00BB " + LimitText(pair.first, 32);
    std::wstring episodes;
    for (const auto& episode : pair.second)
      if (!episode.empty())
        AppendString(episodes, L" #" + episode);
    tip_text += episodes + L"\n";
  }

  tip_text += L"Click to see all.";
  tip_text = LimitText(tip_text, 255);
  taskbar.tip_type = TipType::Torrent;
  taskbar.Tip(L"", L"", 0);
  taskbar.Tip(tip_text.c_str(), tip_title.c_str(), NIIF_INFO | NIIF_NOSOUND);

  return true;
}

////////////////////////////////////////////////////////////////////////////////

void OnMircNotRunning(bool testing) {
  std::wstring title = testing ? L"Test DDE connection" : L"Announce to mIRC";
  win::TaskDialog dlg(title.c_str(), TD_ICON_ERROR);
  dlg.SetMainInstruction(L"mIRC is not running.");
  dlg.AddButton(L"OK", IDOK);
  dlg.Show(testing ? DlgSettings.GetWindowHandle() : DlgMain.GetWindowHandle());
}

void OnMircDdeInitFail(bool testing) {
  std::wstring title = testing ? L"Test DDE connection" : L"Announce to mIRC";
  win::TaskDialog dlg(title.c_str(), TD_ICON_ERROR);
  dlg.SetMainInstruction(L"DDE initialization failed.");
  dlg.AddButton(L"OK", IDOK);
  dlg.Show(testing ? DlgSettings.GetWindowHandle() : DlgMain.GetWindowHandle());
}

void OnMircDdeConnectionFail(bool testing) {
  std::wstring title = testing ? L"Test DDE connection" : L"Announce to mIRC";
  win::TaskDialog dlg(title.c_str(), TD_ICON_ERROR);
  dlg.SetMainInstruction(L"DDE connection failed.");
  dlg.SetContent(L"Please enable DDE server from mIRC Options > Other > DDE.");
  dlg.AddButton(L"OK", IDOK);
  dlg.Show(testing ? DlgSettings.GetWindowHandle() : DlgMain.GetWindowHandle());
}

void OnMircDdeConnectionSuccess(const std::vector<std::wstring>& channels,
                                bool testing) {
  std::wstring title = testing ? L"Test DDE connection" : L"Announce to mIRC";
  win::TaskDialog dlg(title.c_str(), TD_ICON_INFORMATION);
  dlg.SetMainInstruction(L"Successfuly connected to DDE server!");
  std::wstring content = L"Current channels: " + Join(channels, L", ");
  dlg.SetContent(content.c_str());
  dlg.AddButton(L"OK", IDOK);
  dlg.Show(testing ? DlgSettings.GetWindowHandle() : DlgMain.GetWindowHandle());
}

////////////////////////////////////////////////////////////////////////////////

void OnTwitterTokenRequest(bool success) {
  if (success) {
    ClearStatusText();
  } else {
    ChangeStatusText(L"Twitter token request failed.");
  }
}

bool OnTwitterTokenEntry(string_t& auth_pin) {
  ClearStatusText();
  return EnterAuthorizationPin(L"Twitter", auth_pin);
}

void OnTwitterAuth(bool success) {
  ChangeStatusText(success ?
      L"Taiga is now authorized to post to this Twitter account: " +
      Settings[taiga::kShare_Twitter_Username] :
      L"Twitter authorization failed.");

  DlgSettings.RefreshTwitterLink();
}

void OnTwitterPost(bool success, const string_t& error) {
  ChangeStatusText(success ?
      L"Twitter status updated." :
      L"Twitter status update failed. (" + error + L")");
}

////////////////////////////////////////////////////////////////////////////////

void OnLogin() {
  ChangeStatusText(L"Logged in as " + taiga::GetCurrentUserDisplayName());

  Menus.UpdateAll();

  DlgMain.UpdateTip();
  DlgMain.UpdateTitle();
  DlgMain.EnableInput(true);
}

void OnLogout(bool website_login_required) {
  if (website_login_required) {
    const std::wstring tip_text =
        L"Due to restrictions of MyAnimeList API, users are required to have "
        L"logged in via the website within the past 90 days.\n\n"
        L"Click to go to MAL website.";
    taskbar.tip_type = TipType::WebsiteLoginRequired;
    taskbar.Tip(L"", L"", 0);
    taskbar.Tip(tip_text.c_str(), L"Website login required", NIIF_ERROR);
  }

  DlgMain.EnableInput(true);
}

////////////////////////////////////////////////////////////////////////////////

void OnUpdateAvailable() {
  DlgUpdateNew.Create(IDD_UPDATE_NEW, DlgUpdate.GetWindowHandle(), true);
}

void OnUpdateNotAvailable(bool relations, bool season) {
  if (DlgMain.IsWindow()) {
    win::TaskDialog dlg(L"Update", TD_ICON_INFORMATION);
    dlg.SetMainInstruction(L"Taiga is up to date!");
    std::wstring content = L"Current version: " +
                           StrToWstr(Taiga.version.to_string());
    if (relations) {
      content += L"\n\nUpdated anime relations to: " +
                 Taiga.Updater.GetCurrentAnimeRelationsModified();
    }
    if (season) {
      switch (taiga::GetCurrentServiceId()) {
        case sync::kMyAnimeList:
          content += L"\n\nNew anime season: " +
                     SeasonDatabase.available_seasons.second.GetString();
          break;
      }
    }
    dlg.SetContent(content.c_str());
    dlg.AddButton(L"OK", IDOK);
    dlg.Show(DlgUpdate.GetWindowHandle());
  }
}

void OnUpdateFailed() {
  win::TaskDialog dlg;
  dlg.SetWindowTitle(TAIGA_APP_TITLE);
  dlg.SetMainIcon(TD_ICON_ERROR);
  dlg.SetMainInstruction(L"Could not download the latest update.");
  dlg.Show(DlgUpdate.GetWindowHandle());
}

void OnUpdateFinished() {
  DlgUpdate.PostMessage(WM_CLOSE);
}

}  // namespace ui