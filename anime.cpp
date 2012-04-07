/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

#include "std.h"
#include <ctime>
#include "anime.h"
#include "animelist.h"
#include "announce.h"
#include "common.h"
#include "dlg/dlg_anime_info.h"
#include "dlg/dlg_feed_filter.h"
#include "dlg/dlg_main.h"
#include "dlg/dlg_search.h"
#include "event.h"
#include "feed.h"
#include "http.h"
#include "media.h"
#include "myanimelist.h"
#include "process.h"
#include "recognition.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"
#include "theme.h"
#include "win32/win_taskbar.h"
#include "win32/win_taskdialog.h"
#include "xml.h"

Episode CurrentEpisode;

// =============================================================================

Episode::Episode() :
  anime_id(ANIMEID_UNKNOWN)
{
}

void Episode::Clear() {
  anime_id = ANIMEID_UNKNOWN;
  audio_type.clear();
  checksum.clear();
  extras.clear();
  file.clear();
  format.clear();
  group.clear();
  name.clear();
  number.clear();
  resolution.clear();
  title.clear();
  version.clear();
  video_type.clear();
}

void Episode::Set(int anime_id) {
  this->anime_id = anime_id;
  MainDialog.RefreshMenubar(anime_id);
}

// =============================================================================

MalAnime::MalAnime() : 
  series_id(0), series_type(0), series_episodes(0), series_status(0),
  my_id(0), my_watched_episodes(0), my_score(0), my_status(0), my_rewatching(0), my_rewatching_ep(0)
{
}

Anime::Anime() : 
  index(0), new_episode_available(false), last_modified(0), playing(false), settings_keep_title(FALSE)
{
}

void Anime::Start(Episode episode) {
  // Change status
  Taiga.play_status = PLAYSTATUS_PLAYING;
  playing = true;

  // Update main window
  MainDialog.ChangeStatus();
  MainDialog.UpdateTip();
  int status = GetRewatching() ? mal::MYSTATUS_WATCHING : GetStatus();
  MainDialog.RefreshList(status);
  MainDialog.RefreshTabs(status);
  int list_index = MainDialog.GetListIndex(series_id);
  if (list_index > -1) {
    MainDialog.listview.SetItemIcon(list_index, ICON16_PLAY);
    MainDialog.listview.RedrawItems(list_index, list_index, true);
    MainDialog.listview.EnsureVisible(list_index);
  }
  
  // Show balloon tip
  Taskbar.Tip(L"", L"", 0);
  Taskbar.Tip(ReplaceVariables(Settings.Program.Balloon.format, episode).c_str(), 
    L"Watching:", NIIF_INFO);
  
  // Check folder
  if (folder.empty()) {
    if (episode.folder.empty()) {
      HWND hwnd = MediaPlayers.items[MediaPlayers.index].window_handle;
      episode.folder = MediaPlayers.GetTitleFromProcessHandle(hwnd);
      episode.folder = GetPathOnly(episode.folder);
    }
    if (!episode.folder.empty()) {
      for (size_t i = 0; i < Settings.Folders.root.size(); i++) {
        // Set the folder if only it is under a root folder
        if (StartsWith(episode.folder, Settings.Folders.root[i])) {
          SetFolder(episode.folder, true, false);
          break;
        }
      }
    }
  }

  // Get additional information
  if (score.empty() || synopsis.empty()) {
    mal::SearchAnime(series_title, this);
  }
  
  // Update
  if (Settings.Account.Update.time == UPDATE_TIME_INSTANT) {
    End(episode, false, true);
  }
}

