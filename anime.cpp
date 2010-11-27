/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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
#include "ui/ui_taskbar.h"
#include "ui/ui_taskdialog.h"
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
  Index(0), NewEps(false), Playing(false), Score(L"0.00")
{
}

void CAnime::Start(CEpisode episode) {
  // Change status
  Taiga.PlayStatus = PLAYSTATUS_PLAYING;
  Playing = true;

  // Update main window
  wstring status = L"Watching: " + Series_Title + PushString(L" #", episode.Number);
  if (Settings.Account.Update.OutOfRange && ToINT(episode.Number) > GetLastWatchedEpisode() + 1) {
    status += L" (out of range)";
  }
  MainWindow.ChangeStatus(status);
  MainWindow.UpdateTip();
  MainWindow.RefreshList(My_Status);
  MainWindow.RefreshTabs(My_Status);
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
          Settings.Anime.SetItem(Series_ID, Folder, L"%empty%");
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

void CAnime::End(CEpisode episode, bool do_end, bool do_update) {
  if (do_end) {
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
    MainWindow.ChangeStatus(DEFAULT_STATUS);
    MainWindow.UpdateTip();
    int list_index = MainWindow.GetListIndex(Index);
    int ret_value = 0;
    if (list_index > -1) {
      MainWindow.m_List.SetItemIcon(list_index, StatusToIcon(Series_Status));
      MainWindow.m_List.RedrawItems(list_index, list_index, true);
    }
  }

  // Update list
  if (do_update && Taiga.UpdatesEnabled && My_Status != MAL_COMPLETED) {
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
  int number = ToINT(GetLastEpisode(episode.Number));
  if (Series_Episodes == 1) {             // Completed (1 eps.)
    episode.Number = L"1";
    dlg.AddButton(L"Update and move\nUpdate and set as completed", IDCANCEL);
  } else if (Series_Episodes == number) { // Completed (>1 eps.)
    dlg.AddButton(L"Update and move\nUpdate and set as completed", IDCANCEL);
  } else if (My_Status != 1) {            // Watching
    dlg.AddButton(L"Update and move\nUpdate and set as watching", IDCANCEL);
  }
  wstring button = L"Update\nUpdate episode number from " + 
    ToWSTR(GetLastWatchedEpisode()) + L" to " + GetLastEpisode(episode.Number);
  dlg.AddButton(button.c_str(), IDYES);
  dlg.AddButton(L"Cancel\nDon't update anything", IDNO);
  
  // Show dialog
  ActivateWindow(g_hMain);
  dlg.Show(g_hMain);
  return dlg.GetSelectedButtonID();
}

void CAnime::Update(CEpisode episode, bool do_move) {
  int number = ToINT(GetLastEpisode(episode.Number));
  if (Series_Episodes == 1) number = 1;
  episode.Index = Index;
  
  // Set start/finish date
  if (number == 1) SetStartDate(L"", false);
  if (number == Series_Episodes) SetFinishDate(L"", true);
  
  if (do_move) {
    // Move to completed
    if (Series_Episodes == number) {
      EventQueue.Add(L"", Index, Series_ID, number, -1, MAL_COMPLETED, L"%empty%", L"", HTTP_MAL_AnimeEdit);
      return;
    // Move to watching
    } else if (My_Status != MAL_WATCHING || number == 1) {
      EventQueue.Add(L"", Index, Series_ID, number, -1, MAL_WATCHING, L"%empty%", L"", HTTP_MAL_AnimeEdit);
      return;
    }
  }

  // Update normally
  EventQueue.Add(L"", Index, Series_ID, number, -1, -1, L"%empty%", L"", HTTP_MAL_AnimeUpdate);
}

void CAnime::CheckFolder() {
  wstring old_folder = Folder;
  
  if (!Folder.empty()) {
    // Check if that folder still exists
    if (!FolderExists(Folder)) {
      old_folder.clear();
      Folder.clear();
    // Check if it's a valid folder
    } else if (!Meow.CompareTitle(Folder, Index)) {
      Folder.clear();
    }
  }
  if (Folder.empty()) {
    for (unsigned int i = 0; i < Settings.Folders.Root.size(); i++) {
      Folder = SearchFileFolder(Index, Settings.Folders.Root[i], 0, true);
      if (!Folder.empty()) {
        Settings.Anime.SetItem(Series_ID, Folder, L"%empty%");
        return;
      }
    }
  }

  Folder = old_folder;
}

void CAnime::CheckNewEpisode(bool check_folder) {
  if (NewEps) return;
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

  if (file.empty()) {
    NewEps = false;
    return;
  } else {
    NewEps = true;
    if (!Playing) {
      int list_index = MainWindow.GetListIndex(Index);
      if (list_index > -1) {
        MainWindow.m_List.RedrawItems(list_index, list_index, true);
      }
    }
  }
}

int CAnime::EstimateTotalEpisodes() {
  if (Series_Episodes > 0) return Series_Episodes;
  if (My_WatchedEpisodes < 12) return 13;
  if (My_WatchedEpisodes < 24) return 26;
  if (My_WatchedEpisodes < 50) return 51;
  return 0;
}

int CAnime::GetLastWatchedEpisode() {
  int value = EventQueue.GetLastWatchedEpisode(Index);
  if (!value) value = My_WatchedEpisodes;
  return value;
}

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
        MAL.DecodeSynopsis(Synopsis);
        return true;
      }
    }
  }
  return false;
}

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

