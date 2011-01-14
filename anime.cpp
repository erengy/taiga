/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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
#include "animelist.h"
#include "common.h"
#include "dlg/dlg_anime_info.h"
#include "dlg/dlg_main.h"
#include "event.h"
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

CEpisode CurrentEpisode;

// =============================================================================

CEpisode::CEpisode() {
  Clear();
}

void CEpisode::Clear() {
  Index      = -1;
  AudioType  = L"";
  Checksum   = L"";
  Extra      = L"";
  File       = L"";
  Format     = L"";
  Group      = L"";
  Name       = L"";
  Number     = L"";
  Resolution = L""; 
  Title      = L"";
  Version    = L"";
  VideoType  = L"";
}

// =============================================================================

CMALAnime::CMALAnime() : 
  Series_ID(0), Series_Type(0), Series_Episodes(0), Series_Status(0),
  My_ID(0), My_WatchedEpisodes(0), My_Score(0), My_Status(0), My_Rewatching(0), My_RewatchingEP(0)
{
}

CAnime::CAnime() : 
  Index(0), NewEps(false), Playing(false)
{
}

void CAnime::Start(CEpisode episode) {
  // Change status
  Taiga.PlayStatus = PLAYSTATUS_PLAYING;
  Playing = true;

  // Update main window
  MainWindow.ChangeStatus();
  MainWindow.UpdateTip();
  MainWindow.RefreshList(GetStatus());
  MainWindow.RefreshTabs(GetStatus());
  int list_index = MainWindow.GetListIndex(Index);
  if (list_index > -1) {
    MainWindow.m_List.SetItemIcon(list_index, Icon16_Play);
    MainWindow.m_List.RedrawItems(list_index, list_index, true);
    MainWindow.m_List.EnsureVisible(list_index);
  }
  
  // Show balloon tip
  Taskbar.Tip(L"", L"", 0);
  Taskbar.Tip(ReplaceVariables(Settings.Program.Balloon.Format, episode).c_str(), 
    L"Watching:", NIIF_INFO);
  
  // Check folder
  if (Folder.empty()) {
    if (episode.Folder.empty()) {
      HWND hwnd = MediaPlayers.Item[MediaPlayers.Index].WindowHandle;
      episode.Folder = MediaPlayers.GetTitleFromProcessHandle(hwnd);
      episode.Folder = GetPathOnly(episode.Folder);
    }
    if (!episode.Folder.empty()) {
      for (size_t i = 0; i < Settings.Folders.Root.size(); i++) {
        // Set the folder if only it is under a root folder
        if (StartsWith(episode.Folder, Settings.Folders.Root[i])) {
          Folder = episode.Folder;
          Settings.Anime.SetItem(Series_ID, EMPTY_STR, Folder, EMPTY_STR);
        }
      }
    }
  }

  // Get additional information
  if (Score.empty() || Synopsis.empty()) {
    MAL.SearchAnime(Series_Title, this);
  }
  
  // Update
  if (Settings.Account.Update.Time == UPDATE_TIME_INSTANT) {
    End(episode, false, true);
  }
}