void Anime::End(Episode episode, bool end_watching, bool update_list) {
  if (end_watching) {
    // Change status
    Taiga.play_status = PLAYSTATUS_STOPPED;
    playing = false;
    // Announce
    episode.anime_id = series_id;
    Announcer.Do(ANNOUNCE_TO_HTTP, &episode);
    Announcer.Clear(ANNOUNCE_TO_MESSENGER | ANNOUNCE_TO_SKYPE);
    // Update main window
    episode.anime_id = ANIMEID_UNKNOWN;
    MainDialog.ChangeStatus();
    MainDialog.UpdateTip();
    int list_index = MainDialog.GetListIndex(series_id);
    if (list_index > -1) {
      MainDialog.listview.SetItemIcon(list_index, StatusToIcon(GetAiringStatus()));
      MainDialog.listview.RedrawItems(list_index, list_index, true);
    }
  }

  // Update list
  if (update_list) {
    if (GetStatus() == mal::MYSTATUS_COMPLETED && GetRewatching() == 0) return;
    if (Settings.Account.Update.time == UPDATE_TIME_INSTANT || 
      Taiga.ticker_media == -1 || Taiga.ticker_media >= Settings.Account.Update.delay) {
        int number = GetEpisodeHigh(episode.number);
        int number_low = GetEpisodeLow(episode.number);
        int lastwatched = GetLastWatchedEpisode();
        if (Settings.Account.Update.out_of_range) {
          if (number_low > lastwatched + 1 || number < lastwatched + 1) return;
        }
        if (mal::IsValidEpisode(number, lastwatched, series_episodes)) {
          switch (Settings.Account.Update.mode) {
            case UPDATE_MODE_NONE:
              break;
            case UPDATE_MODE_AUTO:
              Update(episode, true);
              break;
            case UPDATE_MODE_ASK:
            default:
              int choice = Ask(episode);
              if (choice != IDNO) {
                Update(episode, (choice == IDCANCEL));
              }
          }
        }
    }
  }
}

int Anime::Ask(Episode episode) {
  // Set up dialog
  win32::TaskDialog dlg;
  wstring title = L"Anime title: " + series_title;
  dlg.SetWindowTitle(APP_TITLE);
  dlg.SetMainIcon(TD_ICON_INFORMATION);
  dlg.SetMainInstruction(L"Do you want to update your anime list?");
  dlg.SetContent(title.c_str());
  dlg.UseCommandLinks(true);

  // Add buttons
  int number = GetEpisodeHigh(episode.number);
  if (number == 0) number = 1;
  if (series_episodes == 1) { // Completed (1 eps.)
    episode.number = L"1";
    dlg.AddButton(L"Update and move\nUpdate and set as completed", IDCANCEL);
  } else if (series_episodes == number) { // Completed (>1 eps.)
    dlg.AddButton(L"Update and move\nUpdate and set as completed", IDCANCEL);
  } else if (GetStatus() != mal::MYSTATUS_WATCHING) { // Watching
    dlg.AddButton(L"Update and move\nUpdate and set as watching", IDCANCEL);
  }
  wstring button = L"Update\nUpdate episode number from " + 
    ToWSTR(GetLastWatchedEpisode()) + L" to " + ToWSTR(number);
  dlg.AddButton(button.c_str(), IDYES);
  dlg.AddButton(L"Cancel\nDon't update anything", IDNO);
  
  // Show dialog
  ActivateWindow(g_hMain);
  dlg.Show(g_hMain);
  return dlg.GetSelectedButtonID();
}

void Anime::Update(Episode episode, bool change_status) {
  // Create event item
  EventItem item;
  item.anime_id = series_id;

  // Set episode number
  item.episode = GetEpisodeHigh(episode.number);
  if (item.episode == 0 || series_episodes == 1) item.episode = 1;
  episode.anime_id = series_id;
  
  // Set start/finish date
  if (item.episode == 1) SetStartDate(L"", false);
  if (item.episode == series_episodes) SetFinishDate(L"", false);

  if (change_status) {
    item.mode = HTTP_MAL_AnimeEdit;
    // Move to completed
    if (series_episodes == item.episode) {
      item.status = mal::MYSTATUS_COMPLETED;
      if (GetRewatching()) {
        item.enable_rewatching = FALSE;
        //item.times_rewatched++; // TODO: Enable when MAL adds to API
      }
      EventQueue.Add(item);
      return;
    // Move to watching
    } else if (GetStatus() != mal::MYSTATUS_WATCHING || item.episode == 1) {
      if (!GetRewatching()) {
        item.status = mal::MYSTATUS_WATCHING;
        EventQueue.Add(item);
        return;
      }
    }
  }

  // Update normally
  item.mode = HTTP_MAL_AnimeUpdate;
  EventQueue.Add(item);
}