void CAnime::Refresh(wstring data) {
  if (EventQueue.GetItemCount() == 0) return;
  int user_index = EventQueue.GetUserIndex();
  int index    = EventQueue.List[user_index].Item[EventQueue.List[user_index].Index].Index;
  int episode  = EventQueue.List[user_index].Item[EventQueue.List[user_index].Index].Episode;
  int score    = EventQueue.List[user_index].Item[EventQueue.List[user_index].Index].Score;
  int status   = EventQueue.List[user_index].Item[EventQueue.List[user_index].Index].Status;
  int mode     = EventQueue.List[user_index].Item[EventQueue.List[user_index].Index].Mode;
  wstring tags = EventQueue.List[user_index].Item[EventQueue.List[user_index].Index].Tags;

  // Check success
  bool success = false;
  switch (Settings.Account.MAL.API) {
    case MAL_API_OFFICIAL:
      switch (mode) {
        case HTTP_MAL_AnimeAdd:
          success = IsNumeric(data);
          if (success) {
            My_ID = ToINT(data);
            AnimeList.Write(index, L"my_id", data, ANIMELIST_EDITANIME);
            status = -1; // TEMP
          } else {
            MainWindow.ChangeStatus(L"Error: " + data);
            return;
          }
          break;
        default:
          success = (data == L"Updated");
          break;
      }
      break;
    case MAL_API_NONE:
      switch (mode) {
        case HTTP_MAL_AnimeAdd:
          // TODO
          break;
        case HTTP_MAL_AnimeUpdate:
          success = (ToINT(data) == episode);
          break;
        case HTTP_MAL_ScoreUpdate:
          success = (InStr(data, L"Updated score", 0) > -1);
          break;
        case HTTP_MAL_TagUpdate:
          success = (tags.empty() ? data.empty() : InStr(data, L"/animelist/", 0) > -1);
          break;
        case HTTP_MAL_AnimeEdit:
        case HTTP_MAL_StatusUpdate:
          success = (InStr(data, L"Success", 0) > -1);
          break;
      }
      break;
  }

  // Update status
  wstring buffer = L"Update "; buffer += (success ? L"succeeded." : L"failed.");
  MainWindow.ChangeStatus(buffer + L" (" + Series_Title + L")");
  // Show balloon tip
  if (episode > -1)       buffer += L"\nEpisode: " + ToWSTR(episode);
  if (score > -1)         buffer += L"\nScore: "   + ToWSTR(score);
  if (status > -1)        buffer += L"\nStatus: "  + MAL.TranslateMyStatus(status, false);
  if (tags != L"%empty%") buffer += L"\nTags: "    + tags;
  Taskbar.Tip(L"", L"", 0);
  Taskbar.Tip(buffer.c_str(), Series_Title.c_str(), (success ? NIIF_INFO : NIIF_ERROR));

  // Update main list
  int list_index = MainWindow.GetListIndex(Index);
  if (list_index > -1) {
    if (success && (score > -1)) {
      MainWindow.m_List.SetItem(list_index, 2, MAL.TranslateNumber(score).c_str());
    }
    MainWindow.m_List.SetItemIcon(list_index, success ? Icon16_Tick : Icon16_Error);
    MainWindow.m_List.RedrawItems(list_index, list_index, true);
  }

  if (success) {
    if (episode > -1) {
      My_WatchedEpisodes = episode;
      AnimeList.Write(index, L"my_watched_episodes", ToWSTR(episode), ANIMELIST_EDITANIME);
      NewEps = false;
      CheckNewEpisode();
    }
    if (score > -1) {
      My_Score = score;
      AnimeList.Write(index, L"my_score", ToWSTR(score), ANIMELIST_EDITANIME);
    }
    if (tags != L"%empty%") {
      My_Tags = tags;
      AnimeList.Write(index, L"my_tags", tags, ANIMELIST_EDITANIME);
    }

    if (status > 0) {
      AnimeList.ChangeItemCount(My_Status, -1);
      AnimeList.ChangeItemCount(status, 1);
      My_Status = status;
      MainWindow.RefreshList(My_Status);
      MainWindow.RefreshTabs(My_Status);
      list_index = MainWindow.GetListIndex(Index);
      if (list_index > -1) {
        MainWindow.m_List.EnsureVisible(list_index);
        MainWindow.m_List.SetSelectedItem(list_index);
      }
      AnimeList.Write(index, L"my_status", ToWSTR(My_Status), ANIMELIST_EDITANIME);
    }

    // Remove item from update buffer
    EventQueue.Remove();
    // Check for more items
    EventQueue.Check();
  }
}