void CAnime::End(CEpisode episode, bool end_watching, bool update_list) {
  if (end_watching) {
    // Change status
    Taiga.PlayStatus = PLAYSTATUS_STOPPED;
    Playing = false;
    // Announce
    episode.Index = Index;
    ExecuteAction(L"AnnounceToHTTP", TRUE, reinterpret_cast<LPARAM>(&episode));
    ExecuteAction(L"AnnounceToMessenger", FALSE);
    ExecuteAction(L"AnnounceToSkype", FALSE);
    // Update main window
    episode.Index = 0;
    MainWindow.ChangeStatus();
    MainWindow.UpdateTip();
    int list_index = MainWindow.GetListIndex(Index);
    int ret_value = 0;
    if (list_index > -1) {
      MainWindow.m_List.SetItemIcon(list_index, StatusToIcon(Series_Status));
      MainWindow.m_List.RedrawItems(list_index, list_index, true);
    }
  }

  // Update list
  if (update_list && Taiga.UpdatesEnabled && (GetStatus() != MAL_COMPLETED || GetRewatching())) {
    if (Settings.Account.Update.Time == UPDATE_TIME_INSTANT || 
      Taiga.TickerMedia == -1 || Taiga.TickerMedia >= Settings.Account.Update.Delay) {
        int number = ToINT(episode.Number);
        if (Settings.Account.Update.OutOfRange == FALSE || number == GetLastWatchedEpisode() + 1) {
          if (MAL.IsValidEpisode(number, GetLastWatchedEpisode(), Series_Episodes)) {
            switch (Settings.Account.Update.Mode) {
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
}

int CAnime::Ask(CEpisode episode) {
  // Set up dialog
  CTaskDialog dlg;
  wstring title = L"Anime title: " + Series_Title;
  dlg.SetWindowTitle(APP_TITLE);
  dlg.SetMainIcon(TD_ICON_INFORMATION);
  dlg.SetMainInstruction(L"Do you want to update your anime list?");
  dlg.SetContent(title.c_str());
  dlg.UseCommandLinks(true);

  // Add buttons
  int number = GetLastEpisode(episode.Number);
  if (number == 0) number = 1;
  if (Series_Episodes == 1) {               // Completed (1 eps.)
    episode.Number = L"1";
    dlg.AddButton(L"Update and move\nUpdate and set as completed", IDCANCEL);
  } else if (Series_Episodes == number) {   // Completed (>1 eps.)
    dlg.AddButton(L"Update and move\nUpdate and set as completed", IDCANCEL);
  } else if (GetStatus() != MAL_WATCHING) { // Watching
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

void CAnime::Update(CEpisode episode, bool change_status) {
  // Create event item
  CEventItem item;
  item.AnimeIndex = Index;
  item.AnimeID = Series_ID;

  // Set episode number
  item.episode = GetLastEpisode(episode.Number);
  if (item.episode == 0 || Series_Episodes == 1) item.episode = 1;
  episode.Index = Index;
  
  // Set start/finish date
  if (item.episode == 1) SetStartDate(L"", false);
  if (item.episode == Series_Episodes) SetFinishDate(L"", false);

  if (change_status) {
    item.Mode = HTTP_MAL_AnimeEdit;
    // Move to completed
    if (Series_Episodes == item.episode) {
      item.status = MAL_COMPLETED;
      if (GetRewatching()) {
        item.enable_rewatching = FALSE;
        item.times_rewatched = 1; // TEMP
      }
      EventQueue.Add(item);
      return;
    // Move to watching
    } else if (GetStatus() != MAL_WATCHING || item.episode == 1) {
      if (!GetRewatching()) {
        item.status = MAL_WATCHING;
        EventQueue.Add(item);
        return;
      }
    }
  }

  // Update normally
  item.Mode = HTTP_MAL_AnimeUpdate;
  EventQueue.Add(item);
}

// =============================================================================

void CAnime::CheckFolder() {
  wstring old_folder = Folder;
  
  if (!Folder.empty()) {
    // Check if that folder still exists
    if (!FolderExists(Folder)) {
      old_folder.clear();
      Folder.clear();
    // Check if it's a valid folder
    } else {
      CEpisode episode; episode.Title = Folder;
      if (!Meow.CompareEpisode(episode, AnimeList.Item[Index])) {
        Folder.clear();
      }
    }
  }
  if (Folder.empty()) {
    for (unsigned int i = 0; i < Settings.Folders.Root.size(); i++) {
      Folder = SearchFileFolder(Index, Settings.Folders.Root[i], 0, true);
      if (!Folder.empty()) {
        Settings.Anime.SetItem(Series_ID, EMPTY_STR, Folder, EMPTY_STR);
        return;
      }
    }
  }

  Folder = old_folder;
}

bool CAnime::CheckNewEpisode(bool check_folder) {
  if (NewEps) return true;
  if (check_folder) CheckFolder();
  int number = Series_Episodes == 1 ? 0 : GetLastWatchedEpisode() + 1;
  wstring file;
  
  // Check anime folder first
  if (!Folder.empty()) {
    file = SearchFileFolder(Index, Folder, number, false);
  }
  // Check other folders
  /*if (file.empty()) {
    for (unsigned int i = 0; i < Settings.Folders.Root.size(); i++) {
      file = SearchFileFolder(Index, Settings.Folders.Root[i], number, false);
      if (!file.empty()) break;
    }
  }*/

  return !file.empty();
}

void CAnime::CheckEpisodeAvailability() {
  if (!Folder.empty()) {
    SearchFileFolder(Index, Folder, -1, false);
  }
}

bool CAnime::SetEpisodeAvailability(int number, bool available) {
  if (number == 0) number = 1;
  if (number <= Series_Episodes || Series_Episodes == 0) {
    if (static_cast<unsigned int>(number) > EpisodeAvailable.size()) {
      EpisodeAvailable.resize(number);
    }
    EpisodeAvailable[number - 1] = available;
    if (number == GetLastWatchedEpisode() + 1) {
      NewEps = available;
      if (!Playing) {
        int list_index = MainWindow.GetListIndex(Index);
        if (list_index > -1) {
          MainWindow.m_List.RedrawItems(list_index, list_index, true);
        }
      }
    }
    return true;
  }
  return false;
}

// =============================================================================

int CAnime::GetIntValue(int mode) {
  CEventItem* item = EventQueue.SearchItem(Index, mode);
  switch (mode) {
    case EVENT_SEARCH_EPISODE:
      return item ? item->episode : My_WatchedEpisodes;
    case EVENT_SEARCH_REWATCH:
      return item ? item->enable_rewatching : My_Rewatching;
    case EVENT_SEARCH_SCORE:
      return item ? item->score : My_Score;
    case EVENT_SEARCH_STATUS:
      return item ? item->status : My_Status;
    default:
      return -1;
  }
}

wstring CAnime::GetStrValue(int mode) {
  CEventItem* item = EventQueue.SearchItem(Index, mode);
  switch (mode) {
    case EVENT_SEARCH_TAGS:
      return item ? item->tags : My_Tags;
    default:
      return L"";
  }
}

int CAnime::GetLastWatchedEpisode() {
  return GetIntValue(EVENT_SEARCH_EPISODE);
}
int CAnime::GetRewatching() {
  return GetIntValue(EVENT_SEARCH_REWATCH);
}
int CAnime::GetScore() {
  return GetIntValue(EVENT_SEARCH_SCORE);
}
int CAnime::GetStatus() {
  return GetIntValue(EVENT_SEARCH_STATUS);
}
wstring CAnime::GetTags() {
  return GetStrValue(EVENT_SEARCH_TAGS);
}

int CAnime::GetTotalEpisodes() {
  if (Series_Episodes > 0) return Series_Episodes;
  if (GetLastWatchedEpisode() < 12) return 13;
  if (GetLastWatchedEpisode() < 24) return 26;
  if (GetLastWatchedEpisode() < 50) return 51;
  return 0;
}

// =============================================================================

bool CAnime::ParseSearchResult(const wstring& data) {
  if (data.empty()) return false;
  xml_document doc;
  xml_parse_result result = doc.load(data.c_str());
  if (result.status == status_ok) {
    xml_node anime = doc.child(L"anime");
    for (xml_node entry = anime.child(L"entry"); entry; entry = entry.next_sibling(L"entry")) {
      if (XML_ReadIntValue(entry, L"id") == Series_ID) {
        Score = XML_ReadStrValue(entry, L"score");
        Synopsis = XML_ReadStrValue(entry, L"synopsis");
        MAL.DecodeText(Synopsis);
        return true;
      }
    }
  }
  return false;
}

// =============================================================================

void CAnime::SetStartDate(wstring date, bool ignore_previous) {
  if (My_StartDate == L"0000-00-00" || My_StartDate.empty() || ignore_previous) {
    My_StartDate = date.empty() ? GetDate(L"yyyy'-'MM'-'dd") : date;
    AnimeList.Write(Index, L"my_start_date", My_StartDate, ANIMELIST_EDITANIME);
  }
}

void CAnime::SetFinishDate(wstring date, bool ignore_previous) {
  if (My_FinishDate == L"0000-00-00" || My_FinishDate.empty() || ignore_previous) {
    My_FinishDate = date.empty() ? GetDate(L"yyyy'-'MM'-'dd") : date;
    AnimeList.Write(Index, L"my_finish_date", My_FinishDate, ANIMELIST_EDITANIME);
  }
}

// =============================================================================

void CAnime::Edit(const wstring& data, CEventItem item) {
  // Check success
  bool success = MAL.UpdateSucceeded(data, item.Mode, item.episode, item.tags);

  // Show balloon tip
  if (!success) {
    wstring text = L"Title: " + Series_Title;
    text += L"\nReason: " + (data.empty() ? L"-" : data);
    text += L"\nClick to try again.";
    Taiga.CurrentTipType = TIPTYPE_UPDATEFAILED;
    Taskbar.Tip(L"", L"", 0);
    Taskbar.Tip(text.c_str(), L"Update failed", NIIF_ERROR);
    return;
  }

  // Edit episode
  if (item.episode > -1) {
    My_WatchedEpisodes = item.episode;
    AnimeList.Write(item.AnimeIndex, L"my_watched_episodes", ToWSTR(My_WatchedEpisodes));
  }
  // Edit score
  if (item.score > -1) {
    My_Score = item.score;
    AnimeList.Write(item.AnimeIndex, L"my_score", ToWSTR(My_Score));
  }
  // Edit status
  if (item.status > 0) {
    AnimeList.User.IncreaseItemCount(My_Status, -1);
    AnimeList.User.IncreaseItemCount(item.status, 1);
    My_Status = item.status;
    AnimeList.Write(item.AnimeIndex, L"my_status", ToWSTR(My_Status));
  }
  // Edit re-watching status
  if (item.enable_rewatching > -1) {
    My_Rewatching = item.enable_rewatching;
    AnimeList.Write(item.AnimeIndex, L"my_rewatching", ToWSTR(My_Rewatching));
  }
  // Edit ID
  if (item.Mode == HTTP_MAL_AnimeAdd) {
    My_ID = ToINT(data);
    AnimeList.Write(item.AnimeIndex, L"my_id", data);
  }
  // Edit tags
  if (item.tags != EMPTY_STR) {
    My_Tags = item.tags;
    AnimeList.Write(item.AnimeIndex, L"my_tags", My_Tags);
  }

  // Remove item from update buffer
  EventQueue.Remove();
  // Check for more items
  EventQueue.Check();

  // Redraw main list item
  int list_index = MainWindow.GetListIndex(Index);
  if (list_index > -1) {
    MainWindow.m_List.RedrawItems(list_index, list_index, true);
  }
}