// =============================================================================

bool Anime::CheckFolder() {
  // Check if current folder still exists
  if (!folder.empty() && !FolderExists(folder)) {
    folder.clear();
  }
  // Search root folders
  if (folder.empty()) {
    wstring new_folder;
    for (unsigned int i = 0; i < Settings.Folders.root.size(); i++) {
      new_folder = SearchFileFolder(*this, Settings.Folders.root[i], 0, true);
      if (!new_folder.empty()) {
        SetFolder(new_folder, true, false);
        break;
      }
    }
  }
  return !folder.empty();
}

void Anime::SetFolder(const wstring& folder, bool save_settings, bool check_episodes) {
  // Save settings
  if (save_settings && folder != EMPTY_STR && folder != this->folder) {
    Settings.Anime.SetItem(series_id, folder, EMPTY_STR);
  }
  // Set new folder
  this->folder = folder;
  // Check episodes
  if (check_episodes) {
    CheckEpisodes();
  }
}

// =============================================================================

bool Anime::CheckEpisodes(int number, bool check_folder) {
  // Check folder
  if (check_folder) CheckFolder();
  if (folder.empty()) {
    for (int i = 1; i <= series_episodes; i++)
      SetEpisodeAvailability(i, false);
    new_episode_available = false;
    return false;
  }
  
  // Check all episodes
  if (number == -1) {
    SearchFileFolder(*this, folder, -1, false);
    return true;

  // Check single episode
  } else {
    if (number == 0) {
      if (new_episode_available) return true;
      next_episode_path.clear();
      number = series_episodes == 1 ? 0 : GetLastWatchedEpisode() + 1;
    }
    wstring file = SearchFileFolder(*this, folder, number, false);
    return !file.empty();
  }
}

bool Anime::PlayEpisode(int number) {
  if (number > series_episodes && series_episodes != 0) return false;

  wstring file;

  // Check saved episode path
  if (number == GetLastWatchedEpisode() + 1) {
    if (!next_episode_path.empty() && FileExists(next_episode_path)) {
      file = next_episode_path;
    }
  }
  
  // Check anime folder
  if (file.empty()) {
    CheckFolder();
    if (!folder.empty()) {
      file = SearchFileFolder(*this, folder, number, false);
    }
  }

  // Check other folders
  if (file.empty()) {
    for (unsigned int i = 0; i < Settings.Folders.root.size(); i++) {
      file = SearchFileFolder(*this, Settings.Folders.root[i], number, false);
      if (!file.empty()) break;
    }
  }

  if (file.empty()) {
    if (number == 0) number = 1;
    MainDialog.ChangeStatus(L"Could not find episode #" + ToWSTR(number) + 
      L" (" + series_title + L").");
  } else {
    Execute(file);
  }
  return !file.empty();
}

bool Anime::IsEpisodeAvailable(int number) {
  if (static_cast<unsigned int>(number) > episode_available.size()) return false;
  return episode_available.at(number - 1);
}

bool Anime::SetEpisodeAvailability(int number, bool available, const wstring& path) {
  if (number == 0) number = 1;
  if (number <= series_episodes || series_episodes == 0) {
    if (static_cast<unsigned int>(number) > episode_available.size()) {
      episode_available.resize(number);
    }
    episode_available[number - 1] = available;
    if (number == GetLastWatchedEpisode() + 1) {
      next_episode_path = path;
      new_episode_available = available;
    }
    if (!playing) {
      int list_index = MainDialog.GetListIndex(series_id);
      if (list_index > -1) {
        MainDialog.listview.RedrawItems(list_index, list_index, true);
      }
    }
    return true;
  }
  return false;
}

// =============================================================================

wstring Anime::GetImagePath() {
  return Taiga.GetDataPath() + L"db\\image\\" + ToWSTR(series_id) + L".jpg";
}

void Anime::SetLocalData(const wstring& folder, const wstring& titles) {
  if ((folder == EMPTY_STR || folder == this->folder) && 
      (titles == EMPTY_STR || titles == synonyms)) {
        return;
  }

  if (folder != EMPTY_STR) this->folder = folder;
  if (titles != EMPTY_STR) synonyms = titles;

  Settings.Anime.SetItem(series_id, folder, titles);
  Settings.Save();

  if (!synonyms.empty() && CurrentEpisode.anime_id == ANIMEID_NOTINLIST) {
    CurrentEpisode.Set(ANIMEID_UNKNOWN);
  }
}

// =============================================================================

int Anime::GetIntValue(int mode) const {
  EventItem* item = EventQueue.FindItem(series_id, mode);
  switch (mode) {
    case EVENT_SEARCH_EPISODE:
      return item ? item->episode : my_watched_episodes;
    case EVENT_SEARCH_REWATCH:
      return item ? item->enable_rewatching : my_rewatching;
    case EVENT_SEARCH_SCORE:
      return item ? item->score : my_score;
    case EVENT_SEARCH_STATUS:
      return item ? item->status : my_status;
    default:
      return -1;
  }
}

wstring Anime::GetStrValue(int mode) const {
  EventItem* item = EventQueue.FindItem(series_id, mode);
  switch (mode) {
    case EVENT_SEARCH_TAGS:
      return item ? item->tags : my_tags;
    default:
      return L"";
  }
}

int Anime::GetLastWatchedEpisode() const {
  return GetIntValue(EVENT_SEARCH_EPISODE);
}
int Anime::GetRewatching() const {
  return GetIntValue(EVENT_SEARCH_REWATCH);
}
int Anime::GetScore() const {
  return GetIntValue(EVENT_SEARCH_SCORE);
}
int Anime::GetStatus() const {
  return GetIntValue(EVENT_SEARCH_STATUS);
}
wstring Anime::GetTags() const {
  return GetStrValue(EVENT_SEARCH_TAGS);
}

int Anime::GetTotalEpisodes() const {
  if (series_episodes > 0) {
    return series_episodes;
  }

  int number = max(my_watched_episodes, GetLastWatchedEpisode());
  number = max(number, static_cast<int>(episode_available.size()));

  if (series_type == mal::TYPE_TV) {
    if (mal::IsValidDate(series_start)) {
      unsigned short year_start = 0, month_start = 0, day_start = 0;
      mal::ParseDateString(series_start, year_start, month_start, day_start);
      unsigned short year_end = 0, month_end = 0, day_end = 0;
      if (mal::IsValidDate(series_end)) {
        mal::ParseDateString(series_end, year_end, month_end, day_end);
      } else {
        mal::ParseDateString(GetDateJapan(L"yyyy'-'MM'-'dd"), year_end, month_end, day_end);
      }
      int days_end = (year_end * 365) + (month_end * 30) + day_end;
      int days_start = (year_start * 365) + (month_start * 30) + day_start;
      number = max(number, (days_end - days_start) / 7);
    }
  }

  if (number < 12) return 13;
  if (number < 24) return 26;
  if (number < 50) return 51;
  return 0;
}

// =============================================================================

int Anime::GetAiringStatus() {
  if (IsFinishedAiring()) return mal::STATUS_FINISHED;
  if (IsAiredYet(true)) return mal::STATUS_AIRING;
  return mal::STATUS_NOTYETAIRED;
}

bool Anime::IsAiredYet(bool strict) const {
  if (series_status == mal::STATUS_NOTYETAIRED) {
    if (!mal::IsValidDate(series_start)) return false;
    if (strict) {
      if (series_start.at(5) == '0' && series_start.at(6) == '0')
        return CompareStrings(GetDateJapan(L"yyyy"), series_start.substr(0, 4)) > 0;
      if (series_start.at(8) == '0' && series_start.at(9) == '0')
        return CompareStrings(GetDateJapan(L"yyyy'-'MM"), series_start.substr(0, 7)) > 0;
    }
    return CompareStrings(GetDateJapan(L"yyyy'-'MM'-'dd"), series_start) >= 0;
  }
  return true;
}

bool Anime::IsFinishedAiring() const {
  if (series_status == mal::STATUS_FINISHED) return true;
  if (!mal::IsValidDate(series_end)) return false;
  if (!IsAiredYet()) return false;
  
  if (series_end.at(5) == '0' && series_end.at(6) == '0') {
    return CompareStrings(GetDateJapan(L"yyyy"), series_end.substr(0, 4)) > 0;
  }
  if (series_end.at(8) == '0' && series_end.at(9) == '0') {
    return CompareStrings(GetDateJapan(L"yyyy'-'MM"), series_end.substr(0, 7)) > 0;
  }
  return CompareStrings(GetDateJapan(L"yyyy'-'MM'-'dd"), series_end) > 0;
}

void Anime::SetStartDate(const wstring& date, bool ignore_previous) {
  if (!mal::IsValidDate(my_start_date) || ignore_previous) {
    my_start_date = date.empty() ? GetDate(L"yyyy'-'MM'-'dd") : date;
    AnimeList.Save(series_id, L"my_start_date", my_start_date, ANIMELIST_EDITANIME);
  }
}

void Anime::SetFinishDate(const wstring& date, bool ignore_previous) {
  if (!mal::IsValidDate(my_finish_date) || ignore_previous) {
    my_finish_date = date.empty() ? GetDate(L"yyyy'-'MM'-'dd") : date;
    AnimeList.Save(series_id, L"my_finish_date", my_finish_date, ANIMELIST_EDITANIME);
  }
}

// =============================================================================

bool Anime::Edit(EventItem& item, const wstring& data, int status_code) {
  if (!mal::UpdateSucceeded(item, data, status_code)) {
    // Show balloon tip on failure
    wstring text = L"Title: " + series_title;
    text += L"\nReason: " + (item.reason.empty() ? L"Unknown" : item.reason);
    text += L"\nClick to try again.";
    Taiga.current_tip_type = TIPTYPE_UPDATEFAILED;
    Taskbar.Tip(L"", L"", 0);
    Taskbar.Tip(text.c_str(), L"Update failed", NIIF_ERROR);
    return false;
  }

  // Edit episode
  if (item.episode > -1) {
    my_watched_episodes = item.episode;
    AnimeList.Save(series_id, L"my_watched_episodes", ToWSTR(my_watched_episodes));
  }
  // Edit score
  if (item.score > -1) {
    my_score = item.score;
    AnimeList.Save(series_id, L"my_score", ToWSTR(my_score));
  }
  // Edit status
  if (item.status > 0) {
    AnimeList.user.IncreaseItemCount(item.status, 1, false);
    AnimeList.user.IncreaseItemCount(my_status, -1);
    my_status = item.status;
    AnimeList.Save(series_id, L"my_status", ToWSTR(my_status));
  }
  // Edit re-watching status
  if (item.enable_rewatching > -1) {
    my_rewatching = item.enable_rewatching;
    AnimeList.Save(series_id, L"my_rewatching", ToWSTR(my_rewatching));
  }
  // Edit ID (Add)
  if (item.mode == HTTP_MAL_AnimeAdd) {
    if (IsNumeric(data)) {
      my_id = ToINT(data);
      AnimeList.Save(series_id, L"my_id", data);
    }
  }
  // Edit tags
  if (item.tags != EMPTY_STR) {
    my_tags = item.tags;
    AnimeList.Save(series_id, L"my_tags", my_tags);
  }
  // Delete
  if (item.mode == HTTP_MAL_AnimeDelete) {
    MainDialog.ChangeStatus(L"Item deleted. (" + series_title + L")");
    AnimeList.DeleteItem(series_id);
    MainDialog.RefreshList();
    MainDialog.RefreshTabs();
    SearchDialog.PostMessage(WM_CLOSE);
    if (CurrentEpisode.anime_id == item.anime_id) {
      CurrentEpisode.Set(ANIMEID_NOTINLIST);
    }
  }

  // Remove item from event queue
  EventQueue.Remove();
  // Check for more events
  EventQueue.Check();

  // Redraw main list item
  int list_index = MainDialog.GetListIndex(series_id);
  if (list_index > -1) {
    MainDialog.listview.RedrawItems(list_index, list_index, true);
  }

  return true;
}

bool Anime::IsDataOld() {
  time_t time_diff = time(nullptr) - last_modified;

  if (GetAiringStatus() == mal::STATUS_FINISHED) {
    return time_diff >= 60 * 60 * 24 * 7; // 1 week
  } else {
    return time_diff >= 60 * 60; // 1 hour
  }
}

bool Anime::Update(Anime& anime, bool modify) {
  bool modified = false;

  #define UPDATE_ANIME_DATA_INT(x) \
    if (this->x != anime.x) { \
      this->x = anime.x; \
      modified = true; \
    }
  #define UPDATE_ANIME_DATA_STR(x) \
    if (!IsEqual(this->x, anime.x)) { \
      if (!anime.x.empty()) { \
        this->x = anime.x; \
        modified = true; \
      } \
    }
  
  UPDATE_ANIME_DATA_INT(series_id);
  UPDATE_ANIME_DATA_INT(series_type);
  UPDATE_ANIME_DATA_INT(series_episodes);
  UPDATE_ANIME_DATA_INT(series_status);
  UPDATE_ANIME_DATA_STR(series_title);
  UPDATE_ANIME_DATA_STR(series_synonyms);
  UPDATE_ANIME_DATA_STR(series_start);
  UPDATE_ANIME_DATA_STR(series_end);
  UPDATE_ANIME_DATA_STR(series_image);
  
  UPDATE_ANIME_DATA_STR(genres);
  UPDATE_ANIME_DATA_STR(popularity);
  UPDATE_ANIME_DATA_STR(producers);
  UPDATE_ANIME_DATA_STR(rank);
  UPDATE_ANIME_DATA_STR(score);
  UPDATE_ANIME_DATA_STR(synopsis);

  #undef UPDATE_ANIME_DATA_STR
  #undef UPDATE_ANIME_DATA_INT

  if (modify && modified) {
    last_modified = time(nullptr);
    return true;
  } else {
    return false;
  }
}

// =============================================================================

bool Anime::GetFansubFilter(vector<wstring>& groups) {
  bool found = false;
  for (auto i = Aggregator.filter_manager.filters.begin(); 
       i != Aggregator.filter_manager.filters.end(); ++i) {
    if (found) break;
    for (auto j = i->anime_ids.begin(); j != i->anime_ids.end(); ++j) {
      if (*j != series_id) continue;
      if (found) break;
      for (auto k = i->conditions.begin(); k != i->conditions.end(); ++k) {
        if (k->element != FEED_FILTER_ELEMENT_ANIME_GROUP) continue;
        groups.push_back(k->value);
        found = true;
      }
    }
  }
  return found;
}

bool Anime::SetFansubFilter(const wstring& group_name) {
  FeedFilter* filter = nullptr;

  for (auto i = Aggregator.filter_manager.filters.begin(); 
       i != Aggregator.filter_manager.filters.end(); ++i) {
    if (filter) break;
    for (auto j = i->anime_ids.begin(); j != i->anime_ids.end(); ++j) {
      if (*j != series_id) continue;
      for (auto k = i->conditions.begin(); k != i->conditions.end(); ++k) {
        if (k->element != FEED_FILTER_ELEMENT_ANIME_GROUP) continue;
        filter = &(*i); break;
      }
    }
  }
  
  if (filter) {
    FeedFilterDialog.filter = *filter;
  } else {
    FeedFilterDialog.filter.Reset();
    FeedFilterDialog.filter.name = L"[Fansub] " + series_title;
    FeedFilterDialog.filter.match = FEED_FILTER_MATCH_ANY;
    FeedFilterDialog.filter.action = FEED_FILTER_ACTION_SELECT;
    FeedFilterDialog.filter.anime_ids.push_back(series_id);
    FeedFilterDialog.filter.AddCondition(FEED_FILTER_ELEMENT_ANIME_GROUP, 
      FEED_FILTER_OPERATOR_IS, group_name);
  }

  ExecuteAction(L"TorrentAddFilter", TRUE, reinterpret_cast<LPARAM>(g_hMain));
  
  if (!FeedFilterDialog.filter.conditions.empty()) {
    if (filter) {
      *filter = FeedFilterDialog.filter;
    } else {
      Aggregator.filter_manager.filters.push_back(FeedFilterDialog.filter);
    }
    return true;
  } else {
    return false;
  }
}